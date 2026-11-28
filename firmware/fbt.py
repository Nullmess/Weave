#!/usr/bin/env python3

import argparse
import os
import posixpath
import signal
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path, PurePosixPath


FIRMWARE_ORIGIN = "Weave"

TARGET_HW = 7

# Taille binaire
COMPACT = 1
# Mode debug
DEBUG = 1

# Version firmware
DIST_SUFFIX = "1.0.4"

COPRO_OB_DATA = "site_scons/tools/ob.data"

# Version copro BLE
COPRO_CUBE_VERSION = "1.20.0"

COPRO_CUBE_DIR = "lib/stm32wb_copro"

COPRO_STACK_BIN = "stm32wb5x_BLE_Stack_light_fw.bin"
# BLE compact
COPRO_STACK_TYPE = "ble_light"

# Adresse automatique
COPRO_STACK_ADDR = "0x0"

# Firmware BLE
COPRO_STACK_BIN_DIR = posixpath.join(COPRO_CUBE_DIR, "firmware")

# Versions toolchain
FBT_TOOLCHAIN_VERSIONS = (" 12.3.", " 13.2.")

OPENOCD_OPTS = [
    "-f",
    "interface/stlink.cfg",
    "-c",
    "transport select hla_swd",
    "-f",
    "${FBT_DEBUG_DIR}/stm32wbx.cfg",
    "-c",
    "stm32wbx.cpu configure -rtos auto",
]

SVD_FILE = "${FBT_DEBUG_DIR}/STM32WB55_CM4.svd"

BLACKMAGIC = "auto"

LOADER_AUTOSTART = ""

FIRMWARE_APPS = {
    "default": [
        "basic_services",
        "weave_apps",
        "weave_hardware",
        "clock_settings",
        "system_apps",
    ],
}

FIRMWARE_APP_SET = "default"

ROOT = Path(__file__).resolve().parent

# Package FBT interne
__path__ = [str(ROOT / "site_scons" / "tools" / "fbt")]

BUILD = ROOT / "build"  # Native FBT path, linked to ../build/.work by root launcher.
USER_CACHE = Path(os.environ.get("WEAVE_CACHE_HOME", str(
    (Path(os.environ.get("LOCALAPPDATA", str(Path.home() / "AppData" / "Local"))) / "Weave" / "cache")
    if os.name == "nt" else
    (Path(os.environ.get("XDG_CACHE_HOME", str(Path.home() / ".cache"))) / "weave")
))).expanduser().resolve()
SYS_USB_DEVICES = Path("/sys/bus/usb/devices")


def _run_command(*args, **options):
    """Lancer un process group supervisé et gérer Ctrl+C."""
    check = options.pop("check", False)
    timeout = options.pop("timeout", None)
    input_bytes = options.pop("input", None)
    capture = options.pop("capture_output", False)
    if capture:
        if "stdout" in options or "stderr" in options:
            raise ValueError("stdout/stderr cannot be combined with capture_output")
        options["stdout"] = subprocess.PIPE
        options["stderr"] = subprocess.PIPE
    if input_bytes is not None:
        options["stdin"] = subprocess.PIPE
    # Groupe process séparé : Ctrl+C coupe tous les jobs.
    if os.name == "nt":
        options["creationflags"] = options.get("creationflags", 0) | subprocess.CREATE_NEW_PROCESS_GROUP
    else:
        options["start_new_session"] = True
    with subprocess.Popen(*args, **options) as child:
        try:
            out, err = child.communicate(input=input_bytes, timeout=timeout)
        except KeyboardInterrupt:
            if child.poll() is None:
                try:
                    if os.name == "nt": child.send_signal(signal.CTRL_BREAK_EVENT)
                    else: os.killpg(child.pid, signal.SIGINT)
                except (OSError, ProcessLookupError):
                    pass
                try:
                    child.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    try:
                        if os.name == "nt": child.terminate()
                        else: os.killpg(child.pid, signal.SIGTERM)
                    except (OSError, ProcessLookupError):
                        pass
                    try:
                        child.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        try:
                            if os.name == "nt": child.kill()
                            else: os.killpg(child.pid, signal.SIGKILL)
                        except (OSError, ProcessLookupError):
                            pass
                        child.wait()
            raise
        except subprocess.TimeoutExpired:
            if os.name == "nt": child.kill()
            else:
                try: os.killpg(child.pid, signal.SIGKILL)
                except ProcessLookupError: pass
            child.communicate()
            raise
        result = subprocess.CompletedProcess(child.args, child.returncode, out, err)
        if check: result.check_returncode()
        return result


