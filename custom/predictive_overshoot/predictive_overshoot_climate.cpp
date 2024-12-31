#include "predictive_overshoot_climate.h"
#include "predictive_overshoot_controller.h"
#include "esphome/components/climate/climate_mode.h"

namespace esphome
{
	namespace predictive_overshoot
	{
		static const char *const TAG = "predictive_overshoot.climate";

		climate::ClimateTraits PredictiveOvershootClimate::traits()
		{
			auto traits = climate::ClimateTraits();

			// TODO: Make this configurable
			traits.set_visual_current_temperature_step(0.5);
			traits.set_visual_min_temperature(-7);
			traits.set_visual_max_temperature(20);

			traits.set_supports_current_temperature(true);
			traits.set_supports_two_point_target_temperature(false);
			traits.set_supports_current_humidity(false);
			traits.add_supported_mode(climate::CLIMATE_MODE_OFF);
			if (this->supports_cooling_)
			{
				traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
			}
			if (this->supports_heating_)
			{
				traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
			}
			if (this - supports_cooling_ && this->supports_heating_)
			{
				traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);
			}
			traits.set_supports_action(true);
			return traits;
		}

		PredictiveOvershootClimate::PredictiveOvershootClimate() : cool_action_trigger_(new Trigger<>()), heat_action_trigger_(new Trigger<>()), idle_action_trigger_(new Trigger<>())
		{
		}

		void PredictiveOvershootClimate::setup()
		{
			auto restore = this->restore_state_();
			if (restore.has_value())
			{
				restore->to_call(this).perform();
			}

			predictive_overshoot_controller_init(&(this->context), this->target_temperature,
																					 {.lower = this->deadband_lower_, .upper = this->deadband_upper_},
																					 {.off = this->cooling_time_off_, .minimum_on = this->cooling_time_minimum_on_, .maximum_on = this->cooling_time_maximum_on_},
																					 {.off = this->heating_time_off_, .minimum_on = this->heating_time_minimum_on_, .maximum_on = this->heating_time_maximum_on_},
																					 PREDICTIVE_OVERSHOOT_OFF_MODE);

			this->set_mode();

			this->sensor_->add_on_state_callback([this](float state)
																					 { this->current_temperature = state; });
			this->current_temperature = this->sensor_->state;
			this->set_interval(this->context.sample_time, std::bind(&PredictiveOvershootClimate::run, this));
			this->setup_complete_ = true;
			this->publish_state();
		}

		void PredictiveOvershootClimate::dump_config()
		{
			predictive_overshoot_controller_t *context = &(this->context);

			LOG_CLIMATE("", "Predictive Overshoot", this);

			ESP_LOGCONFIG(TAG, "  Cooling Deadband: %f", context->deadband.upper);
			ESP_LOGCONFIG(TAG, "  Heating Deadband: %f", context->deadband.lower);
			ESP_LOGCONFIG(TAG, "  Cooling Min Off Time: %i", context->cooling_time.off);
			ESP_LOGCONFIG(TAG, "  Cooling Min Run Time: %i", context->cooling_time.minimum_on);
			ESP_LOGCONFIG(TAG, "  Cooling Max Off Time: %i", context->cooling_time.maximum_on);
			ESP_LOGCONFIG(TAG, "  Heating Min Off Time: %i", context->heating_time.off);
			ESP_LOGCONFIG(TAG, "  Heating Min Run Time: %i", context->heating_time.minimum_on);
			ESP_LOGCONFIG(TAG, "  Heating Max Off Time: %i", context->heating_time.maximum_on);
		}

