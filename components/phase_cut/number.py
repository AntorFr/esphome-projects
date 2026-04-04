import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number

from . import phase_cut_ns, PhaseCutController, CONF_PHASE_CUT_ID

PhaseCutNumber = phase_cut_ns.class_(
    "PhaseCutNumber", number.Number, cg.Component
)

CONFIG_SCHEMA = (
    number.number_schema(PhaseCutNumber, icon="mdi:flash")
    .extend(
        {
            cv.GenerateID(CONF_PHASE_CUT_ID): cv.use_id(PhaseCutController),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await number.new_number(config, min_value=0, max_value=10, step=1)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_PHASE_CUT_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.set_power_number(var))
