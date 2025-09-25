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
static const std::string MOUNT_POINT("/");

std::string build_path(const char *path) { return MOUNT_POINT + path; }

void SdSpi::setup() {
  if (this->power_ctrl_pin_ != nullptr)
    this->power_ctrl_pin_->setup();
    
  // Enable debug logging for SD stack
ESP_LOGI(TAG, "Host ID: %d, CS: %d, Freq: %d kHz",
         this->host_.slot,
         this->slot_config_.gpio_cs,
         this->host_.max_freq_khz);

sdmmc_host_t host = SDSPI_HOST_DEFAULT();
host.slot = SPI3_HOST;
host.max_freq_khz = 100;

ESP_LOGI(TAG, "Host slot: %d, Freq: %d kHz", this->host.slot, this->host_.max_freq_khz);

ESP_LOGI(TAG, "After override: host.slot=%d freq=%d",
         this->host_.slot, this->host_.max_freq_khz);

spi_bus_config_t bus_cfg = {
    .mosi_io_num = (gpio_num_t) this->mosi_pin_,
    .miso_io_num = (gpio_num_t) this->miso_pin_,
    .sclk_io_num = (gpio_num_t) this->clk_pin_,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
};

ESP_LOGI(TAG, "Pins: MISO=%d, MOSI=%d, CLK=%d, CS=%d",
         this->miso_pin_, this->mosi_pin_, this->clk_pin_, this->cs_pin_);


ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t) host.slot, &bus_cfg, SDSPI_DEFAULT_DMA));


sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
slot_config.gpio_cs = (gpio_num_t) this->cs_pin_;
slot_config.host_id = (spi_host_device_t) host.slot;

ESP_LOGI(TAG, "About to mount: slot=%d host_id=%d CS=%d Freq=%d",
         this->host.slot,
         this->slot_config_.host_id,
         this->slot_config_.gpio_cs,
         this->host_.max_freq_khz);

  // Mount config
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

// Host
//this->host_ = SDSPI_HOST_DEFAULT();
//this->host_.slot = SPI3_HOST;   // or HSPI_HOST / VSPI_HOST depending on your board
//this->host_.max_freq_khz = 100; // tune if needed
sdmmc_host_t host = SDSPI_HOST_DEFAULT();
host.slot = SPI3_HOST;
host.max_freq_khz = 100;

ESP_LOGI(TAG, "Host slot: %d, Freq: %d kHz", this->host_.slot, this->host_.max_freq_khz);

ESP_LOGI(TAG, "After override: host.slot=%d freq=%d",
         this->host_.slot, this->host_.max_freq_khz);

// Bus config
//this->bus_cfg_ = {
//    .mosi_io_num = static_cast<gpio_num_t>(this->mosi_pin_),
//    .miso_io_num = static_cast<gpio_num_t>(this->miso_pin_),
//    .sclk_io_num = static_cast<gpio_num_t>(this->clk_pin_),
//    .quadwp_io_num = -1,
//    .quadhd_io_num = -1,
//    .max_transfer_sz = 4000,
//};

//  this->bus_cfg_ = {
//      .mosi_io_num = (gpio_num_t) this->mosi_pin_,
//      .miso_io_num = (gpio_num_t) this->miso_pin_,
//      .sclk_io_num = (gpio_num_t) this->clk_pin_,
//      .quadwp_io_num = -1,
//      .quadhd_io_num = -1,
//      .max_transfer_sz = 4000,
//  };



// Initialize bus
esp_err_t ret = spi_bus_initialize(
    (spi_host_device_t) host.slot,
    &bus_cfg_,
    SDSPI_DEFAULT_DMA
);


if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SPI bus.");
    this->init_error_ = ErrorCode::ERR_PIN_SETUP;
    mark_failed();
    return;
}

ESP_LOGI(TAG, "Mounting at: %s", MOUNT_POINT.c_str());

  // 👉 ADD THIS STEP: initialize SDSPI host driver
  ret = sdspi_host_init();
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "sdspi_host_init failed: %s", esp_err_to_name(ret));
      return;
  }
  
