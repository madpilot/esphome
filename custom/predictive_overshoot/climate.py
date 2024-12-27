import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor
from esphome import automation
from esphome.const import (
    CONF_SENSOR,
    CONF_ID,
    CONF_IDLE_ACTION,
    CONF_COOL_ACTION,
    CONF_HEAT_ACTION,
)

predictive_overshoot_ns = cg.esphome_ns.namespace("predictive_overshoot")
PredictiveOvershootClimate = predictive_overshoot_ns.class_(
    "PredictiveOvershootClimate", climate.Climate, cg.Component
)

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(PredictiveOvershootClimate),
            cv.Required(CONF_SENSOR): cv.use_id(sensor.Sensor),
            cv.Required(CONF_IDLE_ACTION): automation.validate_automation(single=True),
            cv.Optional(CONF_COOL_ACTION): automation.validate_automation(single=True),
            cv.Optional(CONF_HEAT_ACTION): automation.validate_automation(single=True),
            # cv.Optional("deadband_lower"): cv.float_,
            # cv.Optional("deadband_upper"): cv.float_,
            # cv.Optional("cooling_time_off"): cv.int_,
            # cv.Optional("cooling_time_minimum_on"): cv.int_,
            # cv.Optional("cooling_time_maximum_on"): cv.int_,
            # cv.Optional("heating_time_off"): cv.int_,
            # cv.Optional("heating_time_minimum_on"): cv.int_,
            # cv.Optional("heating_time_maximum_on"): cv.int_,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.has_at_least_one_key(CONF_COOL_ACTION, CONF_HEAT_ACTION),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    sens = await cg.get_variable(config[CONF_SENSOR])
    cg.add(var.set_sensor(sens))

    await automation.build_automation(
        var.get_idle_action_trigger(), [], config[CONF_IDLE_ACTION]
    )

    if CONF_COOL_ACTION in config:
        await automation.build_automation(
            var.get_cool_action_trigger(), [], config[CONF_COOL_ACTION]
        )

    if CONF_HEAT_ACTION in config:
        await automation.build_automation(
            var.get_heat_action_trigger(), [], config[CONF_HEAT_ACTION]
        )
