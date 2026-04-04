import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

CODEOWNERS = ["@sberard"]

phase_cut_ns = cg.esphome_ns.namespace("phase_cut")
PhaseCutController = phase_cut_ns.class_("PhaseCutController", cg.Component)

CONF_PHASE_CUT_ID = "phase_cut_id"
CONF_ZERO_CROSS_PIN = "zero_cross_pin"
CONF_TRIAC_PIN = "triac_pin"
CONF_MAX_LEVEL = "max_level"
CONF_MIN_EDGE_INTERVAL = "min_edge_interval"
CONF_INVERTED = "inverted"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PhaseCutController),
        cv.Required(CONF_ZERO_CROSS_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_TRIAC_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_MAX_LEVEL, default=10): cv.int_range(min=2, max=100),
        cv.Optional(CONF_MIN_EDGE_INTERVAL, default="5ms"): cv.positive_time_period_microseconds,
        cv.Optional(CONF_INVERTED, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    zc_pin = await cg.gpio_pin_expression(config[CONF_ZERO_CROSS_PIN])
    cg.add(var.set_zero_cross_pin(zc_pin))

    triac_pin = await cg.gpio_pin_expression(config[CONF_TRIAC_PIN])
    cg.add(var.set_triac_pin(triac_pin))

    cg.add(var.set_max_level(config[CONF_MAX_LEVEL]))
    cg.add(var.set_min_edge_interval(config[CONF_MIN_EDGE_INTERVAL]))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
