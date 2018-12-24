/********************************************************
   Version 1.1.0 BETA
  Key facts: BETA VERSION OF THE SOFTWARE DETECTION!
  - Check the PIN Ports in the CODE! Not the default from MASTER
  - Find your changerate of the machine, can be wrong, test it!


******************************************************/
#include "Arduino.h"
unsigned long previousMillisColdstart = 0;
unsigned long previousMillisColdstartPause = 0;
unsigned long ColdstartPause = 0;
unsigned long KaltstartPause = 0;
unsigned long bruehvorganggestartet = 0;
unsigned long warmstart = 0;
unsigned long previousMillisSwing = 0;
double Onoff = 1 ; // default 1

/********************************************************
   Vorab-Konfig
******************************************************/
int Display = 0;    // 1=U8x8libm, 0=Deaktiviert, 2=Externes 128x64 Display
int OnlyPID = 0;    // 1=Nur PID ohne Preinfussion, 0=PID + Preinfussion
int TempSensor = 2; // 1=DS19B20; 2=TSIC306
int Brewdetection = 2 ;// 0=off ,1=Software ,2= hardware
int standby = 0 ; // 0: Old rancilio not needed, 1: new one , E or V5 with standy, not used in the moment

/*
  char auth[] = ""; //ich lokal
  char ssid[] = "";
  char pass[] = ""; // Andreas
*/
/********************************************************
   moving average
*****************************************************/
const int numReadings = 15; // number of values per Array
double readingstemp[numReadings];      // the readings from Temp
double readingstime[numReadings];      // the readings from time
double readingchangerate[numReadings];
int readIndex = 0;             // the index of the current reading
double total = 0;              // the running
int totaltime = 0 ;            // the running time
double heatrateaverage = 0;    // the average over the numReadings
double changerate = 0;         // local change rate of temprature
double heatrateaveragemin = 0 ;
unsigned long  timeBrewdetection = 0 ;
int timerBrewdetection = 0 ;
int i = 0;

double aggbp = 150 ;
double aggbi = 0 ;
double aggbd = 1500;



double brewtimersoftware = 40; // 20-5 for detection
double brewboarder = 40 ; // border for the detection,
// be carefull: to high: risk of wrong brew detection
// and rising temperature

/********************************************************
   Analog Input - hardware solution brew detection
******************************************************/
int inputVoltage ;
int inputVoltagetime = 10 ;
int Voltagecounter = 0 ;
int Voltagenormal = 0 ;
int Voltagecalc = 0 ;
int Voltageuse = 0 ;

/********************************************************
   BLYNK
******************************************************/
/*
  #define BLYNK_PRINT Serial
  #include <BlynkSimpleEsp8266.h>

  //Zeitserver
  #include <TimeLib.h>
  #include <WidgetRTC.h>
  BlynkTimer timer;
  WidgetRTC rtc;
  WidgetBridge bridge1(V1);
*/
//Update Intervall zur App
unsigned long previousMillis = 0;
const long interval = 5000;


/********************************************************
   Analog Schalter Read
******************************************************/
int analogPin = 0;
int brewcounter = 0;
int brewswitch = 0;


int brewtime = 25000;
long aktuelleZeit = 0;
int totalbrewtime = 0;
int preinfusion = 2000;
int preinfusionpause = 3000;

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

#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 5, /* data=*/ 4, /* reset=*/ 16);   //Display initalisieren

// Display 128x64
#include <Wire.h>
#include <Adafruit_GFX.h>
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
long intervaltempmes = 400 ;

unsigned int windowSize = 1000;
unsigned long windowStartTime;
double acceleration = 1;
double setPoint, Input, Output, setPointTemp, Coldstart;

double aggKp = 28 / acceleration;
double aggKi = 0.07 / acceleration;
double aggKd = 0 / acceleration;
double startKp = 0;
double starttemp = 90;


PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, DIRECT);

/********************************************************
   DALLAS TEMP
******************************************************/
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 14
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress sensorDeviceAddress;

