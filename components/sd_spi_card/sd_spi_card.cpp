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
  host.max_freq_khz = this->spi_freq_khz_;  

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

#ifdef USE_BINARY_SENSOR
  if (this->card_status_binary_sensor_ != nullptr) {
    bool mounted = (this->card_ != nullptr);
    this->card_status_binary_sensor_->publish_state(mounted);
    ESP_LOGI(TAG, "Initial SD card binary state: %s", mounted ? "ON (mounted)" : "OFF (not present)");
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
  std::string full_path = std::string(MOUNT_POINT) + path;
  FILE *f = fopen(full_path.c_str(), "rb");
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
  file_size_sensors_.push_back(FileSizeSensor(s, std::string(path)));
}
#endif

void SdSpiCard::update_sensors() {
 #ifdef USE_SENSOR
 
     // Case 1: No card mounted
  if (this->card_ == nullptr)
 {
    if (this->total_space_sensor_ != nullptr) this->total_space_sensor_->publish_state(NAN);
    if (this->used_space_sensor_  != nullptr) this->used_space_sensor_->publish_state(NAN);
    if (this->free_space_sensor_  != nullptr) this->free_space_sensor_->publish_state(NAN);
    
    for (auto &fs : file_size_sensors_) {
      if (fs.sensor != nullptr) fs.sensor->publish_state(NAN);
    }
    return;
  }


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
    
  } else if (res != FR_OK) {
        ESP_LOGE(TAG, "SD card not accessible, f_getfree() failed (%d)", res);
        if (this->card_ != nullptr) {
          esp_vfs_fat_sdcard_unmount(MOUNT_POINT, this->card_);
          this->card_ = nullptr;
    }

        // Invalidate sensor values
        if (this->total_space_sensor_ != nullptr)
           this->total_space_sensor_->publish_state(NAN);
           
        if (this->used_space_sensor_  != nullptr) 
           this->used_space_sensor_->publish_state(NAN);
           
        if (this->free_space_sensor_  != nullptr) 
           this->free_space_sensor_->publish_state(NAN);
           
       for (auto &fs : file_size_sensors_) {
        if (fs.sensor != nullptr) fs.sensor->publish_state(NAN);
    }
           
  }   

#endif
}

//mount card while running 
void SdSpiCard::try_remount() {
#ifdef USE_ESP_IDF
  if (this->card_ != nullptr) return;

  ESP_LOGI(TAG, "Attempting to re-mount SD card...");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = VSPI_HOST;
  host.max_freq_khz = this->spi_freq_khz_;

  int cs = static_cast<InternalGPIOPin*>(this->cs_pin_)->get_pin();

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = (gpio_num_t) cs;
  slot_config.host_id = (spi_host_device_t) host.slot;

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024
  };

  esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &this->card_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
    this->card_ = nullptr;
  } else {
    ESP_LOGI(TAG, "SD card mounted at %s (freq=%d kHz)", MOUNT_POINT, this->spi_freq_khz_);
  }
#endif
}


// write & appent file 

void SdSpiCard::append_file(const char *path, const char *line) {
  std::string full_path = std::string(MOUNT_POINT) + path;
  FILE *f = fopen(full_path.c_str(), "a");
  if (!f) {
    ESP_LOGE(TAG, "Append failed: %s", full_path.c_str());
    return;
  }
  fputs(line, f);
  fputc('\n', f);
  fclose(f);
  ESP_LOGI(TAG, "Appended to %s: %s", full_path.c_str(), line);
}

//write file
void SdSpiCard::write_file(const char *path, const char *line) {
  std::string full_path = std::string(MOUNT_POINT) + path;
  FILE *f = fopen(full_path.c_str(), "w");
  if (!f) {
    ESP_LOGE(TAG, "Write failed: %s", full_path.c_str());
    return;
  }
  fputs(line, f);
  fputc('\n', f);
  fclose(f);
  ESP_LOGI(TAG, "Wrote new file %s: %s", full_path.c_str(), line);
}


bool SdSpiCard::delete_file(const char *path) {
  std::string full_path = std::string(MOUNT_POINT) + path;
  if (remove(full_path.c_str()) == 0) {
    ESP_LOGI(TAG, "Deleted file: %s", full_path.c_str());
    return true;
  } else {
    ESP_LOGE(TAG, "Delete failed: %s", full_path.c_str());
    return false;
  }
}

// Append a row in csv
bool SdSpiCard::csv_append_row(const char *path, const std::vector<std::string> &cells) {
  std::string full_path = std::string(MOUNT_POINT) + path;
  FILE *f = fopen(full_path.c_str(), "a");
  if (!f) {
    ESP_LOGE(TAG, "Row append failed: %s", full_path.c_str());
    return false;
  }

  // Build line in memory for log + write
  std::string line;
  for (size_t i = 0; i < cells.size(); i++) {
    line += cells[i];
    if (i < cells.size() - 1) line += ",";
  }

  fputs(line.c_str(), f);
  fputc('\n', f);
  fclose(f);

  ESP_LOGI(TAG, "Row appended to %s: %s", full_path.c_str(), line.c_str());
  return true;
}

// Count total rows
int SdSpiCard::csv_row_count(const char *path) {
  std::string full_path = std::string(MOUNT_POINT) + path;
  FILE *f = fopen(full_path.c_str(), "r");
  if (!f) {
    ESP_LOGE(TAG, "Row count failed, file not found: %s", full_path.c_str());
    return -1;
  }
  int count = 0;
  char buf[256];
  while (fgets(buf, sizeof(buf), f)) count++;
  fclose(f);
  ESP_LOGI(TAG, "Row count for %s: %d", full_path.c_str(), count);
  return count;
}

