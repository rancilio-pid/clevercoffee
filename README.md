# CleverCoffee
(formerly Rancilio PID)

<div align="center">
<img src="https://img.shields.io/github/actions/workflow/status/rancilio-pid/clevercoffee/main.yml?branch=master">
<img src="https://img.shields.io/github/last-commit/rancilio-pid/clevercoffee/master"><br>
<a href="https://ko-fi.com/clevercoffee" target="_blank" style="color: black; text-decoration: none;">Buy Me a Coffee at ko-fi.com</a>
</div>

## About

This project implements a PID controller for stable and accurate temperature control, originally for the Rancilio Silvia but it also supports Gaggia and Quickmill machines. Others machines can easily be added or may already be compatible.

Additional features include:

* Shot timer
* Configurable pre-infusion
* Brew by weight
* Brew by time
* Pressure monitoring / profiling (still a work in progress)

The hardware has a small footprint and can easily fit into most compact espresso machines. The original wiring of the machine (mostly) remains intact and is only extended. The machine can be easily reverted back to its original state.

The project has been in active development and supported for over 5 years with continuous improvements. Hundreds of machines have been successfully modded already.

You can find our project website here: [Clever Coffee Website](https://clevercoffee.de).

This software is free and open source and can be customized to your personal needs.

We recommend you have a look at the manual before starting a build, you can find the German one [here](https://rancilio-pid.github.io/ranciliopid-handbook/). It is currently being reworked to include all the latest features. 

## Chat and Support
You will find more information, discussions, and support on our [Discord](https://discord.gg/Kq5RFznuU4) server.
If you want to be part of the project and help with development of hardware, software and documentation you will also find the right channels there.
**Please keep in mind that we primarily give support for our own PCBs. We may not be able to help with any hardware solutions that are not based on our own PCBs.**
**Please do not offer any kind of PCB derivatives of our design or own developments without contacting us before.**

## Installation

Starting with version 4.0.0, CleverCoffee requires no development environment setup. You can flash the firmware directly from any Chromium-based browser using our Web Flasher at:
https://rancilio-pid.github.io/clevercoffee-flasher/

## Version

Version 4.0.0 is a major release that brings significant improvements and new features. Development continues exclusively for ESP32.

### What's New

**Zero Compile-Time Dependencies**

You no longer need to install VS Code, PlatformIO or Git. Flashing is done entirely through the browser-based Web Flasher or our Python script.

**New Configuration System**

* Replaced EEPROM-based configuration with a modern, portable and human-readable JSON config file format
* All configuration now happens via the embedded website
* Hardware features can be configured without editing any source code
* Config backup/restore: Download, edit and re-upload your JSON config file via the website
* Factory/WiFi reset: Reset the firmware or just the WiFi settings to factory defaults without erasing the flash

**Bluetooth Low Energy Scale Support**

* Full support for Bluetooth Low Energy scales, including popular models like Acaia scales
* Automatic connection and reconnection handling
* Auto-tare functionality that activates when starting a brew
* Timer feature for BLE scales
* Visual Bluetooth connection indicator in the status bar

**Feature Complete PCB Support**

Full support for the official CleverCoffee PCB, including control over hot water delivery via the ESP.

**Home Assistant Integration Improvements**

Improved MQTT discovery message generation for better stability and reliability.

**Connectivity Improvements**
* Improved offline mode using an ad hoc WiFi connection for access to the web interface

**Various Bugfixes**
* Improved handling of headless setups without an OLED display
* Fixed momentary power switch operation and added reboot on long press
* Fixed valve control during flush mode
* Fixed brew/flush timer and weight display in display templates
* Fixed MDNS error by setting a default hostname
* Fixed relay pin assignments to prevent issues during boot
* Fixed pre-infusion handling when time is set to zero


## Feature Overview
 * Control of the brew temperature with an accuracy of up to +/- 0.1 degrees
 * Reaches the target temperature within 5 to 10 minutes after switching on (you should, however, wait a bit longer, e.g. 20 min depending on the machine to heat up the group head etc.)
 * Set PID parameters and monitor current temperature and heater output on a web page hosted on the ESP controller
 * Separate PID for steam mode with own parameters and target temperature (can be enabled in the web interface/MQTT or using the steam switch)
 * Automatically brew by set time including pre-infusion timing
 * Automatically brew by weight using integrated weight cells or Bluetooth Low Energy scales
 * Automatic backflush program
 * Programmable standby timer
 * Supports toggle or momentary switches for brew, steam, hot water delivery and power/standby
 * Allows brew switch detection via an optocoupler module for a minimally invasive installation  
 * MQTT support to monitor and manipulate all important parameters
 * Choose from multiple templates for the display (including vertical), possibility to integrate custom designs
 * Over-the-air updates of the firmware via WiFi (requires OTA Flasher or espota.py)

User feedback and suggestions for further development of the software are most welcome.

Thanks to every single supporter!
