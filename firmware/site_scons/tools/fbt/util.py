import os
import re
import subprocess
import sys
import webbrowser
from pathlib import Path, PurePosixPath

import SCons
from SCons.Errors import StopError
from SCons.Subst import quote_spaces

WINPATHSEP_RE = re.compile(r"\\([^\"'\\]|$)")


# Suffixe ~ exclu : backups éditeurs
GLOB_FILE_EXCLUSION = ["*~"]

# Variables système transmises aux processus enfants
FORWARDED_ENV_VARIABLES = [
    # Variables CI/CD
    "WORKFLOW_BRANCH_OR_TAG",
    "DIST_SUFFIX",
    # Python et outils
    "HOME",
    "APPDATA",
    "PYTHONHOME",
    "PYTHONNOUSERSITE",
    "TMP",
    "TEMP",
    "USERPROFILE",
    "LOCALAPPDATA",
    # ccache
    "CCACHE_DISABLE",
    # Couleurs outils
    "TERM",

    "FBT_TOOLCHAIN_PATH",
    "UFBT_HOME",
]


def tempfile_arg_esc_func(arg):
    arg = quote_spaces(arg)
    if SCons.Platform.platform_default() != "win32":
        return arg
    # GCC Windows exige slashs doublés ; séparateur Unix utilisé
    return WINPATHSEP_RE.sub(r"/\1", arg)


def wrap_tempfile(env, command):
    env[command] = '${TEMPFILE("' + env[command] + '","$' + command + 'STR")}'


def link_dir(target_path, source_path, is_windows):

    if os.path.lexists(target_path) or os.path.exists(target_path):
        os.unlink(target_path)
    if is_windows:

        import _winapi

        if not os.path.isdir(source_path):
            raise StopError(f"Source directory {source_path} is not a directory")

        if not os.path.exists(target_path):
            _winapi.CreateJunction(source_path, target_path)
    else:
        os.symlink(source_path, target_path)


def single_quote(arg_list):
    return " ".join(f"'{arg}'" if " " in arg else str(arg) for arg in arg_list)


def resolve_real_dir_node(node):
    if isinstance(node, SCons.Node.FS.EntryProxy):
        node = node.get()

    for repo_dir in node.get_all_rdirs():
        if os.path.exists(repo_dir.abspath):
            return repo_dir

    raise StopError(f"Can't find absolute path for {node.name} ({node})")


class PosixPathWrapper:
    def __init__(self, pathobj):
        self.pathobj = pathobj

    @staticmethod
    def fixup_separators(path):
        if SCons.Platform.platform_default() == "win32":
            return path.replace(os.path.sep, os.path.altsep)
        return path

    @staticmethod
    def fix_path(path):
        return str(PurePosixPath(Path(path).as_posix()))

    def __call__(self, target, source, env, for_signature):
        if for_signature:
            return self.pathobj

        return self.fix_path(env.subst(self.pathobj))


def open_browser_action(target, source, env):
    if sys.platform == "darwin":
        subprocess.run(["open", source[0].abspath])
    else:
        webbrowser.open(source[0].abspath)
