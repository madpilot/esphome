#include "./predictive_overshoot_controller.h"
#include <stdbool.h>

void predictive_overshoot_controller_set_enable(predictive_overshoot_controller_t *context, bool enable);
bool predictive_overshoot_controller_get_enable(predictive_overshoot_controller_t *context);
bool predictive_overshoot_controller_can_cool(predictive_overshoot_controller_t *context);
bool predictive_overshoot_controller_can_heat(predictive_overshoot_controller_t *context);

void predictive_overshoot_controller_init(predictive_overshoot_controller_t *context, float set_point, float_range_t deadband, predictive_overshoot_time_t cooling_time, predictive_overshoot_time_t heating_time, predictive_overshoot_controller_mode_t mode, float kp, float min_percentage_increase, float max_percentage_increase, float min_percentage_decrease, float max_percentage_decrease)
{
  context->set_point = set_point;
  context->deadband = deadband;
  context->cooling_time = cooling_time;
  context->heating_time = heating_time;

  context->state = PREDICTIVE_OVERSHOOT_IDLE;
  context->sample_time = 1000;
  context->peak = 0;
  context->estimated_peak = 0;
  context->current_input = 0;
  context->overshoot_per_hour.lower = 0;
  context->overshoot_per_hour.upper = 0;
  context->last_output = 0;
  context->time_elapsed = 0;

  context->min_percentage_increase = min_percentage_increase;
  context->max_percentage_increase = max_percentage_increase;
  context->min_percentage_decrease = min_percentage_decrease;
  context->max_percentage_decrease = max_percentage_decrease;
  context->kp = kp;

  context->heating_on = false;
  context->cooling_on = false;

  predictive_overshoot_controller_set_enable(context, false);
  predictive_overshoot_controller_set_mode(context, mode);
}

void predictive_overshoot_controller_update(predictive_overshoot_controller_t *context, float value)
{
  context->current_input = value;

  if (!predictive_overshoot_controller_get_enable(context))
  {
    return;
  }

  if (context->state == PREDICTIVE_OVERSHOOT_COOLING_IDLE || context->state == PREDICTIVE_OVERSHOOT_COOLING || context->state == PREDICTIVE_OVERSHOOT_COOLING_OFF)
  {
    if (value < context->peak)
    {
      context->peak = value;
    }
  }

  if (context->state == PREDICTIVE_OVERSHOOT_HEATING_IDLE || context->state == PREDICTIVE_OVERSHOOT_HEATING || context->state == PREDICTIVE_OVERSHOOT_HEATING_OFF)
  {
    if (value > context->peak)
    {
      context->peak = value;
    }
  }
}