		void PredictiveOvershootClimate::debug()
		{
			predictive_overshoot_controller_t *context = &(this->context);
			ESP_LOGD(TAG, "  Enabled: %s", predictive_overshoot_controller_is_enabled(context) ? "true" : "false");
			ESP_LOGD(TAG, "  Setpoint: %f", context->set_point);
			ESP_LOGD(TAG, "  Current Input: %f", context->current_input);

			switch (predictive_overshoot_controller_get_mode(context))
			{
			case PREDICTIVE_OVERSHOOT_OFF_MODE:
				ESP_LOGD(TAG, "  Mode: Off");
				break;
			case PREDICTIVE_OVERSHOOT_COOLING_MODE:
				ESP_LOGD(TAG, "  Mode: Cooling");
				break;
			case PREDICTIVE_OVERSHOOT_HEATING_MODE:
				ESP_LOGD(TAG, "  Mode: Heating");
				break;
			case PREDICTIVE_OVERSHOOT_AUTOMATIC_MODE:
				ESP_LOGD(TAG, "  Mode: Automatic");
				break;
			default:
				ESP_LOGD(TAG, "  Mode: Unknown");
			}
			switch (context->state)
			{
			case PREDICTIVE_OVERSHOOT_IDLE:
				ESP_LOGD(TAG, "  State: IDLE");
				break;
			case PREDICTIVE_OVERSHOOT_COOLING_IDLE:
				ESP_LOGD(TAG, "  State: COOLING IDLE");
				break;
			case PREDICTIVE_OVERSHOOT_COOLING:
				ESP_LOGD(TAG, "  State: COOLING");
				break;
			case PREDICTIVE_OVERSHOOT_COOLING_OFF:
				ESP_LOGD(TAG, "  State: COOLING_OFF");
				break;
			case PREDICTIVE_OVERSHOOT_HEATING_IDLE:
				ESP_LOGD(TAG, "  State: HEATING IDLE");
				break;
			case PREDICTIVE_OVERSHOOT_HEATING:
				ESP_LOGD(TAG, "  State: HEATING");
				break;
			case PREDICTIVE_OVERSHOOT_HEATING_OFF:
				ESP_LOGD(TAG, "  State: HEATING_OFF");
				break;
			default:
				ESP_LOGD(TAG, "  State: UNKNOWN");
				break;
			}
			ESP_LOGD(TAG, "  Overshoot Per Hour: %f - %f", context->overshoot_per_hour.lower, context->overshoot_per_hour.upper);
			ESP_LOGD(TAG, "  Sample Time: %d", context->sample_time);
			ESP_LOGD(TAG, "  Time elapsed: %d", context->time_elapsed);
			ESP_LOGD(TAG, "  Peak: %f", context->peak);
			ESP_LOGD(TAG, "  Estimated Peak: %f", context->estimated_peak);
		}

		void PredictiveOvershootClimate::set_supports_cooling(bool in) { this->supports_cooling_ = in; }
		void PredictiveOvershootClimate::set_supports_heating(bool in) { this->supports_heating_ = in; }

		void PredictiveOvershootClimate::run()
		{
			if (!this->setup_complete_)
			{
				// Setup hasn't finished yet - don't run
				return;
			}

			predictive_overshoot_controller_t *context = &(this->context);
			if (std::isnan(this->current_temperature) && predictive_overshoot_controller_is_enabled(context))
			{
				predictive_overshoot_controller_set_enable(context, false);
				ESP_LOGE(TAG, "Current Temperature is not a number! Disabling controller...");
			}
			else if (!std::isnan(this->current_temperature) && !predictive_overshoot_controller_is_enabled(context))
			{
				predictive_overshoot_controller_set_enable(context, true);
				ESP_LOGE(TAG, "Current Temperature is now valid! Enabling controller...");
			}

			ESP_LOGI(TAG, "Running predictive overshoot loop.");

			if (!std::isnan(this->current_temperature))
			{
				context->current_input = this->current_temperature;
			}

			predictive_overshoot_controller_run(context);

			if (context->cooling_on && this->action != climate::CLIMATE_ACTION_COOLING)
			{
				this->action = climate::CLIMATE_ACTION_COOLING;
				if (this->cool_action_trigger_ != nullptr)
				{
					ESP_LOGI(TAG, "Triggering cooling action");
					this->cool_action_trigger_->trigger();
				}
				this->publish_state();
			}
			else if (context->heating_on && this->action != climate::CLIMATE_ACTION_HEATING)
			{
				this->action = climate::CLIMATE_ACTION_HEATING;
				if (this->heat_action_trigger_ != nullptr)
				{
					ESP_LOGI(TAG, "Triggering heating action");
					this->heat_action_trigger_->trigger();
				}
				this->publish_state();
			}
			else if (!context->cooling_on && !context->heating_on && this->action != climate::CLIMATE_ACTION_IDLE)
			{
				this->action = climate::CLIMATE_ACTION_IDLE;
				if (this->idle_action_trigger_ != nullptr)
				{
					ESP_LOGI(TAG, "Triggering idle action");
					this->idle_action_trigger_->trigger();
				}
				this->publish_state();
			}
		}

