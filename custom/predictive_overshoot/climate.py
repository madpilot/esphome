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
    CONF_COOL_DEADBAND,
    CONF_HEAT_DEADBAND,
    CONF_MIN_COOLING_OFF_TIME,
    CONF_MIN_COOLING_RUN_TIME,
    CONF_MAX_COOLING_RUN_TIME,
    CONF_MIN_HEATING_OFF_TIME,
    CONF_MIN_HEATING_RUN_TIME,
    CONF_MAX_HEATING_RUN_TIME,
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
            cv.Optional(CONF_COOL_DEADBAND): cv.float_,
            cv.Optional(CONF_HEAT_DEADBAND): cv.float_,
            cv.Optional(CONF_MIN_COOLING_OFF_TIME): cv.positive_time_period_seconds,
            cv.Optional(CONF_MIN_COOLING_RUN_TIME): cv.positive_time_period_seconds,
            cv.Optional(CONF_MAX_COOLING_RUN_TIME): cv.positive_time_period_seconds,
            cv.Optional(CONF_MIN_HEATING_OFF_TIME): cv.positive_time_period_seconds,
            cv.Optional(CONF_MIN_HEATING_RUN_TIME): cv.positive_time_period_seconds,
            cv.Optional(CONF_MAX_HEATING_RUN_TIME): cv.positive_time_period_seconds,
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

    if CONF_HEAT_DEADBAND in config:
        cg.add(var.set_deadband_low(config[CONF_HEAT_DEADBAND]))

    if CONF_COOL_DEADBAND in config:
        cg.add(var.set_deadband_high(config[CONF_COOL_DEADBAND]))

    if CONF_MIN_COOLING_OFF_TIME in config:
        cg.add(var.set_cooling_off_time(config[CONF_MIN_COOLING_OFF_TIME]))

    if CONF_MIN_COOLING_RUN_TIME in config:
        cg.add(var.set_cooling_minimum_on_time(config[CONF_MIN_COOLING_RUN_TIME]))

    if CONF_MAX_COOLING_RUN_TIME in config:
        cg.add(var.set_cooling_maximum_on_time(config[CONF_MAX_COOLING_RUN_TIME]))

    if CONF_MIN_HEATING_OFF_TIME in config:
        cg.add(var.set_heating_off_time(config[CONF_MIN_HEATING_OFF_TIME]))

    if CONF_MIN_HEATING_RUN_TIME in config:
        cg.add(var.set_heating_minimum_on_time(config[CONF_MIN_HEATING_RUN_TIME]))

    if CONF_MAX_HEATING_RUN_TIME in config:
        cg.add(var.set_heating_maximum_on_time(config[CONF_MAX_HEATING_RUN_TIME]))
