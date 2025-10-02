# ESPHome SD SPI Card Component

Custom [ESPHome](https://esphome.io) component for **ESP32** that adds **SD card support over SPI**.  
It allows mounting an SD card, monitoring its storage, and handling files (including CSV logs) directly from ESPHome.

---

## ✨ Features

- 📦 **Mount SD card over SPI** (ESP-IDF based, works with ESP32 boards)  
- 📊 **Expose sensors** in ESPHome for:
  - Total space
  - Used space
  - Free space
  - File size for specific files
- 📑 **File operations**:
  - Append text or CSV rows
  - Delete files
  - Write new files
- 📈 **CSV helpers**:
  - Count rows
  - Delete row ranges
  - Keep only last N rows
  - Read a specific column range
- 🔄 **Live detection** of SD card removal & auto-remount  
- ⚡ Lightweight & optimized for ESP devices  

---

## 🛠 Installation

Add this repo as an **external component** in your ESPHome YAML:

```yaml
external_components:
  - source: github://shaikat21/esp32_sdspi
    components: [ sd_spi_card ]
