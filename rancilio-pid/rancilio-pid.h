#ifndef RancilioPid_h
#define RancilioPid_h

#define LIBRARY_VERSION	0.0.1

//returns heater utilization in percent
double convertOutputToUtilisation(double);

//returns heater utilization in Output
double convertUtilisationToOutput(double);

double pastTemperatureChange(int);

bool almostEqual(float, float);

#endif
