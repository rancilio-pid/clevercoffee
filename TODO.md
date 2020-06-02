# TODO
- re-test blynk QR Code (Apple?)
- Add control buttons
- screensaver
- LED stripe support
- Implement an automatic "steam mode".
- Validate and add https://github.com/lebuni/ZACwire-Library
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

# Sample brew detection
(D p:^0004ms) 970 Input= 94.28 | error=-0.28 delta= 0.00 | Output=  4.95 b: 5.60 + p:-0.45 + i:-0.20(-0.20) + d:-0.00
(D p:^4954ms) 975 Brew Detect: prev(5)=94.20 past(3)=-1.30 past(5)=-1.30 | Avg(3)=93.67 | Avg(10)=94.07 Avg(2)=93.40
(D p:^0003ms) 975 ** End of normal mode. Transition to step 4 (brew)
(D p:^0007ms) 975 brew temp: t(0)=92.90
(D p:^0031ms) 975 Input= 93.88 | error= 0.12 delta=-0.70 | Output=  5.60 b: 5.60 + p: 0.19 + i: 0.00( 0.09) + d: 7.17
(D p:^0968ms) 976 brew temp: t(0)=91.80
(D p:^1000ms) 977 brew temp: t(0)=90.90
(D p:^0999ms) 978 brew temp: t(0)=90.20
(D p:^0999ms) 979 brew temp: t(0)=89.70
(D p:^1000ms) 980 brew temp: t(0)=89.50
(D p:^0030ms) 980 Input= 89.96 | error= 4.04 delta=-2.35 | Output= 90.00 b: 5.60 + p: 6.46 + i: 2.94( 2.94) + d:24.06
(D p:^0968ms) 981 brew temp: t(0)=89.40
(D p:^1000ms) 982 brew temp: t(0)=89.30
(D p:^0999ms) 983 brew temp: t(0)=89.30
(D p:^0999ms) 984 brew temp: t(0)=89.20
(D p:^1001ms) 985 brew temp: t(0)=89.20
(D p:^0029ms) 985 Input= 89.24 | error= 4.76 delta=-0.85 | Output= 90.00 b: 5.60 + p: 7.62 + i: 6.40( 3.46) + d: 8.70
(D p:^0968ms) 986 brew temp: t(0)=89.10
(D p:^1000ms) 987 brew temp: t(0)=89.10
(D p:^0999ms) 988 brew temp: t(0)=89.20
(D p:^0999ms) 989 brew temp: t(0)=89.20
(D p:^0999ms) 990 brew temp: t(0)=89.20
(D p:^0029ms) 990 Input= 89.18 | error= 4.82 delta=-0.05 | Output= 90.00 b: 5.60 + p: 7.71 + i: 9.91( 3.51) + d: 0.51
(D p:^0968ms) 991 brew temp: t(0)=89.20
(D p:^0999ms) 992 brew temp: t(0)=89.30
(D p:^1000ms) 993 brew temp: t(0)=89.30
(D p:^0999ms) 994 brew temp: t(0)=89.30
(D p:^0999ms) 995 brew temp: t(0)=89.30
(D p:^0031ms) 995 Input= 89.32 | error= 4.68 delta= 0.15 | Output= 90.00 b: 5.60 + p: 7.49 + i:13.31( 3.40) + d:-1.54
(D p:^0968ms) 996 brew temp: t(0)=89.40
(D p:^1000ms) 997 brew temp: t(0)=89.50
(D p:^0999ms) 998 brew temp: t(0)=89.60
(D p:^1000ms) 999 brew temp: t(0)=89.70
(D p:^0999ms) 1000 brew temp: t(0)=89.80
(D p:^0031ms) 1000 Input= 89.66 | error= 4.34 delta= 0.25 | Output= 90.00 b: 5.60 + p: 6.94 + i:16.47( 3.16) + d:-2.56
(D p:^0969ms) 1001 brew temp: t(0)=89.90
(D p:^0999ms) 1002 brew temp: t(0)=90.00
(D p:^1000ms) 1003 brew temp: t(0)=90.10
(D p:^0999ms) 1004 brew temp: t(0)=90.30
(D p:^0999ms) 1005 brew temp: t(0)=90.40
(D p:^0032ms) 1005 Input= 90.20 | error= 3.80 delta= 0.45 | Output= 90.00 b: 5.60 + p: 6.08 + i:19.23( 2.76) + d:-4.61
(D p:^0968ms) 1006 brew temp: t(0)=90.50
(D p:^1000ms) 1007 brew temp: t(0)=90.60
(D p:^0999ms) 1008 brew temp: t(0)=90.70
(D p:^0999ms) 1009 brew temp: t(0)=90.80
(D p:^0999ms) 1010 brew temp: t(0)=90.90
(D p:^0032ms) 1010 Input= 90.76 | error= 3.24 delta= 0.45 | Output= 90.00 b: 5.60 + p: 5.18 + i:21.59( 2.36) + d:-4.61
(D p:^0968ms) 1011 brew temp: t(0)=91.00
(D p:^0994ms) 1012 Out Zone Detection: past(2)=0.20, past(3)=0.30 | past(5)=0.40 | past(10)=0.90 | bezugsZeit=40
(D p:^0003ms) 1012 t(0)=91.10 | t(1)=91.00 | t(2)=90.90 | t(3)=90.80 | t(5)=90.70 | t(10)=90.50 | t(13)=90.20
(D p:^0008ms) 1012 ** End of Brew. Transition to step 2 (constant steadyPower)
(D p:^3026ms) 1015 Input= 91.20 | error= 2.80 delta= 0.45 | Output=  5.60 b: 5.60 + p: 4.48 + i: 2.04( 2.04) + d:-4.61
