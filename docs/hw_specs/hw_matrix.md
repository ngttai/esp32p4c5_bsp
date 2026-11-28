# KSDIY ESP32-P4/C5 4.3" – Hardware Matrix

> As of: 2026-11-13 (rev 5)
> Sources / Nguồn: `ksdiy_p4c5_bsp` v1.3.3, `ksdiy_c5_sdio_flasher` v1.1.0, esptool output, **stock firmware boot log**, `managed_components` (`esp_lcd_st7102` 1.0.4, `esp_lcd_touch_st7123` 1.0.2~1, `pmic_axp2101` 1.1.2); rev 4 adds the vendor resource pack, kept in this folder: `schematic.pdf` (dated 2026-06-04), the board interface and pin header pictures, `PCBStructuralDimensions.png`, and the module datasheet; `pin_out.md` and the board photos of that pack are not in the repo.
> Upstream: https://github.com/kevincoooool/ESP32P4_KSDIY
> Module datasheet: [ESP32-P4C5-Core_module_datasheet.pdf](ESP32-P4C5-Core_module_datasheet.pdf)
> **(?)** = not yet known. **[log]** = stock boot log. **[sch]** = schematic. **[pin]** = `pin_out.md`. **[photo]** = annotated board photo. **[test]** = measured on the board with this BSP.

---

## 1. English

### 1.1 Hardware matrix

