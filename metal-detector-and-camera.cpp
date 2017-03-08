/*
THIS PROGRAM IS THE COMINATION OF THE CAMERA FUNCTIONS AND THE METAL DETECTOR FUNCTIONS
WHEN UPLOADED, THIS PROGRAM WILL WAIT 10 SECONDS AND THEN ACTIVATE THE METAL DETECTOR. SOON AFTERWARD,
IT WILL SEND THE MEASURED VALUE OVER USB.
*/


//Before compilation, the preprocessor will paste in a bunch of useful classes from mbed.h.
#include "mbed.h"
#include <stdlib.h>


//Define necessary pins for communicatation with the metal detector.
PinName metalDetectorWritePin = PF_13;
PinName metalDetectorReadPin = PE_9;
PinName test = PE_11;
//Instantiate a DigitalOut object called metalDetectorWriter.
//This object can send digital signals to the metal detector to turn it on or off.
DigitalOut metalDetectorWriter(metalDetectorWritePin);
DigitalIn t(test);
//Instantiate a DigitalIn object called metalDetectorReader.
//This object can read digital signals from the metal detector.
//DigitalIn metalDetectorReader(metalDetectorReadPin);

//Instantiate an object to communicate with the computer via USB,
Serial outputToComputer(USBTX, USBRX);
BusOut led(LED1, LED2, LED3);  //controls leds
                               //0x4002 1000 GPIOE start address
#define GPIOE_OSPEEDER (*(volatile unsigned int *)0x40021008)
#define FASTDIG (*(volatile unsigned short *)0x40021010)

                               /*****************************************************************************
                               camera header starts here
                               ******************************************************************************/
                               //#include "mbed.h"


                               //read for i2c is 0x61, write is 0x60

#define MAX_FIFO_SIZE       0x5FFFF
#define RWBIT                                   0x80  //READ AND WRITE BIT IS BIT[7] FOR SPI
#define ARDUCHIP_TEST1          0x00  //TEST register
#define ARDUCHIP_MODE           0x02  //Mode register
#define MCU2LCD_MODE            0x00
#define CAM2LCD_MODE            0x01
#define LCD2MCU_MODE            0x02
#define ARDUCHIP_FIFO           0x04  //FIFO and I2C control
#define FIFO_CLEAR_MASK         0x01
#define FIFO_START_MASK         0x02
#define FIFO_RDPTR_RST_MASK     0x10
#define FIFO_WRPTR_RST_MASK     0x20
#define BURST_FIFO_READ         0x3C  //Burst FIFO read operation
#define SINGLE_FIFO_READ        0x3D  //Single FIFO read operation

#define ARDUCHIP_REV            0x40  //ArduCHIP revision
#define VER_LOW_MASK            0x3F
#define VER_HIGH_MASK           0xC0

#define ARDUCHIP_TRIG           0x41  //Trigger source
#define VSYNC_MASK              0x01
#define SHUTTER_MASK            0x02
#define CAP_DONE_MASK           0x08
#define OV2640_CHIPID_HIGH  0x0A
#define OV2640_CHIPID_LOW   0x0B
using namespace std;
Serial serial(USBTX, USBRX);

//--------Beginning of camera adjustable parameters---------

//Logistical parameters:
const int IMAGE_WIDTH = 160;
const int IMAGE_HEIGHT = 120;

//Strategic parameters:
const int BLACK_WHITE_THRESHOLD = 80; //This should be perfected when the LED is mounted under the robot and we see what
                                      //the lighting will actually be like. For Trevor's early sample images, 90 was definitely best.
                                      //For subsequent sample images, 80 has been effective, though we have not seriously looked for an
                                      //optimal value. 
const int INNER_DOT_RADIUS = 2; //The distance from the center of a dot within which we expect to find black.
                                //I believe 2 is and will remain optimal.
const int OUTER_DOT_RADIUS = 9; //The distance from the center of a dot at which we expect to find white.
                                //For Trevor's early sample images, 7 was superior, though consistently correct results could be achieved with a
                                //variety of values. This parameter should definitely be altered once the camera is mounted and we get a feel for
                                //the distance of the dots.

                                //---------End of camera adjustable parameters---------


                                //The 1D array into which image data is read.
                                //This is where the countDots() function looks for the data.
