# CleverCoffee
(formerly Rancilio PID)

<div align="center">
<img src="https://img.shields.io/github/actions/workflow/status/rancilio-pid/clevercoffee/main.yml?branch=master" alt="pipeline status">
<img src="https://img.shields.io/github/last-commit/rancilio-pid/clevercoffee/master" alt="master branch"><br>
<a href="https://ko-fi.com/clevercoffee" target="_blank" style="color: black; text-decoration: none;">Buy Me a Coffee at ko-fi.com</a>
</div>

# About

This project implements a PID controller for stable and accurate temperature control, originally designed for Rancilio Silvia espresso machines but also supporting Gaggia and Quickmill machines. Other machines can easily be added or may already be compatible.

Additional features include:

* Shot timer
* Pre-infusion (work in progress: reduced initial pressure using a dimmer for the pump)
* Brew by weight (using HX711-based load cells under the drip tray or Bluetooth scales)
  * Currently supported Bluetooth scales:
    * [EspressiScale](https://www.espressiscale.com/)
    * Acaia Lunar (only tested with pre-2021 model)
    * Acaia Pearl (only tested with pre-2021 model)
    * Decent Scale
    * Bookoo Themis (untested)
    * Felicita Arc (untested)
* Brew by time
* Pressure monitoring

The hardware has a compact footprint and can easily fit into most smaller espresso machines. The original machine wiring remains largely intact and is only extended. The machine can be easily restored to its original state after conversion.

The project has been in active development and supported for 4 years with continuous improvements. Hundreds of machines have been successfully converted to PID control.

You can find our project website here: [Clever Coffee Website](https://clevercoffee.de).

This software is Open Source: free of charge and customizable to your personal needs.

We recommend reviewing the manual before starting a build. You can find the German manual [here](https://rancilio-pid.github.io/ranciliopid-handbook/). It is currently being updated to include all the latest features. The English manual is outdated but will also be updated soon.

The firmware can then be flashed with our dedicated [Web Flasher](https://rancilio-pid.github.io/clevercoffee-flasher/)

## Chat and Support
You will find more information, discussions, and support on our [Discord](https://discord.gg/Kq5RFznuU4) server.
If you want to contribute to the project and help with hardware, software, and documentation development, you will find the appropriate channels there.

**Please note that we can only provide support for our own PCBs. We will not provide assistance with solutions that are not based on our own hardware.**
**Please do not offer PCB derivatives of our design or independent developments without contacting us first. This may result in a ban from our Discord server.**

## What is possible after Installation into your Espresso Machine?
* Control brew temperature with accuracy up to ±0.1°C
* Reach target temperature within 5 to 10 minutes after switching on (however, you should wait longer, approximately 20 minutes depending on the machine, to heat up the group head)
* Set PID parameters and monitor current temperature and heater output on a web page hosted on the ESP controller
* Separate PID for steam mode with independent parameters and target temperature (can be enabled via web interface/MQTT or using the steam switch)
* Automatic brewing by set time including pre-infusion timing
* Automatic brewing by weight when scale components are installed or a Bluetooth scale is connected
* Option to change brew and steam switches to push buttons (brew push button has two actions: short press for brew, long press to flush)
* Brew switch detection (for shot timer functionality) using an optocoupler module when not controlling the pump from the ESP ([details](https://rancilio-pid.github.io/ranciliopid-handbook/de/customization/brueherkennung.html#konfiguration-der-erkennung))
* MQTT (IoT) support to monitor and control all important parameters
* Multiple display design options (including vertical orientation) with possibility to integrate custom designs
* Over-the-air firmware updates via WiFi using our [OTA Flash Tool](https://github.com/rancilio-pid/clevercoffee-otaflasher)

User feedback and suggestions for further software development are welcome. We encourage you to help us in our mission to make better espresso.

Thanks to every supporter!