void predictive_overshoot_controller_run(predictive_overshoot_controller_t *context)
{
  if (!predictive_overshoot_controller_get_enable(context) && context->state != PREDICTIVE_OVERSHOOT_COOLING_OFF && context->state != PREDICTIVE_OVERSHOOT_HEATING_OFF)
  {
    return;
  }

  float input = context->current_input;

  switch (context->state)
  {
  case PREDICTIVE_OVERSHOOT_IDLE:
  case PREDICTIVE_OVERSHOOT_COOLING_IDLE:
  case PREDICTIVE_OVERSHOOT_HEATING_IDLE:
    if (predictive_overshoot_controller_can_cool(context) && context->set_point + context->deadband.upper < input)
    {
      predictive_overshoot_controller_tune_estimator(context);
      context->peak = input;
      context->estimated_peak = input;
      context->time_elapsed = 0;

      context->state = PREDICTIVE_OVERSHOOT_COOLING;
      context->cooling_on = true;
    }
    else if (predictive_overshoot_controller_can_heat(context) && context->set_point - context->deadband.lower > input)
    {
      predictive_overshoot_controller_tune_estimator(context);
      context->peak = input;
      context->time_elapsed = 0;
      context->estimated_peak = input;

      context->state = PREDICTIVE_OVERSHOOT_HEATING;
      context->heating_on = true;
    }
    break;
  case PREDICTIVE_OVERSHOOT_COOLING:
    context->estimated_peak = predictive_overshoot_controller_estimate_overshoot(context);
    if ((context->estimated_peak <= context->set_point && context->time_elapsed >= context->cooling_time.minimum_on) || context->time_elapsed >= context->cooling_time.maximum_on)
    {
      context->time_elapsed = 0;
      context->cooling_on = false;

      if (context->cooling_time.off > 0)
      {
        context->state = PREDICTIVE_OVERSHOOT_COOLING_OFF;
      }
      else
      {
        context->state = PREDICTIVE_OVERSHOOT_COOLING_IDLE;
      }
    }
    else
    {
      context->time_elapsed += context->sample_time;
    }
    break;
  case PREDICTIVE_OVERSHOOT_COOLING_OFF:
    if (context->time_elapsed >= context->cooling_time.off)
    {
      context->time_elapsed = 0;
      context->state = PREDICTIVE_OVERSHOOT_COOLING_IDLE;
    }
    else
    {
      context->time_elapsed += context->sample_time;
    }
    break;
  case PREDICTIVE_OVERSHOOT_HEATING:
    context->estimated_peak = predictive_overshoot_controller_estimate_overshoot(context);
    if ((context->estimated_peak >= context->set_point && context->time_elapsed >= context->heating_time.minimum_on) || context->time_elapsed >= context->heating_time.maximum_on)
    {
      context->time_elapsed = 0;
      context->heating_on = true;

      if (context->heating_time.off > 0)
      {
        context->state = PREDICTIVE_OVERSHOOT_HEATING_OFF;
      }
      else
      {
        context->state = PREDICTIVE_OVERSHOOT_HEATING_IDLE;
      }
    }
    else
    {
      context->time_elapsed += context->sample_time;
    }
    break;
  case PREDICTIVE_OVERSHOOT_HEATING_OFF:
    if (context->time_elapsed >= context->heating_time.off)
    {
      context->time_elapsed = 0;
      context->state = PREDICTIVE_OVERSHOOT_HEATING_IDLE;
    }
    else
    {
      context->time_elapsed += context->sample_time;
    }
    break;
  }
}

