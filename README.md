# ranciliopid - Open source PID for your espresso maschine

BLEEDING EDGE MASTER VERSION 

Version 2.3.0 beta

based on the Rancilio-Silvia PID for Arduino described at http://rancilio-pid.de

# Most important features compared to rancilio-pid master:
1. New PID Controller "Multi-state PID with steadyPower (Bias)"
   - Distinct PID settings dependend on the current "state" of the maschine. 
   - Most of the settings are either static or semi-automatically tuned, which does not require an PHD (German: Diplom) to understand.
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
1. Freely choose if you want the software use WIFI, BLYNK and MQTT. Everythink can be enabled/disabled and stil have a flawlessly working PID controller.
1. Offline Modus is fixed and enhanced. If userConfig.h's FORCE_OFFLINE is enabled, then PID fully is working without networking. Ideal in situations when there is no connectivity or you dont want to rely on it.
1. Huge performance tunings and improvements under the hood which stabilizes the system (eg in situations of bad WIFI, hardware issues,..).
1. MQTT support to integrate maschine in smart-home solutions and to easier extract details for graphing/alerting.
1. Added RemoteDebug over telnet so that we dont need USB to debug/tune pid anymore (https://github.com/JoaoLopesF/RemoteDebug). While using OTA updates you can remotely debug and update the software!
1. "Brew Ready" Detection implemented, which detects when the temperature has stabilized at setPoint. It can send an
   MQTT event or have hardware pin 15 triggered (which can be used to turn a LED on).
1. All heater power relevant settings are now set and given in percent (and not absolute output) and therefore better to understand
1. Support for customizable item collections to beautify display output (thanks to helge!)
1. Safetly toogle added to shutdown heater on sensor malfunction (TEMPSENSORRECOVERY)
1. Many useful functions to be used internally getAverageTemperature(), pastTemperatureChange() + updateTemperatureHistory())

# ATTENTION:
- This software is tested thoroughly with the pid-only hardware solution on Silvia 5e, and with a permanently run full-hardware solution on an 10 year old Silvia. Also a 10 year old Gaggia Classic is tested successfully. I am grateful for any further feedback. 
- Please monitor our maschine's temperature closely the first few run times. The muti-state pid controller should never lead to temperatures greater than 5 degress above setpoint!

# Instructions on how to migrate from official rancilio to bleeding-edge
Installation is as explained on http://rancilio-pid.de/ but with following adapations:
1. Make screenshots of the official "Blynk App Dashboard" so that you can revert anytime.
1. Copy file userConfig.h.SAMPLE to userConfig.h and edit this file accordingly.
1. (Optional) Enable blynk in userConfig and build the "Blynk App Dashboard" as described below.
   OR just disable blynk in userConfig, enable debug logs and use one of the methods described in "Debugging Howto" to monitor the first few runs.
1. Flash and enjoy your espresso.
1. No tuning should be required normally. If you want/need to then use the method described below.

# Blynk App Dashboard
Unfortunately you have to manually build your dashboard (config does not fit in QR code).
Please stick to the following screenshots and use the "virtual pin mapping" as described:

## Blynk application screenshots
<p align="center">
<img src="https://github.com/medlor/ranciliopid/blob/master/pictures/blynk-app/blynk_app_status.jpg" height="500">
<img src="https://github.com/medlor/ranciliopid/blob/master/pictures/blynk-app/blynk_app_controller.jpg" height="500">
<img src="https://github.com/medlor/ranciliopid/blob/master/pictures/blynk-app/blynk_app_controller2.jpg" height="500">
<img src="https://github.com/medlor/ranciliopid/blob/master/pictures/blynk-app/blynk_app_preinfusion.jpg" height="500">
</p>

## Blynk Pin Mapping
- Tab "Status":  
  On/OFF (Type: Styled Button) := V13  
  EspressoReady (Type: Led) := V14  
  TargetTemp (Type: Numeric Input) := V7  
  CurrentTemp (Type: Labeled Value) := V2  
  HeaterPower := V23  
  Temperature Error := V11  
  Temperature Change (last 10 sec) := V35  
  CurrentTemp/TargetTemp (Type: SuperChart) := V2/V7  
  HeaterPower := V23  
  Water Temp changes :=V35  

