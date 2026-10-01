Import("env")

import os
import subprocess
from pathlib import Path


PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))


def detect_version() -> str:
    override = os.environ.get("RSVP_FIRMWARE_VERSION", "").strip()
    if override:
        return override

    try:
        value = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty"],
            cwd=PROJECT_DIR,
            text=True,
        ).strip()
        return value or "dev"
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "dev"


version = detect_version().replace('"', "")
env.Append(CPPDEFINES=[("RSVP_FIRMWARE_VERSION", '\\"%s\\"' % version)])

# Release repo the firmware takes updates, fonts and starter books from.
# CI sets it to the repository being released (czytnik01 or
# czytnik01-staging), so a staging build keeps testing against staging.
ota_repo = os.environ.get("RSVP_OTA_REPO", "").strip().replace('"', "")
if ota_repo:
    env.Append(CPPDEFINES=[("RSVP_OTA_DEFAULT_REPO", '\\"%s\\"' % ota_repo)])
