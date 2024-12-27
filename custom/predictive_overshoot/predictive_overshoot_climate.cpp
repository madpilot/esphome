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
			traits.set_supports_current_temperature(true);
			traits.set_supports_two_point_target_temperature(false);
			traits.set_supports_current_humidity(false);
			traits.set_supported_modes({climate::CLIMATE_MODE_OFF});
			if (this->cool_action_trigger_ != nullptr)
			{
				traits.set_supported_modes({climate::CLIMATE_MODE_COOL});
			}
			if (this->heat_action_trigger_ != nullptr)
			{
				traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
			}
			if (this->cool_action_trigger_ != nullptr && this->heat_action_trigger_ != nullptr)
			{
				traits.set_supported_modes({climate::CLIMATE_MODE_HEAT_COOL});
			}
			traits.set_supports_action(true);
			return traits;
		}

		void PredictiveOvershootClimate::setup()
		{
			predictive_overshoot_controller_init(&this->context, 0,
																					 {.lower = this->deadband_lower_, .upper = this->deadband_upper_},
																					 {.off = this->cooling_time_off_, .minimum_on = this->cooling_time_minimum_on_, .maximum_on = this->cooling_time_maximum_on_},
																					 {.off = this->heating_time_off_, .minimum_on = this->heating_time_minimum_on_, .maximum_on = this->heating_time_maximum_on_},
																					 PREDICTIVE_OVERSHOOT_OFF_MODE);

			auto restore = this->restore_state_();
			if (restore.has_value())
			{
				restore->to_call(this).perform();
			}

			this->sensor_->add_on_state_callback([this](float state)
																					 { this->current_temperature = state; });
			this->current_temperature = this->sensor_->state;

			this->set_interval(this->context.sample_time, std::bind(&PredictiveOvershootClimate::run, this));
		}

		void PredictiveOvershootClimate::dump_config()
		{
			LOG_CLIMATE("", "Predictive Overshoot", this);

			ESP_LOGCONFIG(TAG, "  Cooling Deadband: %f", this->context.deadband.upper);
			ESP_LOGCONFIG(TAG, "  Heating Deadband: %f", this->context.deadband.lower);
			ESP_LOGCONFIG(TAG, "  Cooling Min Off Time: %i", this->context.cooling_time.off);
			ESP_LOGCONFIG(TAG, "  Cooling Min Run Time: %i", this->context.cooling_time.minimum_on);
			ESP_LOGCONFIG(TAG, "  Cooling Max Off Time: %i", this->context.cooling_time.maximum_on);
			ESP_LOGCONFIG(TAG, "  Heating Min Off Time: %i", this->context.heating_time.off);
			ESP_LOGCONFIG(TAG, "  Heating Min Run Time: %i", this->context.heating_time.minimum_on);
			ESP_LOGCONFIG(TAG, "  Heating Max Off Time: %i", this->context.heating_time.maximum_on);
		}

		void PredictiveOvershootClimate::run()
		{
			ESP_LOGI(TAG, "Running predictive overshoot loop.");
			this->context.current_input = this->current_temperature;

			predictive_overshoot_controller_run(&this->context);

			switch (this->context.state)
			{
			case PREDICTIVE_OVERSHOOT_IDLE:
				ESP_LOGD(TAG, "State: IDLE");
				break;
			case PREDICTIVE_OVERSHOOT_COOLING_IDLE:
				ESP_LOGD(TAG, "State: COOLING IDLE");
				break;
			case PREDICTIVE_OVERSHOOT_COOLING:
				ESP_LOGD(TAG, "State: COOLING");
				break;
			case PREDICTIVE_OVERSHOOT_COOLING_OFF:
				ESP_LOGD(TAG, "State: COOLING_OFF");
				break;
			case PREDICTIVE_OVERSHOOT_HEATING_IDLE:
				ESP_LOGD(TAG, "State: HEATING IDLE");
				break;
			case PREDICTIVE_OVERSHOOT_HEATING:
				ESP_LOGD(TAG, "State: HEATING");
				break;
			case PREDICTIVE_OVERSHOOT_HEATING_OFF:
				ESP_LOGD(TAG, "State: HEATING_OFF");
				break;
			default:
				ESP_LOGD(TAG, "State: UNKNOWN");
				break;
			}
			ESP_LOGD(TAG, "Overshoot Per Hour: %d", this->context.overshoot_per_hour);
			ESP_LOGD(TAG, "Sample Time: %d", this->context.sample_time);
			ESP_LOGD(TAG, "Time elapsed: %d", this->context.time_elapsed);
			ESP_LOGD(TAG, "Peak: %d", this->context.peak);
			ESP_LOGD(TAG, "Estimated Peak: %d", this->context.estimated_peak);

			if (this->context.cooling_on && this->action != climate::CLIMATE_ACTION_COOLING)
			{
				this->action = climate::CLIMATE_ACTION_COOLING;
				if (this->cool_action_trigger_ != nullptr)
				{
					ESP_LOGI(TAG, "Triggering cooling action");
					this->cool_action_trigger_->trigger();
				}
				this->publish_state();
			}
			else if (this->context.heating_on && this->action != climate::CLIMATE_ACTION_HEATING)
			{
				this->action = climate::CLIMATE_ACTION_HEATING;
				if (this->heat_action_trigger_ != nullptr)
				{
					ESP_LOGI(TAG, "Triggering heating action");
					this->heat_action_trigger_->trigger();
				}
				this->publish_state();
			}
			else if (this->action != climate::CLIMATE_ACTION_IDLE)
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

		void PredictiveOvershootClimate::set_sample_time(unsigned long sample_time)
		{
			predictive_overshoot_controller_set_sample_time(&this->context, sample_time);
		}

		void PredictiveOvershootClimate::set_deadband_low(float in)
		{
			predictive_overshoot_controller_set_deadband(&this->context, {.lower = in,
																																		.upper = this->context.deadband.upper});
		}

		void PredictiveOvershootClimate::set_deadband_high(float in)
		{
			predictive_overshoot_controller_set_deadband(&this->context, {.lower = this->context.deadband.lower,
																																		.upper = in});
		}

		void PredictiveOvershootClimate::set_cooling_off_time(unsigned long in)
		{
			predictive_overshoot_controller_set_cooling_time(&this->context, {.off = in, .minimum_on = this->context.cooling_time.minimum_on, .maximum_on = this->context.cooling_time.maximum_on});
		}

		void PredictiveOvershootClimate::set_cooling_minimum_on_time(unsigned long in)
		{
			predictive_overshoot_controller_set_cooling_time(&this->context, {.off = this->context.cooling_time.off, .minimum_on = in, .maximum_on = this->context.cooling_time.maximum_on});
		}

		void PredictiveOvershootClimate::set_cooling_maximum_on_time(unsigned long in)
		{
			predictive_overshoot_controller_set_cooling_time(&this->context, {.off = this->context.cooling_time.off, .minimum_on = this->context.cooling_time.minimum_on, .maximum_on = in});
		}

		void PredictiveOvershootClimate::set_heating_off_time(unsigned long in)
		{
			predictive_overshoot_controller_set_heating_time(&this->context, {.off = in, .minimum_on = this->context.heating_time.minimum_on, .maximum_on = this->context.heating_time.maximum_on});
		}

		void PredictiveOvershootClimate::set_heating_minimum_on_time(unsigned long in)
		{
			predictive_overshoot_controller_set_heating_time(&this->context, {.off = this->context.heating_time.off, .minimum_on = in, .maximum_on = this->context.heating_time.maximum_on});
		}

		void PredictiveOvershootClimate::set_heating_maximum_on_time(unsigned long in)
		{
			predictive_overshoot_controller_set_heating_time(&this->context, {.off = this->context.heating_time.off, .minimum_on = this->context.heating_time.minimum_on, .maximum_on = in});
		}

		void PredictiveOvershootClimate::control(const climate::ClimateCall &call)
		{
			if (call.get_mode().has_value())
			{
				this->mode = *call.get_mode();
				switch (this->mode)
				{
				case esphome::climate::ClimateMode::CLIMATE_MODE_OFF:
					predictive_overshoot_controller_set_mode(&this->context, PREDICTIVE_OVERSHOOT_OFF_MODE);
					break;
				case esphome::climate::ClimateMode::CLIMATE_MODE_HEAT_COOL:
					predictive_overshoot_controller_set_mode(&this->context, PREDICTIVE_OVERSHOOT_AUTOMATIC_MODE);
					break;
				case esphome::climate::ClimateMode::CLIMATE_MODE_COOL:
					predictive_overshoot_controller_set_mode(&this->context, PREDICTIVE_OVERSHOOT_COOLING_MODE);
					break;
				case esphome::climate::ClimateMode::CLIMATE_MODE_HEAT:
					predictive_overshoot_controller_set_mode(&this->context, PREDICTIVE_OVERSHOOT_HEATING_MODE);
					break;
				default:
					predictive_overshoot_controller_set_mode(&this->context, PREDICTIVE_OVERSHOOT_OFF_MODE);
					return;
				}

				if (call.get_target_temperature().has_value())
				{
					this->target_temperature = *call.get_target_temperature();
					predictive_overshoot_controller_set_set_point(&this->context, this->target_temperature);
				}

				this->publish_state();
			}
		}
	}
}