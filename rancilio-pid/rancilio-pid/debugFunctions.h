#ifndef debugFunctions_h
#define debugFunctions_h

#if (DEBUGMETHOD == 0) // NONE
	#define DEBUG_println(a)
	#define DEBUG_print(a)
	#define DEBUGSTART(a)

	#define DEBUG_printfV(...)
	#define DEBUG_printfD(...)
	#define DEBUG_printfI(...)
	#define DEBUG_printfW(...)
	#define DEBUG_printfE(...)
#endif


/* Hierarchie der Debugmodi:
      
       wenig Ausgabe: E < W < I < D < V viel Ausgabe  
  
   V - Regelmaessige Ausgabe des Maschinenzustands (machinestate, temp, ...)
   D - Ausgaben zu aktuellen Entwicklungen, kommen wieder raus
   I - Infos zu Machinestate, Schalter an / aus, ...
   W - Warnungen
   E - Fehler
*/

#if (DEBUGMETHOD == 1 ) // Serial
	#include "SerialDebug.h" // Debug via seral bus

	// Standard serieller Bus fuer setup()
	#define DEBUG_println(a)  Serial.println(a);
	#define DEBUG_print(a)    Serial.print(a);
	#define DEBUGSTART(a)     Serial.begin(a);

	#define DEBUG_AUTO_FUNC_DISABLED true
	#define DEBUG_INITIAL_LEVEL DEBUG_LEVEL_DEBUG

	#define DEBUG_printfV(...) debugV(__VA_ARGS__);
	#define DEBUG_printfD(...) debugD(__VA_ARGS__);
	#define DEBUG_printfI(...) debugI(__VA_ARGS__);
	#define DEBUG_printfW(...) debugW(__VA_ARGS__);
	#define DEBUG_printfE(...) debugE(__VA_ARGS__);

#endif


#if (DEBUGMETHOD == 2) // RemoteDebug
	#include "RemoteDebug.h" // Debug via wifi

	// Standard serieller Bus fuer setup()
	#define DEBUG_println(a)  Serial.println(a);
	#define DEBUG_print(a)    Serial.print(a);
	#define DEBUGSTART(a)     Serial.begin(a);

	#define DEBUG_printfV(...) debugV(__VA_ARGS__);
	#define DEBUG_printfD(...) debugD(__VA_ARGS__);
	#define DEBUG_printfI(...) debugI(__VA_ARGS__);
	#define DEBUG_printfW(...) debugW(__VA_ARGS__);
	#define DEBUG_printfE(...) debugE(__VA_ARGS__);

	RemoteDebug Debug;

#endif

#endif	