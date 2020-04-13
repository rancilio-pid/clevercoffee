# TODO
- mqtt events/ should deliver json not string.

# TODO Tests

# Quick Performance tests 2.0.3
- Online+mqtt+blynk: loop() is called every 4ms, with 200-500ms peaks every sec
  refreshTemp(): ~70ms -> After fix: 4ms
  pid.compute(): 140 micros (1.6ms with DEBUG_print())
  mqtt(): <2ms
  sendToBlynk(): 70ms if at least 2 virtualWrite() are called. -> After fix: 12ms.
                 12ms if only one virtualWrite() is called.
                 80-100ms per transferred attribute (virtualWrite+syncVirtual)
- convert float/double to smaller bytes to improve performance (sprintf of float takes 0.3ms)