/********************************************************
   B+B Sensors TSIC 306
******************************************************/
#include "TSIC.h"         // include the library
TSIC Sensor1(2);          // only Signalpin, VCCpin unused by default
uint16_t temperature = 0;
float Temperatur_C = 0;

/********************************************************
   BLYNK WERTE EINLESEN
******************************************************/
/*
  //beim starten soll einmalig der gespeicherte Wert aus dem EEPROM 0 in die virtelle
  //Variable geschrieben werden
  BLYNK_CONNECTED() {
  Blynk.syncAll();
  rtc.begin();
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

*/

void setup() {

  Serial.begin(2000000);

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
    display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
    display.clearDisplay();
  }

  /********************************************************
    BrewKnopf SSR Relais
  ******************************************************/
  pinMode(pinRelayVentil, OUTPUT);
  pinMode(pinRelayPumpe, OUTPUT);

  /********************************************************
     BLYNK
  ******************************************************/
  /*
    // Blynk.begin(auth, ssid, pass, "blynk.remoteapp.de", 8080);
     Blynk.begin(auth,ssid,pass,"raspberrypi.local",8080);
  */
  pinMode(pinRelayHeater, OUTPUT);

  windowStartTime = millis();
  Coldstart = 1;

  setPoint = 96;
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
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
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
  if (TempSensor == 1) {
    sensors.requestTemperatures();
    Input = sensors.getTempCByIndex(0);
  }
  if (TempSensor == 2) {
    unsigned long currentMillistemp = millis();
    if (currentMillistemp - previousMillistemp > intervaltempmes) {
      temperature = 0;
      Sensor1.getTemperature(&temperature);
      Temperatur_C = Sensor1.calc_Celsius(&temperature);
      Input = Temperatur_C;
      //  Serial.print(currentMillistemp);
      //  Serial.print(";");
      //   Serial.print(previousMillistemp);
      //   Serial.print(";");
      //Serial.println(Temperatur_C);
      previousMillistemp = currentMillistemp;
      // Logging Temp has to be sync with reading temp.
      if (Brewdetection == 1 && Input > 0) {
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
        // if we're at the end of the array...
        if (heatrateaveragemin > heatrateaverage) {
          heatrateaveragemin = heatrateaverage ;
        }

        //   Serial.print(readIndex);
        //   Serial.print(";");
        //   Serial.print(readingstime[readIndex]);
        //   Serial.print(";");
        //   Serial.print(readingstemp[readIndex]);
        //   Serial.print(";");
        //   Serial.print(readingchangerate[readIndex]);
        //   Serial.print(";");
        Serial.println(heatrateaveragemin, 4);
        Serial.println(heatrateaverage, 4);
        if (readIndex >= numReadings) {
          // ...wrap around to the beginning:
          readIndex = 0;
        }
        readIndex = readIndex + 1;
      }
    }
  }
  //Serial.println(Input);



  /********************************************************
    PreInfusion
  ******************************************************/

  brewswitch = analogRead(analogPin);
  //Serial.println(brewswitch);

  unsigned long startZeit = millis();

  /*
    Serial.print("OnlyPID:");
    Serial.println(OnlyPID);
    Serial.print("startZeit:");
    Serial.print(startZeit);
    Serial.print(",");
    Serial.print("aktuelleZeit:");
    Serial.print(startZeit);
    Serial.print(",");
    Serial.print("totalbrewtime:");
    Serial.print(totalbrewtime);
    Serial.print(",");
    Serial.print("brewcounter:");
    Serial.print(brewcounter);
    Serial.print(",");
    Serial.print("brewswitch:");
    Serial.println(brewswitch);
  */

  if (OnlyPID == 0) {

    if (brewswitch > 1000 && startZeit - aktuelleZeit > totalbrewtime && brewcounter == 0) {
      aktuelleZeit = millis();
      brewcounter = brewcounter + 1;
    }

    totalbrewtime = preinfusion + preinfusionpause + brewtime;

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
    change of rate
  ******************************************************/


  //Sicherheitsabfrage
  if (Input >= 0) {

    /********************************************************
      function Analog put in
    ******************************************************/
    if (Brewdetection == 2) {
      inputVoltage = analogRead(analogPin);
      //   Serial.println(inputVoltage);
      if (startZeit < 40 * 1000) {
        Voltagenormal = inputVoltage + Voltagenormal  ;
        Voltagecounter = Voltagecounter + 1 ;
        Voltagecalc = 0 ;
      } else {
        Voltagecalc = 1 ;
      }
      if (Voltagecalc == 1 && Voltageuse == 0 && Voltagecounter > 0 ) {
        Voltagenormal = Voltagenormal / Voltagecounter ;
        //   Serial.println("Mittelwert ermittelt");
        //   Serial.println(Voltagenormal);
        Voltageuse = 1 ;
      }
    }
    //          /********************************************************
    //             Onoffmachine relay
    //          ******************************************************/
    //          if (standby == 1) {
    //          windowStartOnoff=millis() ;
    //          if (millis()-windowStartOnoff< 10*1000) {
    //          digitalWrite(pinRelaymaschineonoff, HIGH);  }
    //          else {
    //          digitalWrite(pinRelaymaschineonoff, LOW) ;
    //          }
    //          }
    //

    /********************************************************
      PID
    ******************************************************/
    // Brew detecion == 1 software solution , ==2 hardware
    if (Brewdetection == 1 || Brewdetection == 2) {
      if (millis() - timeBrewdetection > 50 * 1000) {
        timerBrewdetection = 0 ;
      }
    }
    if (Brewdetection == 1) {
      if (heatrateaverage <= -brewboarder && timerBrewdetection == 0) {
        Serial.println("Brewdetected") ;
        timeBrewdetection = millis() ;
        timerBrewdetection = 1 ;
      }
    }
    // Brew detecion == 2 hardware solution
    if (Brewdetection == 2) {
      if (abs(inputVoltage - Voltagenormal) > 20 && Voltageuse == 1 && timerBrewdetection == 0) {
        Serial.println("Brewdetected") ;
        timeBrewdetection = millis() ;
        timerBrewdetection = 1 ;
      }
    }

    if (Input < starttemp) {
      bPID.SetTunings(aggKp, 0, 0);
    } else {
      bPID.SetTunings(aggKp, aggKi, aggKd);
    }
    if ( millis() - timeBrewdetection  < brewtimersoftware * 1000 && timerBrewdetection == 1) {
      bPID.SetTunings(aggbp, aggbi, aggbd) ;
      Serial.println("PIDMODEBREW") ;
    }

    bPID.Compute();
    //Serial.print("Onoff:");Serial.println(Onoff);
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
      Sendet Daten zur App
    ******************************************************/
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      // Blynk.run();

      //Blynk.virtualWrite(V2, Input);
      //Blynk.syncVirtual(V2);
      // Send date to the App
      //Blynk.virtualWrite(V3, setPoint);
      //Blynk.syncVirtual(V3);

      Serial.print(bPID.GetKp());
      //Blynk.virtualWrite(V20, bPID.GetKp());
      //Blynk.syncVirtual(V20);
      Serial.print(",");
      Serial.print(bPID.GetKi());
      //Blynk.virtualWrite(V21, bPID.GetKi());
      //Blynk.syncVirtual(V21);
      Serial.print(",");
      Serial.print(bPID.GetKd());
      //Blynk.virtualWrite(V22, bPID.GetKd());
      //Blynk.syncVirtual(V22);
      Serial.print(",");
      Serial.print(Output);
      //Blynk.virtualWrite(V23, Output);
      //Blynk.syncVirtual(V23);
      Serial.print(",");
      Serial.print(setPoint);
      Serial.print(",");
      Serial.println(Input);
      //Blynk.virtualWrite(V35,heatrateaverage);
      //    Blynk.syncVirtual(V35);
      //      Blynk.virtualWrite(V36,heatrateaveragemin);
      //    Blynk.syncVirtual(V36);

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
        u8x8.print(".");
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
  } else {
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
