
/********************************************************
   Version 1.6.0 BETA (06.05.2019)
  Key facts: BETA VERSION WITH FALLBACK
  - Check the PIN Ports in the CODE!
  - Find your changerate of the machine, can be wrong, test it!
******************************************************/
#include "Arduino.h"
#include <EEPROM.h>
unsigned long previousMillisColdstart = 0;
unsigned long previousMillisColdstartPause = 0;
unsigned long ColdstartPause = 0;
unsigned long KaltstartPause = 0;
unsigned long bruehvorganggestartet = 0;
unsigned long warmstart = 0;
unsigned long previousMillisSwing = 0;
unsigned long wifistarttime = 0;
double Onoff = 1 ; // default 1
int status ;   // the Wifi radio's status
String displaymessagetext ;
String displaymessagetext2 ;

/********************************************************
   Vorab-Konfig
******************************************************/
int Offlinemodus = 0;       // 0=Blynk und WLAN wird benötigt 1=OfflineModus (ACHTUNG EINSTELLUNGEN NUR DIREKT IM CODE MÖGLICH)
int debugmodus = 0;         // 0=Keine Seriellen Debug Werte 1=SeriellenDebug aktiv
int Display = 2;            // 1=U8x8libm, 0=Deaktiviert, 2=Externes 128x64 Display
int OnlyPID = 1;            // 1=Nur PID ohne Preinfussion, 0=PID + Preinfussion
int TempSensor = 2;         // 1=DS19B20; 2=TSIC306
int Brewdetection = 1 ;     // 0=off ,1=Software
int standby = 0 ;           // 0: Old rancilio not needed, 1: new one , E or V5 with standy, not used in the moment
int fallback = 1  ;          // 1: fallback auf eeprom Werte, wenn blynk nicht geht


char auth[] = "blynkauthcode";
char ssid[] = "wlanname";
char pass[] = "wlanpass";
char blynkaddress[]  = "blynk.remoteapp.de" ;
// char blynkaddress[]  = "raspberrypi.local" ;
/********************************************************
   moving average - Brüherkennung
*****************************************************/
const int numReadings = 15;           // number of values per Array
float readingstemp[numReadings];       // the readings from Temp
float readingstime[numReadings];       // the readings from time
float readingchangerate[numReadings];

int readIndex = 1;             // the index of the current reading
double total = 0;              // the running
int totaltime = 0 ;            // the running time
double heatrateaverage = 0;    // the average over the numReadings
double changerate = 0;         // local change rate of temprature
double heatrateaveragemin = 0 ;
unsigned long  timeBrewdetection = 0 ;
int timerBrewdetection = 0 ;
int i = 0;
int firstreading = 1 ; // Ini of the field

double aggbp = 80 ;
double aggbi = 0 ;
double aggbd = 800;
double brewtimersoftware = 45; // 20-5 for detection
double brewboarder = 150 ; // border for the detection,
// be carefull: to low: risk of wrong brew detection
// and rising temperature

/********************************************************
   BLYNK
******************************************************/
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>

//Zeitserver
#include <TimeLib.h>
#include <WidgetRTC.h>
BlynkTimer timer;
WidgetRTC rtc;
WidgetBridge bridge1(V1);

//Update Intervall zur App
unsigned long previousMillis = 0;
const long interval = 5000;

//Update für Display
unsigned long previousMillisDisplay = 0;
const long intervalDisplay = 500;

/********************************************************
   Analog Schalter Read
******************************************************/
int analogPin = 0; // will be use in case of hardware
int brewcounter = 0;
int brewswitch = 0;


int brewtime = 25000;
long aktuelleZeit = 0;
int totalbrewtime = 0;
int preinfusion = 2000;
int preinfusionpause = 5000;

#define pinRelayVentil    12
#define pinRelayPumpe     13


/********************************************************
   Onoffmachine relay
******************************************************/

#define pinRelaymaschineonoff  99 // Your Pin for the relay
int Onoffmachine = 0 ;
long windowStartOnoff = 0 ;


/********************************************************
   DISPLAY
******************************************************/
const int numReadingsdisplayerror = 30;           // number of values per Array
float readingsdisplayerror[numReadingsdisplayerror];
int error = 0;
int displayerrorcounter = 0 ;
#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 5, /* data=*/ 4, /* reset=*/ 16);   //Display initalisieren

