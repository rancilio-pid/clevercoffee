/********************************************************
   Analog Input
******************************************************/

#if PINBREWSWITCH == 0
   const int analogPin = 0; // AI0 will be used 
#endif

/********************************************************
   Normal BREW
******************************************************/
int brewcounter = 10;
int brewswitch = 0;
int brewswitchTrigger = 0;
unsigned long brewswitchTriggermillis = 0;
int brewswitchTriggerCase = 10; 
boolean brewswitchWasOFF = false;
double brewtime = 25000;  //brewtime in ms
double totalbrewtime = 0; //total brewtime set in softare or blynk
double preinfusion = 2000;  //preinfusion time in ms
double preinfusionpause = 5000;   //preinfusion pause time in ms
double bezugsZeit = 0;   //total brewed time
double lastbezugszeitMillis = 0; // for shottimer delay after disarmed button
double lastbezugszeit = 0 ; 
unsigned long startZeit = 0;    //start time of brew
const unsigned long analogreadingtimeinterval = 10 ; // ms
unsigned long previousMillistempanalogreading ; // ms for analogreading

/********************************************************
   SHOTTIMER WITH SCALE OR BREWMODE 2 (SCALE)
******************************************************/

#if (ONLYPIDSCALE == 1 || BREWMODE == 2) 
float weightSetpoint = WEIGHTSETPOINT;
int shottimercounter = 10 ; 
float calibrationValue = 3195.83; // use calibration example to get value
float weight = 0;   // value from HX711
float weightPreBrew = 0;  // value of scale before wrew started
float weightBrew = 0;  // weight value of brew
float scaleDelayValue = 2.5;  //value in gramm that takes still flows onto the scale after brew is stopped
bool scaleFailure = false;
const unsigned long intervalWeight = 200;   // weight scale
unsigned long previousMillisScale;  // initialisation at the end of init()
HX711_ADC LoadCell(HXDATPIN, HXCLKPIN);
#endif