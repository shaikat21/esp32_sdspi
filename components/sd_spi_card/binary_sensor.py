import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_TYPE

from . import (
    SdSpiCard,
    CONF_SD_SPI_CARD_ID,
)

DEPENDENCIES = ["sd_spi_card"]

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema().extend(
    {
        cv.GenerateID(CONF_SD_SPI_CARD_ID): cv.use_id(SdSpiCard),
        cv.Optional(CONF_TYPE, default="card_status"): cv.string,
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_SD_SPI_CARD_ID])
    sens = await binary_sensor.new_binary_sensor(config)
    cg.add(parent.set_card_status_binary_sensor(sens))
