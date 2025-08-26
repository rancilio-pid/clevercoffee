## CleverCoffee Configuration Reference

This document describes all configuration parameters available in the `config.json` file in alphabetical order. Each parameter includes its purpose, valid values, and constraints.

---

## Backflush Settings

### `backflush.cycles`
- **Type**: Integer
- **Default**: `5`
- **Range**: 2-20
- **Description**: Number of backflush cycles to perform during cleaning

### `backflush.fill_time`
- **Type**: Double (seconds)
- **Default**: `5.0`
- **Range**: 3.0-10.0
- **Description**: Time to fill the group head with cleaning solution

### `backflush.flush_time`
- **Type**: Double (seconds)
- **Default**: `10.0`
- **Range**: 5.0-20.0
- **Description**: Time to flush cleaning solution back through the 3-way valve into the drip tray

---

## Brew Settings

### `brew.pid_delay`
- **Type**: Double (seconds)
- **Default**: `10.0`
- **Range**: 0.0-60.0
- **Description**: Delay before PID control activates during brewing

### `brew.setpoint`
- **Type**: Double (°C)
- **Default**: `95.0`
- **Range**: 20.0-110.0
- **Description**: Target brewing temperature

### `brew.temp_offset`
- **Type**: Double (°C)
- **Default**: `0.0`
- **Range**: 0.0-20.0
- **Description**: Temperature offset to compensate for sensor placement or other dropoff

### `brew.mode`
- **Type**: Integer (enum)
- **Default**: `0`
- **Valid Values**:
  - `0`: Manual
  - `1`: Automatic
- **Description**: Manual mode gives you full control over the brew time while Automatic mode allows you to activate brew-by-time and/or brew-by-weight. The brew will then stop at whatever target is reached first.

### `brew.by_time.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enables brew by time, so the pump stops automatically when the target brew time is reached

### `brew.by_time.target_time`
- **Type**: Double (seconds)
- **Default**: `25.0`
- **Range**: 1.0-120.0
- **Description**: Target brew time for automatic brewing

### `brew.by_weight.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enables brew by weight, so the pump stops automatically when the target weight is reached

### `brew.by_weight.target_weight`
- **Type**: Double (grams)
- **Default**: `36.0`
- **Range**: 0.0-500.0
- **Description**: Target output weight for automatic brewing (requires scale)

### `brew.by_weight.auto_tare`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enables auto-tare of a connected BLE scale

## Pre-Infusion Settings

### `brew.pre_infusion.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enables pre-wetting of the coffee puck by turning on the pump for a configurable length of time

### `brew.pre_infusion.time`
- **Type**: Double (seconds)
- **Default**: `2.0`
- **Range**: 0.0-60.0
- **Description**: Duration of pre-infusion phase

### `brew.pre_infusion.pause`
- **Type**: Double (seconds)
- **Default**: `5.0`
- **Range**: 0.0-60.0
- **Description**: Pause duration after pre-infusion

---

## Display Settings

### `display.fullscreen_brew_timer`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Show brew timer in fullscreen mode

### `display.blescale_brew_timer`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable starting and stopping the brew timer on a connected BLE scale

### `display.fullscreen_manual_flush_timer`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Show manual flush timer in fullscreen mode

### `display.fullscreen_hot_water_timer`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Show hot water timer in fullscreen mode

### `display.heating_logo`
- **Type**: Boolean
- **Default**: `true`
- **Description**: Display heating indicator logo until setpoint is reached

### `display.language`
- **Type**: Integer (enum)
- **Default**: `0`
- **Valid Values**:
    - `0`: English
    - `1`: Deutsch
    - `2`: Español
- **Description**: OLED display language selection

### `display.post_brew_timer_duration`
- **Type**: Double (seconds)
- **Default**: `3.0`
- **Range**: 0.0-60.0
- **Description**: Duration to show shot timer post-brew

### `display.template`
- **Type**: Integer (enum)
- **Default**: `0`
- **Valid Values**:
    - `0`: Standard
    - `1`: Minimal
    - `2`: Temp-only
    - `3`: Scale
    - `4`: Upright
- **Description**: Display template selection

### `display.inverted`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Display rotation

### `display.blinking.mode`
- **Type**: Integer (enum)
- **Default**: `1`
- **Valid Values**:
    - `0`: Off
    - `1`: Near Setpoint
    - `2`: Away From Setpoint
- **Description**: Set temperature display blinking mode

