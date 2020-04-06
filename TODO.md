# TODO
- mqtt events/ should deliver json not string.
- Fix TSIC delays.

# TODO Tests

# Quick Performance tests 2.0.3
- Online+mqtt+blynk: loop() is called every 4ms, with 200-500ms peaks every sec
  refreshTemp(): ~55ms
  isr(): 15ms
  mqtt(): <2ms
  sendToBlynk(): 70ms if at least 2 virtualWrite() are called.
                 12ms if only one virtualWrite() is called.
                 80-100ms per transferred attribute (virtualWrite+syncVirtual)
- convert float/double to smaller bytes to improve performance (sprintf of float takes 0.3ms)
