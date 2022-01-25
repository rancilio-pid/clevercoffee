/*
 * Default values for parameters
 */

// PID - offline values
#define SETPOINT           95      // Temperatur setpoint
#define STEAMSETPOINT      120     // Temperatur setpoint
#define BREWDETECTIONLIMIT 150     // brew detection limit, be carefull: if too low, then there is the risk of wrong brew detection and rising temperature

#define AGGKP 69                   // Kp normal
#define AGGTN 399                  // Tn
#define AGGTV 0                    // Tv

// PID coldstart
#define STARTKP 50                 // Start Kp during coldstart
#define STARTTN 150                // Start Tn during cold start

// PID - offline brewdetection values
#define AGGBKP 50                  // Kp
#define AGGBTN 0                   // Tn
#define AGGBTV 20                  // Tv

// Backflush values
#define FILLTIME       3000        // time in ms the pump is running
#define FLUSHTIME      6000        // time in ms the 3-way valve is open -> backflush
#define MAXFLUSHCYCLES 5           // number of cycles the backflush should run, 0 = disabled

//Weight SCALE
#define WEIGHTSETPOINT 30          // Gramm

//Pressure sensor
/*
 * messure and verify "offset" value, should be 10% of ADC bit reading @supply volate (3.3V)
 * same goes for "fullScale", should be 90%
 */
#define OFFSET      102            // 10% of ADC input @3.3V supply = 102
#define FULLSCALE   922            // 90% of ADC input @3.3V supply = 922
#define MAXPRESSURE 200

// ToF waterlevel sensor
#define WATER_FULL 102             // value for full water tank (=100%) obtained in calibration procedure (in mm); can also be set in Blynk
#define WATER_EMPTY 205            // value for empty water tank (=0%) obtained in calibration procedure (in mm); can also be set in Blynk

// E-Trigger
#define ETRIGGERTIME 600            // seconds, time between trigger signal

// Display
#define BREWSWITCHDELAY 3000       // time in ms

// WiFi
#define MAXWIFIRECONNECTS 5        // maximum number of reconnection attempts, use -1 to deactivate
#define WIFICINNECTIONDELAY 10000  // delay between reconnects in ms

// InfluxDb
#define INTERVALINFLUX 5000

