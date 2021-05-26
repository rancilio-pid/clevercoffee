#include "Logbook.h"

Logbook::Logbook() 
{
	line = 0; 
	id_first = 0; 
	id_last = 0;
	length = 0;
}

int Logbook::first() { return id_first; }
int Logbook::last()  { return id_last; }
int Logbook::len()   { return length; }