def _remove_generated(path):
    """Supprimer un output build sans suivre les symlinks."""
    if path.is_symlink():
        path.unlink()
    elif hasattr(path, "is_junction") and path.is_junction():
        path.rmdir()
    elif path.is_dir():
        shutil.rmtree(path)
    elif path.exists():
        path.unlink()


def _clean_imported_ringtones():
    """Undo optional GitHub imports without touching bundled sound files.

    Older Weave builds only recorded an import count, so use their reserved
    F_/R_ filename prefixes only when that legacy marker is present.
    """
    stock = ROOT / "assets" / "weave_resources" / ".weave" / "stock-ringtones"
    marker = stock / ".github-import-attempted-v1"
    tracking = stock / ".github-imported-files-v1"
    if not marker.is_file() and not tracking.is_file():
        return
    import re

    contents = marker.read_text(encoding="utf-8") if marker.is_file() else ""
    if tracking.is_file():
        contents += "\n" + tracking.read_text(encoding="utf-8")
    generated = [line[5:] for line in contents.splitlines() if line.startswith("file=")]
    if not generated and contents.startswith("Imported "):
        generated = [item.name for item in stock.iterdir() if item.is_file()
                     and item.name.startswith(("F_", "R_"))]
    for name in generated:
        if not re.fullmatch(r"[FR]_[A-Za-z0-9_-]+\.(?:tone|fmf|txt|rtttl)", name):
            continue
        item = stock / name
        if item.is_file() and not item.is_symlink():
            item.unlink()
            print(f"Weave clean: imported ringtone {name}")
    marker.unlink(missing_ok=True)
    tracking.unlink(missing_ok=True)


def clean():
    """Restore the unpacked source tree; keep external caches for reuse."""
    try:
        sys.dont_write_bytecode = True
        sys.path.insert(0, str(ROOT / "site_scons" / "tools"))
        from weave_deps import clean_links

        # Disconnect only Weave-owned symlinks before deleting build/.work.
        clean_links()
        project = ROOT.parent
        generated = (
            BUILD,
            project / "build",  # Root release files and interrupted build workspace.
            project / "apps" / "desktop" / "node_modules",
            project / "apps" / "desktop" / "dist",
            project / "apps" / "desktop" / "dist-electron",
            project / "apps" / "desktop" / "release",
            project / "apps" / "desktop" / "build",
            project / "apps" / "android" / ".gradle",
            project / "apps" / "android" / "build",
            project / "apps" / "android" / "app" / "build",
            project / "apps" / "android" / ".kotlin",
            project / "apps" / "android" / "app" / ".cxx",
            project / "apps" / "android" / "local.properties",
            ROOT / ".sconsign.dblite",
        )
        for path in generated:
            if path.exists() or path.is_symlink():
                _remove_generated(path)
                print(f"Weave clean: {os.path.relpath(path, project)}")
        _clean_imported_ringtones()

        # Python/TypeScript caches can be created anywhere in the monorepo.
        # Never descend into .git or symlinked dependencies.
        for parent, dirs, files in os.walk(project, followlinks=False):
            dirs[:] = [name for name in dirs if name != ".git"]
            current = Path(parent)
            for name in list(dirs):
                if name in ("__pycache__", ".pytest_cache", ".mypy_cache", ".ruff_cache"):
                    _remove_generated(current / name)
                    dirs.remove(name)
            for name in files:
                if name.endswith((".pyc", ".pyo", ".tsbuildinfo")):
                    _remove_generated(current / name)
        print("Weave clean: source tree restored; build/ removed")
        print("Weave clean: external cached SDK/toolchains kept for faster rebuilds")
    except (OSError, UnicodeError) as error:
        print(f"Weave clean: {error}", file=sys.stderr)
        return 1
    return 0


def build_args(args):
    if not args or args[0] != "build":
        return args
    args = args[1:]
    result = list(args)
    for name, value in (("COMPACT", "1"), ("DEBUG", "0")):
        if not any(arg.startswith(name + "=") for arg in result):
            result.insert(0, name + "=" + value)
    if not any(not arg.startswith("-") and "=" not in arg for arg in result):
        result.append("updater_package")
    return result


