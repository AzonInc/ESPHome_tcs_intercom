#include "tcs_intercom.h"

#include "esphome.h"
#include "esphome/core/application.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "esphome/components/api/custom_api_device.h"
#include "esphome/core/application.h"

#if defined(USE_ESP_IDF) || (defined(USE_ARDUINO) && defined(ESP32))
#include "soc/efuse_reg.h"
#include "soc/efuse_periph.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#endif

#ifdef USE_ARDUINO
#include "Arduino.h"
#endif

#include <stdint.h>
#include <optional>

namespace esphome
{
    namespace tcs_intercom
    {
        static const char *const TAG = "tcs_intercom";

        static const uint8_t TCS_MSG_START_MS = 6; // a new message
        static const uint8_t TCS_ONE_BIT_MS = 4; // a 1-bit is 4ms long
        static const uint8_t TCS_ZERO_BIT_MS = 2; // a 0-bit is 2ms long

        TCSComponent *global_tcs_intercom = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        void TCSComponent::setup()
        {
            ESP_LOGCONFIG(TAG, "Setting up TCS Intercom...");

            #if defined(USE_ESP_IDF) || (defined(USE_ARDUINO) && defined(ESP32))
            ESP_LOGD(TAG, "Check for Doorman Hardware");

            // Doorman Hardware Revision
            uint8_t ver[3];
            uint32_t value;
            esp_efuse_read_block(EFUSE_BLK3, &value, 0, 24);
            ver[0] = value >> 0;
            ver[1] = value >> 8;
            ver[2] = value >> 16;

            if(ver[0] > 0)
            {
                ESP_LOGI(TAG, "Doorman Hardware detected: V%i.%i.%i", ver[0], ver[1], ver[2]);
                this->hardware_version_str_ = "Doorman " + std::to_string(ver[0]) + "." + std::to_string(ver[1]) + "." + std::to_string(ver[2]);

                // Override GPIO
                if(ver[0] == 1 && (ver[1] == 3 || ver[1] == 4))
                {
                    esp32::ESP32InternalGPIOPin *gpio_pin_rx_;
                    gpio_pin_rx_ = new(esp32::ESP32InternalGPIOPin);
                    gpio_pin_rx_->set_pin(static_cast<gpio_num_t>(9));
                    gpio_pin_rx_->set_flags(gpio::Flags::FLAG_INPUT);
                    this->set_rx_pin(gpio_pin_rx_);

                    esp32::ESP32InternalGPIOPin *gpio_pin_tx_;
                    gpio_pin_tx_ = new(esp32::ESP32InternalGPIOPin);
                    gpio_pin_tx_->set_pin(static_cast<gpio_num_t>(8));
                    gpio_pin_tx_->set_flags(gpio::Flags::FLAG_OUTPUT);
                    this->set_tx_pin(gpio_pin_tx_);

                    ESP_LOGD(TAG, "Doorman Hardware GPIO Override: RX (%i), TX (%i)", this->rx_pin_->get_pin(), this->tx_pin_->get_pin());
                }
            }
            #endif

            if (this->hardware_version_ != nullptr)
            {
                this->hardware_version_->publish_state(this->hardware_version_str_);
            }


            this->rx_pin_->setup();
            this->tx_pin_->setup();
            this->tx_pin_->digital_write(false);
            
            auto &s = this->store_;

            s.rx_pin = this->rx_pin_->to_isr();
            this->rx_pin_->attach_interrupt(TCSComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);

            // Reset Sensors
            if (this->bus_command_ != nullptr)
            {
                this->bus_command_->publish_state("");
            }

            for (auto &listener : listeners_)
            {
                listener->turn_off(&listener->timer_);
            }
        }

