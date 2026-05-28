# 1704165 Bootloader

GPL v2 licensed standalone bootloader for the 1704165 Fan Controller (STM32H563VI).

Uses **CycloneBOOT Open** (Oryx Embedded) for dual-bank firmware update with
automatic fallback after 3 failed boot attempts.

## Flash Layout

| Region | Address Range | Size | Content |
|--------|--------------|------|---------|
| Bootloader | `0x08000000 - 0x0800FFFF` | 64 KB | This binary (GPL) |
| Application | `0x08010000 - 0x080FDFFF` | 952 KB | Proprietary firmware |
| Config | `0x080FE000 - 0x080FFFFF` | 8 KB | Persistent settings |
| Bank 2 | `0x08100000 - 0x081FFFFF` | 1 MB | Update staging slot |

## Building

```bash
cmake -GNinja -DCMAKE_TOOLCHAIN_FILE=path/to/gcc_toolchain.cmake -B build
ninja -C build
```

## Flashing

```
JLink -device STM32H563VI -if SWD -speed 4000 -autoconnect 1
loadbin build/bootloader.bin, 0x08000000
```

## License

This bootloader is licensed under GPL v2. See [LICENSE](LICENSE).

CycloneBOOT Open and CycloneCRYPTO are Copyright (c) Oryx Embedded SARL,
licensed under GPL v2.
