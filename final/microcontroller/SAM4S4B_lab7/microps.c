/*
Aaron Trujillo & Pinky King
atrujillo@g.hmc.edu pking@g.hmc.edu
Fall 2019

'setup_rot_encoder' and rotary encoder functionality was adapted from:
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
#define LOAD_PIN PIO_PA29
#define LOAD_COPY PIO_PA16
#define FPGA_RESET PIO_PA15
// Pins for user interface
#define RED_FACE PIO_PA19
#define ORANGE_FACE PIO_PA20
#define YELLOW_FACE PIO_PA21
#define GREEN_FACE PIO_PA22
#define BLUE_FACE PIO_PA23
#define PURPLE_FACE PIO_PA24

struct {
	unsigned int fieldof3bits : 3;
} color;

color red = 0x0;
color orange = 0x1;
color yellow = 0x2;
color green = 0x3;
color blue = 0x4;
color purple = 0x5;

// hard-coded bits to be sent over spi
char cwOrientation[4] = {0x00, 0x29, 0x89, 0x60};

char starting_orientation[24] = {0x05, 0xB6, 0xDB, 0x6D, 
								 0x02, 0x92, 0x49, 0x24, 
								 0x03, 0x6D, 0xB6, 0xDB, 
								 0x02, 0x49, 0x24, 0x92, 
								 0x01, 0x24, 0x92, 0x49, 
								 0x00, 0x00, 0x00, 0x00};

color start_orient_color[54] = {purple, purple, purple, purple, purple, purple, purple, purple, purple,
							    blue, blue, blue, blue, blue, blue, blue, blue, blue,
								green, green, green, green, green, green, green, green, green,
								yellow, yellow, yellow, yellow, yellow, yellow, yellow, yellow, yellow,
								orange, orange, orange, orange, orange, orange, orange, orange, orange,
								red, red, red, red, red, red, red, red, red};
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
void send_orientation(char*);

// these functions are used to determine the new orientation
// of the cube based off the rotation of the rotary encoder
char* counter_clockwise_turn(char*);
char* clockwise_turn(char*);


///////////////////////////
// main
//////////////////////////

int main(void) {
	uint16_t pinCWLast; // used to track the most recent rot. encoder position
	uint16_t rot; // used to read a current rot position 
	samInit();
	pioInit();
	spiInit(MCK_FREQ/244000, 0, 1);
	
	pioPinMode(LOAD_PIN, PIO_OUTPUT);
	pioPinMode(LOAD_COPY, PIO_OUTPUT);
	
	
	
  	pinCWLast = user_interface_setup();
	
	// PIO setup for rot. encoder demo
	pioPinMode(ledCW, PIO_OUTPUT);
	pioPinMode(ledCCW, PIO_OUTPUT);
	pioPinMode(FPGA_RESET, PIO_OUTPUT);
	pioDigitalWrite(ledCW, 0);
	pioDigitalWrite(ledCCW, 0);
	
	send_orientation(start_orient_color);

  while(1){
      rot = pioDigitalRead(pinCW);
      if (rot != pinCWLast){ // Means the knob is rotating
          // if the knob is rotating, we need to determine direction
            // We do that by reading pin B.
          if (pioDigitalRead(pinCCW) != rot) { // Means pin A Changed first ­ We're Rotating Clockwise
			// used for testing rot encoder
			pioDigitalWrite(ledCW, 1);
			pioDigitalWrite(ledCCW, 0);
			// actual code
			clockwise_turn();
          } else { // Otherwise B changed first and we're moving CCW
		  	// used for testing rot encoder
            pioDigitalWrite(ledCW, 0);
			pioDigitalWrite(ledCCW, 1);
          }
        }
			pinCWLast = rot;
    }
}

///////////////////////////
// helper functions
///////////////////////////

int user_interface_setup() {
	// Setup Buttons
	pioPinMode(RED_FACE, PIO_INPUT);
	pioPinMode(ORANGE_FACE, PIO_INPUT);
	pioPinMode(YELLOW_FACE, PIO_INPUT);
	pioPinMode(GREEN_FACE, PIO_INPUT);
	pioPinMode(BLUE_FACE, PIO_INPUT);
	pioPinMode(PURPLE_FACE, PIO_INPUT);

	// Rotary Encoder Setup
	pioPinMode(pinCW, PIO_INPUT);
	pioPinMode(pinCCW, PIO_INPUT);
	/* Read Pin A
	Whatever state it's in will reflect the last position
	*/
	return pioDigitalRead(pinCW);
}

void send_orientation(char* current_orientation){
	int i;
	char *recieved_text;
	pioDigitalWrite(LOAD_PIN, 1);
	pioDigitalWrite(LOAD_COPY, 1);
	for (i = 0; i < 21; i++){
		spiSendReceive(current_orientation[i]);
	}
	
	pioDigitalWrite(LOAD_PIN, 0);
	pioDigitalWrite(LOAD_COPY, 0);
	
	pioDigitalWrite(FPGA_RESET, 1);
	pioDigitalWrite(FPGA_RESET, 0);
}

char* clockwise_turn(char* current_orientation){

}

char* counter_clockwise_turn(char* current_orientation){

}
