# ranciliopid
Rancilio-Silvia PID für Arduino http://rancilio-pid.de

BETA VERSION

Version 2.0.1_alpha

# Additional important information
- Copy file userConfig.h.SAMPLE to userConfig.h and edit this file accordingly.
- Additional Arduino dependency on PubSubClient (tested with Version 2.7.0). 
  Please install this lib by using Arduino->Sketch->Include Library->"Library Manager".
  ![Library Manager](https://raw.githubusercontent.com/medlor/ranciliopid/add-mqtt-support/PubSubClient_Dep.jpg)

# Tunings instructions
- 2 Step Coldstart:  
  Set STARTTEMP variable to around 80 degree celcius and cold start the maschine. Wait until the heater goes from 100% to around 5%. Wait an additional 1min (until when the heater changing output). Adapt STARTTEMP to match the currently missing temperature from the setpoint. 
- All other tuning variables are currently yet hard-coded (mainly outputSum has to match the minimum heater power required to have a stable temperature (for my silviaE it is around 4.9%)).

# Changelog
- 2.0.1_alpha:
  - New PID Controller "Velocity Type C" (Attention: does not work out. Last commit to save stuff)
  - Added RemoteDebug over telnet (https://github.com/JoaoLopesF/RemoteDebug)
  - Recover EmergencyStop when temperature poll in setup() fails
  - EmergencyStop state logging improved
  - getAverageTemperature() is used to steady Input temperatures and therefore improve PID controls
  - Because deadtime of Silvia5E is around 45seconds we can set PID.compute() to run every 5 seconds (previous 1sec) and reduce header on/off switches by factor 5.
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
