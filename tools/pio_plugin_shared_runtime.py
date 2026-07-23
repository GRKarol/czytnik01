"""
PlatformIO pre-build script for plugin binaries.

plugin_runtime.cpp (shared linker stubs: placement-new fallbacks, __cxa_*,
setup()/loop() no-ops — see the file itself for why) lives in plugins/,
one level above each individual plugin's own project dir. PlatformIO's
build_src_filter can't reach outside src_dir with a "../" pattern, so it
has to be added explicitly as an extra build source here instead.

Usage in platformio.ini:
  extra_scripts = pre:../../tools/pio_plugin_shared_runtime.py
"""

import os

Import("env")  # noqa: F821 — PlatformIO injects this

plugins_dir = os.path.abspath(os.path.join(env.subst("$PROJECT_DIR"), ".."))

env.Append(
    PIOBUILDFILES=env.BuildSources(
        os.path.join("$BUILD_DIR", "plugin_shared"),
        plugins_dir,
        src_filter="+<plugin_runtime.cpp>",
    )
)