- Tab "PID Controller":  
  Interzone P := V4  
  Interzone I := V5  
  Interzone D := V6  
  Outerzone P := V30  
  Outerzone I := V31  
  Outerzone D := V32  
  BrewPower   := V36  
  SteadyPower := V41  
  SteadyPower Offset Time := V43  
  SteadyPower Offset Power := V42  
  StartTemp := V12  
  Brew Detection Temperaturdrop := V34  
  BurstShot (Type: Button) := V40  
  BurstPower := V44  

- Tab "Preinfusion":  
  Brew Time := V8  
  Preinfusion Time := V9  
  Preinfusion Pause := V10  

## Alternative way to clone blynk app dashboard (experimental)
- Login to your server on which blynk is installed
- install the programm jq (should be included in any distribution)
- Shutdown blynk Service
- Search for file in blynk/data which looks like <email>.Blynk.user. This is the config file. Make a backup of this file.
- Download this project's config [rancilio_v2.json](https://github.com/medlor/ranciliopid/blob/master/blynk/rancilio_v2.json) and put it in the same folder as the config file.
- Execute following command in this folder:
  ```
  jq --argjson blynkInfo "$(<rancilio_v2.json)" '.profile.dashBoards += [$blynkInfo]' YOUR_EMAIL.Blynk.user
  ```
- Startup blynk service
- Open App, Search newly create project, Open the "Project Settings" then devices-> Master -> Master and Press "Refresh Token". Use this token in the rancilio software as auth token.

# Tunings instructions
```
This step is optional: To my knowledge no tuning is required because all important settings are auto-tuned after a few runs (<10) and test shows that the PID then produces steady < 0.1 degree variance to the setpoint within 600sec of power-up (independent of the espresso hardware).
```
1. Enable debug mode and have a look at the logs
1. Validate STARTTEMP is correctly set:
   - As long as there are "Auto-Tune starttemp .*" logs the auto-tuning is still calibrating. Dont be overly concerned for changes <=0.3 degree. This might be totally normal because of climate changes in the room.
   - The auto-tuning goal is to adapt "StartTemp" that upon reaching "state 3" the temperature is at most 0.5 Celcius below setPoint. 
     - Logline when state 3 is reached: "\*\* End of stabilizing. Transition to step 3 (normal mode)".
   - Loglines like "StartTemp" too high: "Disabled steadyPowerOffset because its too large or starttemp too high" are a bad sign and should never occur after tuning, because this will delay reaching state3 for up to 2 additonal minutes.
   - Loglines like "brewReady (stable last 60 secs. Tuning took XX secs)" tell you how many seconds it took to reach the setPoint.
1. After previous step is completed, validate steadyPowerOffset and steadyPowerOffsetTime.
   - When the maschine's metal is cold additional heat is lost. By configuring an additional power to the heater (steadyPowerOffset in percent) for steadyPowerOffsetTime seconds (counting from power-on) you can compensate this.
   - When "Attention: steadyPowerOffset is probably too high (%0.2f -= %0.2f) (moving up at setPoint)" is reported even after many runs (this garantees auto-tuning is stable), then steadyPowerOffset probably should be decreased in userConfig.h by 0.1 Celcius.
   - When the maschine is cold started, the previous steps are correctly configured and after reaching "state 3" the temperature is still falling steadily over a longer time (>30seconds), you probably should increase steadyPowerOffset by 0.1 Celcius steps.
1. All other values should not be changed. If you you want to improve calibration logic the debug log is very helpful. You can also ask me for assistance.

# Debugging Howto
1. Enable debugging
   - Open your userConfig.h file and uncomment following line:
   ```
   #define DEBUGMODE
   ```