predictive_overshoot_error_t predictive_overshoot_controller_set_set_point(predictive_overshoot_controller_t *context, float set_point)
{
  context->set_point = set_point;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_sample_time(predictive_overshoot_controller_t *context, unsigned long sample_time)
{
  context->sample_time = sample_time;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_deadband(predictive_overshoot_controller_t *context, float_range_t deadband)
{
  context->deadband = deadband;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_cooling_time(predictive_overshoot_controller_t *context, predictive_overshoot_time_t cooling_time)
{
  context->cooling_time = cooling_time;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_heating_time(predictive_overshoot_controller_t *context, predictive_overshoot_time_t heating_time)
{
  context->heating_time = heating_time;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_overshoot_per_hour(predictive_overshoot_controller_t *context, float_range_t overshoot_per_hour)
{
  context->overshoot_per_hour = overshoot_per_hour;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_min_percentage_increase(predictive_overshoot_controller_t *context, float min_percentage_increase)
{
  context->min_percentage_increase = min_percentage_increase;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_max_percentage_increase(predictive_overshoot_controller_t *context, float max_percentage_increase)
{
  context->max_percentage_increase = max_percentage_increase;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_min_percentage_decrease(predictive_overshoot_controller_t *context, float min_percentage_decrease)
{
  context->min_percentage_decrease = min_percentage_decrease;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_max_percentage_decrease(predictive_overshoot_controller_t *context, float max_percentage_decrease)
{
  context->max_percentage_decrease = max_percentage_decrease;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

predictive_overshoot_error_t predictive_overshoot_controller_set_kp(predictive_overshoot_controller_t *context, float kp)
{
  context->kp = kp;
  return E_PREDICTIVE_OVERSHOOT_OK;
}

void predictive_overshoot_controller_enable(predictive_overshoot_controller_t *context, bool enable)
{
  if (enable)
  {
    context->mode |= 0x1; // Set the LSB to 1
  }
  else
  {
    context->mode &= ~(0x1); // Clear the LSB
    if (context->state == PREDICTIVE_OVERSHOOT_COOLING)
    {
      context->time_elapsed = 0;

      if (context->cooling_time.off > 0)
      {
        context->state = PREDICTIVE_OVERSHOOT_COOLING_OFF;
      }
      else
      {
        context->state = PREDICTIVE_OVERSHOOT_COOLING_IDLE;
      }
    }
    else if (context->state == PREDICTIVE_OVERSHOOT_HEATING)
    {
      context->time_elapsed = 0;

      if (context->heating_time.off > 0)
      {
        context->state = PREDICTIVE_OVERSHOOT_HEATING_OFF;
      }
      else
      {
        context->state = PREDICTIVE_OVERSHOOT_HEATING_IDLE;
      }
    }
  }
}

// Effectively a predictor of what the temperature would be if you turned off right now
float predictive_overshoot_controller_estimate_overshoot(predictive_overshoot_controller_t *context)
{
  unsigned long time_elapsed_in_seconds = context->time_elapsed / 1000;
  float overshoot_per_hour = context->state == PREDICTIVE_OVERSHOOT_COOLING ? context->overshoot_per_hour.lower * -1 : context->overshoot_per_hour.upper;
  return context->current_input + (time_elapsed_in_seconds * overshoot_per_hour / 3600);
}

void predictive_overshoot_controller_tune_estimator(predictive_overshoot_controller_t *context)
{
  float error = 0;
  float multiplier = 1;
  float *overshoot;

  if (context->state == PREDICTIVE_OVERSHOOT_HEATING_IDLE)
  {
    error = context->peak - context->estimated_peak;
    overshoot = &context->overshoot_per_hour.upper;
  }
  else if (context->state == PREDICTIVE_OVERSHOOT_COOLING_IDLE)
  {
    error = context->estimated_peak - context->peak;
    overshoot = &context->overshoot_per_hour.lower;
  }
  else
  {
    return;
  }

  if (error > 0)
  {
    multiplier = context->min_percentage_increase + (context->kp * error);
    if (multiplier > context->max_percentage_increase)
    {
      multiplier = context->max_percentage_increase;
    }
    *overshoot *= multiplier;
  }
  else if (error < 0)
  {
    multiplier = context->min_percentage_decrease + (context->kp * -error);

    if (multiplier > context->max_percentage_decrease)
    {
      multiplier = context->max_percentage_decrease;
    }
    *overshoot /= multiplier;
  }
}

void predictive_overshoot_controller_set_enable(predictive_overshoot_controller_t *context, bool enable)
{
  if (enable)
  {
    context->mode |= PREDICTIVE_OVERSHOOT_ENABLE_MASK;
  }
  else
  {
    context->mode &= ~PREDICTIVE_OVERSHOOT_ENABLE_MASK;
  }
}

bool predictive_overshoot_controller_get_enable(predictive_overshoot_controller_t *context)
{
  return (context->mode & PREDICTIVE_OVERSHOOT_ENABLE_MASK) == PREDICTIVE_OVERSHOOT_ENABLE_MASK;
}

void predictive_overshoot_controller_set_mode(predictive_overshoot_controller_t *context, predictive_overshoot_controller_mode_t mode)
{
  context->mode = (context->mode & PREDICTIVE_OVERSHOOT_ENABLE_MASK) | (mode << 1);
}

predictive_overshoot_controller_mode_t predictive_overshoot_controller_get_mode(predictive_overshoot_controller_t *context)
{
  return (context->mode >> 1) & PREDICTIVE_OVERSHOOT_MODE_MASK;
}

bool predictive_overshoot_controller_can_cool(predictive_overshoot_controller_t *context)
{
  return (predictive_overshoot_controller_get_mode(context) & PREDICTIVE_OVERSHOOT_COOLING_MODE) == PREDICTIVE_OVERSHOOT_COOLING_MODE;
}

bool predictive_overshoot_controller_can_heat(predictive_overshoot_controller_t *context)
{
  return (predictive_overshoot_controller_get_mode(context) & PREDICTIVE_OVERSHOOT_HEATING_MODE) == PREDICTIVE_OVERSHOOT_HEATING_MODE;
}

bool predictive_overshoot_controller_is_enabled(predictive_overshoot_controller_t *context)
{
  return predictive_overshoot_controller_get_enable(context);
}

bool predictive_overshoot_controller_is_cooling(predictive_overshoot_controller_t *context)
{
  return context->cooling_on;
}

bool predictive_overshoot_controller_is_heating(predictive_overshoot_controller_t *context)
{
  return context->heating_on;
}