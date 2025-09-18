#pragma once

#include "esphome.h"
#include "esphome/components/spi/spi.h"
#include "driver/sdspi_host.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

namespace esphome {
namespace sdspi {

class SDSPIComponent : public Component, public spi::SPIDevice {
 public:
  SDSPIComponent() : spi::SPIDevice() {}
  
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  // Configuration methods
  void set_cs_pin(GPIOPin *cs_pin) { cs_pin_ = cs_pin; }
  void set_mount_point(const char *mount_point) { mount_point_ = mount_point; }
  void set_max_files(int max_files) { max_files_ = max_files; }
  
  // File operations
  bool file_exists(const char *path);
  bool create_file(const char *path);
  bool write_file(const char *path, const char *data);
  bool read_file(const char *path, char *buffer, size_t buffer_size);
  bool delete_file(const char *path);
  
  // Directory operations
  bool create_directory(const char *path);
  bool directory_exists(const char *path);
  
  // Card info
  uint64_t get_card_size();
  uint64_t get_free_space();
  const char* get_card_type();
  
  bool is_mounted() const { return mounted_; }

 protected:
  GPIOPin *cs_pin_{nullptr};
  const char *mount_point_{"/sdcard"};
  int max_files_{5};
  bool mounted_{false};
  sdmmc_card_t *card_{nullptr};
  
  bool initialize_sd_card();
};

}  // namespace sdspi
}  // namespace esphome
