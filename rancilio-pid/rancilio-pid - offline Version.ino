/********************************************************
   Version 1.1.0
******************************************************/
#include "Arduino.h"
unsigned long previousMillisColdstart = 0;
unsigned long previousMillisColdstartPause = 0;
unsigned long ColdstartPause = 0;
unsigned long KaltstartPause = 0;
unsigned long bruehvorganggestartet = 0;
unsigned long warmstart = 0;
unsigned long previousMillisSwing = 0;
double Onoff = 1 ;

/********************************************************
   Vorab-Konfig
******************************************************/
int Display = 2;    // 1=U8x8libm, 0=Deaktiviert, 2=Externes 128x64 Display
int OnlyPID = 0;    // 1=Nur PID ohne Preinfussion, 0=PID + Preinfussion
int TempSensor = 2; // 1=DS19B20; 2=TSIC306

char auth[] = "e155c41e5202445b899513ca52acf7d3";
char ssid[] = "brunner-it";
char pass[] = "12brunner&";

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
int preinfusionpause = 5000;

#define pinRelayVentil    12
#define pinRelayPumpe     13

/********************************************************
   DISPLAY
******************************************************/
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

int boilerPower = 1000; // Watts
float boilerVolume = 300; // Grams

unsigned int windowSize = 1000;
unsigned long windowStartTime;
double acceleration = 1;
double setPoint, Input, Output, Input2, setPointTemp, Coldstart;

double aggKp = 17.5 / acceleration;
double aggKi = 0.14 / acceleration;
double aggKd = 10 / acceleration;
double startKp = 0;
double starttemp = 90;

PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, DIRECT);

/********************************************************
   B+B Sensors TSIC 306
******************************************************/
#include "TSIC.h"         // include the library
TSIC Sensor1(2);          // only Signalpin, VCCpin unused by default
uint16_t temperature = 0;
float Temperatur_C = 0;


void setup() {

  Serial.begin(115200);

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


  pinMode(pinRelayHeater, OUTPUT);

  windowStartTime = millis();
  setPoint = 95;
  setPointTemp = setPoint;
  
  bPID.SetSampleTime(windowSize);
  bPID.SetOutputLimits(0, windowSize);
  bPID.SetMode(AUTOMATIC);

}

void loop() {

  /********************************************************
    Temp. Request
  ******************************************************/
  if (TempSensor == 2) {
    temperature = 0;
    Sensor1.getTemperature(&temperature);
    Temperatur_C = Sensor1.calc_Celsius(&temperature);
    Input = Temperatur_C;
    //Serial.print(Temperatur_C);
  }
  //Serial.println(Input);



  /********************************************************
    PreInfusion
  ******************************************************/

  brewswitch = analogRead(analogPin);

  unsigned long startZeit = millis();
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

  //Sicherheitsabfrage
  if (Input >= 0) {
    if (Onoff == 1) {

      /********************************************************
        PID
      ******************************************************/
      bPID.Compute();
      if (millis() - windowStartTime > windowSize) {

        if (Input < starttemp) {
          bPID.SetTunings(aggKp, startKp, aggKd);
        } else {
          bPID.SetTunings(aggKp, aggKi, aggKd);
        }
        windowStartTime += windowSize;
      }
      if (Output < millis() - windowStartTime) {
        digitalWrite(pinRelayHeater, LOW);
        //Serial.println("Power off!");
      }
     else {
      digitalWrite(pinRelayHeater, HIGH);
      //Serial.println("Power on!");
    }



    /********************************************************
      Sendet Daten zur App
    ******************************************************/
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {

      previousMillis = currentMillis;

      // Send date to the App


      Serial.print(bPID.GetKp());

      Serial.print(",");
      Serial.print(bPID.GetKi());
 
      Serial.print(",");
      Serial.print(bPID.GetKd());

      Serial.print(",");
      Serial.print(Output);

      Serial.print(",");
      Serial.print(setPoint);
      Serial.print(",");
      Serial.println(Input);


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
  }

}
