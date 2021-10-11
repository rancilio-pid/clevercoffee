#include "Logbook.h"

Logbook::Logbook() 
{
	index = 0; 
	index_first = 0; 
	index_last = 0;
	length = 0;
}

int Logbook::len()   { return length; }