// Delete range of rows [row_start, row_end]
bool SdSpiCard::csv_delete_rows(const char *path, int row_start, int row_end) {
  std::string full_path = std::string(MOUNT_POINT) + path;
  FILE *fin = fopen(full_path.c_str(), "r");
  if (!fin) {
    ESP_LOGE(TAG, "Delete rows failed, file not found: %s", full_path.c_str());
    return false;
  }
  std::string tmp_path = full_path + ".tmp";
  FILE *fout = fopen(tmp_path.c_str(), "w");
  if (!fout) {
    fclose(fin);
    ESP_LOGE(TAG, "Delete rows failed, cannot open temp file: %s", tmp_path.c_str());
    return false;
  }

  char buf[256];
  int row = 0;
  while (fgets(buf, sizeof(buf), fin)) {
    if (row < row_start || row > row_end) {
      fputs(buf, fout);
    }
    row++;
  }
  fclose(fin);
  fclose(fout);
  remove(full_path.c_str());
  rename(tmp_path.c_str(), full_path.c_str());

  ESP_LOGI(TAG, "Deleted rows %d–%d from %s", row_start, row_end, full_path.c_str());
  return true;
}

// Keep only last N rows
bool SdSpiCard::csv_keep_last_n(const char *path, int max_rows) {
  std::string full_path = std::string(MOUNT_POINT) + path;
  int total = csv_row_count(path);
  if (total <= max_rows) {
    ESP_LOGI(TAG, "Keep last %d rows skipped, %s already within limit (%d)", max_rows, full_path.c_str(), total);
    return true;
  }

  FILE *fin = fopen(full_path.c_str(), "r");
  if (!fin) {
    ESP_LOGE(TAG, "Keep last N failed, file not found: %s", full_path.c_str());
    return false;
  }
  std::string tmp_path = full_path + ".tmp";
  FILE *fout = fopen(tmp_path.c_str(), "w");
  if (!fout) {
    fclose(fin);
    ESP_LOGE(TAG, "Keep last N failed, cannot open temp file: %s", tmp_path.c_str());
    return false;
  }

  char buf[256];
  int row = 0;
  int skip = total - max_rows;
  while (fgets(buf, sizeof(buf), fin)) {
    if (row >= skip) fputs(buf, fout);
    row++;
  }
  fclose(fin);
  fclose(fout);
  remove(full_path.c_str());
  rename(tmp_path.c_str(), full_path.c_str());

  ESP_LOGI(TAG, "Trimmed %s to last %d rows (was %d)", full_path.c_str(), max_rows, total);
  return true;
}

// Pull values from column within row range
std::vector<std::string> SdSpiCard::csv_read_column_range(
    const char *path, int col_index, int row_start, int row_end) {

  std::vector<std::string> out;
  std::string full_path = std::string(MOUNT_POINT) + path;
  FILE *f = fopen(full_path.c_str(), "r");
  if (!f) {
    ESP_LOGE(TAG, "Read column failed, file not found: %s", full_path.c_str());
    return out;
  }

  char buf[256];
  int row = 0;

  while (fgets(buf, sizeof(buf), f)) {
    if (row >= row_start && row <= row_end) {
      int col = 0;
      char *token = strtok(buf, ",");
      while (token != nullptr) {
        if (col == col_index) {
          out.push_back(std::string(token));
          break;
        }
        token = strtok(nullptr, ",");
        col++;
      }
    }
    if (row > row_end) break;
    row++;
  }

  fclose(f);

  ESP_LOGI(TAG, "Read column %d, rows %d–%d from %s, got %d values",
           col_index, row_start, row_end, full_path.c_str(), (int) out.size());

  return out;
}

// check sd card presence , if failed try to create , if failed mark the card as failed
bool SdSpiCard::check_kappa() {
#ifdef USE_ESP_IDF
  bool success = false;

  if (this->card_ == nullptr) {
    ESP_LOGW(TAG, "Kappa check skipped: card not mounted");
    success = false;
  } else {
    // --- Step 1: Try a low-level read to test card health ---
    uint8_t sector_buf[512];
    esp_err_t err = sdmmc_read_sectors(this->card_, sector_buf, 0, 1);

    if (err == ESP_OK) {
      this->last_sd_error_ = ESP_OK;
      success = true;
      ESP_LOGD(TAG, "Kappa check OK (sector read succeeded)");
    } else {
      this->last_sd_error_ = err;
      success = false;
      ESP_LOGE(TAG, "Kappa check failed: %s (0x%x)", esp_err_to_name(err), err);
    }
  }

  // --- Step 2: Handle success/failure transitions ---
  static bool last_success = false;
  if (success != last_success) {
#ifdef USE_BINARY_SENSOR
    if (this->card_status_binary_sensor_ != nullptr)
      this->card_status_binary_sensor_->publish_state(success);
#endif

    if (success) {
      ESP_LOGI(TAG, "Card OK");
    } else {
      ESP_LOGE(TAG, "Card failure detected → unmounting");
      if (this->card_ != nullptr) {
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, this->card_);
        this->card_ = nullptr;
      }
    }
  }

  last_success = success;
  return success;
#else
  return false;
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
  
// Retry remount every 5s if card is not mounted
static uint32_t last_remount_try = 0;
if (now - last_remount_try > 5000) {
  last_remount_try = now;

  if (this->card_ == nullptr) {
    // No card mounted → attempt remount
    try_remount();
  } else {
    // Card is mounted → probe it
    check_kappa();
  }
}
#endif
}


}  // namespace sd_spi_card
}  // namespace esphome
