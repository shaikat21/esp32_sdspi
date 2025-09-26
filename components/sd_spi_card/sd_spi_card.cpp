#include "sd_spi_card.h"
#include "esphome/core/log.h"
#include "ff.h"   // FatFs

namespace esphome {
namespace sd_spi_card {

static const char *const TAG = "sd_spi_card";
static const char* MOUNT_POINT = "/sdcard";
std::string build_path(const char *path) {return std::string(MOUNT_POINT) + path;}



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

  esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &this->card_);
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

size_t SdSpiCard::file_size(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    ESP_LOGE(TAG, "Failed to open file %s", path);
    return 0;
  }
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fclose(f);
  return size;
}

#ifdef USE_SENSOR
void SdSpiCard::add_file_size_sensor(sensor::Sensor *s, const char *path) {
  // Store a copy of the path in std::string so it remains valid
  file_size_sensors_.push_back(FileSizeSensor(s, std::string(path)));
}
#endif

void SdSpiCard::update_sensors() {
 #ifdef USE_SENSOR
 
  if (this->card_ == nullptr)
    return;

  FATFS *fs;
  DWORD fre_clust, fre_sect, tot_sect;
  uint64_t total_bytes = -1, free_bytes = -1, used_bytes = -1;
  
  FRESULT res = f_getfree(MOUNT_POINT, &fre_clust, &fs);
  if (res == FR_OK) {
    tot_sect = (fs->n_fatent - 2) * fs->csize;
    fre_sect = fre_clust * fs->csize;

    total_bytes = static_cast<uint64_t>(tot_sect) * FF_SS_SDCARD;
    free_bytes  = static_cast<uint64_t>(fre_sect) * FF_SS_SDCARD;
    used_bytes  = total_bytes - free_bytes;
  } else {
    ESP_LOGE(TAG, "f_getfree failed: %d", res);
  }   

  if (this->total_space_sensor_ != nullptr)
    this->total_space_sensor_->publish_state(total_bytes);

  if (this->used_space_sensor_ != nullptr)
    this->used_space_sensor_->publish_state(used_bytes);

  if (this->free_space_sensor_ != nullptr)
    this->free_space_sensor_->publish_state(free_bytes);

for (auto &fs : file_size_sensors_) {
  if (fs.sensor != nullptr) {
    fs.sensor->publish_state(this->file_size(fs.path.c_str()));
  }
}

#endif
}

void SdSpiCard::loop() {
#ifdef USE_SENSOR
  // Update sensors periodically
  static uint32_t last_update = 0;
  uint32_t now = millis();
  if (now - last_update > 10000) {  // every 10s
    last_update = now;
    update_sensors();
  }
#endif
}


}  // namespace sd_spi_card
}  // namespace esphome
