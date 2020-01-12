# TODO
- Robustness: Improve logging.
- Feature: Enable Pin15 to power a LED which shows if heater is running at that moment.
  - Check if heater is not getting triggered every 400ms near the target temperature (hardware wear out?)
    Update: PID is evaluated every 1 second, but it is currently possible to activate heater in 10ms steps within the 1 second window (see onTimer1ISR() + https://playground.arduino.cc/Code/PIDLibrary/ ).
      - Perhaps the easiest way would be to increase windowSize from 1sec to 3sec. Still q remains on how to directly tune Pid.compute() when near target temp.
      - Update 2: Because Pid does not remember past Outputs, it should be no problem to simply ignore too small "on-time" of "off-times" (based on var Output being near zero or 1 second.) -> Fix as described in "Update 2" implemented.
- Robustness: Check if millis race condition fix, causes issues in PID calculation due to missing entries.
- Feature: Add MQTT support for stats (every 60secs?) and important msgs (and debug?) AND to activate heater.
  - Check for mqtt service network timeouts affecting heater controller.
- Feature: (Optional) add support for messenger telegram?
- Feature: Perhaps add an LED which shows when the ideal temperature is reached (?)

# TODO Tests
- Test if OFFLINEMODUS=1 really reads pid settings from EPPROM
- Add EmergecyStop logic into ISR as safe-guard. useful?