| # | Subsystem | Component | Key specs | Interface | Pins (P4 GPIO) / address | Source |
|---|-----------|-----------|-----------|-----------|--------------------------|--------|
| 1 | Host MCU | ESP32-P4 (Yuying ESP32-P4C5 module) | Rev v3.2, eFuse block rev v1.2, dual core + LP core, 400 MHz, 40 MHz crystal | – | UART0 TXD = 37, RXD = 38 | [log], esptool, [sch] |
| 2 | Flash | Boya SPI flash | 16 MB, QIO 80 MHz, mfr ID 0x68, dev 0x4018 | SPI | – | [log]. Enable `SPI_FLASH_SUPPORT_BOYA_CHIP` (currently generic driver) |
| 3 | PSRAM | AP Memory hex PSRAM (gen 4) | 32 MB (256 Mbit density per die ID), X16 hex mode, 250 MHz, 2048 B burst | Hex PSRAM | – | [log] |
| 4 | Wi-Fi/BLE | ESP32-C5 (inside the module) | ESP-Hosted slave FW 2.12.13, chip id 0x17, board type 23; WLAN + BLE-only HCI over SDIO; IPEX antenna | SDIO slot 1, 4-bit, 40 MHz, streaming | D0–D3 = 14–17, CLK = 18, CMD = 19 (internal to the module, not on the schematic) | [log], flasher |
| 5 | C5 control | EN/RESET, BOOT | RESET high = run; BOOT low at reset = download | GPIO | RESET = 54, BOOT = 2 (net `C5_BOOT`) | flasher, [sch] |
| 6 | 5 V boost | SY7088 | Measured 5.24 V on the JP1 5V pin with GPIO20 high [test]. AXP_VSYS → BOOST_5V; feeds USB-A VBUS, JP1 pin 2, RS485 transceiver and MCP4725 | GPIO enable, active high; off at boot (EN 1 MΩ pull-down) | 20 (`BOOST_ON`) | flasher, [sch] |
| 7 | Display | 4.3" ST7102, panel YDP430BT009-V1 | Yuying module; 480×800, 60 Hz, RGB565 (default)/RGB888; LCD ID 80 A0 FB (read from reg 0x04); DPI clock 37.8 MHz; H sync/back/front = 2/40/40, V sync/back/front = 2/10/310; COLMOD 0x55/0x66/0x77; 30-pin 0.5 mm FPC | MIPI DSI 2 lanes @ 820 Mbps [log] (driver macro default: 500 Mbps) | RST = 22 (shared with touch; 10 kΩ pull-up + 1 µF) | BSP, [log], `esp_lcd_st7102` 1.0.4, [sch] |
| 8 | Backlight | SY7200 LED driver | 40 mA (200 mV / 5.1 Ω); PWM 10 kHz, 10-bit, active high, LEDC T1/CH1 | GPIO PWM on EN | 6 (`LCD_BL`) | BSP, [log], [sch] |
| 9 | Touch | ST7123 | FW 1.71.1.5, 480×800, up to 5 touches, 400 kHz (driver macro default: 100 kHz); 16-bit registers; INT active-low open-drain with internal pull-up; per-finger track IDs; max 10 reports | I2C0 | addr 0x55; SDA = 7, SCL = 8, INT = 23, RST = 22 | BSP, [log], `esp_lcd_touch_st7123` 1.0.2~1, [sch] |
| 10 | PMIC | AXP2101 | DCDC1 3.3 V → VCC3V3; ALDO1 1.8 V → camera; ALDO3 3.3 V → codecs + IMU; ALDO4 2.9 V → camera AVDD; DCDC2–4, ALDO2, BLDO, DLDO unused; driver I2C at 100 kHz, 100 ms timeout; `axp2101_init()` sets no charge parameters | I2C0 | addr 0x34; IRQ = 21 (via 2N7002 level shifter) | BSP, [log], `pmic_axp2101` 1.1.2, [sch] |
| 11 | Battery / charging | AXP2101 charger | Battery connector J2 (MX1.25-2P); 10 kΩ B3950 NTC on TS; red charge LED on CHGLED; power key BTN1 → PWRON | – | – | [sch] |
| 12 | Audio DAC/codec | ES8311 | Slave mode; I2S TX 16 kHz, MCLK 256×; differential out to the amp; powered from ALDO3 | I2C0 + I2S0 | addr 0x18; MCLK = 13, SCLK = 12, LRCK = 10, DSDIN (P4 → codec) = 9 | [log], [sch], [pin] |
| 13 | Speaker amp | NS4150B (mono class D) | Powered from AXP_VSYS; speaker connector J1 (MX1.25-2P) | GPIO enable, active high; off at boot | `PA_CTRL` = 3 (10 kΩ pull-down) | [sch], [pin] |
| 14 | Audio ADC | ES7210 | Slave mode; TDM RX 16-bit 16 kHz (frame layout: section 1.4b); MIC1 + MIC2 = two on-board MEMS mics (MSM381A3729H9CP); MIC3 = AEC reference looped back from the ES8311 output; MIC4 unused | I2C0 + I2S0 TDM | addr 0x40 (AD0 = AD1 = GND); SDOUT (codec → P4) = 11; shares MCLK/SCLK/LRCK with ES8311 | [log], [sch], [pin] |
| 15 | Camera | SC2336 / OV2710 (CSI) | Sensor PID 0xcb3a; `esp_video`; 24-pin 0.5 mm FPC; 2 data lanes; 24 MHz on-board oscillator for XCLK; reset by RC only (no GPIO); AVDD from ALDO4 (2.9 V) | MIPI-CSI + SCCB on I2C0 | addr 0x30; dedicated CSI pins | [log], [sch] |
| 16 | USB HS | P4 high-speed USB | One port wired to both a Type-C (J3) and a Type-A host socket (VBUS from BOOST_5V); used by the stock "USB camera" page. Tested in the phone example: high-speed device on the Type-C socket (Second screen app, about 8.6 MB/s from a PC) and host on the Type-A socket with a UVC webcam (USB camera app) [test] | USB 2.0 HS | Dedicated DP/DM pins | [sch], [photo], [log] |
| 17 | USB debug | USB-Serial/JTAG | Type-C "DEBUG" (J5) | USB FS | 24 / 25 | [sch], esptool |
| 18 | USB-UART | CH343P | Type-C "USB-TTL" (J6); RTS → CHIP_PU, DTR → GPIO35 (auto-download) | UART0 | TXD = 37, RXD = 38 | [sch] |
| 19 | microSD | TF card socket J4 | 4-bit SDMMC; powered from ESP_LDO_VO4; 51 kΩ pull-ups; no card-detect | SDMMC | CLK = 43, CMD = 44, D0–D3 = 39–42 | [sch], [pin] |
| 20 | 4G modem | ML307C (CMCC AT firmware) | Supplied straight from the battery rail through a P-MOSFET; nano-SIM socket; IPEX antenna | UART (level-shifted) | P4 TX = 53, P4 RX = 52, DTR = 51, power enable = 4 (active high; off at boot) | [sch], [pin], [photo] |
| 21 | RS485 | SN65HVD3082E | Powered from BOOST_5V; 120 Ω termination fitted; A/B on JP1 pins 20/19 | UART | TX = 31, RX = 33, DE = 32 | [sch], [pin] |
| 22 | Analog out | MCP4725 12-bit DAC | Powered from BOOST_5V, so it only answers when GPIO20 is high; output on JP1 pin 1 | I2C0 | addr 0x60 expected (A0 = GND); fitted and powered (DAC pin sits at 2.6 V = mid-scale with BOOST_5V at 5.24 V) but never answers on I2C, in the stock firmware or here (scans up to 3 s after enabling GPIO20). Likely cause: its VIH is 0.7 x VDD = 3.67 V while the bus is pulled up to 3.3 V | [sch], [photo], [test] |
| 23 | IMU | LSM6DS3TR-C | 6-axis; powered from ALDO3; INT1/INT2 not connected | I2C0 | addr 0x6A | [log], [sch] |
| 24 | LCD/touch module, second address | ST7123 module (same reset as the touch) | Not a discrete part: it answers only while LCD/touch RESET (GPIO22) is high and disappears together with the touch address 0x55 when the line is held low. Function not identified | I2C0 | addr 0x28 | [log], [test] |
| 25 | RGB LED | WS2812B | One addressable LED; a separate white LED shows 3.3 V power | GPIO | 34 | [sch], [pin] |
| 26 | Buttons | BOOT, IO0, RST, POWER | BOOT and IO0 active low with 10 kΩ pull-ups; RST pulls CHIP_PU low; POWER goes to the PMIC. Stock log names: BTN1 = GPIO35, BTN2 = GPIO0. Schematic designators: BTN1 = POWER, BTN2 = GPIO35, BTN3 = GPIO0 | GPIO | BOOT = 35, IO0 = 0 | [log], [sch] |
| 27 | Expansion | JP1 2×10 header, 2.54 mm | 11 GPIOs through 100 Ω series resistors, DAC, RS485, 5 V, 3.3 V | – | 26–30, 45–50 | [sch] |
| 28 | Mechanical | PCB | 61.28 × 106.48 mm; M3 holes on 53.22 × 98.42 mm centres; silkscreen date 20260521 in the vendor material; the board tested here reads 20260827 | – | – | [PCBStructuralDimensions.png](PCBStructuralDimensions.png), [photo] |

