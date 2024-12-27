#pragma once
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "predictive_overshoot_controller.h"

namespace esphome
{
	namespace predictive_overshoot
	{
		class PredictiveOvershootClimate : public climate::Climate, public Component
		{
		public:
			PredictiveOvershootClimate() = default;
			void setup() override;
			void dump_config() override;

			Trigger<> *get_idle_action_trigger() const { return this->idle_action_trigger_; }
			Trigger<> *get_cool_action_trigger() const { return this->cool_action_trigger_; }
			Trigger<> *get_heat_action_trigger() const { return this->heat_action_trigger_; }

			void set_sensor(sensor::Sensor *sensor);
			void set_sample_time(unsigned long sample_time);
			void set_deadband_low(float in);
			void set_deadband_high(float in);
			void set_cooling_off_time(unsigned long in);
			void set_cooling_minimum_on_time(unsigned long in);
			void set_cooling_maximum_on_time(unsigned long in);
			void set_heating_off_time(unsigned long in);
			void set_heating_minimum_on_time(unsigned long in);
			void set_heating_maximum_on_time(unsigned long in);

			void run();

		protected:
			void control(const climate::ClimateCall &call) override;
			climate::ClimateTraits traits() override;

			sensor::Sensor *sensor_{nullptr};

			Trigger<> *cool_action_trigger_{nullptr};
			Trigger<> *heat_action_trigger_{nullptr};
			Trigger<> *idle_action_trigger_{nullptr};

			float deadband_lower_{0.0};
			float deadband_upper_{0.0};

			unsigned long cooling_time_off_{0};
			unsigned long cooling_time_minimum_on_{0};
			unsigned long cooling_time_maximum_on_{0};

			unsigned long heating_time_off_{0};
			unsigned long heating_time_minimum_on_{0};
			unsigned long heating_time_maximum_on_{0};

			// unsigned long sample_time_{};

		private:
			predictive_overshoot_controller_t context;
		};
	};
}