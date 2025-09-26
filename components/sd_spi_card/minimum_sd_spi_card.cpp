#include "sd_spi_card.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sd_spi_card {

static const char *const TAG = "sd_spi_card";

void SdSpiCard::setup() {
#ifdef USE_ESP_IDF
  ESP_LOGI(TAG, "Mounting SD card via SDSPI...");

  // Configure host
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = VSPI_HOST;
  host.max_freq_khz = this->spi_freq_khz_;  // ✅ use YAML option

  // Convert pins
  int mosi = static_cast<InternalGPIOPin*>(this->mosi_pin_)->get_pin();
  int miso = static_cast<InternalGPIOPin*>(this->miso_pin_)->get_pin();
  int clk  = static_cast<InternalGPIOPin*>(this->clk_pin_)->get_pin();
  int cs   = static_cast<InternalGPIOPin*>(this->cs_pin_)->get_pin();

  // SPI bus config
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = mosi,
      .miso_io_num = miso,
      .sclk_io_num = clk,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };

  ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t) host.slot, &bus_cfg, SDSPI_DEFAULT_DMA));

  // Device config
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = (gpio_num_t) cs;
  slot_config.host_id = (spi_host_device_t) host.slot;

  // Mount config
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &this->card_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "SD card mounted at /sdcard (freq=%d kHz)", this->spi_freq_khz_);
  }
#endif
}

void SdSpiCard::dump_config() {
  ESP_LOGCONFIG(TAG, "SD SPI Card:");
  ESP_LOGCONFIG(TAG, "  SPI Freq: %d kHz", this->spi_freq_khz_);
  if (this->card_ == nullptr) {
    ESP_LOGE(TAG, "Not mounted.");
  } else {
    ESP_LOGI(TAG, "Card size: %lluMB",
             (uint64_t) this->card_->csd.capacity * this->card_->csd.sector_size / (1024 * 1024));
  }
}

}  // namespace sd_spi_card
}  // namespace esphome
