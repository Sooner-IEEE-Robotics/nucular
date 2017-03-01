#ifndef SOONER_IEEE_DETECTOR_HPP
#define SOONER_IEEE_DETECTOR_HPP

#include <stdio.h>
#include <vector>

class Detector {
	private:
	    static int calibrate(void);
	public:
		static int detectMetal(void); // Returns an integer representing the confidence in the presence of metal
};

#endif