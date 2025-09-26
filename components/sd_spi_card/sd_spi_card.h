#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/core/defines.h"

#ifdef USE_ESP_IDF
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#endif

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome {
namespace sd_spi_card {

// Struct for per-file sensors
#ifdef USE_SENSOR
struct FileSizeSensor {
  sensor::Sensor *sensor{nullptr};
  std::string path;

  FileSizeSensor() = default;
  FileSizeSensor(sensor::Sensor *s, const std::string &p) : sensor(s), path(p) {}
};
#endif

struct FileInfo {
  std::string path;
  size_t size;
  bool is_directory;

  FileInfo(std::string const &, size_t, bool);
};

class SdSpiCard : public Component {
 #ifdef USE_SENSOR
  SUB_SENSOR(used_space)
  SUB_SENSOR(total_space)
  SUB_SENSOR(free_space)
#endif
 
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;

  void set_cs_pin(GPIOPin *pin) { cs_pin_ = pin; }
  void set_clk_pin(GPIOPin *pin) { clk_pin_ = pin; }
  void set_mosi_pin(GPIOPin *pin) { mosi_pin_ = pin; }
  void set_miso_pin(GPIOPin *pin) { miso_pin_ = pin; }
  void set_spi_freq(int freq) { spi_freq_khz_ = freq; }
 
  size_t file_size(const char *path);

#ifdef USE_SENSOR
  void add_file_size_sensor(sensor::Sensor *s, const char *path);
#endif

 protected:
  sdmmc_card_t *card_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  GPIOPin *clk_pin_{nullptr};
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *miso_pin_{nullptr};
  int spi_freq_khz_{1000};   // default 1 MHz
  
  
 #ifdef USE_SENSOR
  std::vector<FileSizeSensor> file_size_sensors_{};
 #endif
  void update_sensors();
  std::vector<FileInfo> &list_directory_file_info_rec(const char *path, uint8_t depth, std::vector<FileInfo> &list);
  static std::string error_code_to_string();
  
};

}  // namespace sd_spi_card
}  // namespace esphome
