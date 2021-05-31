#ifndef Logbook_h
#define Logbook_h

#include "Arduino.h"
#include "userConfig.h"

/*
  line:  from 1 ... MAXLOGLINES
  index: from 0 ... MAXLOGLINES-1
         steht immer auf dem nächsten zu beschreibenden Eintrag
*/

class Logbook {

  public:
    Logbook();

    int append(String s)
    {
      // Serial.println(index);
      // Serial.println(s);
      // Serial.println(index_first);
      // Serial.println(index_last);
      // Serial.println(maxlines);
      // Serial.println(length);
      // Serial.println("");

      if (maxlines <= 0) return 0;

      log[index] = s;

      // Laenge anpassen
      if (length<maxlines) length++;

      // "Zeiger" aktualisieren
      if (length < maxlines)
      {
        // index_first bleibt auf 0
        index_last = index;
        index++;
      }
      else
      {
        index_last  = index;
        index_first = (index+1)%maxlines;
        index = index_first;
      }

      return index;
    }

    int len();

    String getline( int line )
    {
      if (line<1 || line>maxlines)
      {
        // Serial.println("*** Error: Logbook::getline() index out of range!");
        return "*** Error: Logbook::getline() index out of range!";
      }

      int index = (index_first + line-1) % (length);
      return log[index];
    }

  private:
    const int maxlines = MAXLOGLINES;
    int index,index_first,index_last,length;
    
    String log[MAXLOGLINES];

};

#endif
