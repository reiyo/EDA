#include <cstdlib>
#include <fstream>
#include <iostream>

#include "util.h"

using namespace std;

double getCurrentMemoryUsage()
{
    ifstream statusFile("/proc/self/status");
    float vmSize;
    string line;

    while(getline(statusFile, line)) 
    {
	if(sscanf(line.c_str(), "VmSize: %f kB", &vmSize) == 1)
	    return vmSize/1024; // return in MB
    }

    return 0;
}

double getPeakMemoryUsage()
{
    ifstream statusFile("/proc/self/status");
    float vmPeak;
    string line;

    while(getline(statusFile, line)) 
    {
	if(sscanf(line.c_str(), "VmPeak: %f kB", &vmPeak) == 1)
	    return vmPeak/1024; // return in MB
    }

    return 0;
}