short imageArray[IMAGE_WIDTH*IMAGE_HEIGHT];

//These 4 variables give us the option to limit the scope of our search.
//If speed is an issue, they can be programmatically optimized at runtime 
//based on the first occurrence of white in each direction.
short searchRegionLeftBound = 0;
short searchRegionRightBound = IMAGE_WIDTH - 1;
short searchRegionTopBound = 0;
short searchRegionBottomBound = IMAGE_HEIGHT - 1;

//I've been told that pre-declaring functions is good practice.
void applyBlackWhiteThreshold();
bool isDotCenter(int y, int x);
void whiteOutDot(int dotCenterY, int dotCenterX);
int countDots();
int getPixel(int y, int x);
void setPixel(int y, int x, int value);

//Applies the BLACK_WHITE_THRESHOLD to the pixels in imageArray.
void applyBlackWhiteThreshold()
{
    for (int i = 0; i < IMAGE_HEIGHT; i++)
    {
        for (int j = 0; j < IMAGE_WIDTH; j++)
        {
            if (getPixel(i, j) > BLACK_WHITE_THRESHOLD)
            {
                setPixel(i, j, 1);
            }
            else {
                setPixel(i, j, 0);
            }
        }
    }
}

//Returns true if the given coordinates are at the center of a dot.
//Returns false otherwise.
bool isDotCenter(int y, int x)
{
    //The center of a dot must not be white.
    if (getPixel(y, x) == 1)
        return false;

    //Points on the outer radius of a dot must not be black.
    if (getPixel(y + OUTER_DOT_RADIUS, x) == 0
        || getPixel(y - OUTER_DOT_RADIUS, x) == 0
        || getPixel(y, x + OUTER_DOT_RADIUS) == 0
        || getPixel(y, x - OUTER_DOT_RADIUS) == 0)
        return false;

    //Points on and within the inner radius of a dot must not be white.
    for (int i = 1; i <= INNER_DOT_RADIUS; i++)
    {
        if (getPixel(y + i, x) == 1
            || getPixel(y - i, x) == 1
            || getPixel(y, x + i) == 1
            || getPixel(y, x - i) == 1)
            return false;
    }

    //This point has passed our checks. It must be a dot center.
    return true;
}

//Takes the coordinates of the center of a dot and whites out the region
//the dot occupies.
void whiteOutDot(int dotCenterY, int dotCenterX)
{
    for (int i = dotCenterY - OUTER_DOT_RADIUS;
        i <= dotCenterY + OUTER_DOT_RADIUS; i++)
    {
        for (int j = dotCenterX - OUTER_DOT_RADIUS;
            j <= dotCenterX + OUTER_DOT_RADIUS; j++)
        {
            setPixel(i, j, 1);
        }
    }
}

