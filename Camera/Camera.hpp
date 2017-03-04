#ifndef SOONER_IEEE_CAMERA_HPP
#define SOONER_IEEE_CAMERA_HPP

#include <stdio.h>
#include <vector>

class Camera {
	private:
		static int hasDoneSetup;
	    static void setup(void);
	public:
		static std::vector<std::vector<short> > takePicture(void);
};

#endif