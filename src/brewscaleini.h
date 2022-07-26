/**
 * @file brewscaleini.h
 *
 * @brief
 */

// Analog Input
#if PINBREWSWITCH == 0
    const int analogPin = 0; // AI0 will be used
#endif

// Normal Brew
int brewcounter = 10;
int brewswitch = 0;
int brewswitchTrigger = LOW;
int buttonStateBrewTrigger;                     // the current reading from the input pin
unsigned long lastDebounceTimeBrewTrigger = 0;  // the last time the output pin was toggled
unsigned long debounceDelayBrewTrigger = 50;
unsigned long brewswitchTriggermillis = 0;
int brewswitchTriggerCase = 10;
boolean brewswitchWasOFF = false;
double brewtime = BREW_TIME;                        // brewtime in s
double totalbrewtime = 0;                           // total brewtime set in software or blynk
double preinfusion = PRE_INFUSION_TIME;             // preinfusion time in s
double preinfusionpause = PRE_INFUSION_PAUSE_TIME;  // preinfusion pause time in s
double timeBrewed = 0;                              // total brewed time
double lastbrewTimeMillis = 0;                      // for shottimer delay after disarmed button
double lastbrewTime = 0 ;
unsigned long startingTime = 0;                     // start time of brew
const unsigned long analogreadingtimeinterval = 10; // ms
unsigned long previousMillistempanalogreading;      // ms for analogreading
double weightSetpoint = SCALE_WEIGHTSETPOINT;

// Shot timer with or without scale
#if (ONLYPIDSCALE == 1 || BREWMODE == 2)
    int shottimercounter = 10 ;
    float calibrationValue = SCALE_CALIBRATION_FACTOR;  // use calibration example to get value
    float weight = 0;                                   // value from HX711
    float weightPreBrew = 0;                            // value of scale before wrew started
    float weightBrew = 0;                               // weight value of brew
    float scaleDelayValue = 2.5;                        // value in gramm that takes still flows onto the scale after brew is stopped
    bool scaleFailure = false;
    const unsigned long intervalWeight = 200;           // weight scale
    unsigned long previousMillisScale;                  // initialisation at the end of init()
    HX711_ADC LoadCell(HXDATPIN, HXCLKPIN);
#endif