1. Getting debug logs 
   - on windows
     - Download [putty](https://the.earth.li/~sgtatham/putty/latest/w64/putty.exe) for windows or use telnet in linux to connect to port 23 of the rancilio-maschine's IP-address: 
       <p align="center">
       <img src="https://github.com/medlor/ranciliopid/blob/master/pictures/putty/putty.jpg" height="300">
       </p>
   - on linux
     ```
     sh> $ telnet rancilio-ip-address 23
     ```
   - on andriod, following apps are recommended
     - [Fing](https://play.google.com/store/apps/details?id=com.overlook.android.fing)
     - [JuiceSSH](https://play.google.com/store/apps/details?id=com.sonelli.juicessh)
     - Just open Fing, search for your "rancilio device" as defined in userConfig.h's HOSTNAME, then press on the device -> "Offene Ports finden" -> "Porr 23" -> "Verbinden mit Telnet-Client"
     - To export the protocol just long press on one of the log-lines and choose "save/share".
   - with Web-Browser
     - Open the file ```rancilio-pid\src\RemoteDebugApp\index.html``` with firefox/chrome and enter your rancilio-ip in the upper left field.
1. Explanation of the PID log line
   ```
   [0m(D p:^5000ms) 435 Input= 93.46 | error= 0.54 delta= 2.45 | Output= 27.88 = b:52.10 + p: 0.86 + i: 0.00( 0.00) + d:-25.09
   ```
   - 435 := Time since power-on of the arduino (in seconds)
   - Input= 93.46 := Current temperature
   - error= 0.54  := Temperature difference calculated by (target_temp - current_temp)
   - delta= 2.45  := Change in temperature in the last 10 seconds
   - Output= 27.88 := Heater Power (in percent)  
     which is calcucated by the sum of:
     - b: 5.10        := steadyPower (in percent)
     - p: 0.86        := PID Kp (in percent)
     - i: 0.00( 0.00) := PID KiSum (KiLast) (in percent)
     - d:-25.09       := PID Kd (in percent)
1. If you need help or have questions, just send me the logs in the [rancilio-pid chat](https://chat.rancilio-pid.de/).

# How to use a simple LED as brewReady signal
- The easiest way is to use arduino GPIO15. For this to work you have to connect a resistor (R3) and in parallel another resistor (R7) with the led (LED2) in series. This has to be connected between GPIO Pin 15 and GROUND as seen in [Schematic](https://www.forward.com.au/pfod/ESP8266/GPIOpins/index.html (R3, R7, LED2).
- Required configuration in config.h:
  - #define BREW_READY_LED 1
  - #define BREW_READY_DETECTION 0.2  # or any other value
  - <p align="center">
    <img src="https://github.com/medlor/ranciliopid/blob/master/pictures/hardware-led/rancilio-brewReadyLed.jpg" height="300">
    </p>

# Instructions on how to create new icon collections
1. Clone the file rancilio-pid\icon_simple.h to a custom filename, eg icon_myown.h .
2. Use gimp of any other "paint" program to create 45x45 pixel black/white .xbm files.
3. Open created .xbm files with any word-editor and paste the hex-codes into the specfic section in icon_myown.h . Following sections/pictures are supported:
   - coldstart
   - outer_zone (>1 degree off from setpoint)
   - brew_acceptable (<0.3 degree off from setpoint)
   - brew_ready (<0.3 degree off from setpoint for at least 40seconds)
   - brewing (brew is detected)
   - steam (steam is ready/in-progress)
4. Change the define ICON_COLLECTION in userConfig.h to 1.
5. Overwrite icon_smiley.h with our created file icon_myown.h .
6. Send me your icon collection, so that I can add it permanently in future versions.
Existing collections are shown here: 
- [simple](https://github.com/medlor/ranciliopid/blob/master/pictures/icons/bleeding_edge_rancilio_pid_simple_2.2.0.mp4)
- [smiley](https://github.com/medlor/ranciliopid/blob/master/pictures/icons/bleeding_edge_rancilio_pid_smiley_2.2.0.mp4)

# Instructions on how to update to the latest version of bleeding-edge
1. Just overwrite all existing files with a newly released version.
2. Open your userConfig.h file, which had not been overwritten in previous step, and manually check (line by line!) that all updates to the new file userConfig.h.SAMPLE are reflected in your own userConfig.h. 
3. Compile, upload and enjoy!

# Changelog
- 2.3.0_beta_3:
  - ATTENTION: New default values in userConfig.h. It is recommended to intially use default values for STEADYPOWER, STEADYPOWER_OFFSET_TIME, STEADYPOWER_OFFSET, STARTTEMP, BREWDETECTION_POWER, BREWDETECTION_SENSITIVITY. Additionally BREWTIME is from now on also used in ONLYPID=1.
  - Improve PID:
    - steadyPowerOffset is gradually decreased over time to better compensate warmup of maschine.
    - steadyPowerOffset is taken into consideration of pid calculation.
    - Once-only output manipulation added when temp is stable but not at setpoint.
    - PID's I parameter filter is reduced in certain situations more strictly.
  - Brew Detection optimized and (for ONLYPID=1) additionally uses userConfig.h's BREWTIME as maximum brew time.
  - When connecting by telnet/blynk the hardware- and software configuration is printed to debug log.
  - Fix: Heater overextending handling working as intended.
  - GPIO Pin Mapping moved to userConfig.h. Therefore you have to adapt your userConfig.h file.
- 2.3.0_beta_2:
  - PID State 2 (stabilize coldstart) also adds steadyPowerOffset.
  - Trigger brewReady when temperature is stable for 60sec (prev: 40s).
  - Add logs for brewReadyStatistic.
  - Fix: burstShot working again.
- 2.3.0_beta_1:
  - Auto-tuning for starttemp is implemented. No need to adapt the STARTTEMP accordingly when SETPOINT is modified.
  - Fix: If PID is manually disabled, heater utilization is correctly reported as 0%.
- 2.2.0_master:
  - Display functionality improved:
    - Replaced display lib Adafruit_SSD1306.h with U8G2. Direct support for SH1106_128X64 and SSD1306_128X64 via userConfig.
    - Completly new display widgets which show informations according to active PID state.
    - Support for icon collections to easily customize/share display widgets. Icon collection "simple" and "smiley" included.
    - Support for simple icon animations.
    - Service Status Icons are displayed if enabled in userConfig.
    - New DEFINES in userConfig.h. Update your config.
  - Improvement: Blynk On/Off button re-inits PID state similar to when machine power is turned on.
  - Fix: If DISABLE_SERVICES_ON_STARTUP_ERRORS=1 then WIFI reconnect attempts are also prevented.
  - Remove define EMERGENCY_TEXT.
  - Fix: PID State "brewing" is correctly detected when ONLYPID=0. (Thanks Helge)
- 2.1.0_master:
  - Networking:
    - Huge improvements in handling unstable WIFI networks and mqtt/blynk service unavailabilities.
    - You can disable/enable WIFI, MQTT or Blynk in userConfig.h and stil have a flawlessly working PID controller. Blynk is no longer an hard requirement!
    - Offline Modus is fixed and enhanced. If userConfig.h's FORCE_OFFLINE is enabled, then PID fully is working without networking. Ideal in situations when there is no connectivity or you dont want to rely on it.
    - Fix of EEPROM functionality: PID settings are correctly saved in EERPOM and correctly used if there are WIFI issues.
    - Instead of using dynamic IPs (over DHCPd) you have the option to set a static IP.
    - If blynk or mqtt is not working during startup, do not retry the connection periodically (configurable by userconfig.h DISABLE_SERVICES_ON_STARTUP_ERRORS)
  - New PID Variable "BREWDETECTION_POWER" introduced which defines the heater power during brewing.
  - Complete rewrite of "TSIC sensor read" based on the excellent ISR code by Adrian. (Thanks Adrian!)
    - Optimized "TSIC sensor read" to further increase performance.
    - Currently it takes <4ms to collect sensor data instead of the previous 78ms).
    - Fix: "sensor errors" do not occur anymore.
  - Performance/Stability:
    - Some system libs are optimized in performance and stability (src/ folder).
    - Remove all unneeded external libraries which are installed in system's arduino search path.
    - Code Tunings all over the place to increase performance and therefor stability.
    - Overall stability better by honoring critical processes.
  - Debuglogs can also be accessed via browser (see documentation).
  - PID calculations are moved from ISR to loop(). This improves stability even further.
  - Fix: Brew detection optimized.
  - Safetly feature: Done start brewing if the brew-button is switched "on" on startup
  - Fix: After power on, the 5 second wait time until heater starts is removed.
  - Library path adapted to support Arduino under Linux.
- 2.0.3_beta2:
  - Wifi disconnects handled better.
  - Implement blynk reconnect exponential backoff.
  - Set blynk reconnection timeout (3sec).
  - Added debug messages in ciritical paths.
- 2.0.2_master:
  - stable release
  - updated docs
- 2.0.2_beta2:
  - removed 8x8 display support
  - Code cleanup
- 2.0.2_beta1:
  - Major improvements in display related stuff (eg new icons,..) (thanks helge)
  - Restructure folders (thanks helge)
- 2.0.1_beta8:
  - ISR performance optimised when debug is active (spend time reduced from 0.8ms to 0.15ms).
  - Move DEBUGMODE to config.h.
- 2.0.1_beta7:
  - Emergency Logo can be replaced by a nice "milk steam" logo (EMERGENCY_ICON). (Thanks helge for the icon)
  - Regular status display is beautified and adapted to normal users needs. (Thanks helge for the code)
- 2.0.1_beta6:
  - Fix: Reducing "temperature sensor reading" errors.
  - Fix: steadyPowerOffset_Time is now correctly configurable via blynk.
  - Fix: Hardware switch sensitivity ("brewswitch") is increased from 1000 to 700.
  - Improved: mySteadyPower failure detections.
  - Code Cleanup + Refactorings.
- 2.0.1_beta5:
  - RemoteDebug inactivity time requires a version not yet in arduino library manager. Therefore a workaround is document in the meantime.
  - Pre-Infusion variables are now configurable in config.h: PREINFUSION, PREINFUSION_PAUSE, BREWTIME
  - Emergency temperature theshold is now configurable in config.h: EMERGENCY_TEMP
  - BrewDetection sensitivity is now configurable in config.h: BREWDETECTION_SENSITIVITY (renamed from brewboarder)
- 2.0.1_beta4:
  - Limit auto-tuning of steadyPower to reduce overly increased values.
  - Fix bug in brew().bezugsZeit calculation.
  - Renamed outputK to outputI.
  - Explanation of PID log line added.
- 2.0.1_beta3:
  - Pre-Infusion Values are now in seconds (previously milliseconds).  
    !! ATTENTION: Therefore you HAVE to change following values via blynk: brewtime, preinfusion, preinfusionpause !!
  - Reduce overhead of preinfusion functionality by having it run just every 50ms (and not every ms).
  - Add support for custom emergency text in display when temp >120 degree. (see EMERGENCY_TEXT in config.h.SAMPLE)
  - Emergency temperature threshold can now be set by variable emergency_temperature.
  - RemoteDebug inactivity time increased from 10min to 30min.
  - Add missing line-break in brew() debug logging.
- 2.0.1_beta2:
  - Better Installation Guide
  - Fix brew() function
  - Support virtual BrewReady LED in blynk
  - Improve pastChange() in Pid.compute(). (default Kd value has to be doubled)
  - Improve Auto-tuning.
  - Default PID Value Tunings
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
    - Just "$ telnet rancilio_ip 23"
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


# Special Thanks
To the great work of the rancilio-pid.de team, just to mention a few: andreas, markus, toppo78, miau.  
Also to the nice people in the rancilio chat and the ones who contribute and give very much appreciated feedback like helge and Adrian!  
  
!! Thank you so much for the tasty cup of coffee I enjoy each day !!  


# Disclaimer
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
