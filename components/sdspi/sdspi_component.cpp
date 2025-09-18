#include "sdspi_component.h"
#include <string>
#include <cstring>

namespace esphome {
namespace sdspi {

static const char *TAG = "sdspi";

void SDSPIComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SDSPI component...");
  
  if (this->cs_pin_ == nullptr) {
    ESP_LOGE(TAG, "CS pin not configured!");
    return;
  }
  
  // Initialize SPI bus if not already done
  this->spi_setup();
  
  if (!initialize_sd_card()) {
    ESP_LOGE(TAG, "Failed to initialize SD card");
    return;
  }
  
  mounted_ = true;
  ESP_LOGI(TAG, "SD card mounted successfully at %s", mount_point_);
}

bool SDSPIComponent::initialize_sd_card() {
  ESP_LOGI(TAG, "Initializing SD card...");
  
  // SDSPI driver configuration
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = spi_bus_;
  
  // SPI bus configuration
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = mosi_pin_,
      .miso_io_num = miso_pin_,
      .sclk_io_num = clk_pin_,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
      .flags = 0,
      .intr_flags = 0};
  
  esp_err_t ret = spi_bus_initialize(static_cast<spi_host_device_t>(spi_bus_), &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
    return false;
  }
  
  // SDSPI slot configuration
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = static_cast<gpio_num_t>(cs_pin_->get_pin());
  slot_config.host_id = static_cast<spi_host_device_t>(spi_bus_);
  
  // Mount configuration
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = max_files_,
      .allocation_unit_size = 16 * 1024};
  
  // Mount filesystem
  ret = esp_vfs_fat_sdspi_mount(mount_point_, &host, &slot_config, &mount_config, &card_);
  
  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
    }
    return false;
  }
  
  // Print card info
  sdmmc_card_print_info(stdout, card_);
  
  return true;
}

void SDSPIComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SDSPI Configuration:");
  LOG_PIN("  CS Pin: ", cs_pin_);
  ESP_LOGCONFIG(TAG, "  Mount point: %s", mount_point_);
  ESP_LOGCONFIG(TAG, "  Max files: %d", max_files_);
  ESP_LOGCONFIG(TAG, "  Mounted: %s", mounted_ ? "Yes" : "No");
  
  if (mounted_) {
    ESP_LOGCONFIG(TAG, "  Card size: %llu MB", get_card_size() / (1024 * 1024));
    ESP_LOGCONFIG(TAG, "  Free space: %llu MB", get_free_space() / (1024 * 1024));
    ESP_LOGCONFIG(TAG, "  Card type: %s", get_card_type());
  }
}

// File operations implementation
bool SDSPIComponent::file_exists(const char *path) {
  if (!mounted_) return false;
  
  FILE *f = fopen(path, "r");
  if (f) {
    fclose(f);
    return true;
  }
  return false;
}

bool SDSPIComponent::create_file(const char *path) {
  if (!mounted_) return false;
  
  FILE *f = fopen(path, "w");
  if (!f) {
    ESP_LOGE(TAG, "Failed to create file: %s", path);
    return false;
  }
  fclose(f);
  return true;
}

bool SDSPIComponent::write_file(const char *path, const char *data) {
  if (!mounted_) return false;
  
  FILE *f = fopen(path, "w");
  if (!f) {
    ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
    return false;
  }
  
  fprintf(f, "%s", data);
  fclose(f);
  return true;
}

bool SDSPIComponent::read_file(const char *path, char *buffer, size_t buffer_size) {
  if (!mounted_) return false;
  
  FILE *f = fopen(path, "r");
  if (!f) {
    ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
    return false;
  }
  
  size_t bytes_read = fread(buffer, 1, buffer_size - 1, f);
  buffer[bytes_read] = '\0';
  fclose(f);
  
  return bytes_read > 0;
}

bool SDSPIComponent::delete_file(const char *path) {
  if (!mounted_) return false;
  
  return unlink(path) == 0;
}

bool SDSPIComponent::create_directory(const char *path) {
  if (!mounted_) return false;
  
  return mkdir(path, 0755) == 0;
}

bool SDSPIComponent::directory_exists(const char *path) {
  if (!mounted_) return false;
  
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

uint64_t SDSPIComponent::get_card_size() {
  if (!mounted_ || !card_) return 0;
  return (uint64_t) card_->csd.capacity * card_->csd.sector_size;
}

uint64_t SDSPIComponent::get_free_space() {
  if (!mounted_) return 0;
  
  FATFS *fs;
  DWORD fre_clust;
  FRESULT res = f_getfree("0:", &fre_clust, &fs);
  if (res != FR_OK) {
    ESP_LOGE(TAG, "Failed to get free space: %d", res);
    return 0;
  }
  
  uint64_t total_sectors = (fs->n_fatent - 2) * fs->csize;
  uint64_t free_sectors = fre_clust * fs->csize;
  
  return free_sectors * 512; // Assuming 512 bytes per sector
}

const char* SDSPIComponent::get_card_type() {
  if (!mounted_ || !card_) return "Unknown";
  
  switch (card_->type) {
    case SDMMC_CARD_TYPE_SDSC:
      return "SDSC";
    case SDMMC_CARD_TYPE_SDHC:
      return "SDHC";
    case SDMMC_CARD_TYPE_SDXC:
      return "SDXC";
    default:
      return "Unknown";
  }
}

}  // namespace sdspi
}  // namespace esphome
