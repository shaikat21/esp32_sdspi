import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.const import (
    CONF_ID,
    CONF_DATA,
    CONF_PATH,
    CONF_CLK_PIN,
    CONF_OUTPUT,
    CONF_PULLUP,
    CONF_PULLDOWN,
)
from esphome.core import CORE

DEPENDENCIES = ["esp32"]

CONF_SD_SPI_CARD_ID = "sd_spi_card_id"
CONF_MOSI_PIN = "mosi_pin"
CONF_MISO_PIN = "miso_pin"
CONF_CS_PIN = "cs_pin"
CONF_POWER_CTRL_PIN = "power_ctrl_pin"

sd_spi_card_component_ns = cg.esphome_ns.namespace("sd_spi_card")
SdSpi = sd_spi_card_component_ns.class_("SdSpi", cg.Component)

# Actions
SdSpiWriteFileAction = sd_spi_card_component_ns.class_("SdSpiWriteFileAction", automation.Action)
SdSpiAppendFileAction = sd_spi_card_component_ns.class_("SdSpiAppendFileAction", automation.Action)
SdSpiCreateDirectoryAction = sd_spi_card_component_ns.class_("SdSpiCreateDirectoryAction", automation.Action)
SdSpiRemoveDirectoryAction = sd_spi_card_component_ns.class_("SdSpiRemoveDirectoryAction", automation.Action)
SdSpiDeleteFileAction = sd_spi_card_component_ns.class_("SdSpiDeleteFileAction", automation.Action)

def validate_raw_data(value):
    if isinstance(value, str):
        return value.encode("utf-8")
    if isinstance(value, list):
        return cv.Schema([cv.hex_uint8_t])(value)
    raise cv.Invalid(
        "data must either be a string wrapped in quotes or a list of bytes"
    )

CONFIG_SCHEMA = cv.All(
    cv.require_esphome_version(2025,7,0),
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SdSpi),
            cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_MISO_PIN): pins.internal_gpio_pin_number,
            cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_POWER_CTRL_PIN) : pins.gpio_pin_schema({
                CONF_OUTPUT: True,
                CONF_PULLUP: False,
                CONF_PULLDOWN: False,
            }),
        }
    ).extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_clk_pin(config[CONF_CLK_PIN]))
    cg.add(var.set_mosi_pin(config[CONF_MOSI_PIN]))
    cg.add(var.set_miso_pin(config[CONF_MISO_PIN]))
    cg.add(var.set_cs_pin(config[CONF_CS_PIN]))

    if CONF_POWER_CTRL_PIN in config:
        power_ctrl = await cg.gpio_pin_expression(config[CONF_POWER_CTRL_PIN])
        cg.add(var.set_power_ctrl_pin(power_ctrl))

# ---------------- Automation Actions ----------------

SD_SPI_PATH_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(SdSpi),
        cv.Required(CONF_PATH): cv.templatable(cv.string_strict),
    }
)

SD_SPI_WRITE_FILE_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(SdSpi),
        cv.Required(CONF_PATH): cv.templatable(cv.string_strict),
        cv.Required(CONF_DATA): cv.templatable(validate_raw_data),
    }
).extend(SD_SPI_PATH_ACTION_SCHEMA)


@automation.register_action(
    "sd_spi_card.write_file", SdSpiWriteFileAction, SD_SPI_WRITE_FILE_ACTION_SCHEMA
)
async def sd_spi_write_file_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    path_ = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    data_ = await cg.templatable(config[CONF_DATA], args, cg.std_vector.template(cg.uint8))
    cg.add(var.set_path(path_))
    cg.add(var.set_data(data_))
    return var


@automation.register_action(
    "sd_spi_card.append_file", SdSpiAppendFileAction, SD_SPI_WRITE_FILE_ACTION_SCHEMA
)
async def sd_spi_append_file_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    path_ = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    data_ = await cg.templatable(config[CONF_DATA], args, cg.std_vector.template(cg.uint8))
    cg.add(var.set_path(path_))
    cg.add(var.set_data(data_))
    return var


@automation.register_action(
    "sd_spi_card.create_directory", SdSpiCreateDirectoryAction, SD_SPI_PATH_ACTION_SCHEMA
)
async def sd_spi_create_directory_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    path_ = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    cg.add(var.set_path(path_))
    return var


@automation.register_action(
    "sd_spi_card.remove_directory", SdSpiRemoveDirectoryAction, SD_SPI_PATH_ACTION_SCHEMA
)
async def sd_spi_remove_directory_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    path_ = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    cg.add(var.set_path(path_))
    return var


@automation.register_action(
    "sd_spi_card.delete_file", SdSpiDeleteFileAction, SD_SPI_PATH_ACTION_SCHEMA
)
async def sd_spi_delete_file_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    path_ = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    cg.add(var.set_path(path_))
    return var
