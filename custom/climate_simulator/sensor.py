import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.automation import maybe_simple_id
from esphome.components import sensor
from esphome import automation
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)


simulator_ns = cg.esphome_ns.namespace("simulator")
ClimateSimulator = simulator_ns.class_(
    "ClimateSimulator", cg.PollingComponent, sensor.Sensor
)

CONFIG_SCHEMA = sensor.sensor_schema(
    ClimateSimulator,
    unit_of_measurement=UNIT_CELSIUS,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_TEMPERATURE,
    state_class=STATE_CLASS_MEASUREMENT,
)

ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(ClimateSimulator),
    }
)

CoolingOffAction = simulator_ns.class_("CoolingOffAction", automation.Action)
CoolingOnAction = simulator_ns.class_("CoolingOnAction", automation.Action)
HeatingOffAction = simulator_ns.class_("HeatingOffAction", automation.Action)
HeatingOnAction = simulator_ns.class_("HeatingOnAction", automation.Action)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)


@automation.register_action(
    "climate_simulator.cooling_off", CoolingOffAction, ACTION_SCHEMA
)
async def climate_simulator_cooling_off_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "climate_simulator.cooling_on", CoolingOnAction, ACTION_SCHEMA
)
async def climate_simulator_cooling_on_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "climate_simulator.heating_off", HeatingOffAction, ACTION_SCHEMA
)
async def climate_simulator_heating_off_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "climate_simulator.heating_on", HeatingOnAction, ACTION_SCHEMA
)
async def climate_simulator_heating_on_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)
