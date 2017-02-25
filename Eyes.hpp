#include <stdio.h>
#include <opencv3/opencv.hpp>

class Eyes {
	private:
		static unsigned filteredNumberOfCircles(vector<cv::Vec3f> circles);
	public:
		static unsigned numberOfDots(void);
}