// Display 128x64
#include <Wire.h>
#include <Adafruit_GFX.h>
//#include <ACROBOTIC_SSD1306.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 16
Adafruit_SSD1306 display(OLED_RESET);
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


/********************************************************
   PID
******************************************************/
#include "PID_v1.h"
#define pinRelayHeater    14
long previousMillistemp = 0;
long currentMillistemp = 0;
long intervaltempmestsic = 200 ;
long intervaltempmesds18b20 = 400  ;

unsigned int windowSize = 1000;
unsigned long windowStartTime;
double acceleration = 1;
double Input, Output, setPointTemp, Coldstart;

double setPoint = 95;
float aggKp = 28.0 / acceleration;
float aggKi = 0.08 / acceleration;
double aggKd = 0 / acceleration;
double startKp = 60;
double starttemp = 85;


PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, DIRECT);

/********************************************************
   DALLAS TEMP
******************************************************/
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress sensorDeviceAddress;

/********************************************************
   B+B Sensors TSIC 306
******************************************************/
#include "TSIC.h"       // include the library
TSIC Sensor1(2);    // only Signalpin, VCCpin unused by default
uint16_t temperature = 0;
float Temperatur_C = 0;

/********************************************************
   BLYNK WERTE EINLESEN
******************************************************/


BLYNK_CONNECTED() {
  if (Offlinemodus == 0) {
    Blynk.syncAll();
    rtc.begin();
  }

}

BLYNK_WRITE(V4) {
  aggKp = param.asDouble();
}

BLYNK_WRITE(V5) {
  aggKi = param.asDouble();
}
BLYNK_WRITE(V6) {
  aggKd =  param.asDouble();
}

BLYNK_WRITE(V7) {
  setPoint = param.asDouble();
}

BLYNK_WRITE(V8) {
  brewtime = param.asDouble() * 1000;
}

BLYNK_WRITE(V9) {
  preinfusion = param.asDouble() * 1000;
}

BLYNK_WRITE(V10) {
  preinfusionpause = param.asDouble() * 1000;
}
BLYNK_WRITE(V11) {
  startKp = param.asDouble();
}
BLYNK_WRITE(V12) {
  starttemp = param.asDouble();
}
BLYNK_WRITE(V13)
{
  Onoff = param.asInt();
}
BLYNK_WRITE(V30)
{
  aggbp = param.asDouble();//
}

BLYNK_WRITE(V31) {
  aggbi = param.asDouble();
}
BLYNK_WRITE(V32) {
  aggbd =  param.asDouble();
}
BLYNK_WRITE(V33) {
  brewtimersoftware =  param.asDouble();
}
BLYNK_WRITE(V34) {
  brewboarder =  param.asDouble();
}

void displaymessage(String displaymessagetext, String displaymessagetext2, int Display) {
  if (Display == 2) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(displaymessagetext);
    display.setCursor(0, 8);
    display.print(displaymessagetext2);
  }
  if (Display == 1) {
    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    u8x8.clear();
    u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
    u8x8.setCursor(0, 0);
    u8x8.print(displaymessagetext);
    u8x8.setCursor(0, 2);
    u8x8.print(displaymessagetext2);
  }

}


