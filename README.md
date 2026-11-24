# esp32p4c5_bsp

Board Support Package for the KSDIY ESP32-P4/C5 4.3" board (ESP32-P4 rev v3.2, ESP-IDF v6.1).

This is an independent BSP written to the [esp-bsp](https://github.com/espressif/esp-bsp) conventions. It is not the vendor BSP; the vendor material it was built from is listed under [References](#references).

## Layout

- `bsp/esp32p4c5_bsp/`: the BSP component (display, touch, audio, buttons, IMU, microSD, camera, Wi-Fi through the ESP32-C5, PMIC, power domains, LVGL adapter).
- `examples/app_test/`: hardware test app with a PASS / FAIL result per part; each `main/item_*.c` file shows how to use one part of the BSP.
- `examples/phone/`: ESP-Brookesia phone UI with settings, lock screen and volume keys.

The BSP follows the esp-bsp API, so the generic esp-bsp examples (display, touch, audio, SD card) can also be built against it.

## Build an example

```bash
source ~/.espressif/tools/activate_idf_v6.1.sh
cd examples/app_test
idf.py set-target esp32p4
idf.py build
```

Each example pulls the BSP from `bsp/esp32p4c5_bsp` through `main/idf_component.yml` (`override_path`), following the esp-bsp example layout.

## API

- Display: `bsp_display_start()`, `bsp_display_lock()`, `bsp_display_backlight_on()`.
- Touch: `bsp_touch_new()`.
- Audio: `bsp_audio_init()`, `bsp_audio_codec_speaker_init()`, `bsp_audio_codec_microphone_init()` (esp_codec_dev handles).
  The microphone runs MIC1, MIC2 and **MIC3 = the speaker signal looped back in hardware (echo reference)**: open it with
  `BSP_MIC_CHANNELS` x `BSP_MIC_BITS` (2 x 32 bit) and read frames of 4 int16, reference at `BSP_MIC_SLOT_REF` (0), microphones at
  `BSP_MIC_SLOT_MIC1` (1) and `BSP_MIC_SLOT_MIC2` (3). While it is open the speaker must use the same 2 x 32 bit frame.
  Measurements and wiring: `docs/hw_specs/hw_matrix.md`, section 1.4b.
- Buttons: `bsp_iot_button_create()` for BOOT (GPIO35) and IO0 (GPIO0), used with the espressif/button API.
- IMU: `bsp_imu_init()`, `bsp_imu_read()`.
- microSD: `bsp_sdcard_mount()`, `bsp_sdcard_unmount()` (never formats the card); `bsp_sdcard_raw_init()`, `bsp_sdcard_raw_deinit()` for raw sector access without a filesystem (a USB mass storage drive, for example). The SDMMC host is shared with the SDIO link of the ESP32-C5 (slot 1): the raw deinit releases slot 0 only.
- Camera: `bsp_camera_start()`, `bsp_camera_stop()` (esp_video, V4L2 device).
- Wi-Fi: `bsp_wifi_init()` (station mode through the ESP32-C5, ESP-Hosted).
- Battery and power key: `bsp_battery_get_info()`, `bsp_pmic_event_register()`, `bsp_power_off()`.
- Power: `bsp_power_set(bsp_power_domain_t, bool)` for `BSP_POWER_AUDIO`, `BSP_POWER_IMU`, `BSP_POWER_CAMERA`, `BSP_POWER_BOOST_5V`, `BSP_POWER_MODEM`.

The GPIO-switched domains are active high and off after reset. Power domains are not initialized automatically at boot.

## ESP32-P4 cache workaround

Since v0.2.1 the BSP includes a fix for a boot crash of the ESP32-P4 (chip rev v3.x) on ESP-IDF v6.1. Apps that use the BSP get it automatically.

### Symptom

At boot, before or just after `app_main()`, the chip panics with a `Store access fault`. The backtrace ends in the ROM `memset` called from `prvInitialiseNewTask` (FreeRTOS clearing a new task stack). Whether it happens depends on the memory layout of the build: adding or removing unrelated code or data can make it appear or go away. In the phone example, an affected layout crashed in 2 to 8 boots out of 8.

### Cause

`cache_hal_init()` in ESP-IDF v6.1 sets the cache preload strategy (`undef_op`) of every cache level to `CACHE_LL_PRELOAD_ARBITRARY` (2, random arbitration). On the ESP32-P4 this is a hardware livelock in the L1 D-cache when one core spins on a cached address while the other core takes a cache miss. At boot, CPU1 spins in this way while it waits for the scheduler on CPU0, so CPU0's memory accesses can fail. Espressif confirmed this in [esp-idf#19369](https://github.com/espressif/esp-idf/issues/19369).

### Fix

As Espressif advised in that issue, only the L1 D-cache is set back to round-robin (`CACHE_LL_PRELOAD_UNTIL_FETCH_DONE`, 0). The L1 I-cache and the L2 cache keep the value 2 that IDF sets, so their preload performance is unchanged.

The fix is in [`bsp/esp32p4c5_bsp/src/esp32p4c5_cache_fix.c`](bsp/esp32p4c5_bsp/src/esp32p4c5_cache_fix.c):

```c
__attribute__((constructor(101))) void esp32p4c5_bsp_l1_dcache_round_robin(void)
{
    cache_ll_preload_set_strategy(1, CACHE_TYPE_DATA, CACHE_LL_ID_ALL, CACHE_LL_PRELOAD_UNTIL_FETCH_DONE);
}
```

- It is a global constructor with priority 101, the earliest priority for application code. It runs during startup, before the other constructors, before the FreeRTOS scheduler starts and before `app_main()`.
- It is compiled only for the ESP32-P4 when the minimum supported chip revision is v3.0 or later (`HAL_CONFIG(CHIP_SUPPORT_MIN_REV) >= 300`, from `CONFIG_ESP_REV_MIN_FULL`). On older revisions IDF does not change this register either. Otherwise the file only defines an empty function with the same name.
- Nothing in an app calls this function, so the linker would normally drop the object file from `libesp32p4c5_bsp.a`. The component `CMakeLists.txt` adds `-u esp32p4c5_bsp_l1_dcache_round_robin` to the link of every app that depends on the BSP, which keeps the file and its `.init_array` entry.

With the fix, the same phone layout booted 22 times out of 22 without a panic.

### Check that an app has it

After a build, the symbol must be in the ELF:

```bash
riscv32-esp-elf-nm build/<app>.elf | grep esp32p4c5_bsp_l1_dcache_round_robin
```

Its address must also appear in the `.flash.init_array` section (`riscv32-esp-elf-objdump -s -j .flash.init_array build/<app>.elf`, little endian).

### Notes

- Apps that already set the L1 D-cache to round-robin in their own constructor (for example the `esp32p4c5_phone` app, which carries it while it uses BSP v0.2.0) can keep it: writing the same value twice is harmless. It can be removed once the app uses this BSP version.
- Remove this workaround once ESP-IDF ships a fix for esp-idf#19369 in `cache_hal_init()`.

## Documentation

- [docs/hw_specs/hw_matrix.md](docs/hw_specs/hw_matrix.md): hardware matrix (parts, interfaces, pins, I2C map) and open questions.
- [docs/hw_specs/schematic.pdf](docs/hw_specs/schematic.pdf): board schematic.
- [docs/hw_specs/ESP32-P4C5-Core_module_datasheet.pdf](docs/hw_specs/ESP32-P4C5-Core_module_datasheet.pdf): datasheet of the ESP32-P4C5-Core module (ESP32-P4 + ESP32-C5, pinout, P4 to C5 connections, strapping pins; in Chinese).
- `docs/hw_specs/*.png`: board interfaces, pin header definition and PCB dimensions.

## References

- Board and vendor firmware: [kevincoooool/ESP32P4_KSDIY](https://github.com/kevincoooool/ESP32P4_KSDIY).
- Vendor BSP component: [kevincoooool/ksdiy_p4c5_bsp](https://components.espressif.com/components/kevincoooool/ksdiy_p4c5_bsp) (v1.3.3 was used as the reference for pins and bring-up).
- Vendor ESP32-C5 flasher component: [kevincoooool/ksdiy_c5_sdio_flasher](https://components.espressif.com/components/kevincoooool/ksdiy_c5_sdio_flasher).
- BSP conventions and generic examples: [espressif/esp-bsp](https://github.com/espressif/esp-bsp); the display, touch, SD card and camera code is ported from its ESP32-P4 Function EV Board BSP.
- Phone UI: [espressif/esp-brookesia](https://github.com/espressif/esp-brookesia).
- Where to buy the board: [KSDIY store on Taobao](https://item.taobao.com/item.htm?id=667230365314) (the link printed by the vendor BSP).

## Disclaimer

This is an unofficial, community project made for research and learning. It is not affiliated with or endorsed by KSDIY or Espressif. The vendor documents in `docs/hw_specs` (schematic, datasheet) belong to their owners and are included for reference only. Everything here is provided as is, without warranty: use it at your own risk.

## License

Apache License 2.0, see [LICENSE](LICENSE). Bundled third-party components and fonts keep their own licenses (see the license files next to them).
