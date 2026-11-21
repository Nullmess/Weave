#!/usr/bin/env python3
"""Fail before publishing artifacts with inconsistent Weave version numbers."""
from __future__ import annotations

import re
from pathlib import Path

root = Path(__file__).resolve().parents[2]
version = (root.parent / "VERSION").read_text().strip()
assert re.fullmatch(r"\d+\.\d+\.\d+", version), "Invalid version"
# Keep release bumps explicit; the 1.0.1 development line is authorized.
if version not in {"1.0.0", "1.0.1"}:
    raise SystemExit("Unsupported Weave release line")
checks = {
    "fbt.py": r'(?m)^DIST_SUFFIX\s*=\s*"([^"]+)"',
    "applications/services/weave/weave_cli.c": r'printf\("Weave ([0-9.]+)',
    "applications/settings/weave_settings/weave_settings.c": r'#define WEAVE_RELEASE "([^"]+)"',
    "../apps/desktop/package.json": r'"version"\s*:\s*"([^"]+)"',
}
for name, pattern in checks.items():
    text = (root / name).read_text()
    match = re.search(pattern, text)
    if not match or match.group(1) != version:
        raise SystemExit(f"Version mismatch in {name}, expected {version}")
print(f"Weave version {version} verified")