def sync():
    if not (ROOT.parent / ".git").exists() or os.environ.get("FBT_NO_SYNC"):
        return 0
    jobs = str(max(1, (os.cpu_count() or 1) * 2))
    cmd = ["git", "submodule", "update", "--init", "--recursive"]
    cmd += ["--jobs", jobs]
    if os.environ.get("FBT_GIT_SUBMODULE_SHALLOW"):
        cmd += ["--depth", "1"]
    return _run_command(cmd, cwd=ROOT.parent, check=False).returncode


def dependencies():
    script = ROOT / "site_scons" / "tools" / "weave_deps.py"
    return _run_command([sys.executable, str(script)], cwd=ROOT).returncode


def compile_firmware(args):
    # Préparer le cache ringtone offline pour SCons.
    if os.environ.get("WEAVE_RINGTONES_FETCH", "1") != "0":
        importer = ROOT / "site_scons" / "tools" / "fetch_weave_ringtones.py"
        if importer.is_file():
            _run_command([sys.executable, str(importer)], cwd=ROOT, check=False)
    flags = ["--warn=target-not-built"]
    if not os.environ.get("FBT_VERBOSE"):
        flags.append("-Q")
    env = os.environ.copy()
    BUILD.mkdir(parents=True, exist_ok=True)
    # Conserver le compilateur ARM dans le cache utilisateur entre les builds.
    env["FBT_TOOLCHAIN_PATH"] = str(USER_CACHE)
    env["PYTHONPYCACHEPREFIX"] = str(USER_CACHE / "pycache")
    script = ROOT / "site_scons" / "tools" / "toolchain"
    if os.name == "nt":
        helper = script / "fbtenv.cmd"
        command = subprocess.list2cmdline(["python", "-m", "SCons", *flags, *args])
        line = f'call "{helper}" env && {command}'
        cmd = ["cmd.exe", "/d", "/s", "/c", line]
    else:
        helper = script / "fbtenv.sh"
        line = '. "$1" && shift && exec python3 -m SCons "$@"'
        cmd = ["sh", "-c", line, "fbt", str(helper), *flags, *args]
    if not helper.is_file():
        print(f"Weave : helper manquant : {helper}", file=sys.stderr)
        return 1
    return _run_command(cmd, cwd=ROOT, env=env).returncode


def find_firmware_package():
    """Chercher un firmware F7 de la version courante."""
    version = (ROOT.parent / "VERSION").read_text(encoding="utf-8").strip()
    candidates = (
        ROOT.parent / "build" / f"Weave-{version}-firmware.tgz",
        BUILD / "releases" / f"Weave-{version}-firmware.tgz",
        BUILD / "dist" / "f7-C" / f"f7-update-{version}.tgz",
    )
    for candidate in candidates:
        if candidate.is_file() and candidate.stat().st_size > 0:
            return candidate
    return None


def extract_firmware_package(package, destination):
    """Extraire un .tgz après validation des chemins."""
    seen = set()
    update_manifest = None
    total_size = 0
    with tarfile.open(package, "r:gz") as archive:
        members = archive.getmembers()
        if not members:
            raise ValueError("Firmware archive is empty")
        package_root = None
        for member in members:
            name = member.name
            path = PurePosixPath(name)
            if (path.is_absolute() or not path.parts or
                    any(part in (".", "..") for part in path.parts) or "\\" in name or
                    not (member.isdir() or member.isfile())):
                raise ValueError(f"Unsupported archive entry: {name}")
            if package_root is None:
                package_root = path.parts[0]
            if path.parts[0] != package_root:
                raise ValueError("Firmware archive contains multiple roots")
            if name in seen:
                raise ValueError(f"Duplicate archive entry: {name}")
            seen.add(name)
            total_size += member.size
            if total_size > 256 * 1024 * 1024:
                raise ValueError("Firmware archive exceeds 256 MiB")
            if member.isfile() and path.name == "update.fuf" and len(path.parts) == 2:
                update_manifest = path
        if update_manifest is None:
            raise ValueError("Firmware archive has no update.fuf manifest")
        for member in members:
            target = destination.joinpath(*PurePosixPath(member.name).parts)
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with archive.extractfile(member) as source, target.open("wb") as output:
                    shutil.copyfileobj(source, output)
    return destination.joinpath(*update_manifest.parts)


