import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number

from . import burst_fire_ns, BurstFireController, CONF_BURST_FIRE_ID

BurstFireNumber = burst_fire_ns.class_(
    "BurstFireNumber", number.Number, cg.Component
)

CONFIG_SCHEMA = (
    number.number_schema(BurstFireNumber, icon="mdi:flash")
    .extend(
        {
            cv.GenerateID(CONF_BURST_FIRE_ID): cv.use_id(BurstFireController),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await number.new_number(config, min_value=0, max_value=10, step=1)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_BURST_FIRE_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_power_number(var))
