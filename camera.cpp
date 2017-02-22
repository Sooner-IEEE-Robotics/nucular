#include "mbed.h"


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

I2C i2c(PB_9, PB_8);       //sda, scl
SPI spi(PB_5, PB_4, PA_5); // mosi, miso, sclk
// int jpeg 238 - 48;        //too lazy to actually math
DigitalOut cs(PA_4);       // chip enable pin for SPI
static char YUV422[]=    //20 bits; sets to YUV mode
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

static char JPEG_INIT[] =     //190; configures as jpeg
{  0xff, 0x00 ,
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
char size_JPEG[]=     //39; JPEG size configuration
{  0xff, 0x01 ,
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
/*void CS_LOW(void)                                  //unused, borrowed from library for reference purposes only
    {
        cs = 0;
    }
 
    //Disable CS signal
    void CS_HIGH(void)
    {
        cs = 1;
    }
     
    //Reset the FIFO pointer to ZERO        
void flush_fifo(void)
    {
        //write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
    }
     
    //Send capture command
void start_capture(void)
    {
        //write_reg(ARDUCHIP_FIFO, FIFO_START_MASK);
    }
     
    //Clear FIFO Complete flag
void clear_fifo_flag(void)
    {
        //write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
    }
     
    //Read FIFO single      
uint8_t read_fifo(void)
    {
        uint8_t data = 0;
        data = spi.write(SINGLE_FIFO_READ);
        return data;
    }*/

Serial serial(USBTX, USBRX); 

class Camera {

  private:

    static int hasDoneSetup = 0;

    static void setup(void) { //read for i2c is 0x61, write is 0x60

      serial.baud(115200);                         //begins communication via USB
      serial.printf("Beginning capture.\r\n");
      spi.frequency(100000);                       //setup SPI
      spi.format(8,0);
      i2c.frequency(100000);                       //setup I2C
      wait(.1);                                    //allows time for things to settle out

      char * point; 
      for(int count = 0; count < 20; count+=2) {//switches to YUV
        i2c.write(0x60, &YUV422[count], 2);
      }
      for(int count = 0; count < 380; count+=2) {//switches to JPEG
        i2c.write(0x60, &JPEG_INIT[count], 2);
      }    
      for(int count = 0; count < 78; count+=2) {
        i2c.write(0x60, &size_JPEG[count],2 );
      }//switches to small size JPEG
      cs = 0; spi.write(0x00); spi.write(0x00);  cs = 1;//wakes up the SPI 
    }

    static void doTheThing(void) {
      for(int x = 0; x < 160; x++) {
        for(int y = 0; y < 120; y++) {
          int data = 0;
          cs = 0;
          data = spi.write(SINGLE_FIFO_READ); // reads out every other pixel in order to capture monochrome b/w image only
          cs = 1;
          cs = 0;
          spi.write(SINGLE_FIFO_READ); // Discard this pixel
          cs = 1;
          // serial.printf("%d\r\n", data);
          Camera::image[x][y] = data;
        }
      }
    }

  public:

    static short image[160][120];

    static void takePicture(void) {

      if(!Camera::hasDoneSetup) {
        Camera::setup();
      }

      int spiTest = 0;

      cs = 0; spi.write(0x00 | RWBIT); spi.write(0xff); cs = 1;               //read/write from SPI to test configuration
      cs = 0; spi.write(0x00);  spiTest = spi.write(0x00); cs = 1;
      wait_ms(1);
      if(spiTest != 0xff)                                                     //error handling
        {serial.printf("SPI is broken af\r\n");}
      if(spiTest == 0xff)
        {serial.printf("SPI is totally working\r\n");}
      
      int data = 0;
      cs = 0; spi.write(ARDUCHIP_FIFO | RWBIT); spi.write(FIFO_CLEAR_MASK|FIFO_WRPTR_RST_MASK); cs = 1;//resets fifo position, clears fifo memory
      wait_us(50);        //lets things happen
      //cs = 0; spi.write(ARDUCHIP_FIFO | RWBIT); spi.write(FIFO_WRPTR_RST_MASK);cs = 1;
      cs = 0; spi.write(ARDUCHIP_FIFO | RWBIT); spi.write(FIFO_START_MASK);cs = 1;            //tells the camera to begin capturing
      
      
      
      while (data == 0) { //waits until camera is done taking the picture, should take a second or two at most
        cs = 0; spi.write(ARDUCHIP_TRIG); data = spi.write(0x00) & CAP_DONE_MASK; cs = 1;
        serial.printf("DONE?: %d\r\n",data);
        wait(1);   //keeps from spamming the serial port
      }
      Camera::doTheThing();
    }

}