void setup() {
  Serial.begin(115200);

  for (int thisReading = 0; thisReading <= numReadings; thisReading++) {
    readingsdisplayerror[thisReading] = 0 ;
  }

  if (Display == 1) {
    /********************************************************
      DISPLAY Intern
    ******************************************************/
    u8x8.begin();
    u8x8.setPowerSave(0);
  }
  if (Display == 2) {
    /********************************************************
      DISPLAY 128x64
    ******************************************************/
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)  for AZ Deliv. Display
    //display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.clearDisplay();
  }
  displaymessage("Version 1.6.0", "", Display);
  delay(5000);
  /********************************************************
    BrewKnopf SSR Relais
  ******************************************************/
  pinMode(pinRelayVentil, OUTPUT);
  pinMode(pinRelayPumpe, OUTPUT);
  digitalWrite(pinRelayVentil, HIGH);
  digitalWrite(pinRelayPumpe, HIGH);

  /********************************************************
     BLYNK & Fallback offline
  ******************************************************/
  if (Offlinemodus == 0) {

    if (fallback == 0) {
      displaymessage("Connect to Blynk", "no Fallback", Display);
      Blynk.begin(auth, ssid, pass, blynkaddress, 8080);

    }

    if (fallback == 1) {

      displaymessage("1: Try Wifi", "to connect", Display);
      // wait 10 seconds for connection:
      status = WiFi.begin(ssid, pass);
      delay (10 * 1000) ;

      if (WiFi.status() == WL_CONNECTED) {

        displaymessage("2: Wifi works, ", "try Blynk   ", Display);
        Serial.println("Wifi works, now try Blynk connection");
        delay(2000);
        Blynk.config(auth, blynkaddress, 8080) ;
        Blynk.connect(1500);

        // Blnky works:
        if (Blynk.connect() == true) {
          displaymessage("3: Blynk works", "", Display);
          delay(2000) ;
          Serial.println(WiFi.status());

          // Werte in den eeprom schreiben
          // ini eeprom mit begin
          EEPROM.begin(1024);
          Serial.println("Blynk is online, new values to eeprom");
          Blynk.syncAll();
          EEPROM.put(0, aggKp);
          EEPROM.put(10, aggKi);
          EEPROM.put(20, aggKd);
          EEPROM.put(30, setPoint);
          EEPROM.put(40, brewtime);
          EEPROM.put(50, preinfusion);
          EEPROM.put(60, preinfusionpause);
          EEPROM.put(70, startKp);
          EEPROM.put(80, starttemp);
          EEPROM.put(90, aggbp);
          EEPROM.put(100, aggbi);
          EEPROM.put(110, aggbd);
          EEPROM.put(120, brewtimersoftware);
          EEPROM.put(130, brewboarder);
          // eeprom schließen
          EEPROM.commit();
        }
      }
      if (WiFi.status() != WL_CONNECTED || Blynk.connect() != true) {
        displaymessage("Begin Fallback,", "No Blynk/Wifi", Display);
        delay(5000);
        Serial.println("Start offline mode with eeprom values, no wifi or blynk :(");
        Offlinemodus = 1 ;
        EEPROM.begin(1024);
        EEPROM.get(0, aggKp);
        EEPROM.get(10, aggKi);
        EEPROM.get(20, aggKd);
        EEPROM.get(30, setPoint);
        EEPROM.get(40, brewtime);
        EEPROM.get(50, preinfusion);
        EEPROM.get(60, preinfusionpause);
        EEPROM.get(70, startKp);
        EEPROM.get(80, starttemp);
        EEPROM.get(90, aggbp);
        EEPROM.get(100, aggbi);
        EEPROM.get(110, aggbd);
        EEPROM.get(120, brewtimersoftware);
        EEPROM.get(130, brewboarder);
      }
    }
    Serial.println(WiFi.localIP());
  }

  pinMode(pinRelayHeater, OUTPUT);

  windowStartTime = millis();
  Coldstart = 1;

  setPointTemp = setPoint;
  bPID.SetSampleTime(windowSize);
  bPID.SetOutputLimits(0, windowSize);
  bPID.SetMode(AUTOMATIC);

  /********************************************************
     TEMP SENSOR
  ******************************************************/
  if (TempSensor == 1) {
    sensors.begin();
    sensors.getAddress(sensorDeviceAddress, 0);
    sensors.setResolution(sensorDeviceAddress, 10) ;
    sensors.requestTemperatures();
  }

  /********************************************************
    movingaverage ini array
  ******************************************************/
  if (Brewdetection == 1) {
    for (int thisReading = 0; thisReading <= numReadings; thisReading++) {
      readingstemp[thisReading] = 0;
      readingstime[thisReading] = 0;
      readingchangerate[thisReading] = 0;
    }

  }
  /********************************************************
     Onoffmachine relay
  ******************************************************/
  if (standby == 1) {
    pinMode(pinRelaymaschineonoff, OUTPUT) ;
  }
}