def flash_package(package, port="auto", chunk_size=1024, chunk_timeout=45):
    """Installer le .tgz via USB CDC avec transfert borné."""
    with tempfile.TemporaryDirectory(prefix="weave-flash-") as temporary:
        manifest = extract_firmware_package(package, Path(temporary))
        script = ROOT / "site_scons" / "tools" / "selfupdate.py"
        helper_dir = ROOT / "site_scons" / "tools" / "toolchain"
        env = os.environ.copy()
        env["FBT_TOOLCHAIN_PATH"] = str(USER_CACHE)
        env["PYTHONPYCACHEPREFIX"] = str(USER_CACHE / "pycache")
        if os.name == "nt":
            helper = helper_dir / "fbtenv.cmd"
            command = subprocess.list2cmdline([
                "python", str(script), "-p", port,
                "--chunk-size", str(chunk_size), "--chunk-timeout", str(chunk_timeout),
                str(manifest),
            ])
            cmd = ["cmd.exe", "/d", "/s", "/c", f'call "{helper}" env && {command}']
        else:
            helper = helper_dir / "fbtenv.sh"
            line = '. "$1" && shift && exec python3 "$@"'
            cmd = [
                "sh", "-c", line, "fbt", str(helper), str(script), "-p", port,
                "--chunk-size", str(chunk_size), "--chunk-timeout", str(chunk_timeout),
                str(manifest),
            ]
        if not helper.is_file():
            print(f"Weave flash: missing toolchain helper: {helper}", file=sys.stderr)
            return 1
        print(f"Weave flash: installing {package.name} over USB (port: {port})", flush=True)
        print(f"Weave flash: USB CDC chunks {chunk_size} bytes; per-chunk timeout {chunk_timeout}s.", flush=True)
        code = subprocess.run(cmd, cwd=ROOT, env=env, check=False).returncode
        if code:
            print("Weave flash: USB upload/installation failed; no update was confirmed.", file=sys.stderr)
            print("  Reconnect the Flipper directly, check the microSD, then retry with", file=sys.stderr)
            print("  python3 fbt.py flash --chunk-size 512", file=sys.stderr)
            print("  Alternative: qFlipper > Install from file >", package, file=sys.stderr)
        return code


def region(args):
    """Restore official geolocated radio region via USB, without firmware flash."""
    script = ROOT / "site_scons" / "tools" / "region_cli.py"
    helper_dir = ROOT / "site_scons" / "tools" / "toolchain"
    env = os.environ.copy()
    BUILD.mkdir(parents=True, exist_ok=True)
    env["FBT_TOOLCHAIN_PATH"] = str(USER_CACHE)
    env["PYTHONPYCACHEPREFIX"] = str(USER_CACHE / "pycache")
    if os.name == "nt":
        helper = helper_dir / "fbtenv.cmd"
        cmdline = subprocess.list2cmdline(["python", str(script), *args])
        command = ["cmd.exe", "/d", "/s", "/c", f'call "{helper}" env && {cmdline}']
    else:
        helper = helper_dir / "fbtenv.sh"
        command = ["sh", "-c", '. "$1" && shift && exec python3 "$@"',
                   "weave", str(helper), str(script), *args]
    if not helper.is_file():
        print(f"Weave region: missing toolchain helper: {helper}", file=sys.stderr)
        return 1
    return _run_command(command, cwd=ROOT, env=env, check=False).returncode


def flash(args):
    parser = argparse.ArgumentParser(prog="python3 fbt.py flash")
    parser.add_argument("-p", "--port", default="auto", help="Flipper USB serial port (default: auto)")
    parser.add_argument("--chunk-size", type=int, choices=(256, 512, 1024, 2048, 4096, 8192),
                        default=1024, help="USB CDC transfer chunk bytes (default: 1024)")
    parser.add_argument("--chunk-timeout", type=int, default=45,
                        help="Seconds before declaring a USB chunk stalled (10-180, default: 45)")
    options = parser.parse_args(args)
    if not 10 <= options.chunk_timeout <= 180:
        parser.error("--chunk-timeout must be between 10 and 180 seconds")
    package = find_firmware_package()
    if package is None:
        print("Weave flash: no matching prebuilt firmware .tgz in build/.", file=sys.stderr)
        print("Run 'python3 fbt.py build' first, then retry flash.", file=sys.stderr)
        return 1
    try:
        return flash_package(package, options.port, options.chunk_size, options.chunk_timeout)
    except (OSError, ValueError, tarfile.TarError) as error:
        print(f"Weave flash: cannot use firmware archive: {error}", file=sys.stderr)
        return 1


