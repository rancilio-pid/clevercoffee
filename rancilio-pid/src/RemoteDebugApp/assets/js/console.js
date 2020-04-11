////// Console messages

var _consoleEnabled = (location.hostname === "localhost" || location.hostname === "127.0.0.1");

function consoleEnable() {
	_consoleEnabled = true;
	consoleD("* console messages now is enabled");
}

function consoleDisable() {
	_consoleEnabled = false;
	consoleD("* console messages now is disabled");
}

function consoleE(message) {

    // Error on console

	if (!_consoleEnabled){
		return;
	}

    try {

        var funcName = "";

        if (consoleE.caller && consoleE.caller.name) { // Support this
            funcName = consoleE.caller.name; 
        } else { // Not support - do it by Error
            funcName = getCallerName();
        }

        if (message != "") {
            if (funcName != "") {
                message = funcName + ": " + message;
            }
        } else if (funcName != "") {
            message = funcName;
        }
    
    } catch (error) {
         message = "(undef.err): " + message;
	}
	
	// Show in console

	if (message != "") {
		console.error("%c(E)" + message, "color: rgb(255, 0, 0)");
	}
}

function consoleW(message) {
    
	// Log on console

	if (!_consoleEnabled){
		return;
	}
	
	try {

		var funcName = "";

		if (consoleW.caller && consoleW.caller.name) { // Support this
			funcName = consoleW.caller.name; 
		} else { // Not support - do it by Error
			funcName = getCallerName();
		}

        if (message != "") {
            if (funcName != "") {
                message = funcName + ": " + message;
            }
        } else if (funcName != "") {
            message = funcName;
        }
    
    } catch (error) {
         message = "(undef.err): " + message;
	}
	
	// Show in console

	if (message != "") {
		console.warn("%c(W)" + message, "color: rgb(160, 82, 45)");
	}
}

function consoleL(message) {
    
	// Log on console

	if (!_consoleEnabled){
		return;
	}
	
	try {

		var funcName = "";

		if (consoleL.caller && consoleL.caller.name) { // Support this
			funcName = consoleL.caller.name; 
		} else { // Not support - do it by Error
			funcName = getCallerName();
		}

        if (message != "") {
            if (funcName != "") {
                message = funcName + ": " + message;
            }
        } else if (funcName != "") {
            message = funcName;
        }
    
    } catch (error) {
         message = "(undef.err): " + message;
	}
	
	// Show in console

	if (message != "") {
		console.log("%c(L)" + message, "color: rgb(218, 165, 32");
	}
}

function consoleI(message) {

    // Info on console

	if (!_consoleEnabled){
		return;
	}
	
    try {

        var funcName = "";

        if (consoleI.caller && consoleI.caller.name) { // Support this
            funcName = consoleI.caller.name; 
        } else { // Not support - do it by Error
            funcName = getCallerName();
        }

        if (message != "") {
            if (funcName != "") {
                message = funcName + ": " + message;
            }
        } else if (funcName != "") {
            message = funcName;
        }
    
    } catch (error) {
         message = "(undef.err): " + message;
	}
	
	// Show in console

	if (message != "") {
		console.info("%c(I)" + message, "color: rgb(0, 128, 0)");
	}
}

function consoleD(message) {

	// Debug (verbose) on console

	if (!_consoleEnabled){
		return;
	}
	
	try {

		var funcName = "";

		if (consoleD.caller && consoleD.caller.name) { // Support this
			funcName = consoleD.caller.name; 
		} else { // Not support - do it by Error
			funcName = getCallerName();
		}

        if (message != "") {
            if (funcName != "") {
                message = funcName + ": " + message;
            }
        } else if (funcName != "") {
            message = funcName;
        }
    
    } catch (error) {
         message = "(undef.err): " + message;
	}
	
	// Show in console

	if (message != "") {
		console.debug("%c(D)" + message, "color: rgb(60, 179, 113)");
	}
}

function getCallerName(){

	// Get a caller name by a stack trace

	const error = new Error();
	const rows = error.stack.split('\n');

	if (isFirefox) {
		var caller = rows[3].replace(new RegExp("@.*"), " ").trim();
		return (caller == 'j')?'(undef.)':caller;
	} else {
		// TODO: do it
	}
}

///// End