void loop() {

  /********************************************************
    Temp. Request
  ******************************************************/
  unsigned long currentMillistemp = millis();
  if (TempSensor == 1) {
    if (currentMillistemp - previousMillistemp > intervaltempmesds18b20) {
      sensors.requestTemperatures();
      Input = sensors.getTempCByIndex(0);
      previousMillistemp = currentMillistemp;
      if (Brewdetection == 1 && Input > 0) {
        if (firstreading == 1) {
          for (int thisReading = 1; thisReading <= numReadings; thisReading++) {
            readingstemp[thisReading] = Input;
            readingstime[thisReading] = 0;
            readingchangerate[thisReading] = 0;
          }
          firstreading = 0 ;
        }
        readingstime[readIndex] = millis() ;
        readingstemp[readIndex] = Input ;
        if (readIndex == 15) {
          changerate = (readingstemp[15] - readingstemp[1]) / (readingstime[15] - readingstime[1]) * 10000;
        } else {
          changerate = (readingstemp[readIndex] - readingstemp[readIndex + 1]) / (readingstime[readIndex] - readingstime[readIndex + 1]) * 10000;
        }
        readingchangerate[readIndex] = changerate ;
        total = 0 ;
        for (i = 1; i < (numReadings); ++i)
        {
          total += readingchangerate[i];
        }
        heatrateaverage = total / numReadings * 100 ;
        if (heatrateaveragemin > heatrateaverage) {
          heatrateaveragemin = heatrateaverage ;
        }
//        Serial.print(heatrateaveragemin, 4);
//        Serial.print(",");
//        Serial.print(heatrateaverage, 4);
//        Serial.print(",");
        if (readIndex >= numReadings) {
          // ...wrap around to the beginning:
          readIndex = 0;
        }
        readIndex = readIndex + 1;
      }
    }

  }
  if (TempSensor == 2) {
    if (currentMillistemp - previousMillistemp > intervaltempmestsic) {
      temperature = 0;
      Sensor1.getTemperature(&temperature);
      Temperatur_C = Sensor1.calc_Celsius(&temperature);
      Input = Temperatur_C;
      //Input = 50 ;// test value
      previousMillistemp = currentMillistemp;
      // Serial.println(OnlyPID);
      // Logging Temp has to be sync with reading temp.
      if (Brewdetection == 1 && Input > 0) {
        if (firstreading == 1) {
          for (int thisReading = 1; thisReading <= numReadings; thisReading++) {
            readingstemp[thisReading] = Input;
            readingstime[thisReading] = 0;
            readingchangerate[thisReading] = 0;
          }
          firstreading = 0 ;
        }
        readingstime[readIndex] = millis() ;
        readingstemp[readIndex] = Input ;
        if (readIndex == 15) {
          changerate = (readingstemp[15] - readingstemp[1]) / (readingstime[15] - readingstime[1]) * 10000;
        } else {
          changerate = (readingstemp[readIndex] - readingstemp[readIndex + 1]) / (readingstime[readIndex] - readingstime[readIndex + 1]) * 10000;
        }
        readingchangerate[readIndex] = changerate ;
        total = 0 ;
        for (i = 1; i < (numReadings); ++i)
        {
          total += readingchangerate[i];
        }
        heatrateaverage = total / numReadings * 100 ;
        if (heatrateaveragemin > heatrateaverage) {
          heatrateaveragemin = heatrateaverage ;
        }
//        Serial.print(heatrateaveragemin, 4);
//        Serial.print(",");
//        Serial.print(heatrateaverage, 4);
//        Serial.print(",");
        if (readIndex >= numReadings) {
          // ...wrap around to the beginning:
          readIndex = 0;
        }
        readIndex = readIndex + 1;
      }
    }
  }
  // Input = random(998.0,1000.0);
  // Serial.print(currentMillistemp);
  // Serial.print(";");
  // Serial.print(previousMillistemp);
  // Serial.print(";");
  // Serial.print(Temperatur_C);
  // Serial.print(";");
  //  Serial.print(setPoint);
  //  Serial.print(",");
  //  Serial.print(Input);
  //  Serial.print(",");
  //Serial.println(Input);




  /********************************************************
    PreInfusion
  ******************************************************/

  brewswitch = analogRead(analogPin);

  unsigned long startZeit = millis();

  //  Serial.print(brewswitch);
  //  Serial.print(",");
  //  Serial.println(OnlyPID);
  if (OnlyPID == 0) {
    if (brewswitch > 1000 && startZeit - aktuelleZeit > totalbrewtime && brewcounter == 0) {
      aktuelleZeit = millis();
      brewcounter = brewcounter + 1;
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;
    //Serial.println(brewcounter);
    if (brewswitch > 1000 && startZeit - aktuelleZeit < totalbrewtime && brewcounter >= 1) {
      if (startZeit - aktuelleZeit < preinfusion) {
        // Serial.println("preinfusion");
        digitalWrite(pinRelayVentil, LOW);
        digitalWrite(pinRelayPumpe, LOW);
        //digitalWrite(pinRelayHeater, HIGH);
      }
      if (startZeit - aktuelleZeit > preinfusion && startZeit - aktuelleZeit < preinfusion + preinfusionpause) {
        //Serial.println("Pause");
        digitalWrite(pinRelayVentil, LOW);
        digitalWrite(pinRelayPumpe, HIGH);
        //digitalWrite(pinRelayHeater, HIGH);
      }
      if (startZeit - aktuelleZeit > preinfusion + preinfusionpause) {
        // Serial.println("Brew");
        digitalWrite(pinRelayVentil, LOW);
        digitalWrite(pinRelayPumpe, LOW);
        digitalWrite(pinRelayHeater, LOW);
      }
    } else {
      digitalWrite(pinRelayVentil, HIGH);
      digitalWrite(pinRelayPumpe, HIGH);
      //Serial.println("aus");
    }
    if (brewswitch < 1000 && brewcounter >= 1) {
      brewcounter = 0;
      aktuelleZeit = 0;
    }
  }
  /********************************************************
    sensor error
  ******************************************************/
  // number of values per Array
  if (Input < 0) {
    readingsdisplayerror[displayerrorcounter] = 1;
  }  else {
    readingsdisplayerror[displayerrorcounter] = 0;
  }
  error = 0;
  for (int i = 0; i < numReadingsdisplayerror; i++) {
    if (readingsdisplayerror[i] == 1) {
      error++;
    }
  }

//  Serial.print(error);
//  Serial.print(",");
//  Serial.print(Input);
//  Serial.print(",");
//  Serial.println(displayerrorcounter);
  displayerrorcounter++;
  if (displayerrorcounter == 16) {
    displayerrorcounter = 0 ;
  }
  //const int numReadingsdisplayerror = 15;           // number of values per Array
  //float readingsdisplayerror[numReadingsdisplayerror];
  //int error = 0;
  //int displayerrorcounter=0 ;
  /********************************************************
    change of rate
  ******************************************************/


  //Sicherheitsabfrage
  if (Input >= 0) {
    // Brew detecion == 1 software solution , ==2 hardware
    if (Brewdetection == 1 || Brewdetection == 2) {
      if (millis() - timeBrewdetection > 50 * 1000) {
        timerBrewdetection = 0 ;
      }
    }

    if (Brewdetection == 1) {
      if (heatrateaverage <= -brewboarder && timerBrewdetection == 0 ) {
        //   Serial.println("Brewdetected") ;
        timeBrewdetection = millis() ;
        timerBrewdetection = 1 ;
      }
    }
    if (Input < starttemp) {
      bPID.SetTunings(startKp, 0, 0);
    } else {
      bPID.SetTunings(aggKp, aggKi, aggKd);
    }
    if ( millis() - timeBrewdetection  < brewtimersoftware * 1000 && timerBrewdetection == 1) {
      bPID.SetTunings(aggbp, aggbi, aggbd) ;
      //   Serial.println("PIDMODEBREW") ;
    }

    bPID.Compute();
    if (Onoff == 0) {
      Output = 0 ;
    }
    if (millis() - windowStartTime > windowSize) {
      windowStartTime += windowSize;
    }
    if (Output < millis() - windowStartTime)    {
      digitalWrite(pinRelayHeater, LOW);
      //Serial.println("Power off!");

    } else {
      digitalWrite(pinRelayHeater, HIGH);
      //Serial.println("Power on!");
    }

    /********************************************************
      Sendet Daten ans Display
    ******************************************************/
    unsigned long currentMillisDisplay = millis();
    if (currentMillisDisplay - previousMillisDisplay >= intervalDisplay) {
      previousMillisDisplay = currentMillisDisplay;

      /********************************************************
        Sendet Daten zur App
      ******************************************************/

      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        //  Serial.print("runblynk");
        if (Offlinemodus == 0) {
          Blynk.run();
          Blynk.virtualWrite(V2, Input);
          Blynk.syncVirtual(V2);
          Blynk.virtualWrite(V3, setPoint);
          Blynk.syncVirtual(V3);
          Blynk.virtualWrite(V20, bPID.GetKp());
          Blynk.syncVirtual(V20);
          Blynk.virtualWrite(V21, bPID.GetKi());
          Blynk.syncVirtual(V21);
          Blynk.virtualWrite(V22, bPID.GetKd());
          Blynk.syncVirtual(V22);
          Blynk.virtualWrite(V23, Output);
          Blynk.syncVirtual(V23);
          Blynk.virtualWrite(V35, heatrateaverage);
          Blynk.syncVirtual(V35);
          Blynk.virtualWrite(V36, heatrateaveragemin);
          Blynk.syncVirtual(V36);
        }
      }

      if (Display == 1) {

        /********************************************************
           DISPLAY AUSGABE
        ******************************************************/
        u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
        u8x8.setCursor(0, 0);
        u8x8.print("               ");
        u8x8.setCursor(0, 1);
        u8x8.print("               ");
        u8x8.setCursor(0, 2);
        u8x8.print("               ");
        u8x8.setCursor(0, 0);
        u8x8.setCursor(1, 0);
        u8x8.print(bPID.GetKp());
        u8x8.setCursor(6, 0);
        u8x8.print(",");
        u8x8.setCursor(7, 0);
        u8x8.print(bPID.GetKi());
        u8x8.setCursor(11, 0);
        u8x8.print(",");
        u8x8.setCursor(12, 0);
        u8x8.print(bPID.GetKd());
        u8x8.setCursor(0, 1);
        u8x8.print("Input:");
        u8x8.setCursor(9, 1);
        u8x8.print("   ");
        u8x8.setCursor(9, 1);
        u8x8.print(Input);
        u8x8.setCursor(0, 2);
        u8x8.print("SetPoint:");
        u8x8.setCursor(10, 2);
        u8x8.print("   ");
        u8x8.setCursor(10, 2);
        u8x8.print(setPoint);
        u8x8.setCursor(0, 3);
        u8x8.print(round((Input * 100) / setPoint));
        u8x8.setCursor(4, 3);
        u8x8.print("%");
        u8x8.setCursor(6, 3);
        u8x8.print(Output);
      }
      if (Display == 2) {
        /********************************************************
           DISPLAY AUSGABE
        ******************************************************/
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Ist-Temp:");
        display.print("  ");
        display.println(Input);
        display.print("Soll-Temp:");
        display.print(" ");
        display.println(setPoint);
        display.print("PID:");
        display.print(" ");
        display.print(bPID.GetKp());
        display.print(",");
        display.print(bPID.GetKi());
        display.print(",");
        display.println(bPID.GetKd());
        display.println(" ");
        display.setTextSize(3);
        display.setTextColor(WHITE);
        display.print(round((Input * 100) / setPoint));
        display.println("%");
        display.display();
      }

    }
  } else if (error == numReadingsdisplayerror) {
    if (Display == 2) {
      /********************************************************
         DISPLAY AUSGABE
      ******************************************************/
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Error:");
      display.print("  ");
      display.println(Input);
      display.print("Check Temp. Sensor!");
      display.display();
    }
    if (Display == 1) {
      /********************************************************
         DISPLAY AUSGABE
      ******************************************************/
      u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
      u8x8.setCursor(0, 0);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("               ");
      u8x8.setCursor(0, 2);
      u8x8.print("               ");
      u8x8.setCursor(0, 1);
      u8x8.print("Error: Temp.");
      u8x8.setCursor(0, 2);
      u8x8.print(Input);
    }
  }
}
