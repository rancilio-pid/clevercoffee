# RemoteDebugApp - HTML5 web app to debug Arduino in web browser

This is local copy of RemoteDebugApp to use if internet is offline.  

![versiom](https://img.shields.io/badge/version-v0.3.2-blue.svg?style=flat)
[![GitHub](https://img.shields.io/github/license/mashape/apistatus.svg)](https://github.com/JoaoLopesF/RemoteDebug/blob/master/LICENSE.txt)
[![contributions welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat)](#github)

It have a periodic check for new version on server,
to you realize a download of new release.

It is generated with lastest version of web app,
after it is publised in web server: [http://joaolopesf.net/remotedebugapp](http://joaolopesf.net/remotedebugapp/)

Download it, for use when internet is offline.

![logo](https://raw.githubusercontent.com/JoaoLopesF/RemoteDebug/master/extras/readme_media/logo.png)

## Contents

- [About](#about)
- [How it looks](#how-it-looks)
- [Github](#github)
- [Installing](#installing)
- [Known issues](#known-issues)
- [Releases](#releases)

## About

RemoteDebug (v3) have an app, the RemoteDebugApp,
to debug in web browser.

This app is an HTM5 web app, with websocket to comunicate to Arduino board.  
For it, RemoteDebug v3 have a web socket server (can be disabled).

Note: this not uses SSL (https), due web server socket on Arduino, not supports SSL (wss).
But after page load, all traffic is in local network, no data is exposed on internet.

The RemoteDebugApp is a modern HTML5 and needs a modern browsers to work.

Internet Explorer 11 and Safari 10 are an examples that not supported.
But you can use anothers, as Chrome, Edge, Firefox.

The web app is in beta, please add an issue,
for problems or suggestions.

This repository is to store local copies of this web app,
for download and use when internet is offline.

## Installing

Just download, unzip the file, and double-click in index.html.
When have a new version, this web app is warning you.

### Knew issues

    - In some browsers and S.O, the console data not do autoscroll,
      if you get it, please add an issue.

## How it looks

![webapp](https://raw.githubusercontent.com/JoaoLopesF/RemoteDebug/master/extras/readme_media/remotedebug_webapp.png)

## Releases

### 0.3.2 - 2019-03-20

    - Adjustments for a local copy version
    - And on version check
    - Added "Conv" button to access github RemoteDebugConverter

### 0.3.1 - 2019-03-18  

    - Using ajax to get version of app (to check if needs reload)
      Prepare to use local (saved in disk)
      Another adjustments

### 0.3.0 - 019-03-15

    - Fullscreen 
      Adjustments in all  
      Better design responsible for web and mobile

### 0.2.0 - 2019-03-13

    - Design responsible for web and mobile

### 0.1.0 - 2019-03-07

    - First alpha

## Thanks

This web app is made with Pinegrow and VSCode Insiders.