        void TCSComponent::dump_config()
        {
            ESP_LOGCONFIG(TAG, "TCS Intercom:");
            LOG_PIN("  Pin RX: ", this->rx_pin_);
            LOG_PIN("  Pin TX: ", this->tx_pin_);

            if (strcmp(event_, "esphome.none") != 0)
            {
                ESP_LOGCONFIG(TAG, "  Event: %s", event_);
            }
            else
            {
                ESP_LOGCONFIG(TAG, "  Event: disabled");
            }

            if (this->hardware_version_ != nullptr)
            {
                this->hardware_version_->publish_state("Generic");
            }

            ESP_LOGCONFIG(TAG, "  Hardware: %s", this->hardware_version_str_.c_str());
        }

        void TCSComponent::loop()
        {
            // Turn off binary sensor after ... milliseconds
            uint32_t now_millis = millis();
            for (auto &listener : listeners_)
            { 
                if (listener->timer_ && now_millis > listener->timer_)
                {
                    listener->turn_off(&listener->timer_);
                }
            }

            auto &s = this->store_;

            if(s.s_cmdReady)
            {
                ESP_LOGD(TAG, "Received command %x", s.s_cmd);
                this->publish_command(s.s_cmd, true);

                s.s_cmdReady = false;
                s.s_cmd = 0;
            }
        }

        void TCSComponent::register_listener(TCSIntercomListener *listener)
        {
            this->listeners_.push_back(listener); 
        }

        volatile uint32_t TCSComponentStore::s_cmd = 0;
        volatile uint8_t TCSComponentStore::s_cmdLength = 0;
        volatile bool TCSComponentStore::s_cmdReady = false;

        void bitSetIDF(uint32_t *variable, int bitPosition) {
            *variable |= (1UL << bitPosition);
        }

        uint8_t bitReadIDF(uint32_t variable, int bitPosition) {
            return (variable >> bitPosition) & 0x01;
        }

        void IRAM_ATTR HOT TCSComponentStore::gpio_intr(TCSComponentStore *arg)
        {
            // Made by https://github.com/atc1441/TCSintercomArduino
            static uint32_t curCMD;
            static uint32_t usLast;

            static uint8_t curCRC;
            static uint8_t calCRC;
            static uint8_t curLength;
            static uint8_t cmdIntReady;
            static uint8_t curPos;

            uint32_t usNow = micros();
            uint32_t timeInUS = usNow - usLast;
            usLast = usNow;

            uint8_t curBit = 4;

            if (timeInUS < 1000)
            {
                curBit = 5; // invalid glitches typical 29ms
            }
            else if (timeInUS >= 1000 && timeInUS <= 2999)
            {
                curBit = 0;
            }
            else if (timeInUS >= 3000 && timeInUS <= 4999)
            {
                curBit = 1;
            }
            else if (timeInUS >= 5000 && timeInUS <= 6999)
            {
                curBit = 2;
            }
            else if (timeInUS >= 7000 && timeInUS <= 24000)
            {
                curBit = 3;
                curPos = 0;
            }

            if (curBit != 5) { // skip processing for glitches
                if (curPos == 0)
                {
                    if (curBit == 2)
                    {
                        curPos++;
                    }

                    curCMD = 0;
                    curCRC = 0;
                    calCRC = 1;
                    curLength = 0;
                }
                else if (curBit == 0 || curBit == 1)
                {
                    if (curPos == 1)
                    {
                        curLength = curBit;
                        curPos++;
                    }
                    else if (curPos >= 2 && curPos <= 17)
                    {
                        if (curBit)
                        {
                            #if defined(USE_ESP_IDF)
                            bitSetIDF(&curCMD, (curLength ? 33 : 17) - curPos);
                            #else
                            bitSet(curCMD, (curLength ? 33 : 17) - curPos);
                            #endif
                        }

                        calCRC ^= curBit;
                        curPos++;
                    }
                    else if (curPos == 18)
                    {
                        if (curLength)
                        {
                            if (curBit)
                            {
                                #if defined(USE_ESP_IDF)
                                bitSetIDF(&curCMD, 33 - curPos);
                                #else
                                bitSet(curCMD, 33 - curPos);
                                #endif
                            }

                            calCRC ^= curBit;
                            curPos++;
                        }
                        else
                        {
                            curCRC = curBit;
                            cmdIntReady = 1;
                        }
                    }
                    else if (curPos >= 19 && curPos <= 33)
                    {
                        if (curBit)
                        {
                            #if defined(USE_ESP_IDF)
                            bitSetIDF(&curCMD, 33 - curPos);
                            #else
                            bitSet(curCMD, 33 - curPos);
                            #endif
                        }
                        
                        calCRC ^= curBit;
                        curPos++;
                    }
                    else if (curPos == 34)
                    {
                        curCRC = curBit;
                        cmdIntReady = 1;
                    }
                }
                else
                {
                    curPos = 0;
                }

                if (cmdIntReady)
                {
                    cmdIntReady = 0;

                    if (curCRC == calCRC)
                    {
                        arg->s_cmdReady = true;
                        arg->s_cmd = curCMD;
                    }

                    curCMD = 0;
                    curPos = 0;
                }
            }
        }

