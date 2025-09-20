#pragma once

#include "esphome.h"
#include "sdspi_component.h"

namespace esphome {
namespace sdspi {

class CSVSensor : public PollingComponent, public Sensor {
 public:
  CSVSensor(SDSPIComponent *sdspi, const char *filename) 
    : PollingComponent(5000), sdspi_(sdspi), filename_(filename) {}
  
  void set_value_column(size_t column) { value_column_ = column; }
  void set_timestamp_column(size_t column) { timestamp_column_ = column; }
  
  void setup() override {
    // Create headers if file doesn't exist
    std::vector<std::string> headers = {"timestamp", "value"};
    sdspi_->append_csv_header(filename_, headers);
  }
  
  void update() override {
    if (!sdspi_->is_mounted()) {
      publish_state(NAN);
      return;
    }
    
    size_t row_count = sdspi_->count_csv_rows(filename_);
    if (row_count == 0) {
      publish_state(NAN);
      return;
    }
    
    // Read the last row
    std::vector<std::string> columns;
    if (sdspi_->read_csv_row(filename_, columns, row_count - 1)) {
      if (value_column_ < columns.size()) {
        try {
          float value = std::stof(columns[value_column_]);
          publish_state(value);
        } catch (...) {
          ESP_LOGE("CSV", "Failed to parse value: %s", columns[value_column_].c_str());
          publish_state(NAN);
        }
      }
    } else {
      publish_state(NAN);
    }
  }
  
  // Method to log new data
  void log_data(float value, const char *timestamp = nullptr) {
    if (!sdspi_->is_mounted()) return;
    
    std::vector<std::string> columns;
    
    // Add timestamp if provided
    if (timestamp) {
      columns.push_back(timestamp);
    } else {
      // Generate current timestamp
      char time_buf[64];
      time_t now = time(nullptr);
      struct tm *timeinfo = localtime(&now);
      strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", timeinfo);
      columns.push_back(time_buf);
    }
    
    // Add the value
    columns.push_back(to_string(value));
    
    // Write to CSV
    sdspi_->write_csv_row(filename_, columns, true);
    
    ESP_LOGD("CSV", "Logged data: %s, %.2f", columns[0].c_str(), value);
  }
  
 protected:
  SDSPIComponent *sdspi_;
  const char *filename_;
  size_t value_column_{1}; // Default to second column (after timestamp)
  size_t timestamp_column_{0};
};

}  // namespace sdspi
}  // namespace esphome