		void PredictiveOvershootClimate::set_sensor(sensor::Sensor *sensor)
		{
			this->sensor_ = sensor;
		}

		void PredictiveOvershootClimate::set_deadband_low(float in)
		{
			this->deadband_lower_ = in;
		}

		void PredictiveOvershootClimate::set_deadband_high(float in)
		{
			this->deadband_upper_ = in;
		}

		void PredictiveOvershootClimate::set_cooling_off_time(unsigned long in)
		{
			this->cooling_time_off_ = in;
		}

		void PredictiveOvershootClimate::set_cooling_minimum_on_time(unsigned long in)
		{
			this->cooling_time_minimum_on_ = in;
		}

		void PredictiveOvershootClimate::set_cooling_maximum_on_time(unsigned long in)
		{
			this->cooling_time_maximum_on_ = in;
		}

		void PredictiveOvershootClimate::set_heating_off_time(unsigned long in)
		{
			this->heating_time_off_ = in;
		}

		void PredictiveOvershootClimate::set_heating_minimum_on_time(unsigned long in)
		{
			this->heating_time_minimum_on_ = in;
		}

		void PredictiveOvershootClimate::set_heating_maximum_on_time(unsigned long in)
		{
			this->heating_time_maximum_on_ = in;
		}

		void PredictiveOvershootClimate::set_mode()
		{
			predictive_overshoot_controller_t *context = &(this->context);
			switch (this->mode)
			{
			case esphome::climate::ClimateMode::CLIMATE_MODE_OFF:
				predictive_overshoot_controller_set_mode(context, PREDICTIVE_OVERSHOOT_OFF_MODE);
				break;
			case esphome::climate::ClimateMode::CLIMATE_MODE_HEAT_COOL:
				predictive_overshoot_controller_set_mode(context, PREDICTIVE_OVERSHOOT_AUTOMATIC_MODE);
				break;
			case esphome::climate::ClimateMode::CLIMATE_MODE_COOL:
				predictive_overshoot_controller_set_mode(context, PREDICTIVE_OVERSHOOT_COOLING_MODE);
				break;
			case esphome::climate::ClimateMode::CLIMATE_MODE_HEAT:
				predictive_overshoot_controller_set_mode(context, PREDICTIVE_OVERSHOOT_HEATING_MODE);
				break;
			default:
				predictive_overshoot_controller_set_mode(context, PREDICTIVE_OVERSHOOT_OFF_MODE);
				break;
			}
		}

		void PredictiveOvershootClimate::control(const climate::ClimateCall &call)
		{
			predictive_overshoot_controller_t *context = &(this->context);
			if (call.get_target_temperature().has_value())
			{
				this->target_temperature = *call.get_target_temperature();
				this->validate_target_temperature();
				predictive_overshoot_controller_set_set_point(context, this->target_temperature);
			}

			if (call.get_mode().has_value())
			{
				this->mode = *call.get_mode();
				this->set_mode();
			}

			this->publish_state();
		}

		void PredictiveOvershootClimate::validate_target_temperature()
		{
			if (std::isnan(this->target_temperature))
			{
				this->target_temperature =
						((this->get_traits().get_visual_max_temperature() - this->get_traits().get_visual_min_temperature()) / 2) +
						this->get_traits().get_visual_min_temperature();
			}
			else
			{
				// target_temperature must be between the visual minimum and the visual maximum
				if (this->target_temperature < this->get_traits().get_visual_min_temperature())
				{
					this->target_temperature = this->get_traits().get_visual_min_temperature();
				}

				if (this->target_temperature > this->get_traits().get_visual_max_temperature())
				{
					this->target_temperature = this->get_traits().get_visual_max_temperature();
				}
			}
		}
	}
}