### 1.2 I2C0 bus map (SDA = 7, SCL = 8, 2.2 kΩ pull-ups to 3.3 V)

| Address | Device |
|---------|--------|
| 0x18 | ES8311 codec |
| 0x28 | LCD/touch module (second address, disappears with 0x55 when GPIO22 is low) |
| 0x30 | SC2336 camera (SCCB) |
| 0x34 | AXP2101 PMIC |
| 0x40 | ES7210 ADC |
| 0x55 | ST7123 touch |
| 0x60 | MCP4725 DAC (expected; powered but does not answer, see row 22) |
| 0x6A | LSM6DS3TR-C IMU |

### 1.3 Pin map (P4 GPIOs, sorted)

| GPIO | Function |
|------|----------|
| 0 | IO0 button (active low); also P4 strapping pin |
| 1 | Not used on the board |
| 2 | C5 BOOT strap |
| 3 | Speaker amp enable (`PA_CTRL`), active high |
| 4 | 4G power enable, active high |
| 5 | Not used on the board |
| 6 | LCD backlight PWM |
| 7 | I2C0 SDA |
| 8 | I2C0 SCL |
| 9 | I2S0 DSDIN (P4 → ES8311) |
| 10 | I2S0 LRCK |
| 11 | I2S0 SDOUT (ES7210 → P4) |
| 12 | I2S0 SCLK |
| 13 | I2S0 MCLK |
| 14–17 | SDIO D0–D3 (C5, inside module) |
| 18 | SDIO CLK (C5, inside module) |
| 19 | SDIO CMD (C5, inside module) |
| 20 | 5 V boost enable |
| 21 | AXP2101 IRQ |
| 22 | LCD + touch RESET (shared) |
| 23 | Touch INT |
| 24 / 25 | USB-Serial/JTAG (DEBUG Type-C) |
| 26–30 | JP1 header |
| 31 | RS485 TX |
| 32 | RS485 DE |
| 33 | RS485 RX |
| 34 | WS2812B data |
| 35 | BOOT button (active low); boot-mode strap; CH343P DTR |
| 36 | 10 kΩ pull-up only |
| 37 | UART0 TXD (console, to CH343P) |
| 38 | UART0 RXD (console, from CH343P) |
| 39–42 | microSD D0–D3 |
| 43 | microSD CLK |
| 44 | microSD CMD |
| 45–50 | JP1 header |
| 51 | 4G DTR |
| 52 | 4G UART, P4 RX |
| 53 | 4G UART, P4 TX |
| 54 | C5 EN/RESET (inside module) |

