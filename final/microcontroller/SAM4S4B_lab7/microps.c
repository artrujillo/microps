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

#define pinCCW PIO_PA9 // Connected to DT on KY­040
#define pinCW PIO_PA10 // Connected to CLK on KY­040
#define ledCW PIO_PA17
#define ledCCW PIO_PA18
#define LOAD_PIN PIO_PA29
#define LOAD_COPY PIO_PA16
#define FPGA_RESET PIO_PA15

// mid-point demo orientations

// hard-coded bits to be sent over spi
char cwOrientation[4] = {0x00, 0x29, 0x89, 0x60};


///////////////////////////
// function prototypes
///////////////////////////

// does basic PIO setup for the rotary encoder
// returns the initial read of the rotary encoder this
// read is used to later detect changes in rotation
int setup_rot_encoder();

// spi communication for microcontroller
// takes in an array of bytes that are then sent over 
// using spiSendRecieve and then waits for a DONE signal
void send_orientation(char*);

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
	
	
	
  	pinCWLast = setup_rot_encoder();
	
	// PIO setup for rot. encoder demo
	pioPinMode(ledCW, PIO_OUTPUT);
	pioPinMode(ledCCW, PIO_OUTPUT);
	pioPinMode(FPGA_RESET, PIO_OUTPUT);
	pioDigitalWrite(ledCW, 0);
	pioDigitalWrite(ledCCW, 0);
	
	send_orientation(cwOrientation);

  while(1){
      rot = pioDigitalRead(pinCW);
      if (rot != pinCWLast){ // Means the knob is rotating
          // if the knob is rotating, we need to determine direction
            // We do that by reading pin B.
          if (pioDigitalRead(pinCCW) != rot) { // Means pin A Changed first ­ We're Rotating Clockwise
						pioDigitalWrite(ledCW, 1);
						pioDigitalWrite(ledCCW, 0);
          } else { // Otherwise B changed first and we're moving CCW
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

int setup_rot_encoder() {
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
	for (i = 0; i < 4; i++){
		spiSendReceive(current_orientation[i]);
	}
	
	pioDigitalWrite(LOAD_PIN, 0);
	pioDigitalWrite(LOAD_COPY, 0);
	
	pioDigitalWrite(FPGA_RESET, 1);
	pioDigitalWrite(FPGA_RESET, 0);
	
	
}