### `display.blinking.delta`
- **Type**: Double
- **Default**: `0.3`
- **Range**: 0.2-10.0
- **Description**: Difference from setpoint for blinking temperature display

---

## Hardware Configuration

## OLED Display

### `hardware.oled.enabled`
- **Type**: Boolean
- **Default**: `true`
- **Description**: Enable OLED display

### `hardware.oled.address`
- **Type**: Integer (hex)
- **Default**: `60` (0x3C)
- **Range**: 0-255
- **Description**: I2C address of OLED display

### `hardware.oled.type`
- **Type**: Integer (enum)
- **Default**: `0`
- **Valid Values**:
    - `0`: SSD1306
    - `1`: SH1106
- **Description**: OLED controller type

## LEDs

For each LED type (status, brew, steam):

### `hardware.leds.{type}.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable the LED

### `hardware.leds.{type}.inverted`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Invert LED logic (active low)

## Relays

### `hardware.relays.heater.trigger_type`
- **Type**: Integer (enum)
- **Default**: `1`
- **Valid Values**:
    - `0`: Low trigger (active low)
    - `1`: High trigger (active high)
- **Description**: Heater relay trigger type

### `hardware.relays.valve.trigger_type`
- **Type**: Integer (enum)
- **Default**: `1`
- **Valid Values**:
    - `0`: Low trigger (active low)
    - `1`: High trigger (active high)
- **Description**: Valve relay trigger type

### `hardware.relays.pump.trigger_type`
- **Type**: Integer (enum)
- **Default**: `1`
- **Valid Values**:
    - `0`: Low trigger (active low)
    - `1`: High trigger (active high)
- **Description**: Pump relay trigger type

## Sensors

## Temperature Sensor
### `hardware.sensors.temperature.type`
- **Type**: Integer (enum)
- **Default**: `0`
- **Valid Values**:
    - `0`: TSIC 306
    - `1`: DS18B20
- **Description**: Temperature sensor type

## Pressure Sensor
### `hardware.sensors.pressure.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable pressure sensor

## Scale
### `hardware.sensors.scale.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable scale/weight sensor

### `hardware.sensors.scale.calibration`
- **Type**: Double
- **Default**: `1.0`
- **Range**: -999999.0-999999.0
- **Description**: Primary scale calibration factor

### `hardware.sensors.scale.calibration2`
- **Type**: Double
- **Default**: `1.0`
- **Range**: -999999.0-999999.0
- **Description**: Secondary scale calibration factor

##### `hardware.sensors.scale.known_weight`
- **Type**: Double (grams)
- **Default**: `267.0`
- **Range**: 1.0-2000.0
- **Description**: Known weight for calibration

##### `hardware.sensors.scale.samples`
- **Type**: Integer
- **Default**: `2`
- **Range**: 1-20
- **Description**: Number of samples to average

### `hardware.sensors.scale.type`
- **Type**: Integer (enum)
- **Default**: `0`
- **Valid Values**:
    - `0`: 2 load cells
    - `1`: 1 load cell
    - `2`: Bluetooth
- **Description**: Scale configuration

## Water Tank
### `hardware.sensors.watertank.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable water tank level sensor

### `hardware.sensors.watertank.mode`
- **Type**: Integer (enum)
- **Default**: `1`
- **Valid Values**:
    - `0`: Normally open
    - `1`: Normally closed
- **Description**: Water tank sensor switch mode

## Switches

For each switch type (brew, power, steam):

### `hardware.switches.{type}.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable the switch

### `hardware.switches.{type}.mode`
- **Type**: Integer (enum)
- **Default**: `0`
- **Valid Values**:
    - `0`: Normally open
    - `1`: Normally closed
- **Description**: Switch electrical configuration

### `hardware.switches.{type}.type`
- **Type**: Integer (enum)
- **Default**: `1`
- **Valid Values**:
    - `0`: Momentary
    - `1`: Toggle
- **Description**: Switch behavior type

---

## MQTT Settings

### `mqtt.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable MQTT connectivity

### `mqtt.broker`
- **Type**: String
- **Default**: `""`
- **Max Length**: 253 characters
- **Description**: MQTT broker hostname or IP address

### `mqtt.port`
- **Type**: Integer
- **Default**: `1883`
- **Range**: 1-65535
- **Description**: MQTT broker port number

### `mqtt.username`
- **Type**: String
- **Default**: `"rancilio"`
- **Max Length**: 64 characters
- **Description**: MQTT authentication username

