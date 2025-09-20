import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.const import CONF_ID, CONF_CS_PIN

CODEOWNERS = ["@shaikat21"]
DEPENDENCIES = ["spi"]

sdspi_ns = cg.esphome_ns.namespace("sdspi")
SDSPIComponent = sdspi_ns.class_("SDSPIComponent", cg.Component, spi.SPIDevice)

CONF_MOUNT_POINT = "mount_point"
CONF_MAX_FILES = "max_files"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SDSPIComponent),
    cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_MOUNT_POINT, default="/sdcard"): cv.string,
    cv.Optional(CONF_MAX_FILES, default=5): cv.int_range(min=1, max=10),
}).extend(spi.spi_device_schema(cs_pin_required=False))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
    
    cs_pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs_pin))
    cg.add(var.set_mount_point(config[CONF_MOUNT_POINT]))
    cg.add(var.set_max_files(config[CONF_MAX_FILES]))
