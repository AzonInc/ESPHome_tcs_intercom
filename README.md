# ⚠️ This Repository Has Moved! ⚠️

All future updates, improvements, and support are now hosted in our new home: [**Doorman**](https://github.com/AzonInc/Doorman)!

### Key Resources for a Smooth Transition:
- 🚀 **New TC:BUS Component**: Explore the latest version [here](https://github.com/AzonInc/Doorman/tree/master/components/tc_bus), with improved functionality and streamlined setup.
- 📚 **Expanded Documentation**: Visit the updated [TC:BUS ESPHome docs](https://doorman.azon.ai/reference/esphome-component) for in-depth guides, examples, and usage tips.

## Ready to Migrate?
Check out [this example configuration](https://doorman.azon.ai/reference/esphome-component#example-yaml-configuration) to help you migrate from `tcs_intercom` to the upgraded `tc_bus` ESPHome component. It's as simple as replacing `tcs_intercom` with `tc_bus` and updating platform keys accordingly.

Thank you for moving with us! The Doorman repository is ready to offer you the latest and most advanced features for TC:BUS.


# ❌ TCS/Koch Intercom Component (DEPRECATED)
![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/AzonInc/ESPHome_tcs_intercom/build.yaml?branch=master&style=for-the-badge&logo=buddy&logoColor=ffffff&label=Build)
===================

The `tcs_intercom` Component allows you to easily communicate on a [TCS:Bus](https://www.tcsag.de/) and [Koch TC:Bus](https://kochag.ch/) with ESPHome and Home Assistant.

Minimal ESPHome Configuration:
```YAML
external_components:
  - source: github://AzonInc/ESPHome_tcs_intercom

tcs_intercom:
```
 
Example of a Configuration, which generates a binary_sensor entity that goes to on when receiving the Apartment Doorbell Command (in this case 0x1c30ba41), and a button entity that when pressed sends the command to open entrance door:
```YAML
external_components:
  - source: github://AzonInc/ESPHome_tcs_intercom

tcs_intercom:

binary_sensor:
  - platform: tcs_intercom
    name: "Apartment Doorbell"
    command: 0x1c30ba41

button:
  - platform: template
    name: Open Door
    on_press:
      - tcs_intercom.send:
          command: 0x1100
```

Full Configuration:
```YAML
external_components:
  - source: github://AzonInc/ESPHome_tcs_intercom

tcs_intercom:
  rx_pin: GPIO9
  tx_pin: GPIO8
  event: "tcs"
  bus_command:
    name: "Last Bus Command"

binary_sensor:
  - platform: tcs_intercom
    name: "Apartment Doorbell"
    command: 0x1c30ba41
    auto_off: 5s

  - platform: tcs_intercom
    name: "Apartment Doorbell Lambda"
    lambda: return 0x1c30ba41;
    auto_off: 5s


button:
  - platform: template
    name: Open Door
    on_press:
      - tcs_intercom.send:
          command: 0x1100
```

Configuration variables:
------------------------

- **rx_pin** (*Optional*, pin): The pin used to receive commands. Defaults to ``GPIO9``.
- **tx_pin** (*Optional*, pin): The pin used to transmitt commands. Defaults to ``GPIO8``.

- <a id="eventlist">**event**</a>  (_Optional_, string): The name of the event that will be generated on home assistant when receiving a command from the bus. For example, if  set to `tcs`, the event generated will be "esphome.tcs".
Read more about how to use it in the [event section](#event)
Default to `tcs`.
If this parameter is set to `none` no event will be generated.

- **bus_command** (*Optional*, text_sensor): A Text Sensor which shows the last received Bus Command.

Binary sensor
===================

You can configure binary sensors that go on when a particular command is received from the bus.

Configuration examples:
```YAML
binary_sensor:
  - platform: tcs_intercom
    name: Apartment Doorbell
    command: 0x1c30ba41

  - platform: tcs_intercom
    lambda: return 0x0c30ba80;
    name: Entrance Doorbell
    auto_off: 5s
```

- **command** (**Optional**, int): The command that when received sets the sensor to on.
- **lambda** (**Optional**, int): The command that when received sets the sensor to on.
- **auto_off** (*Optional*,  [Time](https://esphome.io/guides/configuration-types#config-time)):  The time after which the sensor returns to off. If set to `0s` the sensor once it goes on, it stays there until it is turned off by an automation. Defaults to  `3s`.
- **icon** (*Optional*, icon): Manually set the icon to use for the sensor in the frontend. Default to `mdi:doorbell`.
- **id** (*Optional*, string): Manually specify the ID for code generation.
- **name** (*Optional*, string): The name for the sensor. Default to `Doorbell`.

    Note:
    If you have friendly_name set for your device and you want 
    the sensor to use that name, you can set `name: None`.


Event
========
If the [event](#eventlist) parameter is not set to `none`, an event will be generated each time a command is received.

You can intercept events on home assistant on the page "developer tools -> event"

Each time a command is received, an event like this will be generated:
```YAML
event_type: esphome.tcs
data:
  device_id: xxxxxxxxxxxxxxxxxxxxxxxxx
  command: "0x00002480"
origin: LOCAL
time_fired: "2024-01-01T00:00:00.000000+00:00"
context:
  id: xxxxxxxxxxxxxxxxxxxxxxxx
  parent_id: null
  user_id: null
```

To intercept this event to trigger an home assistant automation, you can use a trigger of type "event."

The trigger configuration will look like this:
```YAML
platform: event
event_type: esphome.tcs
event_data:
  command: "0x00002480"
```

You have to change the command and the name of the event you have chosen, if you have set a different one.


Transmit a command
==================
To send commands to the bus, the following action is available:
```YAML
on_...:
  - tcs_intercom.send:
      command: 0x1100
```

- **command** (**Required**, int)

### Button:
The action can be easily inserted into a button type entity:
```YAML
button:
  - platform: template
    name: Open Door
    on_press:
      - tcs_intercom.send:
          command: 0x1100
```

### Service:
You can create a home assistant service, which can easily be invoked by an automation or script:
```YAML
api:
  encryption:
    key: "xxxxxxxxxxxxxxxxxxx"
    services:
      - service: open_door
    then:
      - tcs_intercom.send:
          command: 0x1100
```

### Sending multiple commands:
There are some special configurations that require sending 2 or more commands consecutively on the bus.
In this case, a delay of at least 90ms must be inserted between the commands (one command takes about 70ms to be sent)

```YAML
on_...:
  - tcs_intercom.send:
      command: 0x1100
  - delay: 90ms
  - tcs_intercom.send:
      command: 0x2100
```


## Credits

**[TCSIntercomArduino](https://github.com/atc1441/TCSintercomArduino)**\
Different Methods to read from and write to the TCS/TC Bus.\
Feel free to watch the [Reverse Engineering Video](https://www.youtube.com/watch?v=xFLoauqj9yA&t=11s) if you're interested.

**[tcs-monitor](https://github.com/Syralist/tcs-monitor)**\
An mqtt monitor for listening to the TCS/TC Bus.\
You can find more information in this [Blog Post](https://blog.syralist.de/posts/smarthome/klingel/).

**[Doorman](https://github.com/peteh/doorman)**\
A custom Firmware for ESP8266 / ESP32 to control a TCS/Koch Intercom.