#ifndef SOONER_IEEE_DETECTOR_HPP
#define SOONER_IEEE_DETECTOR_HPP

#include <stdio.h>
#include <vector>

//0x4002 1000 GPIOE start address
#define GPIOE_OSPEEDER (*(volatile unsigned int *)0x40021008)
#define FASTDIG (*(volatile unsigned short *)0x40021010)

#define DEFAULT_SAMPLE 50

class Detector {
	public:
		static int detectMetal(void); // Returns an integer representing the confidence in the presence of metal
	    static int calibrate(int sampleSize = DEFAULT_SAMPLE);
};

#endif