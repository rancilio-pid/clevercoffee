#!/usr/bin/env python3
import json
import os
import shutil
import shlex
import subprocess
import sys
from pathlib import Path


BUILD_ENV = "esp32_usb"
BUILD_DIR = Path(".pio") / "build" / BUILD_ENV
OUTPUT_NAME = "wokwi_flasher_args.json"
MERGED_NAME = "firmware_merged.bin"


FLASH_FILES = {
    "0x1000": "bootloader.bin",
    "0x8000": "partitions.bin",
    "0xe000": "boot_app0.bin",
    "0x10000": "firmware.bin",
    "0x350000": "littlefs.bin",
}


def resolve_boot_app0_path():
    override = os.environ.get("BOOT_APP0_BIN")
    if override and Path(override).exists():
        return Path(override)

    candidate_paths = [
        Path.home()
        / ".platformio"
        / "packages"
        / "framework-arduinoespressif32"
        / "tools"
        / "partitions"
        / "boot_app0.bin",
    ]
    for candidate in candidate_paths:
        if candidate.exists():
            return candidate

    raise FileNotFoundError(
        "Could not find boot_app0.bin. "
        "Set BOOT_APP0_BIN to its full path."
    )


def resolve_pio_command():
    override = os.environ.get("PIO_CMD")
    if override:
        return shlex.split(override)

    try:
        import platformio  # noqa: F401

        return [sys.executable, "-m", "platformio"]
    except Exception:
        pass

    pio_path = shutil.which("pio")
    if pio_path:
        return [pio_path]

    candidate_paths = [
        Path.home() / ".platformio" / "penv" / "bin" / "pio",
        Path.home() / ".platformio" / "penv" / "Scripts" / "pio.exe",
    ]
    for candidate in candidate_paths:
        if candidate.exists():
            return [str(candidate)]

    raise FileNotFoundError(
        "Could not find the PlatformIO 'pio' executable. "
        "Add it to PATH or set PIO_CMD to its full path."
    )


def resolve_esptool_command():
    override = os.environ.get("ESPTOOL_CMD")
    if override:
        return shlex.split(override)

    candidate_paths = [
        Path.home() / ".platformio" / "packages" / "tool-esptoolpy" / "esptool.py",
    ]
    for candidate in candidate_paths:
        if candidate.exists():
            return [sys.executable, str(candidate)]

    esptool_path = shutil.which("esptool.py") or shutil.which("esptool")
    if esptool_path:
        return [esptool_path]

    raise FileNotFoundError(
        "Could not find esptool. "
        "Set ESPTOOL_CMD to its full path."
    )


def run_pio_builds():
    pio = resolve_pio_command()
    subprocess.run(pio + ["run", "-e", BUILD_ENV], check=True)
    subprocess.run(pio + ["run", "-e", BUILD_ENV, "-t", "buildfs"], check=True)


def write_flasher_args():
    output_path = BUILD_DIR / OUTPUT_NAME
    boot_app0_dest = BUILD_DIR / "boot_app0.bin"
    if not boot_app0_dest.exists():
        boot_app0_src = resolve_boot_app0_path()
        boot_app0_dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(boot_app0_src, boot_app0_dest)

    flasher_args = {
        "flash_settings": {
            "flash_mode": "dio",
            "flash_size": "4MB",
            "flash_freq": "40m",
        },
        "flash_files": FLASH_FILES,
    }
    output_path.write_text(json.dumps(flasher_args, indent=2) + "\n")
    print(f"Wrote {output_path}")


def write_merged_firmware():
    merged_path = BUILD_DIR / MERGED_NAME
    esptool = resolve_esptool_command()
    flash_args = []
    for offset, filename in FLASH_FILES.items():
        flash_args.extend([offset, str(BUILD_DIR / filename)])

    subprocess.run(
        esptool
        + [
            "--chip",
            "esp32",
            "merge_bin",
            "--flash_mode",
            "dio",
            "--flash_freq",
            "40m",
            "--flash_size",
            "4MB",
            "-o",
            str(merged_path),
        ]
        + flash_args,
        check=True,
    )
    print(f"Wrote {merged_path}")


def main():
    run_pio_builds()
    write_flasher_args()
    write_merged_firmware()


if __name__ == "__main__":
    main()
