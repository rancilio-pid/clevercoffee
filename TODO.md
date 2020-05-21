# TODO
- re-test blynk QR Code (Apple?)
- Implement an automatic "steam mode".
- mqtt events/ should deliver json not string.
- String replacement (https://cpp4arduino.com/2020/02/07/how-to-format-strings-without-the-string-class.html)
- convert all float to lu.


# TODO Tests

# Quick Performance tests 
- mqtt(): <2ms
  sendToBlynk(): 70ms if at least 2 virtualWrite() are called. -> After fix: 12ms.
                 12ms if only one virtualWrite() is called.
                 80-100ms per transferred attribute (virtualWrite+syncVirtual)
- convert float/double to smaller bytes to improve performance (sprintf of float takes 0.3ms)
