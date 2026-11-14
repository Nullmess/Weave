import os
import shutil

from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Errors import StopError
from SCons.Node.FS import Dir, File


WEAVE_DEFAULT_FAPS = frozenset({
    "bad_usb", "gpio", "ibutton", "infrared", "lfrfid", "nfc", "subghz", "u2f",
    "hid_usb", "hid_ble", "snake_game",
})


def __generate_resources_dist_entries(env):
    src_target_entries = []

    resources_root = env.Dir(env["RESOURCES_ROOT"])

    for app_artifacts in env["FW_EXTAPPS"].application_map.values():
        for _, dist_path in filter(
            lambda dist_entry: dist_entry[0], app_artifacts.dist_entries
        ):
            src_target_entries.append(
                (
                    app_artifacts.compact,
                    resources_root.File(dist_path),
                )
            )
            # Backup offline des FAP natives, intégré au bundle de MAJ.
            if app_artifacts.app.appid in WEAVE_DEFAULT_FAPS and dist_path.startswith("apps/") and dist_path.endswith(".fap"):
                src_target_entries.append(
                    (app_artifacts.compact, resources_root.File(".weave/default-faps/" + dist_path))
                )

    # Déploiement des ressources applications
    for app in env["APPBUILD"].apps:
        if not app.resources:
            continue
        apps_resource_dir = app._appdir.Dir(app.resources)
        for res_file in env.GlobRecursive("*", apps_resource_dir):
            if not isinstance(res_file, File):
                continue
            src_target_entries.append(
                (
                    res_file,
                    resources_root.File(
                        res_file.get_path(apps_resource_dir),
                    ),
                )
            )

    # Déploiement des éléments _EXTRA_DIST
    for extra_dist in env["_EXTRA_DIST"]:
        if isinstance(extra_dist, Dir):
            src_target_entries.append(
                (
                    extra_dist,
                    resources_root.Dir(extra_dist.name),
                )
            )
        else:
            raise StopError(f"Unsupported extra dist type: {type(extra_dist)}")

    return src_target_entries


def _resources_dist_emitter(target, source, env):
    src_target_entries = __generate_resources_dist_entries(env)
    source = list(map(lambda entry: entry[0], src_target_entries))
    return (target, source)


def _resources_dist_action(target, source, env):
    dist_entries = __generate_resources_dist_entries(env)
    assert len(dist_entries) == len(source)
    shutil.rmtree(env.Dir(env["RESOURCES_ROOT"]).abspath, ignore_errors=True)
    for src, target in dist_entries:
        if isinstance(src, File):
            os.makedirs(os.path.dirname(target.path), exist_ok=True)
            shutil.copy(src.path, target.path)
        elif isinstance(src, Dir):
            # Fusionner les ressources partagées sans perdre les backups FAP.
            shutil.copytree(src.path, target.path, dirs_exist_ok=True)
        else:
            raise StopError(f"Unsupported dist entry type: {type(src)}")


def generate(env, **kw):
    env.SetDefault(
        ASSETS_COMPILER="${FBT_SCRIPT_DIR}/assets.py",
    )

    if not env["VERBOSE"]:
        env.SetDefault(
            RESOURCEDISTCOMSTR="\tRESDIST\t${RESOURCES_ROOT}",
            RESMANIFESTCOMSTR="\tMANIFST\t${TARGET}",
        )

    env.Append(
        BUILDERS={
            "ManifestBuilder": Builder(
                action=[
                    Action(
                        _resources_dist_action,
                        "${RESOURCEDISTCOMSTR}",
                    ),
                    Action(
                        [
                            [
                                "${PYTHON3}",
                                "${ASSETS_COMPILER}",
                                "manifest",
                                "${TARGET.dir.posix}",
                                "--timestamp=${GIT_UNIX_TIMESTAMP}",
                            ]
                        ],
                        "${RESMANIFESTCOMSTR}",
                    ),
                ],
                emitter=_resources_dist_emitter,
            ),
        }
    )


def exists(env):
    return True
