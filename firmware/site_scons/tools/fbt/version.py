import datetime
import subprocess
import os
import time
from pathlib import Path
from functools import cache


@cache
def get_git_commit_unix_timestamp():
    if not Path(".git").exists():
        return int(os.environ.get("SOURCE_DATE_EPOCH", time.time()))
    return int(subprocess.check_output(["git", "show", "-s", "--format=%ct"]))


@cache
def get_fast_git_version_id():
    if not Path(".git").exists():
        return None
    try:
        version = (
            subprocess.check_output(
                [
                    "git",
                    "describe",
                    "--always",
                    "--dirty",
                    "--all",
                    "--long",
                ]
            )
            .strip()
            .decode()
        )
        return (version, datetime.date.today())
    except Exception as e:
        print("Failed to check for git changes", e)
