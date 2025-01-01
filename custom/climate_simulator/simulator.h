#pragma once
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome
{
  namespace simulator
  {
    class ClimateSimulator : public PollingComponent,
                             public sensor::Sensor
    {
    public:
      ClimateSimulator() : PollingComponent(1000) {}

      void setup() override
      {
        this->publish_state(this->temperature);
      }

      void update()
      {
        float new_temp = this->update_temp();
        this->publish_state(new_temp);
      }

      float delta_t(float power)
      {
        // P = Q / t
        // Q = c * m * 𝚫t
        // 𝚫t = (P*t) / (c*m)
        float c = this->specific_heat_capacity;
        float t = this->update_interval;
        float p = power / 1000; //  in kW
        float m = this->mass;
        return (p * t) / (c * m);
      }

      float update_temp()
      {
        if (this->cooling_on)
        {
          float power = cooling_power * efficiency;
          temperature -= this->delta_t(power);
        }
        else if (this->heating_on)
        {
          float power = heat_power * efficiency;
          temperature += this->delta_t(power);
        }

        // Adjust for ambient temperature
        float dt = temperature - ambient_temperature;
        float power = (thermal_conductivity * surface * dt) / update_interval;
        temperature -= this->delta_t(power);

        // // Delay temperature readings
        delayed_temps.push_back(temperature);
        if (delayed_temps.size() > delay_cycles)
          delayed_temps.erase(delayed_temps.begin());
        float prev_temp = this->delayed_temps[0];
        float alpha = 0.1f;
        float ret = (1 - alpha) * prev_temp + alpha * prev_temp;
        return ret;
      }

      void set_cooling_on() { this->cooling_on = true; };
      void set_cooling_off() { this->cooling_on = false; };
      void set_heating_on() { this->heating_on = true; };
      void set_heating_off() { this->heating_on = false; };

    private:
      float surface = 1;                    /// surface area in m²
      float mass = 3;                       /// mass of simulated object in kg
      float temperature = 21;               /// current temperature of object in °C
      float efficiency = 0.98;              /// heating efficiency, 1 is 100% efficient
      float thermal_conductivity = 15;      /// thermal conductivity of surface are in W/(m*K), here: steel
      float specific_heat_capacity = 4.182; /// specific heat capacity of mass in kJ/(kg*K), here: water
      float heat_power = 500;               /// Heating power in W
      float cooling_power = 500;            /// Cooling power in W
      float ambient_temperature = 20;       /// Ambient temperature in °C
      float update_interval = 1;            /// The simulated updated interval in seconds

      std::vector<float> delayed_temps; /// storage of past temperatures for delaying temperature reading
      size_t delay_cycles = 15;         /// how many update cycles to delay the output

      bool cooling_on{false};
      bool heating_on{false};
    };

    template <typename... Ts>
    class CoolingOffAction : public Action<Ts...>
    {
    public:
      CoolingOffAction(ClimateSimulator *simulator) : simulator_(simulator) {}
      void play(Ts... x) override { this->simulator_->set_cooling_off(); }

    protected:
      ClimateSimulator *simulator_;
    };

    template <typename... Ts>
    class CoolingOnAction : public Action<Ts...>
    {
    public:
      CoolingOnAction(ClimateSimulator *simulator) : simulator_(simulator) {}
      void play(Ts... x) override { this->simulator_->set_cooling_on(); }

    protected:
      ClimateSimulator *simulator_;
    };

    template <typename... Ts>
    class HeatingOffAction : public Action<Ts...>
    {
    public:
      HeatingOffAction(ClimateSimulator *simulator) : simulator_(simulator) {}
      void play(Ts... x) override { this->simulator_->set_heating_off(); }

    protected:
      ClimateSimulator *simulator_;
    };

    template <typename... Ts>
    class HeatingOnAction : public Action<Ts...>
    {
    public:
      HeatingOnAction(ClimateSimulator *simulator) : simulator_(simulator) {}
      void play(Ts... x) override { this->simulator_->set_heating_on(); }

    protected:
      ClimateSimulator *simulator_;
    };
  }
}