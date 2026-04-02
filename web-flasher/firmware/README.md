# web-flasher/firmware/

This folder is populated automatically by the CI build pipeline.

- **GitHub Actions** runs on every push to `main` and creates `firmware-merged.bin`
- The file is then deployed to GitHub Pages together with `index.html` and `manifest.json`
- **Do not commit** `firmware-merged.bin` manually — it is built from source

## Local build

After `pio run`, the merge script runs automatically and places the file here.
You can also build manually:

```bash
esptool.py --chip esp32 merge_bin \
  --output web-flasher/firmware/firmware-merged.bin \
  0x1000  .pio/build/cyd_esp32/bootloader.bin \
  0x8000  .pio/build/cyd_esp32/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/cyd_esp32/firmware.bin
```
