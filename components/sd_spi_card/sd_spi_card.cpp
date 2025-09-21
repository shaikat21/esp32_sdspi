#include "sd_spi_card.h"

#ifdef USE_ESP_IDF
#include "math.h"
#include "esphome/core/log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/gpio.h"

namespace esphome {
namespace sd_spi_card {

static constexpr size_t FILE_PATH_MAX = ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN;
static const char *TAG = "sd_spi_card";
static const std::string MOUNT_POINT("/sdcard");

std::string build_path(const char *path) { return MOUNT_POINT + path; }

void SdSpi::setup() {
  if (this->power_ctrl_pin_ != nullptr)
    this->power_ctrl_pin_->setup();

  // Mount config
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  // SPI host config
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = 40000; // can adjust frequency

  // SPI slot config
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = static_cast<gpio_num_t>(this->miso_pin_);
  slot_config.gpio_mosi = static_cast<gpio_num_t>(this->mosi_pin_);
  slot_config.gpio_sck  = static_cast<gpio_num_t>(this->clk_pin_);
  slot_config.gpio_cs   = static_cast<gpio_num_t>(this->cs_pin_);
  slot_config.gpio_cd   = GPIO_NUM_NC; // no card detect
  slot_config.host_id   = host.slot;   // default host

  auto ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT.c_str(), &host, &slot_config, &mount_config, &this->card_);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      this->init_error_ = ErrorCode::ERR_MOUNT;
    } else {
      this->init_error_ = ErrorCode::ERR_NO_CARD;
    }
    mark_failed();
    return;
  }

#ifdef USE_TEXT_SENSOR
  if (this->sd_card_type_text_sensor_ != nullptr)
    this->sd_card_type_text_sensor_->publish_state(sd_card_type());
#endif

  update_sensors();
}

// File write
void SdSpi::write_file(const char *path, const uint8_t *buffer, size_t len, const char *mode) {
  std::string absolut_path = build_path(path);
  FILE *file = fopen(absolut_path.c_str(), mode);
  if (file == nullptr) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return;
  }
  size_t ok = fwrite(buffer, 1, len, file);
  if (ok != len) {
    ESP_LOGE(TAG, "Failed to write to file");
  }
  fclose(file);
  this->update_sensors();
}

// File read
std::vector<uint8_t> SdSpi::read_file(char const *path) {
  ESP_LOGV(TAG, "Read File: %s", path);

  std::string absolut_path = build_path(path);
  FILE *file = fopen(absolut_path.c_str(), "rb");
  if (!file) {
    ESP_LOGE(TAG, "Failed to open file for reading");
    return {};
  }

  std::vector<uint8_t> res;
  size_t fileSize = this->file_size(path);
  res.resize(fileSize);
  size_t len = fread(res.data(), 1, fileSize, file);
  fclose(file);

  if (len != fileSize) {
    ESP_LOGE(TAG, "Failed to read file: %s", strerror(errno));
    return {};
  }

  return res;
}

// Card type
std::string SdSpi::sd_card_type() const {
  if (this->card_ == nullptr) return "UNKNOWN";

  switch (this->card_->csd->mmc_ver) {
    case MMC_VERSION_UNKNOWN: return "UNKNOWN";
    case MMC_VERSION_4: return "MMC v4";
    default: break;
  }

  return (this->card_->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC";
}

// Other functions (create_directory, remove_directory, delete_file, list_directory_file_info_rec, etc.)
// remain the same as your original SDMMC code
// just replace SdMmc -> SdSpi

}  // namespace sd_spi_card
}  // namespace esphome

#endif  // USE_ESP_IDF
