import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.light.types import AddressableLightEffect
from esphome.components.light.effects import register_addressable_effect

from esphome.const import (
    CONF_NAME
)

CONF_STARS_PROBABILITY = "stars_probability"
CONF_CHRISTMASS_BIT_SIZE = "bit_size"
CONF_CHRISTMASS_BLANK_SIZE = "blank_size"

light_ns = cg.esphome_ns.namespace("light")
AddressableStarsEffect = light_ns.class_("AddressableStarsEffect", AddressableLightEffect)
AddressableChristmasEffect = light_ns.class_("AddressableChristmasEffect", AddressableLightEffect)

CONFIG_SCHEMA = cv.All(cv.Schema({}), cv.only_with_arduino)

@register_addressable_effect(
    "addressable_stars",
    AddressableStarsEffect,
    "Stars",
    {
        cv.Optional(CONF_STARS_PROBABILITY, default="1%"): cv.percentage,
    },
)
async def addressable_stars_effect_to_code(config, effect_id):
    var = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(var.set_stars_probability(config[CONF_STARS_PROBABILITY]))
    return var

@register_addressable_effect(
    "addressable_christmas",
    AddressableChristmasEffect,
    "Christmas",
    {
        cv.Optional(CONF_CHRISTMASS_BIT_SIZE, default="1"): cv.uint8_t,
        cv.Optional(CONF_CHRISTMASS_BLANK_SIZE, default="0"): cv.uint8_t,
    },

)
async def addressable_christmas_effect_to_code(config, effect_id):
    var = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(var.set_bit_size(config[CONF_CHRISTMASS_BIT_SIZE]))
    cg.add(var.set_blank_size(config[CONF_CHRISTMASS_BLANK_SIZE]))
    return var
