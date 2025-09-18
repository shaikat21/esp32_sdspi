import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

sdspi_ns = cg.esphome_ns.namespace("sdspi")
SDSPICard = sdspi_ns.class_("SDSPICard", cg.Component)

CONF_CS = "cs_pin"
CONF_SCLK = "sclk_pin"
CONF_MOSI = "mosi_pin"
CONF_MISO = "miso_pin"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SDSPICard),
        cv.Required(CONF_CS): cv.pin_schema(),
        cv.Required(CONF_SCLK): cv.pin_schema(),
        cv.Required(CONF_MOSI): cv.pin_schema(),
        cv.Required(CONF_MISO): cv.pin_schema(),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    cs = await cg.gpio_pin_expression(config[CONF_CS])
    sclk = await cg.gpio_pin_expression(config[CONF_SCLK])
    mosi = await cg.gpio_pin_expression(config[CONF_MOSI])
    miso = await cg.gpio_pin_expression(config[CONF_MISO])

    var = cg.new_Pvariable(config[CONF_ID], cs, sclk, mosi, miso)
    await cg.register_component(var, config)
