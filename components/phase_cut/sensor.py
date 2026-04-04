import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    STATE_CLASS_MEASUREMENT,
    UNIT_HERTZ,
    UNIT_PERCENT,
    ICON_FLASH,
)

from . import phase_cut_ns, PhaseCutController, CONF_PHASE_CUT_ID

CONF_ZC_FREQUENCY = "zc_frequency"
CONF_POWER = "power"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_PHASE_CUT_ID): cv.use_id(PhaseCutController),
        cv.Optional(CONF_ZC_FREQUENCY): sensor.sensor_schema(
            unit_of_measurement=UNIT_HERTZ,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
            icon="mdi:sine-wave",
        ),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            icon=ICON_FLASH,
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PHASE_CUT_ID])

    if zc_conf := config.get(CONF_ZC_FREQUENCY):
        sens = await sensor.new_sensor(zc_conf)
        cg.add(parent.set_zc_frequency_sensor(sens))

    if power_conf := config.get(CONF_POWER):
        sens = await sensor.new_sensor(power_conf)
        cg.add(parent.set_power_sensor(sens))
