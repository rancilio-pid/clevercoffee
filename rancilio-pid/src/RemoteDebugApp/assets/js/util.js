//////// Utilities 

function setEnable(elem, enabled) {

	// Enable/disable element

	$(elem).prop('disabled', !enabled);
}

function setImgSrc(id, src) {

	// Set image source, in vanilla due no working in jQuery

	var elem = document.getElementById(id);
	// log("id=" + id + " vis=" + visible + " elem=" + elem);

	if (elem) {
		elem.src = src;
	}

}

function setText(id, value) {
	
	// Set element text 
	
	var elem = document.getElementById(id);
	if (elem != null) {
		elem.innerText = value;
	}
}

function setHtml(id, value) {
	
	// Set element html 
	
	var elem = document.getElementById(id);
	if (elem != null) {
		elem.innerHTML = value;
	}
}

function setFocus (id) {

    // Set focus (2x to avoid problems)

    $("#" + id).focus(); // jQuery
    document.getElementById(id).focus(); // DOM
}

function setVisible (id, visible) {

	// Show/hide elem, in vanilla, due no working in jQuery

	var elem = document.getElementById(id);
	// consoleL("id=" + id + " vis=" + visible + " elem=" + elem);

	if (elem) {
        elem.style.visibility = (!visible)?'hidden':'visible';
	}
}

function setDisplay (id, display, grid) {

	// Show/hide elem, in vanilla, due no working in jQuery

	var elem = document.getElementById(id);
    // consoleL("id=" + id + " disp=" + display + " grid=" + grid + " elem=" + elem);

	if (elem) {
        elem.style.display = (!display)?'none':(grid)?'grid':'block';
	}
}

// Cookies

function createCookie(name, value, days) {
    if (days) {
        var date = new Date();
        date.setTime(date.getTime() + (days * 24 * 60 * 60 * 1000));
        var expires = "; expires=" + date.toGMTString();
    }
    else var expires = "";               

    document.cookie = name + "=" + value + expires + "; path=/";
}

function readCookie(name) {
    var nameEQ = name + "=";
    var ca = document.cookie.split(';');
    for (var i = 0; i < ca.length; i++) {
        var c = ca[i];
        while (c.charAt(0) == ' ') c = c.substring(1, c.length);
        if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length, c.length);
    }
    return null;
}

function eraseCookie(name) {
    createCookie(name, "", -1);
}


///// Snackbar - saw it in: https://www.w3schools.com/howto/howto_js_snackbar.asp

function snackbarHide() {

    var div = document.getElementById("snackbar");
    if (!div) {
        return;
    }

    div.className = div.className.replace("show", "");
    setHtml("span_snackbar_message", "");

}

function snackbar(message, timeout) {

    var div = document.getElementById("snackbar");
    if (!div) {
        return;
    }

    if (message == "") {
        snackbarHide();
        return;
    }

    // Show

    setHtml("span_snackbar_message", message);
    div.className = "show";
    
    // Timeout
    
    if (!timeout) {
        timeout = 5000;
    }

    if (timeout > 0) {
        setTimeout(function(){ 
            snackbarHide();
         }, timeout);

    }
}

function toast (message, type, timeout) {

    consoleD("");

    // Show a toast (type: success, error, warning, info and question)
    try {
        
        if (Swal) {

            if (!timeout) timeout = 3000;
        
            Swal.fire({
                toast: true,
                html: '<span style=\"color:#FFF\">' + message + '</span>',
                background: '#333',
                type: (type)?type:'info',
                position: 'bottom',
                showConfirmButton: false,
                timer: timeout
              });
      
              
        } else {
            snackbar (message, timeout);
        }

    } catch (error) {
        console.error ("Error -> " + error);        
    }
}

function toastHide() {

    // Hide a toast

    try {
        if (Swal) {
            Swal.close();
        } else {
            snackbarHide();
        }
    } catch (error) {
        console.error ("Error -> " + error);
    }

}

///// Extensions

String.prototype.replaceAllRegExp = function(search, replacement) {
    // Replace all- sow it in: https://stackoverflow.com/questions/1144783/how-to-replace-all-occurrences-of-a-string-in-javascript
    var target = this;
    return target.replace(new RegExp(search, 'g'), replacement);
}
String.prototype.replaceAll = function(search, replacement) {
	// saw it in: https://theburningmonk.com/2016/04/javascript-string-replace-all-without-regex/
    var target = this;
	return target.split(search).join(replacement);
};

/// End