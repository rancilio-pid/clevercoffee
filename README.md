# ranciliopid
Rancilio-Silvia PID für Arduino http://rancilio-pid.de

BETA VERSION

Version 1.8.0 (09.06.2019)

WICHTIG: ÄNDERUNG VOM PID Betrieb!

1) Umrechnung der PID WERTE
P bleibt
I_NEU = P / I_ALT
D_NEU = D_NEU / P 

2) PID wurde geändert, damit Schwankungen minimiert werden und stabiler läuft.
Daher müssen ggf. auch umgerechnete Werte angepasst werden.
Aktuell kann mit
P: 25
I: 166
ein Betrieb an der Rancilio ermöglicht werden. 
Optimale Werte müssen noch gefunden werden. 

