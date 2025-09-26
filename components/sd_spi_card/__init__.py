import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

sd_spi_card_ns = cg.esphome_ns.namespace("sd_spi_card")
SdSpiCard = sd_spi_card_ns.class_("SdSpiCard", cg.Component)

CONF_SPI_FREQ = "spi_freq"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SdSpiCard),
    cv.Required("cs_pin"): pins.gpio_output_pin_schema,
    cv.Required("clk_pin"): pins.gpio_output_pin_schema,
    cv.Required("mosi_pin"): pins.gpio_output_pin_schema,
    cv.Required("miso_pin"): pins.gpio_output_pin_schema,
    cv.Optional(CONF_SPI_FREQ, default=1000): cv.int_range(min=100, max=40000),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cs = await cg.gpio_pin_expression(config["cs_pin"])
    clk = await cg.gpio_pin_expression(config["clk_pin"])
    mosi = await cg.gpio_pin_expression(config["mosi_pin"])
    miso = await cg.gpio_pin_expression(config["miso_pin"])

    cg.add(var.set_cs_pin(cs))
    cg.add(var.set_clk_pin(clk))
    cg.add(var.set_mosi_pin(mosi))
    cg.add(var.set_miso_pin(miso))

    cg.add(var.set_spi_freq(config[CONF_SPI_FREQ]))