def detect_dfu_devices():
    """Détecter les périphériques STM32 en DFU."""
    if sys.platform.startswith("linux"):
        usb_root = SYS_USB_DEVICES
        devices = []
        if usb_root.is_dir():
            for entry in usb_root.iterdir():
                try:
                    vendor = (entry / "idVendor").read_text(encoding="ascii").strip().lower()
                    product = (entry / "idProduct").read_text(encoding="ascii").strip().lower()
                except (OSError, UnicodeError):
                    continue
                if vendor == "0483" and product == "df11":
                    devices.append(str(entry))
            return devices
        # Fallback lsusb si sysfs est indisponible.
        if shutil.which("lsusb"):
            try:
                result = subprocess.run(
                    ["lsusb"], capture_output=True, text=True, check=False, timeout=5
                )
            except (OSError, subprocess.TimeoutExpired):
                return []
            if result.returncode == 0:
                return [line for line in result.stdout.splitlines()
                        if "0483:df11" in line.lower()]
    return []


def detect_normal_flipper_usb():
    """Détecter les périphériques USB CDC sans écriture."""
    devices = []
    if sys.platform.startswith("linux") and SYS_USB_DEVICES.is_dir():
        for entry in SYS_USB_DEVICES.iterdir():
            try:
                vendor = (entry / "idVendor").read_text(encoding="ascii").strip().lower()
                product = (entry / "idProduct").read_text(encoding="ascii").strip().lower()
            except (OSError, UnicodeError):
                continue
            if vendor == "0483" and product == "5740":
                devices.append(str(entry))
    return devices


def find_repair_tool(requested=None):
    """Trouver qFlipper-cli, pas le binaire GUI."""
    if requested:
        explicit = Path(requested).expanduser()
        if explicit.is_file():
            return str(explicit.resolve()) if os.access(explicit, os.X_OK) else None
        return shutil.which(requested)
    for name in ("qFlipper-cli", "qflipper-cli", "qFlipperCLI"):
        found = shutil.which(name)
        if found:
            return found
    return None


def install_repair_tool_interactively():
    """Proposer qflipper via pacman avec confirmation."""
    if not sys.platform.startswith("linux") or not shutil.which("pacman"):
        return None
    if not sys.stdin.isatty():
        return None
    print("Weave repair: CachyOS/Arch detected. Official qFlipper is available")
    print("  from pacman; this package includes the qFlipper-cli recovery tool.")
    try:
        answer = input("Install qflipper via sudo pacman now? [y/N]: ")
    except (EOFError, KeyboardInterrupt):
        print("\nWeave repair: package installation cancelled.")
        return None
    if answer.strip().lower() not in ("y", "yes"):
        print("Weave repair: package installation declined.")
        return None
    try:
        result = subprocess.run(
            ["sudo", "pacman", "-S", "--needed", "qflipper"],
            cwd=ROOT, check=False
        )
    except OSError as error:
        print(f"Weave repair: could not invoke pacman: {error}", file=sys.stderr)
        return None
    if result.returncode:
        print(f"Weave repair: package installation failed (exit {result.returncode}).",
              file=sys.stderr)
        return None
    tool = find_repair_tool()
    if not tool:
        print("Weave repair: qflipper package was installed, but qFlipper-cli")
        print("  is still missing from PATH. Check 'command -v qFlipper-cli'.")
    return tool


def print_recovery_instructions():
    print("Weave repair: enter the Flipper Zero's *USB DFU* recovery mode:")
    print("  1. Unplug the USB cable from the Flipper.")
    print("  2. Hold BACK + OK for 30 seconds; a blank screen is normal.")
    print("  3. Release the buttons and reconnect the Flipper via a USB data cable.")
    print("  4. On Linux, 'lsusb' should show 0483:df11 (STM32 DFU).")
    print("If it does not appear, try another data cable/USB port and charge the device.")


