#include <stdio>
#include "mbed.h"

//Instantiate an object to communicate with the computer via USB.
const static Serial outputToComputer(USBTX, USBRX);
const static BusOut led(LED1,LED2,LED3);  //controls leds

int main() {
    int calval = Detector::calibrate();                   //calibrates metal detector
    GPIOE_OSPEEDER |= (1 << 23);                //sets gpio to fastest possible speed supposedly
    GPIOE_OSPEEDER |= (1 << 22);
    int metalDetectorReply = 0;
    while(1){    
        //For debugging purposes, alert the computer that the main method has been called.
        outputToComputer.printf("FOR DEBUG: Main method now running. Detector will activate in 10 seconds.\r\n");
        metalDetectorReply = 0;
        //Someday we can start the measurement on button press, but for now let's just allow 10 seconds and then measure.
        wait_ms(200);

        //Get a measurement from the metal detector.
        metalDetectorReply = Dector::detectMetal();
        outputToComputer.printf("CalVal: %d",calval);

        //Inform the computer of the metal detector's measurement. Higher means greater likelihood of metal!
        outputToComputer.printf("Detected:%d\r\n", metalDetectorReply);
        if(metalDetectorReply > calval + 10) { //controls leds on the nucleo
            led = 0b111;
        } else {
            led = 000;
        }
    }
}