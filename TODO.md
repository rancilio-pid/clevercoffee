# TODO
- re-test blynk QR Code (Apple?)
- Add control buttons
- screensaver
- LED stripe support
- Implement an automatic "steam mode".
- mqtt events/ should deliver json not string.
- String replacement (https://cpp4arduino.com/2020/02/07/how-to-format-strings-without-the-string-class.html)
- convert all float to lu.
- Add telnet commands: https://github.com/JoaoLopesF/RemoteDebug/blob/7812322cc1724dd0e4c139f7353149ec8a96b4b9/examples/RemoteDebug_Advanced/RemoteDebug_Advanced.ino


# TODO Tests

# Quick Performance tests 
- mqtt(): <2ms
  sendToBlynk(): 70ms if at least 2 virtualWrite() are called. -> After fix: 12ms.
                 12ms if only one virtualWrite() is called.
                 80-100ms per transferred attribute (virtualWrite+syncVirtual)
- convert float/double to smaller bytes to improve performance (sprintf of float takes 0.3ms)
