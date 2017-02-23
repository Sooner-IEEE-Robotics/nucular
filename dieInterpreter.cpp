#include "mbed.h"

using namespace std;

//--------Beginning of adjustable parameters---------

//Logistical parameters:
const int IMAGE_WIDTH = 160;
const int IMAGE_HEIGHT = 120;

//Strategic parameters:
const int BLACK_WHITE_THRESHOLD = 90; //This threshold is excellent for the existing sample images. I wouldn't change it.
const int INNER_DOT_RADIUS = 3; //The distance from the center of a dot within which we expect to find black.
const int OUTER_DOT_RADIUS = 7; //The distance from the center of a dot at which we expect to find white.

//---------End of adjustable parameters---------

short imageArray[IMAGE_WIDTH*IMAGE_HEIGHT];
short searchRegionLeftBound = 0; //These bounds can (and probably should) be programatically optimized at runtime.
short searchRegionRightBound = IMAGE_WIDTH - 1;
short searchRegionTopBound = 0;
short searchRegionBottomBound = IMAGE_HEIGHT - 1;

void readImageMatrixIntoArray(short imageMatrix[IMAGE_WIDTH][IMAGE_HEIGHT]);
bool isDotCenter(int y, int x);
void whiteOutDot(int dotCenterY, int dotCenterX);
int countDots(short imageMatrix[IMAGE_WIDTH][IMAGE_HEIGHT]);
int getPixel(int y, int x);
void setPixel(int y, int x, int value);

//Reads pixel data from a 2D matrix into the 1D array called imageArray,
//applying the black/white threshold. Pixels below the threshold are stored
//as 0, and pixels at or above the threshold are stored as 1.
void readImageMatrixIntoArray(short imageMatrix[IMAGE_WIDTH][IMAGE_HEIGHT])
{
	int arrayIndex = 0;

	//For each pixel in the image matrix...
	for(int i = 0; i < IMAGE_HEIGHT; i++)
	{
		for(int j = 0; j < IMAGE_WIDTH; j++)
		{
			//Apply the threshold and fill the appropriate image array entry.
			if(imageMatrix[j][i] < BLACK_WHITE_THRESHOLD)
				imageArray[arrayIndex] = 0;
			else
				imageArray[arrayIndex] = 1;

			arrayIndex++;
		}
	}
}

//Returns true if the given coordinates are at the center of a dot.
//Returns false otherwise.
bool isDotCenter(int y, int x)
{
	//The center of a dot must not be white.
	if(getPixel(y, x) == 1)
		return false;
	//Points on the outer radius of a dot must not be black.
	if(getPixel(y + OUTER_DOT_RADIUS, x) == 0
			|| getPixel(y - OUTER_DOT_RADIUS, x) == 0
			|| getPixel(y, x + OUTER_DOT_RADIUS) == 0
			|| getPixel(y, x - OUTER_DOT_RADIUS) == 0)
		return false;

	//Points on and within the inner radius of a dot must not be white.
	for(int i = 1; i <= INNER_DOT_RADIUS; i++)
	{
		if(getPixel(y + i, x) == 1
					|| getPixel(y - i, x) == 1
					|| getPixel(y, x + i) == 1
					|| getPixel(y, x - i) == 1)
				return false;
	}

	return true;
}

//Takes the coordinates of the center of a dot and whites out the region
//the dot occupies.
void whiteOutDot(int dotCenterY, int dotCenterX)
{
	for(int i = dotCenterY - OUTER_DOT_RADIUS;
			i <= dotCenterY + OUTER_DOT_RADIUS; i++)
	{
		for(int j = dotCenterX - OUTER_DOT_RADIUS;
				j <= dotCenterX + OUTER_DOT_RADIUS; j++)
		{
			setPixel(i, j, 1);
		}
	}
}

//Returns the number of dots on the die in the given image matrix.
int countDots(short imageMatrix[IMAGE_WIDTH][IMAGE_HEIGHT])
{
	//Store the 2D image matrix in our 1D array, applying the black/white threshold.
	readImageMatrixIntoArray(imageMatrix);

	int dotsCounted = 0;

	for(int i = searchRegionTopBound + OUTER_DOT_RADIUS;
			i <= searchRegionBottomBound - OUTER_DOT_RADIUS; i++)
	{
		for(int j = searchRegionLeftBound + OUTER_DOT_RADIUS;
				j <= searchRegionRightBound - OUTER_DOT_RADIUS; j++)
		{
			if(isDotCenter(i, j))
			{
				whiteOutDot(i, j);
				dotsCounted++;
			}
		}
	}

	return dotsCounted;
}

//Returns the pixel at given coordinates in the imageArray.
int getPixel(int y, int x)
{
	return imageArray[(x + IMAGE_WIDTH*y)];
}

//Sets the pixel at given coordinates in the imageArray.
//Acceptable pixel values are 0 for black and 1 for white.
void setPixel(int y, int x, int value)
{
	imageArray[(x + IMAGE_WIDTH*y)] = value;
}
