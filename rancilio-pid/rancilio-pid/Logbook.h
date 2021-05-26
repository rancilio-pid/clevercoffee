#ifndef Logbook_h
#define Logbook_h

#include "Arduino.h"

#define MAXLOGLINES 500

class Logbook {

  public:
  	Logbook();

  	int append(String s)
  	{
  		int line_ = line;
  		log[line] = s;
  		id_last = line;

  		line ++;

  		if (length < maxlines) { length ++; }
  		if (line == maxlines)  { line = 0; }

  		id_first = (length == maxlines) ? (id_last + 1) % (length-1) : 0;
  		
  		return line_;
  	}

  	int first();
  	int last();
  	int len();

  	String getline( int i )
  	{
  		int id = (id_first + i) % (length);
  		return log[id];
  	}

  private:
  	const int maxlines = MAXLOGLINES;
    int line,id_first,id_last,length;
    
    String log[MAXLOGLINES];

};


#endif