        void TCSComponent::publish_command(uint32_t command, bool fire_events)
        {
            // Convert to HEX
            char byte_cmd[9];
            sprintf(byte_cmd, "%08X", command);

            // Publish Command to Sensors
            if (this->bus_command_ != nullptr)
            {
                this->bus_command_->publish_state(byte_cmd);
            }

            if(fire_events)
            {
                // Fire Binary Sensors
                for (auto &listener : listeners_)
                {
                    if (listener->f_.has_value())
                    {
                        auto val = (*listener->f_)();
                        
                        if (val == command)
                        {
                            listener->turn_on(&listener->timer_, listener->auto_off_);
                        }
                    }
                    else
                    {
                        if (listener->command_ == command)
                        {
                            listener->turn_on(&listener->timer_, listener->auto_off_);
                        }
                    }
                }
                
                // Fire Home Assistant Event
                if (strcmp(event_, "esphome.none") != 0)
                {
                    auto capi = new esphome::api::CustomAPIDevice();

                    ESP_LOGD(TAG, "Send event to home assistant on %s", event_);
                    capi->fire_homeassistant_event(event_, {{"command", byte_cmd}});
                }
            }
        }

        void TCSComponent::send_command(uint32_t command)
        {
            ESP_LOGD(TAG, "Sending command %x", command);

            if (this->sending)
            {
                ESP_LOGD(TAG, "Sending of command %i cancelled, another sending is in progress", command);
            }
            else
            {
                // Pause reading
                ESP_LOGD(TAG, "Pause reading");
                this->rx_pin_->detach_interrupt();

                // Made by https://github.com/atc1441/TCSintercomArduino
                this->sending = true;

                int length = 16;
                uint8_t checksm = 1;
                bool isLongMessage = false;
                bool output_state = false;

                if (command > 0xFFFF)
                {
                    length = 32;
                    isLongMessage = true;
                }

                this->tx_pin_->digital_write(true);
                delay(TCS_MSG_START_MS);

                this->tx_pin_->digital_write(false);
                delay(isLongMessage ? TCS_ONE_BIT_MS : TCS_ZERO_BIT_MS);

                int curBit = 0;
                for (uint8_t i = length; i > 0; i--)
                {
                    #if defined(USE_ESP_IDF)
                    curBit = bitReadIDF(command, i - 1);
                    #else
                    curBit = bitRead(command, i - 1);
                    #endif
                    output_state = !output_state;
                    this->tx_pin_->digital_write(output_state);
                    delay(curBit ? TCS_ONE_BIT_MS : TCS_ZERO_BIT_MS);
                    checksm ^= curBit;
                }

                this->tx_pin_->digital_write(!output_state);
                delay(checksm ? TCS_ONE_BIT_MS : TCS_ZERO_BIT_MS);
                this->tx_pin_->digital_write(false);

                this->sending = false;

                // Resume reading
                ESP_LOGD(TAG, "Resume reading");
                this->rx_pin_->attach_interrupt(TCSComponentStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);

                // Publish received Command on Sensors, Events, etc.
                this->publish_command(command, false);
            }
        }

    }  // namespace tcs_intercom
}  // namespace esphome