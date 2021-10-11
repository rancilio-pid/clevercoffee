# SerialDebugApp
Freeware desktop App, companion for SerialDebug Library.

<a href="#releases">![build badge](https://img.shields.io/badge/version-v0.9.4-blue.svg)</a> [![Gitter chat](https://badges.gitter.im/SerialDebug/gitter.png)](https://gitter.im/SerialDebug/SerialDebugApp)

__SerialDebugApp__ is a freeware desktop app, companion for __SerialDebug__ library for Arduino.

Note: the open source __SerialDebugApp__ library is not depending on this software.

Why this app is an freeware and not open source ?

This app is based in my another commercial projects,
through the serial port, control devices with the Arduino.

If dont want use this freeware, just use default serial monitor of Arduino IDE.

# Contents

- [About](#about)
- [Beta version](#beta-version)
- [Github](#github)
- [How it looks](#how-it-looks)
- [Install](#install)
- [Release](#releases)

## About

__SerialDebugApp__ is a serial monitor desktop app, made for __SerialDebug__ library

This have some improvements:

- Show debug messages with different __colors__, depending on each level.

- Have buttons to most of __commands__ of __SerialDebug__

- Have __themes__, light and dark themes.

- Have a __auto-disconnection__ feature.
  If you leave app, e.g Cmd-Tab/Alt-Tab to switch to Arduino IDE,
  the app will disconnect from serial port, to allow a new upload.

- And more, e.g. filter, converter, etc.

Note: Professional debug/logging have levels and it is show with colors,
to differentiate messages.

An example of this, is the ESP-IDF logging for ESP32 boards.

With __SerialDebugApp__ now we have it for Arduino.

## Beta version

This is a beta version.
Not yet fully tested and optimized.

## Github

Contribute to this library development by creating an account on GitHub.

Please give a star, if you find this library usefull, 
this help a another people, discover it too.

Please add a issue for problems or suggestion.

And please join in the Gitter chat room ([SerialDebugApp on Gitter](https://gitter.im/SerialDebug/SerialDebugApp)).

## How it looks

[![Youtube](https://img.youtube.com/vi/ba_eu06mkng/0.jpg)](https://www.youtube.com/watch?v=ba_eu06mkng)

[![Youtube](https://img.youtube.com/vi/C4qRwwjyZwg/0.jpg)](https://www.youtube.com/watch?v=C4qRwwjyZwg)

## Install

Please download the __SerialDebugApp__:

Installers:

- MS Windows

    [Download MSI installer](http://joaolopesf.net/downloads/serialdebugapp/windows/SerialDebugApp.msi.zip)

    This installer will create a shortcut on desktop and in menu too.

- Linux

    For Linux yet not have an installer (due issues on JavaPackager :-( )

    For now please download and use the jar file (see below)

- MacOSX

    [Download PKG installer](http://joaolopesf.net/downloads/serialdebugapp/macosx/SerialDebugApp.pkg.zip)

    This installer will create app in MacOSX Applications.
    Note: for this, you need provider superuser login. If you not have it, can use Java jar runnable.
    Tip: to run you can do it: command+space serialdebugapp and enter.


Or:

- Java jar runnable (needs Java runtime) (runs in MS Windows, Linux, MacOSX and other S.O. supported by Java)

    [Download runnable Java jar file](http://joaolopesf.net/downloads/serialdebugapp/SerialDebugApp.jar.zip)

    This needs a java runtime version 1.8.0 or newer.

    Just unzip, copy to any directory and run.

## Releases

### 0.9.4 - 2018-11-15
  
    - Few Adjustments for check versions

### 0.9.3 - 2018-11-14
  
    - Few Adjustments for dark modes and in check versions

### 0.9.2 - 2018-10-25
  
    - Debugger panel
    - Few adjstments

### 0.9.1 - 2018-10-23
  
    - Few adjstments

### 0.9.0 - 2018-09-15

    - First beta