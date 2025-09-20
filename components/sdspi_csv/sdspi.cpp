#include "esphome/core/log.h"
#include "sdspi_card.h"

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/periph_ctrl.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_cmd.h"
#include "esp_vfs_fat.h"

static const char *TAG = "sdspi_card";

namespace esphome {
namespace sdspi_card {

SDSPICard::SDSPICard(int8_t cs_pin, int8_t sck_pin, int8_t mosi_pin, int8_t miso_pin)
    : cs_pin_(cs_pin), sck_pin_(sck_pin), mosi_pin_(mosi_pin), miso_pin_(miso_pin) {}

void SDSPICard::setup() {
  ESP_LOGI(TAG, "Setting up SD SPI Card with CS %d, SCLK %d, MOSI %d, MISO %d",
           this->cs_pin_, this->sck_pin_, this->mosi_pin_, this->miso_pin_);

  // SPI bus config
  spi_bus_config_t bus_cfg = {
    .mosi_io_num = this->mosi_pin_,
    .miso_io_num = this->miso_pin_,
    .sclk_io_num = this->sck_pin_,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000
  };
  esp_err_t ret = spi_bus_initialize((spi_host_device_t)SDSPI_HOST_DEFAULT().slot, &bus_cfg, SDSPI_DEFAULT_DMA());
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // Device config
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = this->cs_pin_;
  // host_id normally from SDSPI_HOST_DEFAULT().slot
  slot_config.host_id = (spi_host_device_t)SDSPI_HOST_DEFAULT().slot;

  sdmmc_card_t *card;
  const char *mount_point = "/sdcard";
  esp_vfs_fat_sdspi_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  ret = esp_vfs_fat_sdspi_mount(mount_point, &SDSPI_HOST_DEFAULT(), &slot_config, &mount_config, &card);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  sdmmc_card_print_info(stdout, card);
  ESP_LOGI(TAG, "SD card mounted at %s", mount_point);
}

void SDSPICard::dump_config() {
  ESP_LOGCONFIG(TAG, "SDSPICard:");
  ESP_LOGCONFIG(TAG, "  CS Pin: %d", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "  SCLK Pin: %d", this->sck_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI Pin: %d", this->mosi_pin_);
  ESP_LOGCONFIG(TAG, "  MISO Pin: %d", this->miso_pin_);
}

void SDSPICard::list_root() {
  // Example: list files in /sdcard root
  DIR *dir = opendir("/sdcard");
  if (!dir) {
    ESP_LOGE(TAG, "Failed to open /sdcard");
    return;
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    ESP_LOGI(TAG, "Found: %s", entry->d_name);
  }
  closedir(dir);
}

}  // namespace sdspi_card
}  // namespace esphome