### `mqtt.password`
- **Type**: String
- **Default**: `"silvia"`
- **Max Length**: 64 characters
- **Description**: MQTT authentication password

### `mqtt.topic`
- **Type**: String
- **Default**: `"custom/kitchen/"`
- **Max Length**: 180 characters
- **Description**: Base MQTT topic prefix

## Home Assistant Integration

### `mqtt.hassio.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable Home Assistant auto-discovery

### `mqtt.hassio.prefix`
- **Type**: String
- **Default**: `"homeassistant"`
- **Max Length**: 64 characters
- **Description**: Home Assistant discovery topic prefix

---

## PID Controller Settings

### `pid.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable PID temperature control

### `pid.use_ponm`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Use Proportional on Measurement mode

### `pid.ema_factor`
- **Type**: Double
- **Default**: `0.6`
- **Range**: 0.0-1.0
- **Description**: Exponential moving average factor for temperature smoothing

## Regular PID Parameters

### `pid.regular.kp`
- **Type**: Double
- **Default**: `62.0`
- **Range**: 0.0-999.0
- **Description**: Proportional gain

### `pid.regular.tn`
- **Type**: Double (seconds)
- **Default**: `52.0`
- **Range**: 0.0-999.0
- **Description**: Integral time constant

### `pid.regular.tv`
- **Type**: Double (seconds)
- **Default**: `11.5`
- **Range**: 0.0-999.0
- **Description**: Derivative time constant

### `pid.regular.i_max`
- **Type**: Double
- **Default**: `55.0`
- **Range**: 0.0-999.0
- **Description**: Maximum integral term contribution

## Brew Detection PID Parameters

### `pid.bd.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable brew detection PID mode

### `pid.bd.kp`
- **Type**: Double
- **Default**: `50.0`
- **Range**: 0.0-999.0
- **Description**: Brew detection proportional gain

### `pid.bd.tn`
- **Type**: Double (seconds)
- **Default**: `0.0`
- **Range**: 0.0-999.0
- **Description**: Brew detection integral time constant

### `pid.bd.tv`
- **Type**: Double (seconds)
- **Default**: `20.0`
- **Range**: 0.0-999.0
- **Description**: Brew detection derivative time constant

## Steam PID Parameters

### `pid.steam.kp`
- **Type**: Double
- **Default**: `150.0`
- **Range**: 0.0-999.0
- **Description**: Steam mode proportional gain

---

## Standby Settings

### `standby.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable automatic standby mode

### `standby.time`
- **Type**: Double (minutes)
- **Default**: `35.0`
- **Range**: 1.0-120.0
- **Description**: Time before entering standby mode

---

## Steam Settings

### `steam.setpoint`
- **Type**: Double (°C)
- **Default**: `120.0`
- **Range**: 100.0-140.0
- **Description**: Target temperature for steam mode

---

## System Settings

### `system.hostname`
- **Type**: String
- **Default**: `"silvia"`
- **Max Length**: 32 characters
- **Description**: Network hostname for the device

### `system.log_level`
- **Type**: Integer (enum)
- **Default**: `2`
- **Valid Values**:
    - `0`: TRACE (most verbose)
    - `1`: DEBUG
    - `2`: INFO
    - `3`: WARNING
    - `4`: ERROR
    - `5`: CRITICAL (least verbose)
- **Description**: Logging verbosity level

### `system.timing_debug.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: When DEBUG log level is enabled select whether to also show loop timing information

### `system.showdisplay.enabled`
- **Type**: Boolean
- **Default**: `true`
- **Description**: When DEBUG log level is enabled select whether to include display refresh in the timing information

### `system.offline_mode`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Disable WiFi and run in offline mode

### `system.ota_password`
- **Type**: String
- **Default**: `"otapass"`
- **Max Length**: 64 characters
- **Description**: Password for over-the-air firmware updates

### `system.auth.enabled`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enables prompt for username and password

### `system.auth.username`
- **Type**: String
- **Default**: `"admin"`
- **Max Length**: 32 characters
- **Description**: Username for accessing webpages

### `system.auth.password`
- **Type**: String
- **Default**: `"admin"`
- **Max Length**: 64 characters
- **Description**: Password for accessing webpages

---

## Notes

- All temperature values are in Celsius
- Time values are in seconds with one decimal point
- Weight values are in grams
- Boolean values: `true` or `false`
- String values should be enclosed in double quotes
- Integer enums must use the exact numeric values shown
- Invalid values will be rejected and the previous valid value will be retained

Always backup your working configuration before making changes!