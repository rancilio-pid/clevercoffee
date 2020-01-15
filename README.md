# ranciliopid
Rancilio-Silvia PID für Arduino http://rancilio-pid.de

MASTER VERSION

Version 1.9.7

# Additional information
- Copy file userConfig.h.SAMPLE to userConfig.h and edit this file accordingly.
- Additional Arduino dependency on PubSubClient (tested with Version 2.7.0). 
  Please install this lib by using Arduino->Sketch->Include Library->"Library Manager".
  ![Library Manager](https://raw.githubusercontent.com/medlor/ranciliopid/add-mqtt-support/PubSubClient_Dep.jpg)

Changelog:
- 1.9.7:
  - Fix: If temperature sensor detects more than 150Celcius then it must be an error.
  - Fix: Typos / explanations.
  - Fix: Race condition when code is blocked for some time
  - Fix: Added initial temp poll to prevent error on start.
  - Feature: Detect missing temperature polls.
  - Feature: Add configurable BLYNKPORT.
  - Feature: Wifi hostname configuable.
  - Fix: Safe-guard: Stop heating forever if sensor is flapping!
  - Beta Feature: Prevent Pid to have the heater activated and deactivated within the timeframe defined in HeaterPreventFlapping (in ms).
  - Fix: Pid.Compute() is now in sync with isrCounter and is not loosing one tick each second. Heater flapping is reduced.
>>>>>>> origin/minor_improvements
