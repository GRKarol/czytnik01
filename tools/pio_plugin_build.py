"""
PlatformIO post-build script for plugin binaries.

Patches PluginBinaryHeader in the output .bin file:
  - byte 8-11  (binarySize):  total file size as uint32_t LE
  - byte 12-15 (entryOffset): offset of .plugin_vtable section as uint32_t LE

Header layout (32 bytes total):
  magic(4) + sdkVersion(4) + binarySize(4) + entryOffset(4) + reserved(16)

Usage in platformio.ini:
  extra_scripts = post:../../tools/pio_plugin_build.py
"""

import os
import re
import struct

Import("env")  # noqa: F821 — PlatformIO injects this


def find_map_file(build_dir):
    """Locate the linker map file in the build directory."""
    map_file = os.path.join(build_dir, "firmware.map")
    if os.path.isfile(map_file):
        return map_file
    # Fallback: search for any .map file
    for f in os.listdir(build_dir):
        if f.endswith(".map"):
            return os.path.join(build_dir, f)
    return None


def parse_vtable_offset_from_map(map_path):
    """
    Parse the linker map file to find the load address (offset) of the
    .plugin_vtable section or the _plugin_vtable_start symbol.
    
    Returns the offset as an integer, or None if not found.
    """
    vtable_offset = None

    with open(map_path, "r", encoding="utf-8", errors="replace") as f:
        content = f.read()

    # Strategy 1: Look for .plugin_vtable section address in the memory map.
    # Typical format in GNU ld map:
    #   .plugin_vtable
    #                0x00001234       0x1c ...
    # or on one line:
    #   .plugin_vtable  0x00001234       0x1c ...
    pattern_section = re.compile(
        r"^\.plugin_vtable\s+(?:0x)?([0-9a-fA-F]+)",
        re.MULTILINE,
    )
    match = pattern_section.search(content)
    if match:
        vtable_offset = int(match.group(1), 16)
        if vtable_offset > 0:
            return vtable_offset

    # Sometimes the address is on the next line after the section name
    pattern_section_multiline = re.compile(
        r"^\.plugin_vtable\s*\n\s+(?:0x)?([0-9a-fA-F]+)",
        re.MULTILINE,
    )
    match = pattern_section_multiline.search(content)
    if match:
        vtable_offset = int(match.group(1), 16)
        if vtable_offset > 0:
            return vtable_offset

    # Strategy 2: Look for _plugin_vtable_start symbol
    # Format: 0x00001234  _plugin_vtable_start
    pattern_symbol = re.compile(
        r"(?:0x)?([0-9a-fA-F]+)\s+_plugin_vtable_start",
        re.MULTILINE,
    )
    match = pattern_symbol.search(content)
    if match:
        vtable_offset = int(match.group(1), 16)
        if vtable_offset > 0:
            return vtable_offset

    # Strategy 3: Look for KEEP(*(.plugin_vtable)) output address
    pattern_keep = re.compile(
        r"\*\(\.plugin_vtable\)\s*\n\s+\.plugin_vtable\s+(?:0x)?([0-9a-fA-F]+)",
        re.MULTILINE,
    )
    match = pattern_keep.search(content)
    if match:
        vtable_offset = int(match.group(1), 16)
        if vtable_offset > 0:
            return vtable_offset

    return vtable_offset


def patch_plugin_binary(source, target, env):
    """
    Post-build action: patch PluginBinaryHeader with binarySize and entryOffset.
    """
    # Get the output binary path
    bin_path = str(target[0])

    # PlatformIO may give us .elf — we need the .bin
    if bin_path.endswith(".elf"):
        bin_path = bin_path.replace(".elf", ".bin")

    if not os.path.isfile(bin_path):
        print(f"[plugin-build] WARNING: Binary not found at {bin_path}")
        return

    build_dir = env.subst("$BUILD_DIR")
    map_path = find_map_file(build_dir)

    if not map_path:
        print(f"[plugin-build] WARNING: Map file not found in {build_dir}")
        print("[plugin-build] Skipping header patch — run with -Wl,-Map,firmware.map")
        return

    print(f"[plugin-build] Patching plugin binary: {bin_path}")
    print(f"[plugin-build] Using map file: {map_path}")

    # Parse vtable offset from map
    vtable_offset = parse_vtable_offset_from_map(map_path)

    if vtable_offset is None or vtable_offset == 0:
        print("[plugin-build] WARNING: Could not find .plugin_vtable offset in map")
        print("[plugin-build] Header entryOffset will be set to 0")
        vtable_offset = 0

    # Read binary
    with open(bin_path, "rb") as f:
        data = bytearray(f.read())

    binary_size = len(data)

    # Validate: check magic at offset 0
    if len(data) < 32:
        print(f"[plugin-build] ERROR: Binary too small ({binary_size} bytes), expected >= 32")
        return

    magic = struct.unpack_from("<I", data, 0)[0]
    expected_magic = 0x504C5547  # "PLUG"

    if magic != expected_magic:
        print(
            f"[plugin-build] ERROR: Invalid magic 0x{magic:08X}, "
            f"expected 0x{expected_magic:08X} ('PLUG')"
        )
        return

    # Patch binarySize at offset 8
    struct.pack_into("<I", data, 8, binary_size)

    # Patch entryOffset at offset 12
    struct.pack_into("<I", data, 12, vtable_offset)

    # Write patched binary
    with open(bin_path, "wb") as f:
        f.write(data)

    print(f"[plugin-build] Patched: binarySize={binary_size}, entryOffset=0x{vtable_offset:08X}")


# Register post-build action
env.AddPostAction("$BUILD_DIR/firmware.bin", patch_plugin_binary)  # noqa: F821
