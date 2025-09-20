import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID, CONF_CS_PIN, CONF_MAX_FILES, CONF_MOUNT_POINT

DEPENDENCIES = ["spi"]

sdspi_csv_ns = cg.esphome_ns.namespace("sdspi_csv")
SDSpiCSV = sdspi_csv_ns.class_("SDSpiCSV", cg.Component, spi.SPIDevice)

CONF_CS_PIN = "cs_pin"
CONF_MAX_FILES = "max_files"
CONF_MOUNT_POINT = "mount_point"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SDSpiCSV),
            cv.Required(CONF_CS_PIN): cv.gpio_pin_schema(),
            cv.Optional(CONF_MAX_FILES, default=10): cv.int_,
            cv.Optional(CONF_MOUNT_POINT, default="/sdcard"): cv.string,
        }
    )
    .extend(spi.SPI_DEVICE_SCHEMA)   # << needed to get `spi_id` and reuse existing SPI bus 
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    cs = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs))
    cg.add(var.set_max_files(config[CONF_MAX_FILES]))
    cg.add(var.set_mount_point(config[CONF_MOUNT_POINT]))
