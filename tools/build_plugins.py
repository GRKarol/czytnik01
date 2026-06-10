#!/usr/bin/env python3
"""
Build plugin binaries for ESP32-S3 using the Xtensa toolchain directly.

Compiles each plugin in plugins/*/ into a .bin file ready for deployment.
Output: dist/{plugin_id}-plugin.bin

Usage:
  python tools/build_plugins.py
  python tools/build_plugins.py --plugin focus-timer
"""

import argparse
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PLUGINS_DIR = REPO_ROOT / "plugins"
SDK_DIR = REPO_ROOT / "firmware" / "src" / "plugins" / "sdk"
LINKER_SCRIPT = PLUGINS_DIR / "plugin.ld"
PLUGIN_NEW_H = PLUGINS_DIR / "plugin_new.h"
DIST_DIR = REPO_ROOT / "dist"

# PlatformIO toolchain path
TOOLCHAIN_DIR = Path.home() / ".platformio" / "packages" / "toolchain-xtensa-esp32s3"

PLUGIN_HEADER_MAGIC = 0x504C5547  # "PLUG"


def find_toolchain():
    """Find the Xtensa ESP32-S3 toolchain."""
    gcc = TOOLCHAIN_DIR / "bin" / "xtensa-esp32s3-elf-g++"
    if sys.platform == "win32":
        gcc = gcc.with_suffix(".exe")
    if not gcc.exists():
        # Try alternative locations
        alt = Path.home() / ".platformio" / "packages" / "toolchain-xtensa-esp32s3" / "bin"
        for f in alt.iterdir() if alt.exists() else []:
            if "g++" in f.name:
                return f.parent
        raise FileNotFoundError(
            f"Xtensa toolchain not found at {TOOLCHAIN_DIR}\n"
            "Install PlatformIO and build the firmware once to get the toolchain."
        )
    return gcc.parent


def compile_plugin(plugin_dir, toolchain_bin, output_dir):
    """Compile a single plugin from source files."""
    plugin_id = plugin_dir.name
    src_dir = plugin_dir / "src"
    
    if not src_dir.exists():
        print(f"  SKIP: No src/ directory in {plugin_dir}")
        return None

    # Find all .cpp source files
    sources = list(src_dir.glob("*.cpp"))
    if not sources:
        print(f"  SKIP: No .cpp files in {src_dir}")
        return None

    prefix = "xtensa-esp32s3-elf-"
    ext = ".exe" if sys.platform == "win32" else ""
    gpp = toolchain_bin / f"{prefix}g++{ext}"
    objcopy = toolchain_bin / f"{prefix}objcopy{ext}"
    
    build_dir = plugin_dir / "build"
    build_dir.mkdir(exist_ok=True)

    # Compile flags (C++)
    cxx_flags = [
        "-c",
        "-Os",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-exceptions",
        "-fno-rtti",
        "-fno-threadsafe-statics",
        "-DPLUGIN_BUILD=1",
        f"-I{SDK_DIR}",
        f"-I{PLUGINS_DIR}",  # for plugin_new.h
        f"-I{src_dir}",      # for local headers
        "-std=gnu++17",
        "-mlongcalls",       # Required for Xtensa
    ]

    # Compile flags (C — for runtime)
    c_flags = [
        "-c",
        "-Os",
        "-ffunction-sections",
        "-fdata-sections",
        "-DPLUGIN_BUILD=1",
        "-mlongcalls",
    ]

    # Compile each source file
    objects = []
    for src in sources:
        obj = build_dir / (src.stem + ".o")
        cmd = [str(gpp)] + cxx_flags + [str(src), "-o", str(obj)]
        print(f"  CC {src.name}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"  ERROR compiling {src.name}:")
            print(result.stderr)
            return None
        objects.append(obj)

    # Compile plugin_runtime.c (shared minimal C runtime)
    runtime_src = PLUGINS_DIR / "plugin_runtime.c"
    if runtime_src.exists():
        gcc = toolchain_bin / f"{prefix}gcc{ext}"
        runtime_obj = build_dir / "plugin_runtime.o"
        cmd = [str(gcc)] + c_flags + [str(runtime_src), "-o", str(runtime_obj)]
        print(f"  CC plugin_runtime.c")
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"  ERROR compiling plugin_runtime.c:")
            print(result.stderr)
            return None
        objects.append(runtime_obj)

    # Link — two-step: merge objects into relocatable, then final link with script
    elf_path = build_dir / "plugin.elf"
    map_path = build_dir / "plugin.map"
    merged_path = build_dir / "merged.o"
    
    ld = toolchain_bin / f"{prefix}ld{ext}"
    
    # Step 1: Merge all objects into single relocatable
    rel_objects = [os.path.relpath(o, plugin_dir) for o in objects]
    merge_cmd = f'"{ld}" -r {" ".join(rel_objects)} -o build\\merged.o'
    
    bat_path = build_dir / "_merge.bat"
    bat_path.write_text(f"@echo off\n{merge_cmd}\n", encoding="utf-8")
    result = subprocess.run(["cmd.exe", "/c", str(bat_path)], 
                           capture_output=True, text=True, cwd=str(plugin_dir))
    if result.returncode != 0:
        print(f"  ERROR merging objects:")
        print(f"  {(result.stdout + result.stderr).strip()}")
        return None
    
    if not merged_path.exists() or merged_path.stat().st_size == 0:
        print(f"  ERROR: merged.o not created")
        return None
    print(f"  MERGE merged.o ({merged_path.stat().st_size} bytes)")
    
    # Step 2: Final link with linker script
    rel_linker = os.path.relpath(LINKER_SCRIPT, plugin_dir)
    link_cmd = f'"{ld}" -nostdlib -T{rel_linker} -Map=build\\plugin.map build\\merged.o -o build\\plugin.elf'
    
    bat_path2 = build_dir / "_link.bat"
    bat_path2.write_text(f"@echo off\n{link_cmd}\n", encoding="utf-8")
    result = subprocess.run(["cmd.exe", "/c", str(bat_path2)],
                           capture_output=True, text=True, cwd=str(plugin_dir))
    if result.returncode != 0:
        errors = (result.stdout + result.stderr).strip()
        print(f"  ERROR linking {plugin_id} (exit code {result.returncode}):")
        if errors:
            for line in errors.split("\n")[:10]:
                print(f"    {line}")
        return None
    
    if not elf_path.exists() or elf_path.stat().st_size == 0:
        print(f"  ERROR: plugin.elf not created")
        return None

    # Convert ELF to binary
    bin_path = build_dir / "plugin.bin"
    cmd = [str(objcopy), "-O", "binary", str(elf_path), str(bin_path)]
    print(f"  OBJCOPY plugin.bin")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"  ERROR objcopy {plugin_id}:")
        print(result.stderr)
        return None

    # Patch the binary header
    patch_binary(bin_path, map_path)

    # Copy to output
    output_path = output_dir / f"{plugin_id}-plugin.bin"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    import shutil
    shutil.copy2(bin_path, output_path)
    
    size_kb = output_path.stat().st_size / 1024
    print(f"  OK: {output_path.name} ({size_kb:.1f} KB)")
    return output_path


