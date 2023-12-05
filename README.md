# CleverCoffee
(formerly Rancilio PID)

<div align="center">
<img src="https://img.shields.io/github/actions/workflow/status/rancilio-pid/clevercoffee/main.yml?branch=master">
<img src="https://img.shields.io/github/last-commit/rancilio-pid/clevercoffee/master"><br>
<a href='https://ko-fi.com/clevercoffee' target='_blank'><img height='35' style='border:0px;height:46px;' src='https://az743702.vo.msecnd.net/cdn/kofi3.png?v=0' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>
</div>

# About

This project implements a PID controller for stable and accurate temperature control, originally for Rancilio Silvia espresso machines but also includes support for Gaggia and Quickmill machines. Others can easily be added or are already compatible.

Additional features include:

* shot timer
* pre-infusion (reduced initial pressure using a dimmer for the pump)
* brew by weight (using weight cells, no support for external scales yet)
* brew by time
* pressure monitoring

The hardware has a small footprint and can easily fit into most smaller espresso machines. The original wiring of the machine (mostly) remains and is only extended. The machine can be easily reversed to the original state after the conversion.

The project has been in active development and supported for 4 years with continuous improvements. Hundreds of machines have been converted to PID control already.

You can find our project website here: [Clever Coffee Website](https://clevercoffee.de).

This software is Open Source: free of charge for you and customizable to your personal needs.

We recommend you have a look at the manual before starting a build, you can find the german one [here](https://rancilio-pid.github.io/ranciliopid-handbook/). It is currently being reworked to include all the latest features. The english one is sadly still very outdated but will also be updated soon.

Our chat for collaboration and questions can be found[here](https://discord.gg/Kq5RFznuU4).

Video tutorial on how to flash the firmware (a little outdated but mostly still valid):<br>
https://youtu.be/KZPjisOEcQ4

## Version
With Version 3.2.0 we bring the last major release with support for ESP8266 and ESP32.
There will only be bug fix releases for ESP8266 from there on. 
Further development, with new features, will only be done for ESP32.
"master" branch contains the current development only for esp32.
"master-esp8266" branch contains last version for ESP8266 and ESP32.

## What is possible after installation into your espresso machine?
 * Control of the brew temperature with an accuracy of up to +/- 0,1°.
 * Reaches the target temperature within 5 to 10 minutes after switching on (you should, however, wait a bit longer, e.g. 20 min depending on the machine to heat up the group head etc.)
 * Set PID parameters and monitor current temperature and heater output on a web page hosted on the ESP controller
 * Separate PID for steam mode with own parameters and target temperature (can be enabled in the web interface/MQTT or using the steam switch)
 * Automatically brew by set time (including pre-infusion with additional dimmer for the pump).
 * Automatically brew by weight when scale components are built in.
 * Allows brew switch detection (e.g. for the shot timer) by using an optocoupler module when deciding not to control the pump from the ESP ([details](https://rancilio-pid.github.io/ranciliopid-handbook/de/customization/brueherkennung.html#konfiguration-der-erkennung)).
* MQTT (IoT) support to monitor and manipulate all important parameters.
 * Extended data monitoring via Influxdb/Grafana.
 * Choose from multiple designs for the display (including vertical), possibility to integrate custom designs
 * Over-The-Air updates of the firmware (WiFi)

User feedback and suggestions for further development of the software are most welcome.
You are welcome to help us in our mission to make better espresso. :)

Thanks to every single supporter!