def repair(args):
    """Récupérer le firmware via DFU et qFlipper-cli."""
    parser = argparse.ArgumentParser(prog="python3 fbt.py repair")
    parser.add_argument("--diagnose", action="store_true",
                        help="Report DFU and tool state without writing anything")
    parser.add_argument("--tool", metavar="PATH", default=None,
                        help="Path to the official qFlipper-cli executable")
    options = parser.parse_args(args)

    dfu_devices = detect_dfu_devices()
    tool = find_repair_tool(options.tool)
    print("Weave repair: USB DFU firmware recovery (not a regular flash)")
    normal_devices = detect_normal_flipper_usb()
    print(f"  Normal USB CDC devices (0483:5740): {len(normal_devices)}")
    print(f"  DFU devices (0483:df11): {len(dfu_devices)}")
    if normal_devices and not dfu_devices:
        print("  Flipper detected in normal USB mode. DFU recovery requires a separate reboot into Recovery mode.")
    print(f"  qFlipper CLI: {tool or 'not found'}")
    package = find_firmware_package()
    print(f"  Local Weave update: {package if package else 'not built'}")
    print("  Recovery installs official firmware first, NOT Weave 1.0.4.")
    print("  WARNING: recovery may ERASE Flipper internal storage, including")
    print("  settings, Dolphin progress and Bluetooth pairings. microSD is preserved.")

    if options.diagnose:
        if not dfu_devices:
            print_recovery_instructions()
        if not tool:
            print("Install official qFlipper / qFlipper-cli. On CachyOS/Arch:")
            print("  sudo pacman -S qflipper")
            print("On a real repair, Weave can offer to run this installer")
            print("with your confirmation. --diagnose never installs anything.")
            print("The qFlipper GUI can repair the Flipper in DFU mode too.")
        print("Diagnostic only: nothing was written to the Flipper.")
        return 0

    if not dfu_devices:
        print_recovery_instructions()
        if not sys.stdin.isatty():
            print("Weave repair: DFU device not found; connect in DFU and retry.",
                  file=sys.stderr)
            return 1
        try:
            input("When the Flipper is connected in DFU mode, press Enter to detect it: ")
        except (EOFError, KeyboardInterrupt):
            print("\nWeave repair: cancelled; nothing was flashed.")
            return 1
        dfu_devices = detect_dfu_devices()
    if len(dfu_devices) != 1:
        print("Weave repair: expected exactly one STM32 DFU device; found "
              f"{len(dfu_devices)}. Check the USB connection.", file=sys.stderr)
        return 1

    if not tool:
        if options.tool:
            print(f"Weave repair: requested qFlipper CLI is not executable: {options.tool}",
                  file=sys.stderr)
            return 1
        tool = install_repair_tool_interactively()
        if not tool:
            print("Weave repair: qFlipper-cli is required; no firmware was written.",
                  file=sys.stderr)
            print("On CachyOS/Arch, install it with: sudo pacman -S qflipper",
                  file=sys.stderr)
            print("Then run: python3 fbt.py repair", file=sys.stderr)
            print("Alternatively run 'qFlipper' and click Repair in DFU mode.",
                  file=sys.stderr)
            return 1
        print(f"Weave repair: qFlipper CLI ready: {tool}")

    if not sys.stdin.isatty():
        print("Weave repair: interactive confirmation required; no unattended erase.",
              file=sys.stderr)
        return 1
    try:
        answer = input("Type REPAIR to proceed with potentially destructive recovery: ")
    except (EOFError, KeyboardInterrupt):
        print("\nWeave repair: cancelled; nothing was flashed.")
        return 1
    if answer != "REPAIR":
        print("Weave repair: cancelled; nothing was flashed.")
        return 1

    print("Weave repair: launching official qFlipper CLI recovery.", flush=True)
    try:
        result = subprocess.run([tool], cwd=ROOT, check=False)
    except OSError as error:
        print(f"Weave repair: cannot launch qFlipper CLI: {error}", file=sys.stderr)
        return 1
    if result.returncode:
        print(f"Weave repair: qFlipper CLI returned {result.returncode}.", file=sys.stderr)
        print("If needed, use the official qFlipper GUI Repair function.", file=sys.stderr)
        return result.returncode
    print("Weave repair: qFlipper CLI exited successfully; verify that the")
    print("Flipper boots normally before attempting any further flash.")
    print("To restore Weave 1.0.4 after successful recovery:")
    print("  python3 fbt.py flash")
    print("Note: a crash caused by Weave source code may recur after reinstalling it.")
    return 0


