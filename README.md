# Rancilio PID meets Clevercoffee

<div align="center">
<img src="https://img.shields.io/github/workflow/status/rancilio-pid/ranciliopid/Build/master">
<img src="https://img.shields.io/github/last-commit/rancilio-pid/ranciliopid/master"><br>
<a href='https://ko-fi.com/clevercoffee' target='_blank'><img height='35' style='border:0px;height:46px;' src='https://az743702.vo.msecnd.net/cdn/kofi3.png?v=0' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>
</div>

# About

This project implements a PID for stable and accurate temperature control, originally for Rancilio Silvia espresso machines but also includes support for Gaggia and Quickmill machines. Others can easily be added or are already compatible.

The hardware has a small footprint and can easily fit also into most smaller espresso machines. The original wiring of the machine (mostly) remains and is only extended. The machine can be easily reversed to the orignal state after the conversion.

The project has been in active development and supported for 4 years with continuous improvements. Hundreds of machines have been converted to PID control already.

You can find our project website here: [Clever Coffee Website](https://clevercoffee.de).

The PID software is Open Source: free of charge for you and customizable to your personal needs. 

We recommend you to have a look at the manual before starting a build, you can find the german one [here](https://rancilio-pid.github.io/ranciliopid-handbook/). It is currently being reworked to include all the latest features. The english one is sadly still very outdated but will also be updated soon. 

Our chat for collaboration and questions can be found[here](https://chat.rancilio-pid.de).

Video tutorial on how to flash the firmware (a little outdated but mostly still valid):<br>
https://youtu.be/KZPjisOEcQ4

## Version
The next version is going to be 3.1.0 (xx.09.2022)

## What is possible after installation into your espresso machine? 
 * Control of the brewing temperature with an accuracy of up to +/- 0,1°.
 * Reach the target temperature within 8 to 12 minutes after switching on (you should however wait a bit longer, e.g. 20 min depending on the machine to heat up brew head etc.)
 * **NEW** Control all parameters and see temperature and heater graphs on a web page hosted on the ESP controller
 * PID for steam mode with own parameters and target temperature (can be enabled in the web interface/MQTT or using the steam switch when all features are built in) 
 * Control the brewing time (including pre-infusion) when all features are built in.
 * Integration of a scale function, allows brewing by weight.
 * Integration of a water level sensor.
 * Multiple designs for the display, possibility to integrate further designs easily (including vertical template)
 * **NEW** MQTT (IoT) support to manipulate all important parameters.
 * **NEW** Transition from steam mode to normal mode optimized.
 * Brew switch detection also supported using an optocoupler when using OnlyPID ([details](https://rancilio-pid.github.io/ranciliopid-handbook/de/customization/brueherkennung.html#konfiguration-der-erkennung)).
 * Shot timer adjustable to see the reference time.
 * Support for Siliva E machines: trigger with definable interval for a relay to bypass the Eco function
 * Extended data monitoring via Influxdb/Grafana.
 * Over-The-Air updates of the firmware are possible via Wifi

User feedback and suggestions for further development of the software are most welcome.
You are welcome to help us in our mission to make better espresso :)

Thanks to every single supporter!