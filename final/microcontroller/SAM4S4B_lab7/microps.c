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
#define LOAD_PIN 29
#define DONE_PIN 30
///////////////////////////
// function prototypes
///////////////////////////

int setup_rot_encoder();
void send_orientation(char*);

///////////////////////////
// main
//////////////////////////

int main(void) {
	uint16_t pinCWLast;
	uint16_t rot;
	samInit();
	pioInit();
	
  pinCWLast = setup_rot_encoder();
	
	pioPinMode(ledCW, PIO_OUTPUT);
	pioPinMode(ledCCW, PIO_OUTPUT);
	pioDigitalWrite(ledCW, 0);
	pioDigitalWrite(ledCCW, 0);
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
	
	pioDigitalWrite(LOAD_PIN, 1);
	
	for (i = 0; i < 4; i++){
		spiSendReceive(current_orientation[i]);
	}
	
	pioDigitalWrite(LOAD_PIN, 0);
	
	while(!pioDigitalRead(DONE_PIN));
}