def main():
    if not (ROOT / "SConstruct").is_file():
        print("Weave: racine invalide", file=sys.stderr)
        return 1
    args = sys.argv[1:]
    if args and args[0] == "flash":
        return flash(args[1:])
    if args and args[0] == "region":
        return region(args[1:])
    if args and args[0] in ("backup", "restore"):
        # Utiliser le Python toolchain pour pyserial.
        helper = ROOT / "site_scons" / "tools" / "flipper_snapshot.py"
        env = os.environ.copy()
        env["PYTHONPYCACHEPREFIX"] = str(USER_CACHE / "pycache")
        USER_CACHE.mkdir(parents=True, exist_ok=True)
        toolchain = ROOT / "site_scons" / "tools" / "toolchain"
        env["FBT_TOOLCHAIN_PATH"] = str(USER_CACHE)
        if os.name == "nt":
            setup = toolchain / "fbtenv.cmd"
            command = subprocess.list2cmdline(["python", str(helper), args[0], *args[1:]])
            launch = ["cmd.exe", "/d", "/s", "/c", f'call "{setup}" env && {command}']
        else:
            setup = toolchain / "fbtenv.sh"
            launch = ["sh", "-c", '. "$1" && shift && exec python3 "$@"',
                      "weave", str(setup), str(helper), args[0], *args[1:]]
        return _run_command(launch, cwd=ROOT, env=env, check=False).returncode
    if args and args[0] in ("dev", "app"):
        if len(args) != 1:
            print("Usage: python3 fbt.py dev | app", file=sys.stderr)
            return 2
        return _run_command([sys.executable, str(ROOT / "site_scons" / "tools" /
                              "launch_companion.py"), args[0]], cwd=ROOT, check=False).returncode
    if args and args[0] == "desktop":
        if len(args) != 1:
            print("Usage: python3 fbt.py desktop", file=sys.stderr)
            return 2
        return _run_command([sys.executable, str(ROOT / "site_scons" / "tools" / "build_all.py"),
                               "--desktop-only"], cwd=ROOT, check=False).returncode
    if args and args[0] in ("doctor", "repair", "fix"):
        return repair(args[1:])
    if args and args[0] in ("clean", "clear"):
        if len(args) != 1:
            print("Usage: python3 fbt.py clean | clear", file=sys.stderr)
            return 2
        return clean()
    if args and args[0] in ("--help-weave", "--help", "-h", "help"):
        print("Usage: python3 fbt.py build [--firmware-only] | sdk | flash [-p PORT] | region [-p PORT] [--dry-run] | dev | app | doctor [--diagnose] | fix | backup [--output PATH] | restore [--input PATH] [--dry-run] | clean | clear")
        print("build attempts firmware, APK and native desktop ZIPs; missing platform builds are reported (GitHub Actions builds all six release assets).")
        return 0
    full_build = bool(args and args[0] == "build")
    firmware_only = full_build and "--firmware-only" in args
    if firmware_only:
        args = [arg for arg in args if arg != "--firmware-only"]
    args = build_args(args)
    # Vérifier les versions avant fetch/build.
    version_check = _run_command([sys.executable, str(ROOT / "site_scons" / "tools" / "check_version.py")], cwd=ROOT, check=False)
    if version_check.returncode:
        return version_check.returncode
    BUILD.mkdir(parents=True, exist_ok=True)
    os.environ["PYTHONPYCACHEPREFIX"] = str(USER_CACHE / "pycache")
    for step in (sync, dependencies):
        result = step()
        if result:
            return result
    result = compile_firmware(args)
    if result:
        return result
    if full_build:
        release_cmd = [sys.executable, str(ROOT / "site_scons" / "tools" / "build_all.py")]
        if firmware_only:
            release_cmd.append("--firmware-only")
        return _run_command(release_cmd, cwd=ROOT, check=False).returncode
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nWeave: interrupted by Ctrl+C; child processes stopped.", file=sys.stderr)
        sys.exit(130)
