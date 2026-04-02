"""
merge_firmware.py — PlatformIO post-build script
Runs after every successful build and merges the four ESP32 binary
parts into a single firmware-merged.bin suitable for:
  • ESP Web Tools (web browser flashing)
  • esptool.py write_flash 0x0
  • Any single-binary flashing workflow

Usage: referenced in platformio.ini as  extra_scripts = post:merge_firmware.py
"""
import subprocess
import sys
import os

Import("env")  # noqa: F821 — PlatformIO injects this

def merge_bins(source, target, env):  # noqa: F841
    build_dir   = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")

    # Paths to the four required binaries
    bootloader  = os.path.join(build_dir, "bootloader.bin")
    partitions  = os.path.join(build_dir, "partitions.bin")
    boot_app0   = os.path.join(
        env.subst("$PIOHOME_DIR"),
        "packages", "framework-arduinoespressif32",
        "tools", "partitions", "boot_app0.bin"
    )
    firmware    = os.path.join(build_dir, "firmware.bin")
    output      = os.path.join(build_dir, "firmware-merged.bin")

    # Also copy to web-flasher/firmware/ for convenience
    web_flasher_dir = os.path.join(project_dir, "web-flasher", "firmware")
    os.makedirs(web_flasher_dir, exist_ok=True)
    output_web = os.path.join(web_flasher_dir, "firmware-merged.bin")

    print("\n" + "─" * 60)
    print("  Merging firmware parts into single binary...")
    print("─" * 60)

    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", "esp32",
        "merge_bin",
        "--output", output,
        "0x1000",  bootloader,
        "0x8000",  partitions,
        "0xe000",  boot_app0,
        "0x10000", firmware,
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            size_kb = os.path.getsize(output) / 1024
            print(f"  ✓ firmware-merged.bin created ({size_kb:.0f} KB)")

            # Copy to web-flasher folder
            import shutil
            shutil.copy2(output, output_web)
            print(f"  ✓ Copied to web-flasher/firmware/firmware-merged.bin")
            print(f"\n  Flash command:")
            print(f"  esptool.py --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 {output}")
        else:
            print(f"  ✗ merge_bin failed:\n{result.stderr}")
    except FileNotFoundError:
        print("  ⚠ esptool not found — skipping merge step")
        print("    Install with:  pip install esptool")

    print("─" * 60 + "\n")

# Register as post-action on the firmware.bin target
env.AddPostAction("$BUILD_DIR/firmware.bin", merge_bins)
