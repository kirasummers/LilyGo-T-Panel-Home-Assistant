# LilyGO T-Panel S3 - Home Assistant Dashboard & ESPHome Driver

ESPHome firmware configuration and custom **CST3240** capacitive touch driver for the **LilyGO T-Panel S3** (ESP32-S3 + 4.0" 480x480 ST7701S RGB IPS display).

This repository provides an out-of-the-box working setup for running modern ESPHome and LVGL on the LilyGO T-Panel S3, resolving common community hardware hurdles (such as broken touch reads, missing repeated-start I2C transactions, and PSRAM framebuffer allocation).

---

## Hardware Overview

| Subsystem | Component | Bus / Pinout |
|---|---|---|
| **MCU** | ESP32-S3 (16MB Flash, 8MB PSRAM) | Quad SPI mode @ 80MHz |
| **Display** | 4.0" 480x480 ST7701S RGB IPS | Parallel RGB + 3-wire SPI init via XL9535 |
| **Touchscreen** | Hynitron CST3240 Capacitive Touch | I2C (`0x5A`), INT: GPIO 21, RST: XL9535 Pin 4 |
| **IO Expander** | XL9535 16-bit Expander | I2C (`0x20`), SDA: GPIO 17, SCL: GPIO 18 (400kHz) |
| **Backlight** | LEDC PWM (1220 Hz) | GPIO 14 |

### Pin Mapping Reference

* **ST7701S RGB Data Lines**:
  * Blue: B0 (GPIO 1), B1 (GPIO 2), B2 (GPIO 3), B3 (GPIO 4), B4 (GPIO 5)
  * Green: G0 (GPIO 6), G1 (GPIO 7), G2 (GPIO 8), G3 (GPIO 9), G4 (GPIO 10), G5 (GPIO 11)
  * Red: R0 (GPIO 12), R1 (GPIO 13), R2 (GPIO 42), R3 (GPIO 46), R4 (GPIO 45)
* **RGB Sync & Clock**:
  * PCLK: GPIO 41
  * HSYNC: GPIO 39
  * VSYNC: GPIO 40
* **ST7701S SPI Initialization (routed via XL9535)**:
  * CS: XL9535 Pin 17
  * CLK: XL9535 Pin 15
  * MOSI: XL9535 Pin 16
  * Reset: XL9535 Pin 5
* **CST3240 Touch Controller**:
  * I2C Address: `0x5A`
  * SDA: GPIO 17
  * SCL: GPIO 18
  * Interrupt: GPIO 21
  * Reset: XL9535 Pin 4

---

## Key Hardware & Driver Fixes

1. **Custom CST3240 Driver (`components/cst3240`)**:
   * Uses atomic I2C repeated-start (`write_read`) instead of STOP-start sequences to properly read registers.
   * Accurately parses touch coordinates from bytes 1-3 and active touch count from register `0xD005` (byte 5).
   * Implements hardware frame synchronization by writing `0xAB` to register `0xD000` after each poll.
   * Auto-calibrates 480x480 coordinate scaling directly to the LVGL display buffer.
2. **PSRAM Configuration**:
   * Uses `psram: mode: quad` at `80MHz`. Avoids memory allocation errors (`ESP_ERR_NO_MEM`) and ensures the 480x480 16-bit RGB framebuffer and LVGL draw buffers fit cleanly into memory.
3. **Display Backlight**:
   * Configured via `ledc` on GPIO 14 with `on_boot` initialization to guarantee visible illumination immediately upon startup.

---

## Dashboard Features

The included [lilygo-t-panel-s3.yaml](lilygo-t-panel-s3.yaml) provides a dark-mode LVGL user interface:

* **Header Bar**: Displays status and device information with clean typography.
* **Quick Scenes**: One-tap scene pills ("All Off", "Relax", "Bright").
* **2x2 Lighting Grid**: Rounded cards with tactile touch feedback for individual lights (Overhead, Kitchen, Dining Room, Living Mood).
* **Backlight Card**: Interactive slider for dimming or boosting screen brightness in real-time.

---

## Getting Started

### 1. Prerequisites

Ensure you have ESPHome installed (version 2024.x or later recommended):

```bash
pip install esphome
```

### 2. Configure Credentials

Copy `secrets.yaml.example` to `secrets.yaml`:

```bash
cp secrets.yaml.example secrets.yaml
```

Edit `secrets.yaml` with your Wi-Fi credentials:

```yaml
wifi_ssid: "Your_WiFi_Network"
wifi_password: "Your_WiFi_Password"
```

> **Note**: `secrets.yaml` is listed in `.gitignore` and will never be committed to Git.

### 3. Compile & Flash

Connect your LilyGO T-Panel S3 via USB (typically `/dev/ttyACM0` on Linux or `COMx` on Windows):

```bash
# Compile and flash via USB serial
esphome run lilygo-t-panel-s3.yaml --device /dev/ttyACM0
```

Once flashed, subsequent updates can be deployed wirelessly over your local network:

```bash
esphome run lilygo-t-panel-s3.yaml
```

### 4. Home Assistant Integration

1. In Home Assistant, navigate to **Settings -> Devices & Services**.
2. The T-Panel should appear automatically under **Discovered Devices**.
3. Click **Configure** and submit.
4. Your T-Panel will link directly with your Home Assistant lights and scenes.
