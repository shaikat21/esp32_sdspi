#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/core/defines.h"
#include <vector>
#include <string>

#ifdef USE_ESP_IDF
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#endif

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
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

class SdSpiCard : public PollingComponent {
 #ifdef USE_SENSOR
  SUB_SENSOR(used_space)
  SUB_SENSOR(total_space)
  SUB_SENSOR(free_space)
#endif
 
 public:
 
  void setup() override;
  void dump_config() override;
  void update() override;

  void set_cs_pin(GPIOPin *pin) { cs_pin_ = pin; }
  void set_clk_pin(GPIOPin *pin) { clk_pin_ = pin; }
  void set_mosi_pin(GPIOPin *pin) { mosi_pin_ = pin; }
  void set_miso_pin(GPIOPin *pin) { miso_pin_ = pin; }
  void set_spi_freq(int freq) { spi_freq_khz_ = freq; }
 
  size_t file_size(const char *path);
  
  // --- CSV Helpers (vector based) ---
  bool csv_append_row(const char *path, const std::vector<std::string> &cells);
  int  csv_row_count(const char *path);
  bool csv_delete_rows(const char *path, int row_start, int row_end);
  bool csv_keep_last_n(const char *path, int max_rows);
  std::vector<std::vector<std::string>> csv_read_rows_range(
          const char *path, int row_start, int row_end,
            int cond_col_index = -1, const char *condition = "");

  

#ifdef USE_SENSOR
  void add_file_size_sensor(sensor::Sensor *s, const char *path);
#endif

#ifdef USE_BINARY_SENSOR
  SUB_BINARY_SENSOR(card_status)
#endif


  bool check_kappa();
  void handle_sd_failure(const char *reason);
  void try_remount();
  void mount_card();
  void append_file(const char *path, const char *line);
  void write_file(const char *path, const char *line);
  bool delete_file(const char *path);
  bool csv_replace_col(const char *path, int row_index, int col_index, const char *new_value);

 protected:
  sdmmc_card_t *card_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  GPIOPin *clk_pin_{nullptr};
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *miso_pin_{nullptr};
  int spi_freq_khz_{1000};   // default 1 MHz
  esp_err_t last_sd_error_ = ESP_OK;
  
  
 #ifdef USE_SENSOR
  std::vector<FileSizeSensor> file_size_sensors_{};
 #endif
  void update_sensors();
  std::vector<FileInfo> &list_directory_file_info_rec(const char *path, uint8_t depth, std::vector<FileInfo> &list);
  static std::string error_code_to_string();
  
};

}  // namespace sd_spi_card
}  // namespace esphome
