# TODO
- Feature: Enable Pin15 to power a LED which shows if heater is running at that moment.
  - Check if heater is not getting triggered every 400ms near the target temperature (hardware wear out?)
    Update: PID is evaluated every 1 second, but it is currently possible to activate heater in 10ms steps within the 1 second window (see onTimer1ISR() + https://playground.arduino.cc/Code/PIDLibrary/ ).
      - Perhaps the easiest way would be to increase windowSize from 1sec to 3sec. Still q remains on how to directly tune Pid.compute() when near target temp.
      - Update 2: Because Pid does not remember past Outputs, it should be no problem to simply ignore too small "on-time" of "off-times" (based on var Output being near zero or 1 second.) -> Fix as described in "Update 2" implemented.
- Feature: (Optional) add support for messenger telegram?
- Feature: Perhaps add an LED which shows when the ideal temperature is reached (?)
- mqtt events/ should deliver json not string.
## Fix Block 1
- Feature: Add MQTT support for stats (every 60secs?) and important msgs (and debug?) AND to activate heater.
  - Check for mqtt service network timeouts affecting heater controller.
- Robustness: Improve logging.
## Fix Block 2
- Fix: Have pid.Compute() in sync with getTemp() (currently 1sec vs 0.4s interval w/ no sync).
  - Perhaps it is better to have move pid.Compute() in main.loop() and only have heater-control running in ISR.
- Fix: ISR variables must be volatile.
  - Is an easy fix in solution above.
- Add EmergecyStop logic into ISR as safe-guard. useful?
## Fix Block 3
- MovAvg Calc is wrong:
  - readIndex ist niemals "0". Somit updaten sich die readings[0] Werte nie und die Berechnung ist fehlerhaft.
  - Die Variable changerate wird berechnet indem man den derzeitigen readIndex mit dem readIndex+1 vergleicht. readIndex+1 ist aber nicht der Tempsensor-Wert 400ms davor, sondern der 14400=6sec vorherige Wert. Im Ergebnis liefert heatrateaverage dann nicht den Average Wert der TemperaturUnterschiedeJeSekunde, sondern was ganz anderes!
  - Warum wird der Value heatrateaverage mit 100 multipliziert? Da das ein double ist macht, das doch keinen Sinn, oder?


# TODO Tests
- Robustness: Check if millis race condition fix, causes issues in PID calculation due to missing entries.
- Test if OFFLINEMODUS=1 really reads pid settings from EPPROM
- Do we need a measurement to detect and notify when maschine is ready to brew?
