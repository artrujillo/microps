/*
Aaron Trujillo & Pinky King
atrujillo@g.hmc.edu pking@g.hmc.edu
Fall 2019

'user_interface_setup' and rotary encoder functionality was adapted from:
 http://eeshop.unl.edu/pdf/KEYES%20Rotary%20encoder%20module%20KY-040.pdf
*/

#include "SAM4S4B.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Rot Encoder Pins
#define pinCCW PIO_PA9 // Connected to DT on KY­040
#define pinCW PIO_PA10 // Connected to CLK on KY­040
// Testing Rot Encoder Pins
#define ledCW PIO_PA17
#define ledCCW PIO_PA18
// SPI Pins
//#define LOAD_PIN PIO_PA29
#define LOAD_COPY PIO_PA16
#define FPGA_RESET PIO_PA15


// hard-coded bits to be sent over spi
//char starting_orientation[21] = {0x2D, 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, 0xB6, 0xDB, 0x6D, 0xB6, 0xD0};
//char starting_orientation[21] ={0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49, 0x24, 0x92, 0x49};
char newOritentation[9] = {0x02, 0x00, 0x03, 0x04, 0x05, 0x00, 0x00, 0x00, 0x01};
//char starting_orientation[21] = {0xD6, 0x6B, 0x35, 0x9A, 0xCD, 0x66, 0xB3, 0x59, 0xAC, 0xD6, 0x6B, 0x35, 0x9A, 0xCD, 0x66, 0xB3, 0x59, 0xAC, 0xD6, 0x6B, 0x00};
//char starting_orientation[21] =	{0xA7, 0x29, 0x4E, 0x14, 0xE5, 0x29, 0xC2, 0x9C, 0xA5, 0x38, 0x53, 0x94, 0xA7, 0x0A, 0x72, 0x94, 0xE1, 0x4E, 0x52, 0x9C};
char starting_orientation[21] = {0x01, 0x4E, 0x50, 0x50, 0x29, 0xCA, 0x0A, 0x05, 0x39, 0x41, 0x40, 0xA7, 0x28, 0x28, 0x14, 0xE5, 0x05, 0x02, 0x9C, 0xA0, 0xA0};
	
	/*
char starting_orientation[54] = {0x01, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
																 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
																 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
																 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
																 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
																 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
*/
char* shifted_orientation[21];
///////////////////////////
// function prototypes
///////////////////////////

// does basic PIO setup for the rotary encoder and buttons
// returns the initial read of the rotary encoder this
// read is used to later detect changes in rotation
int user_interface_setup();

// spi communication for microcontroller
// takes in an array of bytes that are then sent over 
// using spiSendRecieve and then waits for a DONE signal
void send_orientation_1(char*);
void send_orientation_2(char*);
void send_orientation_3(char*);

// these functions are used to determine the new orientation
// of the cube based off the rotation of the rotary encoder
char* counter_clockwise_turn(char*);
char* clockwise_turn(char*);

//////////////////////////
// main
//////////////////////////

int main(void) {
	uint16_t pinCWLast; // used to track the most recent rot. encoder position
	uint16_t rot; // used to read a current rot position 
	samInit();
	pioInit();
	tcDelayInit();
	spiInit(MCK_FREQ/244000, 0, 1); // 244000
	
	//pioPinMode(LOAD_PIN, PIO_OUTPUT);
	pioPinMode(LOAD_COPY, PIO_OUTPUT);
	
  	pinCWLast = user_interface_setup();
	
	// PIO setup for rot. encoder demo
	pioPinMode(ledCW, PIO_OUTPUT);
	pioPinMode(ledCCW, PIO_OUTPUT);
	pioPinMode(FPGA_RESET, PIO_OUTPUT);
	pioDigitalWrite(FPGA_RESET, 0);
	pioDigitalWrite(ledCW, 0);
	pioDigitalWrite(ledCCW, 0);
	
	send_orientation_1(starting_orientation);
	//send_orientation_2(starting_orientation);
	//send_orientation_3(starting_orientation);
	/*
  while(1){
      rot = pioDigitalRead(pinCW);
      if (rot != pinCWLast){ // Means the knob is rotating
          // if the knob is rotating, we need to determine direction
            // We do that by reading pin B.
          if (pioDigitalRead(pinCCW) != rot) { // Means pin A Changed first ­ We're Rotating Clockwise
			// used for testing rot encoder
			pioDigitalWrite(ledCW, 1);
			pioDigitalWrite(ledCCW, 0);
          } else { // Otherwise B changed first and we're moving CCW
		  	// used for testing rot encoder
            pioDigitalWrite(ledCW, 0);
			pioDigitalWrite(ledCCW, 1);
          }
        }
			pinCWLast = rot;
    }
	*/
}

///////////////////////////
// helper functions
///////////////////////////

int user_interface_setup() {
	// Setup Buttons
	/*
	pioPinMode(RED_FACE, PIO_INPUT);
	pioPinMode(ORANGE_FACE, PIO_INPUT);
	pioPinMode(YELLOW_FACE, PIO_INPUT);
	pioPinMode(GREEN_FACE, PIO_INPUT);
	pioPinMode(BLUE_FACE, PIO_INPUT);
	pioPinMode(PURPLE_FACE, PIO_INPUT);
	*/
	// Rotary Encoder Setup
	pioPinMode(pinCW, PIO_INPUT);
	pioPinMode(pinCCW, PIO_INPUT);
	/* Read Pin A
	Whatever state it's in will reflect the last position
	*/
	return pioDigitalRead(pinCW);
}

void send_orientation_1(char* current_orientation){
	int i;
	//pioPinMode(LOAD_PIN, PIO_OUTPUT);
	
	//pioDigitalWrite(LOAD_PIN, 1);
	// used for logic analyzer
	pioDigitalWrite(LOAD_COPY, 1);
	
	for (i = 0; i < 21; i++){
		spiSendReceive(current_orientation[i]);
	}

	pioDigitalWrite(LOAD_COPY, 0);

}

void send_orientation_2(char* current_orientation){
	int i;
	//pioPinMode(LOAD_PIN, PIO_OUTPUT);
  pioPinMode(LOAD_COPY, PIO_OUTPUT);
	
	//pioDigitalWrite(LOAD_PIN, 1);
	// used for logic analyzer
	pioDigitalWrite(LOAD_COPY, 1);
	
	for (i = 18; i < 4; i++){
		spiSendReceive(current_orientation[i]);
	}

	pioDigitalWrite(LOAD_COPY, 0);

}

void send_orientation_3(char* current_orientation){
	int i;
	//pioPinMode(LOAD_PIN, PIO_OUTPUT);
  pioPinMode(LOAD_COPY, PIO_OUTPUT);
	
	//pioDigitalWrite(LOAD_PIN, 1);
	// used for logic analyzer
	pioDigitalWrite(LOAD_COPY, 1);
	
	for (i = 36; i < 54; i++){
		spiSendReceive(current_orientation[i]);
	}

	pioDigitalWrite(LOAD_COPY, 0);

	pioDigitalWrite(FPGA_RESET, 1);
	pioDigitalWrite(FPGA_RESET, 0);
}

char* shift_orientation(char* current_orientation){
	int i;
	int j = 27;
	int shift = 0;
	
	for(i = 53; i >= 0; i--){
		if (i % 2 == 0){
			j--;
			shift = 0;
		} else {
			shift = 3;
		}
	}
}
/*
char* clockwise_turn(char* current_orientation){

}

char* counter_clockwise_turn(char* current_orientation){

}
*/