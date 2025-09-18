#pragma once

#include "esphome/core/component.h"

namespace esphome {
namespace sdspi {

class SDSPICard : public Component {
 public:
  // Constructor: pass pins etc
  SDSPICard(int8_t cs_pin, int8_t sck_pin, int8_t mosi_pin, int8_t miso_pin);

  void setup() override;
  void dump_config() override;

  // Example method: list root directory
  void list_root();

 protected:
  int8_t cs_pin_, sck_pin_, mosi_pin_, miso_pin_;
};

}  // namespace sdspi_card
}  // namespace esphome

