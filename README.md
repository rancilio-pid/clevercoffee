# ranciliopid
Rancilio-Silvia PID für Arduino http://rancilio-pid.de

BLEEDING EDGE BETA VERSION

Version 2.0.1_beta

# Most important features in comparison to original rancilio master:
1. New PID Controller "Multi-state PID with steadyPower (Bias)"
   - Distinct PID settings dependend on the current "state" of the maschine. Most of the settings are either static or semi-automatically tuned.
   - Currently 5 states are implemented:
     - Coldstart (maschine is cold)
     - Coldstart stabilisation (directly after coldstart)
     - Inner Zone (temperature near setPoint)
     - Outer Zone (temperature outside of "inner zone")
     - Brewing
   - steadyPower is introduced which compensates the constant temperature loss due to environment
   - steadyPowerOffset is introduced which compensates the increased temperature loss when the maschine (brew head etc.) are still very cold.
   - PidController offers feature like filtering, special handling of setPoint crossings and more (hard-coded)
   - PID Controller is now integral part of the software and not an external library.
1. MQTT support to integrate maschine in smart-home solutions and to easier extract details for graphing/alerting.
1. Added RemoteDebug over telnet so that we dont need USB to debug/tune pid anymore (https://github.com/JoaoLopesF/RemoteDebug)
   - Just "$ telnet <rancilio ip> 23"
1. "Brew Ready" Detection implemented, which detects when the temperature has stabilized at setPoint. It can send an
   MQTT event or have hardware pin 15 triggered (which can be used to turn a LED on).
1. All heater power relevant settings are now set and given in percent (and not absolute output) and therefore better to understand
1. Many useful functions to be used internally  getAverageTemperature(), pastTemperatureChange() + updateTemperatureHistory())
1. Safetly toogle added to shutdown heater on sensor malfunction (TEMPSENSORRECOVERY)

# ATTENTION:
- EEPROM has changed. Therefore you have to connect to blynk at least once after flashing, and manually set correct settings in blynk app (see screenshots for default values).
- I only own the pid-only hardware solution, so I am greatful for feedback(bug reports) from persons using the full version. Please monitor our maschine's temperature closely the first few times. The muti-state pid controller should never lead to temperatures greater than 5 degress above setpoint!

# Additional important information
- Installation is as defined on http://rancilio-pid.de/ but with following adapations.
- Copy file userConfig.h.SAMPLE to userConfig.h and edit this file accordingly.
- Additional Arduino dependency on PubSubClient (tested with Version 2.7.0). 
  Please install this lib by using Arduino->Sketch->Include Library->"Library Manager".
  ![Library Manager](https://github.com/medlor/ranciliopid/blob/master/PubSubClient_Dep.jpg)
- You have to use following Blynk Application settings. Just import following QR-Code:
  <img src="https://github.com/medlor/ranciliopid/blob/master/blynk_application_qr_code.png" height="250">


# Blynk application screenshots (including default values)
<p align="center">
<img src="https://github.com/medlor/ranciliopid/blob/master/blynk_app_status.jpg" height="500">
<img src="https://github.com/medlor/ranciliopid/blob/master/blynk_app_controller.jpg" height="500">
<img src="https://github.com/medlor/ranciliopid/blob/master/blynk_app_preinfusion.jpg" height="500">
</p>


# Tunings instructions
1. Maschine has to be cold (<40 C): Adjust coldstart step 1 and step 2 to cleanly reach setPoint without any PID controls.
   - tbd
1. After this is configured correctly, you have to determine your steadyPower value. 
   - tbd
1. After this is configured correctly, you should configure steadyPowerOffset and steadyPowerOffsetTime.
   - tbd (maschine has to be cold)
1. After this is configured correctly, you should configure the PID values for inner-zone and outer-zone.
   - tbd


# Changelog
- 2.0.1_beta:
  - ATTENTION: EEPROM has changed. Therefore you have to connect to blynk at least once, and manually set correct settings in blynk app (see screenshots for default values).
  - New PID Controller "Multi-state PID with steadyPower (Bias)"
  - "Brew Ready" Detection implemented, which detects when the temperature has stabilized at setPoint. It can send an
    MQTT event or have hardware pin 15 triggered (which can be used to turn a LED on).
  - Because deadtime of Silvia5E is around 45seconds PID.compute() runs every 5 seconds (previous 1sec) and reduce header on/off switches by factor 5
  - Temperature polling is also now set to once every second (previous 400ms)
  - Code refactoring/cleanup and fixes of bugs.
  - BurstShot feature added to temporary overwrite PID controls (useful mainly for tests)
  - Added RemoteDebug over telnet so that we dont need USB to debug/tune pid anymore (https://github.com/JoaoLopesF/RemoteDebug)
   - Just "$ telnet <rancilio ip> 23"
  - Fix: Recover EmergencyStop when temperature poll in setup() fails
  - EmergencyStop state logging improved
- 1.9.8h_alpha:
  - Feature: Implemented special 3 step cold-start mechanism. See special tuning instructions above.
- 1.9.8g:
  - Improvement: Broken temperature is detected when temp has increased more than >5 degrees (previous 25 degrees) in the last 0.4 seconds.
  - Improvement: Better brewReady detection by waiting for stable temperature within a longer time window (from 6 to 14secs).
  - TemperatureHistory increased from 6 seconds to 30seconds.
- 1.9.8f:
  - Added missing config.h.SAMPLE variables.
  - Added some more mqtt events.
- 1.9.8e:
  - Removed movAvg() due to several issues:
    - This is no moving avg, but something different (but it is working for "stable" temperature curves).
    - readIndex=0 is not used.
    - movingAvg does not compares currentValue with previousValue.
    - Faktor *100 makes no sense (?).
    - initializing methods/firstreading is refactored.
  - movAvg() is replaced by pastTemperatureChange() (+ updateTemperatureHistory())
  - Change order of brew-detection in main loop() to not confuse bPid.compute() due to flapping setTunings()
  - Feature: isBrewReady() to determine if temperature is stable to start brewing
  - Bugfix: Replace abs() with fabs().
- 1.9.8d:
- 1.9.8c:
  - Add more mqtt updates: kp,ki,kd, coldstart
  - Safe-guard: be sure pid internals are cleared when pid is dis/enabled.
- 1.9.8b:
  - Feature: Add support for MQTT.
  - Safeguard: HeaterPreventFlapping must never be > windowSize.
- 1.9.7:
  - Fix: If temperature sensor detects more than 150Celcius then it must be an error.
  - Fix: Typos / explanations.
  - Fix: Race condition when code is blocked for some time
  - Fix: Added initial temp poll to prevent error on start.
  - Feature: Detect missing temperature polls.
  - Feature: Add configurable BLYNKPORT.
  - Feature: Wifi hostname configuable.
  - New feature: Safe-guard: Stop heating forever if sensor is flapping!
  - Fix: Pid.Compute() is now in sync with isrCounter and is not loosing one tick each second. Heater flapping is reduced.

# Disclaimer
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
