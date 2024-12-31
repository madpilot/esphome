#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct float_range
  {
    float lower;
    float upper;
  } float_range_t;

#define PREDICTIVE_OVERSHOOT_ENABLE_MASK 0x1
#define PREDICTIVE_OVERSHOOT_MODE_MASK 0x3

  typedef enum predictive_overshoot_state
  {
    PREDICTIVE_OVERSHOOT_IDLE,
    PREDICTIVE_OVERSHOOT_COOLING_IDLE,
    PREDICTIVE_OVERSHOOT_COOLING,
    PREDICTIVE_OVERSHOOT_COOLING_OFF,
    PREDICTIVE_OVERSHOOT_HEATING_IDLE,
    PREDICTIVE_OVERSHOOT_HEATING,
    PREDICTIVE_OVERSHOOT_HEATING_OFF
  } predictive_overshoot_state_t;

  typedef enum predictive_overshoot_error
  {
    E_PREDICTIVE_OVERSHOOT_OK = 0x0
  } predictive_overshoot_error_t;

  typedef struct predictive_overshoot_time
  {
    unsigned long off;
    unsigned long minimum_on;
    unsigned long maximum_on;
  } predictive_overshoot_time_t;

  // Use the LSB to indicate disabled state, so when we re-enable we can remember what on mode we need to be in
  typedef enum predictive_overshoot_controller_mode
  {
    PREDICTIVE_OVERSHOOT_OFF_MODE = 0x0,
    PREDICTIVE_OVERSHOOT_COOLING_MODE = 0x1,
    PREDICTIVE_OVERSHOOT_HEATING_MODE = 0x2,
    PREDICTIVE_OVERSHOOT_AUTOMATIC_MODE = PREDICTIVE_OVERSHOOT_COOLING_MODE | PREDICTIVE_OVERSHOOT_HEATING_MODE
  } predictive_overshoot_controller_mode_t;

  typedef struct predictive_overshoot_controller
  {
    predictive_overshoot_state_t state;
    float set_point;
    float_range_t overshoot_per_hour;

    float current_input;
    float last_output;
    unsigned long sample_time;
    unsigned long time_elapsed;

    float peak;
    float estimated_peak;
    float_range_t deadband; // Temperature difference the system will need to reach before switching between heating and cooling. Stops lots of short cycles. User definable

    predictive_overshoot_time_t cooling_time;
    predictive_overshoot_time_t heating_time;

    predictive_overshoot_controller_mode_t mode;

    bool cooling_on;
    bool heating_on;
  } predictive_overshoot_controller_t;

  void predictive_overshoot_controller_init(predictive_overshoot_controller_t *context, float set_point, float_range_t deadband, predictive_overshoot_time_t cooling_time, predictive_overshoot_time_t heating_time, predictive_overshoot_controller_mode_t mode);
  void predictive_overshoot_controller_run(predictive_overshoot_controller_t *context);
  void predictive_overshoot_controller_update(predictive_overshoot_controller_t *context, float value);

  predictive_overshoot_error_t predictive_overshoot_controller_set_set_point(predictive_overshoot_controller_t *context, float set_point);
  predictive_overshoot_error_t predictive_overshoot_controller_set_sample_time(predictive_overshoot_controller_t *context, unsigned long sample_time);
  predictive_overshoot_error_t predictive_overshoot_controller_set_deadband(predictive_overshoot_controller_t *context, float_range_t dead_band);
  predictive_overshoot_error_t predictive_overshoot_controller_set_cooling_time(predictive_overshoot_controller_t *context, predictive_overshoot_time_t cooling_time);
  predictive_overshoot_error_t predictive_overshoot_controller_set_heating_time(predictive_overshoot_controller_t *context, predictive_overshoot_time_t heating_time);
  predictive_overshoot_error_t predictive_overshoot_controller_set_overshoot_per_hour(predictive_overshoot_controller_t *context, float_range_t overshoot_per_hour);

  float predictive_overshoot_controller_estimate_overshoot(predictive_overshoot_controller_t *context);
  void predictive_overshoot_controller_tune_estimator(predictive_overshoot_controller_t *context);

  void predictive_overshoot_controller_set_mode(predictive_overshoot_controller_t *context, predictive_overshoot_controller_mode_t mode);
  predictive_overshoot_controller_mode_t predictive_overshoot_controller_get_mode(predictive_overshoot_controller_t *context);

  bool predictive_overshoot_controller_is_cooling(predictive_overshoot_controller_t *context);
  bool predictive_overshoot_controller_is_heating(predictive_overshoot_controller_t *context);

  void predictive_overshoot_controller_set_enable(predictive_overshoot_controller_t *context, bool enabled);
  bool predictive_overshoot_controller_is_enabled(predictive_overshoot_controller_t *context);
#ifdef __cplusplus
}
#endif
