#pragma once

#include <utility>
#include <vector>
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/automation.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome
{
    namespace tcs_intercom
    {
        class TCSIntercomListener
        {
            public:
                void set_command(uint32_t command) { this->command_ = command; }
                void set_auto_off(uint16_t auto_off) { this->auto_off_ = auto_off; }

                virtual void turn_on(uint32_t *timer, uint16_t auto_off) {};
                virtual void turn_off(uint32_t *timer) {};

                uint32_t timer_;
                uint32_t command_;
                uint16_t auto_off_;
        };

        struct TCSComponentStore
        {
            static void gpio_intr(TCSComponentStore *arg);

            static volatile uint32_t s_cmd;
            static volatile uint8_t s_cmdLength;
            static volatile bool s_cmdReady;

            ISRInternalGPIOPin rx_pin;
        };

        class TCSComponent : public Component
        {
            public:
                void set_rx_pin(InternalGPIOPin *pin) { rx_pin_ = pin; }
                void set_tx_pin(InternalGPIOPin *pin) { tx_pin_ = pin; }

                void set_event(const char *event) { this->event_ = event; }

                void setup() override;
                void dump_config() override;
                void loop() override;

                void register_listener(TCSIntercomListener *listener);

                void set_bus_command_sensor(text_sensor::TextSensor *bus_command) { this->bus_command_ = bus_command; }
                void set_hardware_version_sensor(text_sensor::TextSensor *hardware_version) { this->hardware_version_ = hardware_version; }

                void send_command(uint32_t command);
                void publish_command(uint32_t command);

                bool sending;

            protected:
                InternalGPIOPin *rx_pin_;
                InternalGPIOPin *tx_pin_;
                const char* event_;

                TCSComponentStore store_;
                std::vector<TCSIntercomListener *> listeners_{};

                text_sensor::TextSensor *bus_command_{nullptr};
                text_sensor::TextSensor *hardware_version_{nullptr};
        };

        template<typename... Ts> class TCSIntercomSendAction : public Action<Ts...>
        {
            public:
                TCSIntercomSendAction(TCSComponent *parent) : parent_(parent) {}
                TEMPLATABLE_VALUE(uint32_t, command)

                void play(Ts... x) { this->parent_->send_command(this->command_.value(x...)); }

            protected:
                TCSComponent *parent_;
        };

    }  // namespace tcs_intercom

}  // namespace esphome