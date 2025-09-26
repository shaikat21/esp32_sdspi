#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP_IDF
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#endif

namespace esphome {
namespace sd_spi_card {

class SdSpiCard : public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_cs_pin(GPIOPin *pin) { cs_pin_ = pin; }
  void set_clk_pin(GPIOPin *pin) { clk_pin_ = pin; }
  void set_mosi_pin(GPIOPin *pin) { mosi_pin_ = pin; }
  void set_miso_pin(GPIOPin *pin) { miso_pin_ = pin; }
  void set_spi_freq(int freq) { spi_freq_khz_ = freq; }

 protected:
  sdmmc_card_t *card_{nullptr};
  GPIOPin *cs_pin_{nullptr};
  GPIOPin *clk_pin_{nullptr};
  GPIOPin *mosi_pin_{nullptr};
  GPIOPin *miso_pin_{nullptr};
  int spi_freq_khz_{1000};   // default 1 MHz
};

}  // namespace sd_spi_card
}  // namespace esphome
