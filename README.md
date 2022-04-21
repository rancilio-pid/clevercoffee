# Clever Coffee

<div align="center">
  <img src="https://img.shields.io/github/workflow/status/rancilio-pid/clevercoffee/Build/main">
  <img src="https://img.shields.io/github/last-commit/rancilio-pid/clevercoffee/main"><br>
</div>


## About
ESP32-focused rewrite of [Rancilio-PID](https://github.com/rancilio-pid/ranciliopid/) without the need for cloud-based solutions.
The goal of this project is to provide stable hardware and software platforms to minimize the amount and complexity of configuration by the end user.

For the previous commit history/contributions please refer to the old repository.


## Resources
[Project website](https://clevercoffee.de)

[Manual (German)](https://rancilio-pid.github.io/ranciliopid-handbook/)

[Mattermost Community Chat](https://chat.rancilio-pid.de)

[How to Build Video (German)](https://youtu.be/KZPjisOEcQ4)


## Currently implemented features
* Brew temperature control with up to +/- 0,1° C accuracy
* Seperate cold start stage for optimized approximation of the brew temperature set point after ~8-12 minutes (depending on the machine type)
* Integrated brew scale
* Brew detection via optocoupler with PID-only setup
* Integrated shot timer
* 3 different horizontal and 1 vertical display template
* Brew by time (with an additional relay to control the pump)
* InfluxDB Client for data logging/monitoring
* MQTT supprt to read/adjust parameters from smart home systems or other IoT devices
* Over-the-Air firmware updates

