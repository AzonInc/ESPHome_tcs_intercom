TCS Intercom Component
===================

The ``tcs_intercom`` Component allows you to easily communicate on a TCS:Bus with ESPHome and Home Assistant.

Basic ESPHome Configuration Entry:
```
external_components:
  - source: github://AzonInc/ESPHome-TCS-Intercom

tcs_intercom:
```
 
Example of a Configuration Entry, which generates a binary_sensor entity that goes to on when receiving the Apartment Doorbell Command (in this case 0x1c30ba41), and a button entity that when pressed sends the command to open entrance door:
```
external_components:
  - source: github://AzonInc/ESPHome-TCS-Intercom
	
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
```
external_components:
  - source: github://AzonInc/ESPHome-TCS-Intercom
	
tcs_intercom:
  rx_pin: GPIO22
  tx_pin: GPIO23

binary_sensor:
  - platform: tcs_intercom
	name: "Apartment Doorbell"
	command: 0x1c30ba41
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

- **rx_pin** (*Optional*, pin): The pin used to receive commands. Defaults to ``GPIO47``.
- **tx_pin** (*Optional*, pin): The pin used to transmitt commands. Defaults to ``GPIO48``.

- <a id="eventlist">**event**</a>  (_Optional_, string): The name of the event that will be generated on home assistant when receiving a command from the bus. For example, if  set to `tcs`, the event generated will be "esphome.tcs".
Read more about how to use it in the [event section](#event)
Default to `tcs`.
If this parameter is set to `none` no event will be generated.


Binary sensor
===================

You can configure binary sensors that go on when a particular command is received from the bus.

Configuration examples:

	binary_sensor:
	  - platform: tcs_intercom
        name: Apartment Doorbell
        command: "0x1c30ba41"

	  - platform: tcs_intercom
	    command: "0x0c30ba80"
	    name: Entrance Doorbell
	    auto_off: 5s

- **command** (**Required**, int): The command that when received sets the sensor to on.
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

To intercept this event to trigger an home assistant automation, you can use a trigger of type "event."

The trigger configuration will look like this:

	platform: event
	event_type: esphome.tcs
	event_data:
	  command: "0x00002480"

You have to change the command and the name of the event you have chosen, if you have set a different one.

Transmit a command
==================
To send commands to the bus, the following action is available:

	- tcs_intercom.send:
	    command: "0x1100"

- **command** (**Required**, int)

### Button:
The action can be easily inserted into a button type entity:

	button:
	  - platform: template
	    name: Open Door
	    on_press:
	      - tcs_intercom.send:
	          command: "0x1100"


### Service:
You can create a home assistant service, which can easily be invoked by an automation or script:

	api:
	  encryption:
	    key: "xxxxxxxxxxxxxxxxxxx"
	  services:
	    - service: open_door
	      then:
	        - tcs_intercom.send:
	            command: "0x1100"

### Sending multiple commands:
There are some special configurations that require sending 2 or more commands consecutively on the bus.
In this case, a delay of at least 200ms must be inserted between the commands (one command takes about 50ms to be sent)

	- tcs_intercom.send:
	    command: "0x1100"
	- delay: 90ms
	- tcs_intercom.send:
	    command: "0x2100"