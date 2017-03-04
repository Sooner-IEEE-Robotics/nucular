/*

THIS PROGRAM IS FOR TESTING THE METAL DETECTOR.
WHEN UPLOADED, THIS PROGRAM WILL WAIT 10 SECONDS AND THEN ACTIVATE THE METAL DETECTOR. SOON AFTERWARD,
IT WILL SEND THE MEASURED VALUE OVER USB.

*/


//Before compilation, the preprocessor will paste in a bunch of useful classes from mbed.h.
#include "mbed.h"

#include "Detector.hpp"

//Define necessary pins for communicatation with the metal detector.
const static PinName metalDetectorWritePin = PF_13;
const static PinName metalDetectorReadPin = PE_9;
const static PinName test = PE_11;
//Instantiate a DigitalOut object called metalDetectorWriter.
//This object can send digital signals to the metal detector to turn it on or off.
const static DigitalOut metalDetectorWriter(metalDetectorWritePin);
const static DigitalIn t(test);
//Instantiate a DigitalIn object called metalDetectorReader.
//This object can read digital signals from the metal detector.
//DigitalIn metalDetectorReader(metalDetectorReadPin);


//0x4002 1000 GPIOE start address
#define GPIOE_OSPEEDER (*(volatile unsigned int *)0x40021008)
#define FASTDIG (*(volatile unsigned short *)0x40021010)

//Activates the metal detector and returns an integer that represents its confidence in the presence of metal.
int Detector::detectMetal(void) {
    
    //Define a variable to measure the duration of the metal detector's reply.
    int replyDuration = 0;
    
    //Activate metal detector.
    metalDetectorWriter.write(1);
    
    //Wait 1 millisecond.
    wait_ms(1);
    
    //Deactivate metal detector.
    metalDetectorWriter.write(0);
    wait_us(2);
    
    
    //As long as the metalDetector is sending a HIGH signal, continuously increment the reply duration.      
    while((FASTDIG >> 11 )&(1)) { // rewrote using register access to give faster read times
        replyDuration++;
    }
    
    //Return the reply duration.
    return replyDuration; 
}

int Detector::calibrate(int calNum) {               //calibrates via averaging calNum number of detector reads
    int result = 0;             //assumes no metal at the calibration spot
    for(int counter = 0; counter < calNum; counter++){
        result += detectMetal();
    } 
    return result/calNum;
}