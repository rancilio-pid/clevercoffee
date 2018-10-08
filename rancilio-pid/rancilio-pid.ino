/********************************************************
   Version 1.0.1
******************************************************/

#include "Arduino.h"

/********************************************************
   BLYNK
******************************************************/
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
char auth[] = "";

char ssid[] = "";
char pass[] = "";

//Zeitserver
#include <TimeLib.h>
#include <WidgetRTC.h>
BlynkTimer timer;
WidgetRTC rtc;
WidgetBridge bridge1(V1);

/********************************************************
   Analog Schalter Read
******************************************************/
int analogPin = 0;
int brewcounter = 0;
int brewswitch = 0;

int brewtime = 25; // 5 Sekunden
long aktuelleZeit = 0; // speichert wie viele Sekunden seit derletzten Änderung vergangen sind
int totalbrewtime = 0;
int preinfusion = 2;
int preinfusionpause = 5;

#define pinRelayVentil    12
#define pinRelayPumpe     2



/********************************************************
   DISPLAY
******************************************************/

#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=*/ 5, /* data=*/ 4, /* reset=*/ 16);   //Display initalisieren

/********************************************************
   PID
******************************************************/
#include "PID_v1.h"

#define pinRelayHeater    15

int boilerPower = 1000; // Watts
float boilerVolume = 300; // Grams

unsigned int windowSize = 1000 / 1;
unsigned long windowStartTime;
double acceleration = 1;
double setPoint, Input, Output, Input2, setPointTemp, Coldstart;

double aggKp = 55 / acceleration;
double aggKi = 0.5 / acceleration;
double aggKd = 35 / acceleration;

PID bPID(&Input, &Output, &setPoint, aggKp, aggKi, aggKd, DIRECT);

/********************************************************
   DALLAS TEMP
******************************************************/
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 13
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);



/********************************************************
   BLYNK WERTE EINLESEN
******************************************************/

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



void setup() {

  Serial.begin(115200);

  /********************************************************
    DISPLAY
  ******************************************************/
  u8x8.begin();
  u8x8.setPowerSave(0);

  /********************************************************
    BrewKnopf SSR Relais
  ******************************************************/
  pinMode(pinRelayVentil, OUTPUT);
  pinMode(pinRelayPumpe, OUTPUT);

  /********************************************************
     BLYNK
  ******************************************************/

  Blynk.begin(auth, ssid, pass, "blynk.remoteapp.de", 8080);
  //Blynk.begin(auth, ssid, pass, "blynk.rancilio-pid.de", 8080);

  pinMode(pinRelayHeater, OUTPUT);
  Input = 20.0;
  windowStartTime = millis();
  setPoint = 96.0;
  Coldstart = 1;

  setPointTemp = setPoint;
  bPID.SetSampleTime(windowSize);
  bPID.SetOutputLimits(0, windowSize);
  bPID.SetMode(AUTOMATIC);

  sensors.begin();
  sensors.requestTemperatures();

  Serial.println();
  Serial.println("Starting!");
}

void loop() {

  /********************************************************
    BLYNK
  ******************************************************/
  Blynk.run();
  timer.run();

  brewswitch = analogRead(analogPin);
  unsigned long startZeit = millis();


  if (brewswitch > 1000 && startZeit - aktuelleZeit > totalbrewtime && brewcounter == 0) {
    aktuelleZeit = millis();
    brewcounter = brewcounter + 1;
  }

  totalbrewtime = preinfusion + preinfusionpause + brewtime;
  //Serial.println(brewcounter);
  if (brewswitch > 1000 && startZeit - aktuelleZeit < totalbrewtime && brewcounter >= 1) {
    if (startZeit - aktuelleZeit < preinfusion) {
      // Serial.println("preinfusion");
      digitalWrite(pinRelayVentil, HIGH);
      digitalWrite(pinRelayPumpe, HIGH);
      digitalWrite(pinRelayHeater, HIGH);
    }
    if (startZeit - aktuelleZeit > preinfusion && startZeit - aktuelleZeit < preinfusion + preinfusionpause) {
      //Serial.println("Pause");
      digitalWrite(pinRelayVentil, HIGH);
      digitalWrite(pinRelayPumpe, LOW);
      digitalWrite(pinRelayHeater, LOW);
    }
    if (startZeit - aktuelleZeit > preinfusion + preinfusionpause) {
      // Serial.println("Brew");
      digitalWrite(pinRelayVentil, HIGH);
      digitalWrite(pinRelayPumpe, HIGH);
      digitalWrite(pinRelayHeater, HIGH);
    }


  } else {
    digitalWrite(pinRelayVentil, LOW);
    digitalWrite(pinRelayPumpe, LOW);
    //Serial.println("aus");
  }

  if (brewswitch < 1000 && brewcounter >= 1) {
    brewcounter = 0;
    aktuelleZeit = 0;
  }


  sensors.requestTemperatures();
  Input = sensors.getTempCByIndex(0);


  // wenn Maschine kalt ist setpoint reduzieren um Overshoot zu vermeiden

  if (Input < 65 && Coldstart >= 1) {
    setPoint = (setPoint * 0.82);
    Coldstart = 0;
    Serial.println(setPoint);
    Serial.println("setPoint für Kaltstart");
  }
  if (Input >= setPoint && setPointTemp > 0 ) {
    Serial.println(setPoint);
    Serial.println(setPointTemp);
    Serial.println("Alte werte oben neue unten nach 15sec einschwingpause jetzt AUS + PAUSE");
    digitalWrite(pinRelayHeater, LOW);
    delay (15000);
    //Überschwingen abwarten
    setPoint = setPointTemp;
    // auf original Setpoint setzen
    Serial.println(setPoint);
    Serial.println(setPointTemp);
    setPointTemp = 0;
  }

  // NORMALE PID

  bPID.Compute();
  if (millis() - windowStartTime > windowSize) {

    bPID.SetTunings(aggKp, aggKi, aggKd);

    Blynk.virtualWrite(V2, Input);
    Blynk.syncVirtual(V2);
    // Send date to the App
    Blynk.virtualWrite(V3, setPoint);
    Blynk.syncVirtual(V3);


    /********************************************************
       DISPLAY AUSGABE
    ******************************************************/
    u8x8.setFont(u8x8_font_chroma48medium8_r);  //Ausgabe vom aktuellen Wert im Display
    u8x8.setCursor(0, 1);
    u8x8.print("IstTemp:");
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

    // Serial.println(digitalRead(pinRelayHeater));
    windowStartTime += windowSize;
    // Input = Input + ((boilerPower / (4.184 * boilerVolume) * (Output / windowSize)));
    // Input = Input + ((0.0004375 * (20 - Input)));

  }
  if (Output < millis() - windowStartTime) {
    digitalWrite(pinRelayHeater, LOW);
    //Serial.println("Power off!");
  } else {
    if (Input <= setPoint + 0.2 ) {
      digitalWrite(pinRelayHeater, HIGH);
      //Serial.println("Power on!");
    }
  }
}