def patch_binary(bin_path, map_path):
    """Patch PluginBinaryHeader with binarySize and entryOffset."""
    # Parse vtable offset from map
    vtable_offset = parse_vtable_offset(map_path)
    
    with open(bin_path, "rb") as f:
        data = bytearray(f.read())

    if len(data) < 32:
        print(f"  WARNING: Binary too small ({len(data)} bytes)")
        return

    # Check magic
    magic = struct.unpack_from("<I", data, 0)[0]
    if magic != PLUGIN_HEADER_MAGIC:
        print(f"  WARNING: Bad magic 0x{magic:08X} (expected 0x{PLUGIN_HEADER_MAGIC:08X})")
        return

    # Patch binarySize (offset 8) and entryOffset (offset 12)
    struct.pack_into("<I", data, 8, len(data))
    struct.pack_into("<I", data, 12, vtable_offset or 0)

    with open(bin_path, "wb") as f:
        f.write(data)

    print(f"  PATCH: size={len(data)}, vtable=0x{vtable_offset or 0:08X}")


def parse_vtable_offset(map_path):
    """Parse linker map to find .plugin_vtable section offset."""
    if not map_path.exists():
        return None

    content = map_path.read_text(encoding="utf-8", errors="replace")

    # Look for .plugin_vtable section address
    patterns = [
        re.compile(r"^\.plugin_vtable\s+0x([0-9a-fA-F]+)", re.MULTILINE),
        re.compile(r"^\.plugin_vtable\s*\n\s+0x([0-9a-fA-F]+)", re.MULTILINE),
        re.compile(r"0x([0-9a-fA-F]+)\s+_plugin_vtable_start", re.MULTILINE),
    ]

    for pattern in patterns:
        match = pattern.search(content)
        if match:
            offset = int(match.group(1), 16)
            if offset > 0:
                return offset

    return None


def main():
    parser = argparse.ArgumentParser(description="Build ESP32-S3 plugin binaries")
    parser.add_argument("--plugin", help="Build only this plugin (by ID)")
    parser.add_argument("--output", default=str(DIST_DIR), help="Output directory")
    args = parser.parse_args()

    output_dir = Path(args.output)
    
    print("=== Plugin Build System ===")
    print(f"Repo root: {REPO_ROOT}")
    print(f"SDK: {SDK_DIR}")
    print(f"Output: {output_dir}")
    print()

    # Find toolchain
    try:
        toolchain_bin = find_toolchain()
    except FileNotFoundError as e:
        print(f"ERROR: {e}")
        return 1

    print(f"Toolchain: {toolchain_bin}")
    print()

    # Find plugins to build
    if args.plugin:
        plugin_dirs = [PLUGINS_DIR / args.plugin]
        if not plugin_dirs[0].exists():
            print(f"ERROR: Plugin '{args.plugin}' not found at {plugin_dirs[0]}")
            return 1
    else:
        plugin_dirs = sorted([
            d for d in PLUGINS_DIR.iterdir()
            if d.is_dir() and (d / "src").exists() and (d / "manifest.json").exists()
        ])

    if not plugin_dirs:
        print("No plugins found to build.")
        return 0

    # Build each plugin
    results = []
    for plugin_dir in plugin_dirs:
        plugin_id = plugin_dir.name
        print(f"\n{'='*50}")
        print(f"Building: {plugin_id}")
        print(f"{'='*50}")
        
        result = compile_plugin(plugin_dir, toolchain_bin, output_dir)
        results.append((plugin_id, result))

    # Summary
    print(f"\n{'='*50}")
    print("Build Summary:")
    success_count = 0
    for plugin_id, result in results:
        status = "OK" if result else "FAILED"
        if result:
            success_count += 1
        print(f"  {plugin_id}: {status}")

    print(f"\n{success_count}/{len(results)} plugins built successfully")
    return 0 if success_count == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
