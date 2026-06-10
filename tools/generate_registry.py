#!/usr/bin/env python3
"""
Generate plugins-registry.json from plugin manifest files.

Reads all plugins/*/manifest.json files and produces a registry JSON
with download URLs based on a GitHub release tag.

Usage:
  python tools/generate_registry.py --tag v1.0.0
  python tools/generate_registry.py --tag v1.0.0 --output dist/plugins-registry.json
"""

import argparse
import json
import os
import sys
from pathlib import Path

GITHUB_REPO = "GRKarol/czytnik01"
REGISTRY_VERSION = 1
DEFAULT_MIN_FIRMWARE_VERSION = "1.0.0"


def find_plugin_manifests(plugins_dir):
    """Find all manifest.json files in plugins/*/ directories."""
    manifests = []
    plugins_path = Path(plugins_dir)

    if not plugins_path.is_dir():
        return manifests

    for entry in sorted(plugins_path.iterdir()):
        if not entry.is_dir():
            continue
        manifest_path = entry / "manifest.json"
        if manifest_path.is_file():
            manifests.append(manifest_path)

    return manifests


def parse_manifest(manifest_path):
    """Parse a manifest.json file and return its contents."""
    with open(manifest_path, "r", encoding="utf-8") as f:
        return json.load(f)


def validate_manifest(manifest, manifest_path):
    """Validate that a manifest has all required fields."""
    required_fields = ["id", "name", "version", "author", "sdk_version", "description"]
    missing = [field for field in required_fields if field not in manifest]

    if missing:
        print(
            f"WARNING: {manifest_path} missing fields: {', '.join(missing)} — skipping",
            file=sys.stderr,
        )
        return False

    return True


def build_registry_entry(manifest, tag):
    """Build a registry entry from a manifest and release tag."""
    plugin_id = manifest["id"]
    base_url = f"https://github.com/{GITHUB_REPO}/releases/download/{tag}"

    return {
        "id": plugin_id,
        "name": manifest["name"],
        "description": manifest.get("description", ""),
        "version": manifest["version"],
        "author": manifest["author"],
        "sdk_version": manifest["sdk_version"],
        "binary_url": f"{base_url}/{plugin_id}-plugin.bin",
        "manifest_url": f"{base_url}/{plugin_id}-manifest.json",
        "size_bytes": 0,
        "min_firmware_version": DEFAULT_MIN_FIRMWARE_VERSION,
    }


def generate_registry(plugins_dir, tag):
    """Generate the full registry structure."""
    manifests = find_plugin_manifests(plugins_dir)

    if not manifests:
        print(f"WARNING: No plugin manifests found in {plugins_dir}/*/", file=sys.stderr)

    plugins = []
    for manifest_path in manifests:
        manifest = parse_manifest(manifest_path)
        if validate_manifest(manifest, manifest_path):
            entry = build_registry_entry(manifest, tag)
            plugins.append(entry)

    registry = {
        "registry_version": REGISTRY_VERSION,
        "plugins": plugins,
    }

    return registry


def main():
    parser = argparse.ArgumentParser(
        description="Generate plugins-registry.json from plugin manifests"
    )
    parser.add_argument(
        "--tag",
        required=True,
        help="GitHub release tag (e.g., v1.0.0)",
    )
    parser.add_argument(
        "--output",
        default="dist/plugins-registry.json",
        help="Output path for registry JSON (default: dist/plugins-registry.json)",
    )
    parser.add_argument(
        "--plugins-dir",
        default="plugins",
        help="Path to plugins directory (default: plugins)",
    )
    args = parser.parse_args()

    # Resolve plugins directory relative to script location or CWD
    plugins_dir = args.plugins_dir
    if not os.path.isabs(plugins_dir):
        # Try relative to CWD first
        if not os.path.isdir(plugins_dir):
            # Try relative to repo root (one level up from tools/)
            script_dir = Path(__file__).parent
            repo_root = script_dir.parent
            plugins_dir = str(repo_root / plugins_dir)

    registry = generate_registry(plugins_dir, args.tag)

    # Ensure output directory exists
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Write registry JSON
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(registry, f, indent=2, ensure_ascii=False)
        f.write("\n")

    print(f"Generated {output_path} with {len(registry['plugins'])} plugin(s)")
    for plugin in registry["plugins"]:
        print(f"  - {plugin['id']} v{plugin['version']}")


if __name__ == "__main__":
    main()
