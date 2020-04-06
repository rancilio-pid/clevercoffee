/********************************************************
 * Code      : RemoteDebugApp
 * File      : app-check.js - check if the requisites,
 *             to run app is OK
 * Programmer: Joao Lopes
 * Comments  : Note this file is obfuscated,
 *             due this code is protected,
 *             because it uses same portions
 * 			   of my commercial codes
 ********************************************************/


{
    var message = "";
    var newLocation = "about:blank";
    
    try {
        
        // Check if is HTTPS
        if (location.protocol == 'https:') {
            message = "*** HTTPS (SSL) not supported:\n"+ 
                    "RemoteDebugApp not support HTTPS yet, \n" +
                    "due a limitation on Arduino web socket server," +
                    "that not support a web socket with SSL (wss)." +
                    "This is required with in HTTPS connection.\n" +
                    "If try access a web socket without SSL," +
                    "a error occurs in javacript, and it not is accessed.\n\n" +
                    "Note: Though it yet not secure HTTPS,\n" +
                    "After page load, all of data transmit is between " +
                    "the Javascript (client) and web socket server" +
                    "on Arduino board.\n" +
                    "And all traffic is only in local network," +
                    "none of data is exposed to internet\n\n" +
                    "Click in button to reload the app in HTTP";
            newLocation = 'http:' + window.location.href.substring(window.location.protocol.length);    
        }

        // Check browser version
        var parser = new UAParser();
        var browser = parser.getBrowser();
        var os = parser.getOS();
        console.info("browser: " + browser);
        if (browser) {
            console.info("browser name: " + browser.name + " os: " + os.name + " " + os.version);
            //alert("browser name: " + browser.name + " os: " + os.name + " " + os.version);
            if (browser.name.substring(0, 2) == "IE") {
                message = "*** Internet explorer not supported:\n" + 
                            "This HTML5 Web App not support Internet Explorer\n" +
                            "Please use the Edge or another modern browser, as Chrome or Firefox\n\n" +
                            "Click in button to exit";
            }
            if (os.name == "iOS" && os.version.substring(0, 3) == "10.") {
                message = "*** iOS 10 not supported\n"+ 
                            "This HTML5 Web App not support iOS version 10\n"+
                            // "Please use another modern browser, as Chrome or Firefox\n\n" +
                            "Click in button to exit"; 
            }    
        }
    } catch (error) {
        console.error("error on app-check");	
        message = "An error occurs on app checking";				
    } finally {

        if (message != "") {

            alert (message);

            // Redirect

            location.replace (newLocation);
        }
    }
}

// End