
# create_flash_package.py
import os
import shutil
from pathlib import Path

def create_flash_package():
    """Create ESP Flash Download Tool package"""

    # Build first
    print("Building firmware...")
    os.system("~/.platformio/penv/bin/pio run -e esp32_usb")
    os.system("~/.platformio/penv/bin/pio run --target buildfs -e esp32_usb")

    # Paths
    build_dir = Path(".pio/build/esp32_usb")
    release_dir = Path("CleverCoffee_Flash_Package")

    # Create release directory
    release_dir.mkdir(exist_ok=True)

    # Files with addresses from your partitions_4M.csv
    files = {
        "bootloader.bin": "0x1000",
        "partitions.bin": "0x8000",
        "firmware.bin": "0x10000",
        "littlefs.bin": "0x310000"
    }

    # Copy files
    for filename, address in files.items():
        src = build_dir / filename
        if src.exists():
            shutil.copy2(src, release_dir / filename)
            print(f"Copied {filename} -> {address}")
        else:
            print(f"Missing: {filename}")

    # Create flash addresses file
    with open(release_dir / "flash_addresses.txt", "w") as f:
        f.write("ESP Flash Download Tool Settings:\n")
        f.write("==================================\n\n")
        f.write("File Name          Address\n")
        f.write("------------------------\n")
        for filename, address in files.items():
            f.write(f"{filename:<18} {address}\n")
        f.write("\nFlash Settings:\n")
        f.write("- SPI Speed: 40MHz\n")
        f.write("- SPI Mode: DIO\n")
        f.write("- Flash Size: 4MB\n")

    print(f"Flash package ready: {release_dir}")

if __name__ == "__main__":
    create_flash_package()
