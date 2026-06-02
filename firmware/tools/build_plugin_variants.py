#!/usr/bin/env python3
"""
build_plugin_variants.py — buduje wszystkie warianty firmware z pluginami
i generuje plugins.json manifest.

Warianty:
  variant_base        → flower-firmware.bin          (brak pluginów)
  variant_timer       → flower-firmware-timer.bin    (timer)
  variant_rss         → flower-firmware-rss.bin      (rss)
  variant_timer_rss   → flower-firmware-timer-rss.bin (timer + rss)

plugins.json jest uploadowany razem z .bin do GitHub Release
i pobierany przez urządzenie przy wyświetlaniu listy pluginów.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]            # firmware/
REPO_ROOT = ROOT.parent                                # czytnik01/
OUTPUT_DIR = REPO_ROOT / "public" / "firmware"        # public/firmware/

# Definicja wszystkich pluginów — identyczna kolejność jak enum PluginId w C++
PLUGINS = [
    {
        "id": "timer",
        "bit": 0,
        "name": "Focus Timer",
        "name_pl": "Klepsydra",
        "description": "Reading session timer",
        "description_pl": "Timer sesji czytania",
    },
    {
        "id": "rss",
        "bit": 1,
        "name": "RSS feeds",
        "name_pl": "Kanaly RSS",
        "description": "Article downloads",
        "description_pl": "Pobieranie artykulow",
    },
]

# Mapowanie bitmask → (env name, output filename)
# Bitmask = OR po bitach aktywnych pluginów
VARIANTS = [
    {"mask": 0b00, "env": "variant_base",      "file": "flower-firmware.bin"},
    {"mask": 0b01, "env": "variant_timer",     "file": "flower-firmware-timer.bin"},
    {"mask": 0b10, "env": "variant_rss",       "file": "flower-firmware-rss.bin"},
    {"mask": 0b11, "env": "variant_timer_rss", "file": "flower-firmware-timer-rss.bin"},
]


def run(command: list[str], version: str | None = None) -> None:
    print("+", " ".join(command))
    env = os.environ.copy()
    env.setdefault("PLATFORMIO_SETTING_ENABLE_TELEMETRY", "No")
    if version:
        env["RSVP_FIRMWARE_VERSION"] = version
    subprocess.run(command, cwd=ROOT, check=True, env=env)


def pio_command() -> str:
    local = Path.home() / ".platformio" / "penv" / "bin" / "pio"
    if local.exists():
        return str(local)
    found = shutil.which("pio")
    if found:
        return found
    raise SystemExit("PlatformIO Core not found. Install it or activate the PlatformIO env.")


def git_version() -> str:
    try:
        value = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty"],
            cwd=ROOT, text=True,
        ).strip()
        return value or "dev"
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "dev"


def export_ota_binary(env: str, output: Path) -> int:
    firmware_path = ROOT / ".pio" / "build" / env / "firmware.bin"
    if not firmware_path.exists():
        raise SystemExit(f"Missing OTA binary for env={env}: {firmware_path}")
    output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(firmware_path, output)
    size_kb = output.stat().st_size // 1024
    print(f"  Exported {output.name} ({size_kb} KB)")
    return size_kb


def mask_to_plugin_ids(mask: int) -> list[str]:
    return [p["id"] for p in PLUGINS if (mask >> p["bit"]) & 1]


def build_plugins_json(version: str, variants_with_sizes: list[dict]) -> dict:
    return {
        "version": 1,
        "firmware_version": version,
        "plugins": [
            {
                "id": p["id"],
                "name": p["name"],
                "name_pl": p["name_pl"],
                "description": p["description"],
                "description_pl": p["description_pl"],
            }
            for p in PLUGINS
        ],
        "variants": variants_with_sizes,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build all plugin variants and generate plugins.json manifest."
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Use existing .pio build outputs instead of running PlatformIO.",
    )
    parser.add_argument(
        "--version",
        default=git_version(),
        help="Version string for firmware (default: git describe).",
    )
    parser.add_argument(
        "--variants",
        nargs="*",
        default=None,
        help="Subset of variant env names to build (default: all).",
    )
    args = parser.parse_args()

    pio = pio_command()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    filter_envs = set(args.variants) if args.variants else None
    variants_with_sizes = []

    for variant in VARIANTS:
        env = variant["env"]
        if filter_envs and env not in filter_envs:
            continue

        output_path = OUTPUT_DIR / variant["file"]

        if not args.skip_build:
            print(f"\n{'='*60}")
            print(f"Building env={env} -> {variant['file']}")
            print(f"{'='*60}")
            run([pio, "run", "-e", env], args.version)

        size_kb = export_ota_binary(env, output_path)

        plugin_ids = mask_to_plugin_ids(variant["mask"])
        variants_with_sizes.append({
            "mask": variant["mask"],
            "plugins": plugin_ids,
            "file": variant["file"],
            "size_kb": size_kb,
        })

    # Generuj plugins.json
    plugins_json = build_plugins_json(args.version, variants_with_sizes)
    plugins_json_path = OUTPUT_DIR / "plugins.json"
    plugins_json_path.write_text(json.dumps(plugins_json, indent=2, ensure_ascii=False) + "\n")
    print(f"\nGenerated {plugins_json_path}")
    print(json.dumps(plugins_json, indent=2, ensure_ascii=False))

    print(f"\nAll plugin variants exported to {OUTPUT_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
