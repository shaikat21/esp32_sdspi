#pragma once

#include "esphome.h"
#include "sdspi_component.h"

namespace esphome {
namespace sdspi {

class CSVReader : public PollingComponent, public text_sensor::TextSensor {
 public:
  CSVReader(SDSPIComponent *sdspi, const char *filename) 
    : PollingComponent(10000), sdspi_(sdspi), filename_(filename) {}
  
  void update() override {
    if (!sdspi_->is_mounted()) {
      publish_state("SD card not mounted");
      return;
    }
    
    size_t row_count = sdspi_->count_csv_rows(filename_);
    if (row_count == 0) {
      publish_state("No data in CSV");
      return;
    }
    
    // Read the last row
    std::vector<std::string> columns;
    if (sdspi_->read_csv_row(filename_, columns, row_count - 1)) {
      std::string last_row;
      for (size_t i = 0; i < columns.size(); i++) {
        if (!last_row.empty()) last_row += ", ";
        last_row += columns[i];
      }
      publish_state(last_row);
    } else {
      publish_state("Failed to read CSV");
    }
  }
  
  // Method to get specific row
  std::string get_row(size_t index) {
    if (!sdspi_->is_mounted()) return "SD card not mounted";
    
    std::vector<std::string> columns;
    if (sdspi_->read_csv_row(filename_, columns, index)) {
      std::string row;
      for (size_t i = 0; i < columns.size(); i++) {
        if (!row.empty()) row += ", ";
        row += columns[i];
      }
      return row;
    }
    return "Row not found";
  }
  
  // Method to get total rows
  size_t get_row_count() {
    return sdspi_->count_csv_rows(filename_);
  }
  
 protected:
  SDSPIComponent *sdspi_;
  const char *filename_;
};

}  // namespace sdspi
}  // namespace esphome