### 1.4 JP1 expansion header (2×10, 2.54 mm)

| Pin | Signal | Pin | Signal |
|-----|--------|-----|--------|
| 1 | DAC (MCP4725 out) | 2 | BOOST_5V |
| 3 | GND | 4 | VCC3V3 |
| 5 | GPIO50 | 6 | GPIO49 |
| 7 | GPIO48 | 8 | GPIO47 |
| 9 | GPIO46 | 10 | GND |
| 11 | GPIO45 | 12 | GPIO26 |
| 13 | GPIO27 | 14 | GPIO28 |
| 15 | GPIO29 | 16 | GPIO30 |
| 17 | GND | 18 | GND |
| 19 | RS485 B | 20 | RS485 A |

### 1.4b Microphone frame and echo reference (measured 2026-11-24)

- **Wiring** (schematic, block "ADC&AEC"): the ES8311 output `DAC_OUT_P/N` goes to the speaker amplifier and also, through R7 / R10
  (0 ohm), to `ADC_MIC3_P/N` of the ES7210. **MIC3 is the speaker signal looped back in hardware**, taken before the amplifier: an
  echo reference with no room sound in it. MIC1 and MIC2 are the two MEMS microphones; MIC4 is not connected.
- **Mode**: the ES7210 outputs MIC3 only in TDM. The esp_codec_dev driver turns TDM on when 3 or more inputs are selected
  (`mic_selected = MIC1 | MIC2 | MIC3`). The P4 side stays in **standard I2S**: open the microphone with **2 channels of
  32 bits** (`BSP_MIC_CHANNELS`, `BSP_MIC_BITS`); each frame read is then **4 int16** (`BSP_MIC_FRAME_SLOTS`). No I2S TDM
  driver is needed (the November 2026 attempt with an RX TDM channel crashed: no internal DMA memory left while esp-sr ran).
- **Slot order** (int16 index inside a frame, measured with a 1 kHz tone played while recording, 3 s on / 3 s off):

  | Index | Signal | Tone on | Tone off |
  |---|---|---|---|
  | 0 | MIC3, speaker reference (`BSP_MIC_SLOT_REF`) | 452 rms (volume 50 %, 0 dB) | exactly 0 |
  | 1 | MIC1 (`BSP_MIC_SLOT_MIC1`) | ~570 | 60-95 (room) |
  | 2 | MIC4, not connected | 0 | 0 |
  | 3 | MIC2 (`BSP_MIC_SLOT_MIC2`) | ~440 | 60-95 (room) |

  (MIC1 above MIC2 here only because the speaker was a little nearer to MIC1.) With only MIC1 + MIC2 selected, the same frame
  shows index 1 and 3 and zeros at 0 and 2; writing REG12 (SDP interface 2) while running changes nothing.
- **Shared frame**: TX and RX share BCLK / LRCK. While the microphone is open, the speaker must be opened with the same
  2 x 32 bits (the sound in the upper 16 bits of each 32-bit sample, both channels); a mono 16-bit speaker open fails then.
  Music / radio / video (microphone closed) keep mono 16 bit. Both directions also run at one sample rate.
- **Gains**: microphones 30 dB (`esp_codec_dev_set_in_gain`); the reference is line level, set apart with
  `esp_codec_dev_set_in_channel_gain(mic, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(2), 12)` (the AI assistant uses 12 dB).
- **Mixing the two microphones**: their mean is quieter than one microphone (they pick the voice slightly out of phase).

### 1.5 Open questions
(Answered: the charge current and voltage of the AXP2101 are read back by `bsp_battery_get_info()` (`charge_current_ma`,
`charge_voltage_mv`); this board reads 300 mA with the BSP leaving the chip as it is. `CONFIG_BSP_PMIC_SET_CHARGER` writes
200-1000 mA and 4.10 / 4.20 / 4.35 / 4.40 V from the battery datasheet. **[test]**)
- What the LCD/touch module does at I2C address 0x28 (it belongs to the module: gone while GPIO22 is low).
- Source of the 820 Mbps DSI lane rate (the `esp_lcd_st7102` macro default is 500 Mbps).
- MCP4725: confirm the logic-level explanation by measuring SDA/SCL at the chip (5.24 V supply, 3.3 V bus).