//Returns the number of dots on the die whose image data is contained in imageArray.
//For now, this is willing to return a count over 6, which will be useful for testing.
//Before competition, we should force it to return 6 the moment it counts the sixth dot.
//BE AWARE: THIS FUNCTION APPLIES A THRESHOLD TO imageArray, ALTERING IT PERMANENTLY.
int countDots()
{
    applyBlackWhiteThreshold();

    int dotsCounted = 0;

    for (int i = searchRegionTopBound + OUTER_DOT_RADIUS;
        i <= searchRegionBottomBound - OUTER_DOT_RADIUS; i++)
    {
        for (int j = searchRegionLeftBound + OUTER_DOT_RADIUS;
            j <= searchRegionRightBound - OUTER_DOT_RADIUS; j++)
        {
            if (isDotCenter(i, j))
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

I2C i2c(PB_9, PB_8);       //sda, scl
SPI spi(PB_5, PB_4, PA_5); // mosi, miso, sclk
short image[160][120];     //unused here
int jpeg = 238 - 48;       //too lazy to actually math
DigitalOut cs(PA_4);       // chip enable pin for SPI
char YUV422[] =    //20 bits; sets to YUV mode
{
    0xFF, 0x00 ,
    0x05, 0x00 ,
    0xDA, 0x10 ,
    0xD7, 0x03 ,
    0xDF, 0x00 ,
    0x33, 0x80 ,
    0x3C, 0x40 ,
    0xe1, 0x77 ,
    0x00, 0x00 ,
    0xff, 0xff
};

char JPEG_INIT[] =     //190; configures as jpeg
{ 0xff, 0x00 ,
0x2c, 0xff ,
0x2e, 0xdf ,
0xff, 0x01 ,
0x3c, 0x32 ,
0x11, 0x00 ,
0x09, 0x02 ,
0x04, 0x28 ,
0x13, 0xe5 ,
0x14, 0x48 ,
0x2c, 0x0c ,
0x33, 0x78 ,
0x3a, 0x33 ,
0x3b, 0xfB ,
0x3e, 0x00 ,
0x43, 0x11 ,
0x16, 0x10 ,
0x39, 0x92 ,
0x35, 0xda ,
0x22, 0x1a ,
0x37, 0xc3 ,
0x23, 0x00 ,
0x34, 0xc0 ,
0x36, 0x1a ,
0x06, 0x88 ,
0x07, 0xc0 ,
0x0d, 0x87 ,
0x0e, 0x41 ,
0x4c, 0x00 ,
0x48, 0x00 ,
0x5B, 0x00 ,
0x42, 0x03 ,
0x4a, 0x81 ,
0x21, 0x99 ,
0x24, 0x40 ,
0x25, 0x38 ,
0x26, 0x82 ,
0x5c, 0x00 ,
0x63, 0x00 ,
0x61, 0x70 ,
0x62, 0x80 ,
0x7c, 0x05 ,
0x20, 0x80 ,
0x28, 0x30 ,
0x6c, 0x00 ,
0x6d, 0x80 ,
0x6e, 0x00 ,
0x70, 0x02 ,
0x71, 0x94 ,
0x73, 0xc1 ,
0x12, 0x40 ,
0x17, 0x11 ,
0x18, 0x43 ,
0x19, 0x00 ,
0x1a, 0x4b ,
0x32, 0x09 ,
0x37, 0xc0 ,
0x4f, 0x60 ,
0x50, 0xa8 ,
0x6d, 0x00 ,
0x3d, 0x38 ,
0x46, 0x3f ,
0x4f, 0x60 ,
0x0c, 0x3c ,
0xff, 0x00 ,
0xe5, 0x7f ,
0xf9, 0xc0 ,
0x41, 0x24 ,
0xe0, 0x14 ,
0x76, 0xff ,
0x33, 0xa0 ,
0x42, 0x20 ,
0x43, 0x18 ,
0x4c, 0x00 ,
0x87, 0xd5 ,
0x88, 0x3f ,
0xd7, 0x03 ,
0xd9, 0x10 ,
0xd3, 0x82 ,
0xc8, 0x08 ,
0xc9, 0x80 ,
0x7c, 0x00 ,
0x7d, 0x00 ,
0x7c, 0x03 ,
0x7d, 0x48 ,
0x7d, 0x48 ,
0x7c, 0x08 ,
0x7d, 0x20 ,
0x7d, 0x10 ,
0x7d, 0x0e ,
0x90, 0x00 ,
0x91, 0x0e ,
0x91, 0x1a ,
0x91, 0x31 ,
0x91, 0x5a ,
0x91, 0x69 ,
0x91, 0x75 ,
0x91, 0x7e ,
0x91, 0x88 ,
0x91, 0x8f ,
0x91, 0x96 ,
0x91, 0xa3 ,
0x91, 0xaf ,
0x91, 0xc4 ,
0x91, 0xd7 ,
0x91, 0xe8 ,
0x91, 0x20 ,
0x92, 0x00 ,
0x93, 0x06 ,
0x93, 0xe3 ,
0x93, 0x05 ,
0x93, 0x05 ,
0x93, 0x00 ,
0x93, 0x04 ,
0x93, 0x00 ,
0x93, 0x00 ,
0x93, 0x00 ,
0x93, 0x00 ,
0x93, 0x00 ,
0x93, 0x00 ,
0x93, 0x00 ,
0x96, 0x00 ,
0x97, 0x08 ,
0x97, 0x19 ,
0x97, 0x02 ,
0x97, 0x0c ,
0x97, 0x24 ,
0x97, 0x30 ,
0x97, 0x28 ,
0x97, 0x26 ,
0x97, 0x02 ,
0x97, 0x98 ,
0x97, 0x80 ,
0x97, 0x00 ,
0x97, 0x00 ,
0xc3, 0xed ,
0xa4, 0x00 ,
0xa8, 0x00 ,
0xc5, 0x11 ,
0xc6, 0x51 ,
0xbf, 0x80 ,
0xc7, 0x10 ,
0xb6, 0x66 ,
0xb8, 0xA5 ,
0xb7, 0x64 ,
0xb9, 0x7C ,
0xb3, 0xaf ,
0xb4, 0x97 ,
0xb5, 0xFF ,
0xb0, 0xC5 ,
0xb1, 0x94 ,
0xb2, 0x0f ,
0xc4, 0x5c ,
0xc0, 0x64 ,
0xc1, 0x4B ,
0x8c, 0x00 ,
0x86, 0x3D ,
0x50, 0x00 ,
0x51, 0xC8 ,
0x52, 0x96 ,
0x53, 0x00 ,
0x54, 0x00 ,
0x55, 0x00 ,
0x5a, 0xC8 ,
0x5b, 0x96 ,
0x5c, 0x00 ,
0xd3, 0x00 ,   //{ 0xd3, 0x7f },
0xc3, 0xed ,
0x7f, 0x00 ,
0xda, 0x00 ,
0xe5, 0x1f ,
0xe1, 0x67 ,
0xe0, 0x00 ,
0xdd, 0x7f ,
0x05, 0x00 ,
0x12, 0x40 ,
0xd3, 0x04 ,   //{ 0xd3, 0x7f },
0xc0, 0x16 ,
0xC1, 0x12 ,
0x8c, 0x00 ,
0x86, 0x3d ,
0x50, 0x00 ,
0x51, 0x2C ,
0x52, 0x24 ,
0x53, 0x00 ,
0x54, 0x00 ,
0x55, 0x00 ,
0x5A, 0x2c ,
0x5b, 0x24 ,
0x5c, 0x00 ,
0xff, 0xff ,
};    /* JPG 160x120 */
char size_JPEG[] =     //39; JPEG size configuration
{ 0xff, 0x01 ,
0x12, 0x40 ,
0x17, 0x11 ,
0x18, 0x43 ,
0x19, 0x00 ,
0x1a, 0x4b ,
0x32, 0x09 ,
0x4f, 0xca ,
0x50, 0xa8 ,
0x5a, 0x23 ,
0x6d, 0x00 ,
0x39, 0x12 ,
0x35, 0xda ,
0x22, 0x1a ,
0x37, 0xc3 ,
0x23, 0x00 ,
0x34, 0xc0 ,
0x36, 0x1a ,
0x06, 0x88 ,
0x07, 0xc0 ,
0x0d, 0x87 ,
0x0e, 0x41 ,
0x4c, 0x00 ,
0xff, 0x00 ,
0xe0, 0x04 ,
0xc0, 0x64 ,
0xc1, 0x4b ,
0x86, 0x35 ,
0x50, 0x92 ,
0x51, 0xc8 ,
0x52, 0x96 ,
0x53, 0x00 ,
0x54, 0x00 ,
0x55, 0x00 ,
0x57, 0x00 ,
0x5a, 0x28 ,
0x5b, 0x1e ,
0x5c, 0x00 ,
0xe0, 0x00 ,
0xff, 0xff ,
};




void setup() {//read for i2c is 0x61, write is 0x60
    char * point;            // is this here for a reason?
    for (int count = 0; count < 20; count += 2) {//switches to YUV
        i2c.write(0x60, &YUV422[count], 2);
    }
    for (int count = 0; count < 380; count += 2) {//switches to JPEG
        i2c.write(0x60, &JPEG_INIT[count], 2);
    }
    for (int count = 0; count < 78; count += 2)
    {
        i2c.write(0x60, &size_JPEG[count], 2);
    }//switches to small size JPEG
    cs = 0; spi.write(0x00); spi.write(0x00);  cs = 1;//wakes up the SPI 
}

void start() {
    int spiTest = 0;

    cs = 0; spi.write(0x00 | RWBIT); spi.write(0xff); cs = 1;               //read/write from SPI to test configuration
    cs = 0; spi.write(0x00);  spiTest = spi.write(0x00); cs = 1;
    wait_ms(1);
    if (spiTest != 0xff)                                                     //error handling
    {
        serial.printf("SPI is broken af\r\n");
    }
    if (spiTest == 0xff)
    {
        serial.printf("SPI is totally working\r\n");
    }

    int data = 0;
    cs = 0; spi.write(ARDUCHIP_FIFO | RWBIT); spi.write(FIFO_CLEAR_MASK | FIFO_WRPTR_RST_MASK); cs = 1;//resets fifo position, clears fifo memory
    wait_us(50);        //lets things happen
                        //cs = 0; spi.write(ARDUCHIP_FIFO | RWBIT); spi.write(FIFO_WRPTR_RST_MASK);cs = 1;
    cs = 0; spi.write(ARDUCHIP_FIFO | RWBIT); spi.write(FIFO_START_MASK); cs = 1;            //tells the camera to begin capturing



    while (data == 0)    //waits until camera is done taking the picture, should take a second or two at most
    {
        cs = 0; spi.write(ARDUCHIP_TRIG); data = spi.write(0x00) & CAP_DONE_MASK; cs = 1;
        serial.printf("DONE?: %d\r\n", data);
        wait(1);   //keeps from spamming the serial port
    }
}

//Writes pixel data from the camera into imageArray.
void writePixelDataToImageArray() {
    int data = 0;
    for (int count = 0; count < 19200; count++) {                             //reads out every other pixel in order to capture monochrome b/w image only
        cs = 0; spi.write(SINGLE_FIFO_READ); data = spi.write(0x00); cs = 1;
        cs = 0; spi.write(SINGLE_FIFO_READ); spi.write(0x00); cs = 1;
        imageArray[count] = data;
    }

    //The above code can be trusted to faithfully write the pixel data to imageArray.        
    //The below code was found to produce a matrix containing flawed pixel data.

}

//Prints the image in imageArray over serial.
//NOTE: This function assumes the threshold has yet to be applied to imageArray, 
//so it should NOT be called after the countDots() function, which permanently thresholds imageArray.
void printImage()
{
    for (int i = 0; i < IMAGE_HEIGHT; i++)
    {
        for (int j = 0; j < IMAGE_WIDTH; j++)
        {
            if (getPixel(i, j) > BLACK_WHITE_THRESHOLD)
            {
                serial.printf("1");
            }
            else {
                serial.printf("0");
            }
        }
        serial.printf("\r\n");
    }
}

// formerly main to CameraWithDotCounting.cpp
int cameraWithDotCounting() {

    serial.printf("Beginning capture.\r\n");
    spi.frequency(100000);                       //setup SPI
    spi.format(8, 0);
    i2c.frequency(100000);                       //setup I2C
    wait(.1);                                    //allows time for things to settle out


    setup();  //tested, probably working         //configures camera
    start();  //tested, probably working         //takes picture
    writePixelDataToImageArray();                //Sends data to imageArray, where it can later be counted by countDots().

                                                 //printImage();                                //Prints via serial the image that has just been read into imageArray.
                                                 //This is only for testing purposes and should be removed before
                                                 //competition.

                                                 //serial.printf("dots: %d",countDots());
    return countDots();
}
/*****************************************************************
            camera ends and metal detector starts
*******************************************************************/

//--------------------adjustable global variables (metal detector)------------------

int calNum = 100;

short check_frequency = 00;
int slope_of_metal_loss = 15;  // how rsignificant the change needs to be to triger a change in is_metal
int mesurment_delay = 100;      // the delay betweensamples in calibrate 
int one = 1000, two = 1000, three = 1000, four = 1000; // a recorf of the last four values with four being the most resent value
int old_one = 1000, old_two = 1000, old_three = 1000, old_four = 1000;
int avg1 = 888, avg2 = 888;
bool last_return = false;
int count = 0;

int aray_size = calNum * 2;
int vals[200];


// ---------------------end of adjustable variables---------------------------------

//Activates the metal detector and returns an integer that represents its confidence in the presence of metal.
int detectMetal() {

    //Define a variable to measure the duration of the metal detector's reply.
    int replyDuration = 0;

    //Activate metal detector.
    metalDetectorWriter.write(1);

    //Wait 1 millisecond.
    wait_ms(1);

    //Deactivate metal detector.
    metalDetectorWriter.write(0);
    wait_us(3);


    //As long as the metalDetector is sending a HIGH signal, continuously increment the reply duration.      
    while ((FASTDIG >> 11)&(1)) {    //rewrote using register access to give faster read times
        replyDuration++;
    }

    //Return the reply duration.
    return replyDuration;
}

int calibrate() {               //calibrates via averaging calNum number of detector reads
    int result = 0;            //assumes no metal at the calibration spot
   int mesurment;                          /*for(int count = 0; count < 1000; count++){
                               detectMetal();
                               }*/
    for (int counter = 0; counter < calNum; counter++) {
        mesurment = detectMetal();
        result += mesurment;
        wait_ms(100);
        vals[counter] = mesurment;
        vals[counter + calNum] = mesurment;
    }
    return result / calNum;
}

bool is_metal(int mesurment) {
    /* the first if statment checks if there is a sharp change,
    the second is ment to prevent false negitives by the reading slow slow changes in the output */

    // takes four samples 100 miliseconds apart


        
   // old_one = old_two;
//    old_two = old_three;
//    old_three = old_four;
//    old_four = one;
//
//
//    one = two;
//    two = three;
//    three = four;
//    four = mesurment; // gives four the most resent value
//    avg1 = (old_one + old_two + old_three + old_four) / 4;
//    avg2 = (one + two + three + four) / 4;

avg1= 0;
avg2=0;
    for (int i = 0; i < aray_size; i++){
        vals[i] = vals[i + 1];
        }
        vals[aray_size - 1] = mesurment;
for (int i = 0; i < aray_size / 2; i++){
    avg1 += vals[i];
    avg2 += vals[i + (aray_size / 2)];
    }
avg1 /= aray_size / 2;
avg2 /= aray_size / 2;

    outputToComputer.printf("avg1: %d \t avg2: %d\t diff: %d\r\n", avg1, avg2, avg2 - avg1);

    // checks for an upward slope in values 
    if ((avg2 - avg1) > slope_of_metal_loss) {
        last_return = true;
    }
    // checks for a downward slope in values 
    else if ((avg1 - avg2) > slope_of_metal_loss) {
        last_return = false;
    }
    
    return last_return;
    

        // adjustes the value to help with the dedredation in metaldetector values  
    //    if( mesurment >= ( ideal_metal_reply - error_range )) {
    //        ideal_metal_reply = avg2;
    //        return true;
    //    }
    //    else return false;



}

bool initiate_camera() {//return true when you want the camera to start

                        // magic
    return false;
}

// return false when you want to stop the detector and true to start

int main() {
    //For debugging purposes, alert the computer that the main method has been called.
    serial.baud(115200);                         //begins communication via USB
    outputToComputer.printf("FOR DEBUG: Main method now running. Detector will activate in 10 seconds.\r\n");
    int calval = calibrate();                   //calibrates metal detector
    short dots;
    short j;

    GPIOE_OSPEEDER |= (1 << 23);                //sets gpio to fastest possible speed supposedly
    GPIOE_OSPEEDER |= (1 << 22);
    int metalDetectorReply = 0;

    while (true) {

        //Someday we can start the measurement on button press, but for now let's just allow 10 seconds and then measure.

        //Get a measurement from the metal detector.
        metalDetectorReply = detectMetal();
     

        //Inform the computer of the metal detector's measurement. Higher means greater likelihood of metal!
        

      wait_ms(mesurment_delay);  // a delay so it can check for a sharp change
if(check_frequency <= j){   
       outputToComputer.printf("\r\n\r\nCalVal: %d   ", calval);
       outputToComputer.printf("Detected:%d\r\n", metalDetectorReply);
        if (is_metal(metalDetectorReply))
        {
            led = 0b111;  //controls leds on the nucleo-------------
            outputToComputer.printf("this is metal\r\n");
// COMMUNICATE HERE
        }
        else {
            led = 000;
            outputToComputer.printf("this is  not metal\r\n");
// COMMUNICATE HERE
        }
 j = 0;
}
j++;
  //      if (initiate_camera()) {
//            dots = cameraWithDotCounting(); // need to print this
// COMMUNICATE HERE
//        }
        
    }
}
