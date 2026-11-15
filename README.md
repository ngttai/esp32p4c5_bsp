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
- Buttons: `bsp_iot_button_create()` for BOOT (GPIO35) and IO0 (GPIO0), used with the espressif/button API.
- IMU: `bsp_imu_init()`, `bsp_imu_read()`.
- microSD: `bsp_sdcard_mount()`, `bsp_sdcard_unmount()` (never formats the card).
- Camera: `bsp_camera_start()`, `bsp_camera_stop()` (esp_video, V4L2 device).
- Wi-Fi: `bsp_wifi_init()` (station mode through the ESP32-C5, ESP-Hosted).
- Battery and power key: `bsp_battery_get_info()`, `bsp_pmic_event_register()`, `bsp_power_off()`.
- Power: `bsp_power_set(bsp_power_domain_t, bool)` for `BSP_POWER_AUDIO`, `BSP_POWER_IMU`, `BSP_POWER_CAMERA`, `BSP_POWER_BOOST_5V`, `BSP_POWER_MODEM`.

The GPIO-switched domains are active high and off after reset. Power domains are not initialized automatically at boot.

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