// Device (slot) config
//this->slot_config_ = SDSPI_DEVICE_CONFIG_DEFAULT();
//this->slot_config_.gpio_cs = (gpio_num_t) this->cs_pin_;
//this->slot_config_.host_id = (spi_host_device_t) this->host_.slot;


  

// mount sd card
//ret = esp_vfs_fat_sdspi_mount(
//    MOUNT_POINT.c_str(),
//    &this->host_,
//    &this->slot_config_,
//    &mount_config,
//    &this->card_
//);

esp_err_t ret = esp_vfs_fat_sdspi_mount(
    MOUNT_POINT.c_str(),
    &host,
    &slot_config,
    &mount_config,
    &this->card_
);


  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      this->init_error_ = ErrorCode::ERR_MOUNT;
    } else {
      this->init_error_ = ErrorCode::ERR_NO_CARD;
    }
    mark_failed();
    return;
  }

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

  if (this->card_->is_sdio) return "SDIO";
  if (this->card_->is_mmc) return "MMC";
//  if (this->card_->is_sd) {
//    return (this->card_->ocr & (1 << 30)) ? "SDHC/SDXC" : "SDSC";
//  }

  return "UNKNOWN";
}

void SdSpi::update_sensors() {
  // If you have a text_sensor to update, do it here.
#ifdef USE_TEXT_SENSOR
  if (this->sd_card_type_text_sensor_ != nullptr)
    this->sd_card_type_text_sensor_->publish_state(sd_card_type());
#endif
}

void SdSpi::loop() {
  // Nothing to do here for now
}

void SdSpi::dump_config() {
  ESP_LOGCONFIG(TAG, "SD SPI Card:");
  ESP_LOGCONFIG(TAG, "  MISO Pin: %d", this->miso_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI Pin: %d", this->mosi_pin_);
  ESP_LOGCONFIG(TAG, "  CLK Pin:  %d", this->clk_pin_);
  ESP_LOGCONFIG(TAG, "  CS Pin:   %d", this->cs_pin_);
}

void SdSpi::set_miso_pin(uint8_t pin) { miso_pin_ = pin; }
void SdSpi::set_mosi_pin(uint8_t pin) { mosi_pin_ = pin; }
void SdSpi::set_clk_pin(uint8_t pin) { clk_pin_ = pin; }
void SdSpi::set_cs_pin(uint8_t pin) { cs_pin_ = pin; }

#ifdef USE_SENSOR
// FileSizeSensor constructor
FileSizeSensor::FileSizeSensor(sensor::Sensor *s, const std::string &p)
  : sensor(s), path(p) {}
#endif

// Convert bytes to another memory unit
long double convertBytes(uint64_t bytes, MemoryUnits unit) {
  switch (unit) {
    case MemoryUnits::Byte:    return bytes;
    case MemoryUnits::KiloByte: return bytes / 1024.0;
    case MemoryUnits::MegaByte: return bytes / (1024.0 * 1024.0);
    case MemoryUnits::GigaByte: return bytes / (1024.0 * 1024.0 * 1024.0);
    case MemoryUnits::TeraByte: return bytes / (1024.0L * 1024.0L * 1024.0L * 1024.0L);
    case MemoryUnits::PetaByte: return bytes / (1024.0L * 1024.0L * 1024.0L * 1024.0L * 1024.0L);
    default: return bytes;
  }
}

#ifdef USE_SENSOR
// Add a file size sensor
void SdSpi::add_file_size_sensor(sensor::Sensor *sens, const std::string &path) {
  file_size_sensors_.push_back({sens, path});
}
#endif

// Append raw bytes to file
void SdSpi::append_file(const char *path, const uint8_t *buffer, size_t len) {
  this->write_file(path, buffer, len, "ab");
}

// Append string (e.g., CSV line) to file
void SdSpi::append_line(const char *path, const std::string &line) {
  std::string data = line + "\n";
  this->append_file(path, reinterpret_cast<const uint8_t *>(data.c_str()), data.size());
}


// Other functions (create_directory, remove_directory, delete_file, list_directory_file_info_rec, etc.)
// remain the same as your original SDMMC code
// just replace SdMmc -> SdSpi

}  // namespace sd_spi_card
}  // namespace esphome

#endif  // USE_ESP_IDF
