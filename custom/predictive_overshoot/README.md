# Predictive Overshoot Climate Controller

The `predictive_overshoot` climate controllers is an adaptive bang-bang temperature controller, which automatically learns the effects of "overshoot".

The controller will turn the cooling or heating off early using the natural overshoot of the system to hit the target set point.

## Example configuration

```yaml
climate:
  - platform: predictive_overshoot
    name: "Predictive Overshoot Climate Controller"
    sensor: my_temperature_sensor
    cool_action:
      - switch.turn_on: compressor
    idle_action:
      - switch.turn_off: heater
    cool_deadband: 1
    heat_deadband: 1
    min_cooling_off_time: 20s
    min_cooling_run_time: 5s
    max_cooling_run_time: 20s
    min_heating_off_time: 20s
    min_heating_run_time: 5s
    max_heating_run_time: 20s
    kp: 0.03
    min_percentage_increase: 1.2
    max_percentage_increase: 1.5
    min_percentage_decrease: 1.17
    max_percentage_decrease: 1.33
```

## Configuration Variables

The thermostat controller uses the sensor to determine whether it should heat or cool.

- `sensor` (**Required**, ID): The sensor that is used to measure the current temperature.

## Heating and Cooling Actions

- `idle_action` (**Required**, Action): The action to call when the climate device should enter its idle state (not cooling, not heating).
- `cool_action` (**Optional**, Action): The action to call when the climate device should enter cooling mode to decrease the current temperature.
- `heat_action` (**Optional**, Action): The action to call when the climate device should enter heating mode to increase the current temperature.

## Controller Behavior and Hysteresis

- `cool_deadband` (**Optional**, float): The minimum temperature differential (temperature above the set point) before **engaging** cooling
- `heat_deadband` (**Optional**, float): The minimum temperature differential (temperature below the set point) before **engaging** heat
- `min_cooling_off_time` (**Optional**, [Time](https://esphome.io/guides/configuration-types#config-time)): Minimum time the cooling system needs to be idle between cycles.
- `min_cooling_run_time` (**Optional**, [Time](https://esphome.io/guides/configuration-types#config-time)): Minimum run time the cooling system needs to run before becoming idle.
- `max_cooling_run_time` (**Optional**, [Time](https://esphome.io/guides/configuration-types#config-time)): Maximum run time the cooling system can run for.
- `min_heating_off_time` (**Optional**, [Time](https://esphome.io/guides/configuration-types#config-time)): Minimum time the heating system needs to be idle between cycles.
- `min_heating_run_time` (**Optional**, [Time](https://esphome.io/guides/configuration-types#config-time)): Minimum run time the heating system needs to run before becoming idle.
- `max_heating_run_time` (**Optional**, [Time](https://esphome.io/guides/configuration-types#config-time)): Maximum run time the heating system can run for.

- `kp` (**Optional**, float): Proportional error correction factor. Default: 0.03

- `min_percentage_increase` (**Optional**, float): Minimum percentage to increase the overshoot correction by. Default: 1.2
- `max_percentage_increase` (**Optional**, float): Maximum percentage to increase the overshoot correction by. Default: 1.5
- `min_percentage_decrease` (**Optional**, float): Minimum percentage to decrease the overshoot correction by. Default: 1.17
- `max_percentage_decrease` (**Optional**, float): Maximum percentage to decrease the overshoot correction by. Default: 1.33
