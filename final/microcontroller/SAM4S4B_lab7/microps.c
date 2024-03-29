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
#include <time.h>

// Rot Encoder Pins
#define pinCCW PIO_PA9 // Connected to DT on KY­040
#define pinCW PIO_PA10 // Connected to CLK on KY­040
// Testing Rot Encoder Pins
#define ledCW PIO_PA17
#define ledCCW PIO_PA18
// SPI Pins
#define LOAD_PIN PIO_PA29
#define LOAD_COPY PIO_PA16
#define FPGA_RESET PIO_PA5
// Buttin Pins
#define RED_FACE PIO_PA19
#define ORANGE_FACE PIO_PA20
#define YELLOW_FACE PIO_PA21
#define GREEN_FACE PIO_PA22
#define BLUE_FACE PIO_PA23
#define PURPLE_FACE PIO_PA24
#define RESET_BUTTON PIO_PA25

#define SCRAMBLE_NUM 10

// hard-coded bits to be sent over spi
char cwOrientation[4] = {0x00, 0x29, 0x89, 0x60};

char newOritentation[9] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};

char starting_orientation[54] = {0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
																 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4,
								                 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
								                 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
								                 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
								                 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

///////////////////////////
// function prototypes
///////////////////////////

// these functions do basic PIO setup for the rotary encoder and buttons
// and read the user input
int user_interface_setup();
void read_input(char*);

// spi communication for microcontroller
// takes in an array of bytes that are then sent over 
// using spiSendRecieve and then waits for a DONE signal
void send_orientation(char*);

// these functions are used to determine the new orientation
// of the cube based off the rotation of the rotary encoder
void counter_clockwise_turn(char*, char);
void clockwise_turn(char*, char);

void scramble_cube(char*);
void rotate_cube(char*, char*);



//////////////////////////
// main
//////////////////////////

int main(void) {
	samInit();
	pioInit();
	spiInit(MCK_FREQ/244000, 0, 1);
	
	pioPinMode(LOAD_PIN, PIO_OUTPUT);
	pioPinMode(LOAD_COPY, PIO_OUTPUT);
	pioDigitalWrite(LOAD_PIN, 0);
	pioDigitalWrite(LOAD_COPY, 0);
	
	// PIO setup for rot. encoder demo
	pioPinMode(ledCW, PIO_OUTPUT);
	pioPinMode(ledCCW, PIO_OUTPUT);
	pioPinMode(FPGA_RESET, PIO_OUTPUT);
	pioDigitalWrite(ledCW, 0);
	pioDigitalWrite(ledCCW, 0);

	char user_input[2]; // first char is the color, second char is the rotation direction
	user_input[0] = 0x7;
	
	char orientation[54];
	scramble_cube(orientation);
	
	while (1) {
		
		read_input(user_input);
		
		if (user_input[0] == 0x6) scramble_cube(orientation); // RESET
		else rotate_cube(orientation, user_input); // ROTATE

		send_orientation(orientation);		
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
	pioPinMode(RESET_BUTTON, PIO_INPUT);

	
	// Rotary Encoder Setup
	pioPinMode(pinCW, PIO_INPUT);
	pioPinMode(pinCCW, PIO_INPUT);
}

// scrambles the cube
void scramble_cube(char* orientation) {
		char user_input[2];
		for (int i = 0; i < 54; i++) {
			orientation[i] = starting_orientation[i];
		}
		for (int i = 0; i < SCRAMBLE_NUM; i++) {
			srand(time(NULL));
			user_input[0] = rand()%6;
			user_input[1] = rand()%2;
			rotate_cube(orientation, user_input);
		}
}


// sends a new orientation over SPI
void send_orientation(char* current_orientation){
	int i;

	pioDigitalWrite(LOAD_PIN, 1);
	pioDigitalWrite(LOAD_COPY, 1); // used for logic analyzer
	for (i = 0; i < 9; i++){
		spiSendReceive(current_orientation[i]);
	}
	
	pioDigitalWrite(LOAD_PIN, 0);
	pioDigitalWrite(LOAD_COPY, 0);
	
	pioDigitalWrite(FPGA_RESET, 1);
	pioDigitalWrite(FPGA_RESET, 0);
}


// reads the user input and outputs an array of two chars
// the first char indicates the color (reset or a face color)
// the second char indicates the rotation direction (0 = CW, 1 = CCW)
void read_input(char* user_input) {
	
	uint16_t pinCWLast; // used to track the most recent rot. encoder position
	uint16_t rot; // used to read a current rot position 
	char red, orange, yellow, green, blue, purple, reset;
	
	pinCWLast = pioDigitalRead(pinCW);
	
	while(1){
		
		// read buttons
		red = pioDigitalRead(RED_FACE);
		orange = pioDigitalRead(ORANGE_FACE);
		yellow = pioDigitalRead(YELLOW_FACE);
		green = pioDigitalRead(GREEN_FACE);
		blue = pioDigitalRead(BLUE_FACE);
		purple = pioDigitalRead(PURPLE_FACE);
	  reset = pioDigitalRead(RESET_BUTTON);
		if      (red)     user_input[0] = 0x0;
		else if (orange)  user_input[0] = 0x1;
		else if (yellow)  user_input[0] = 0x2;
	  else if (green)   user_input[0] = 0x3;
		else if (blue)    user_input[0] = 0x4;
		else if (purple)  user_input[0] = 0x5;
		else if (reset) 	user_input[0] = 0x6;
		else if (user_input[0] == 0x7) continue;
			
		// read rotary encoder
    rot = pioDigitalRead(pinCW);
    if (rot != pinCWLast){ // Means the knob is rotating
			// if the knob is rotating, we need to determine direction
      // We do that by reading pin B.
      if (pioDigitalRead(pinCCW) != rot) { // Means pin A Changed first ­ We're Rotating Clockwise
				// used for testing rot encoder
				pioDigitalWrite(ledCW, 1);
				pioDigitalWrite(ledCCW, 0);
				user_input[1] = 0x0;
      } else { // Otherwise B changed first and we're moving CCW
				// used for testing rot encoder
        pioDigitalWrite(ledCW, 0);
				pioDigitalWrite(ledCCW, 1);
				user_input[1] = 0x1;
      }
			break;
    }
		pinCWLast = rot;		
  }
	
}

void rotate_cube(char* current_orientation, char* user_input) {
	if (user_input[1] == 0) { // clockwise
		clockwise_turn(current_orientation, user_input[0]);
	}
	else { // counter clockwise
		counter_clockwise_turn(current_orientation, user_input[0]);
	}
}



void clockwise_turn(char* current_orientation, char color){
	char temp[54];
	for (int i = 0; i < 54; i++) {
	  temp[i] = current_orientation[i];
	}
	// purple face starts at 0
	// blue face starts at 9
	// green face starts at 18
	// yellow face starts at 27
	// orange face starts at 36
	// red face starts at 45
	if (color == 0x0) { // red face rotates
		// fix red face
		temp[45] = current_orientation[45+6];
		temp[46] = current_orientation[45+3];
		temp[47] = current_orientation[45];
		temp[48] = current_orientation[45+7];
		temp[50] = current_orientation[45+1];
		temp[51] = current_orientation[45+8];
		temp[52] = current_orientation[45+5];
		temp[53] = current_orientation[45+2];
		// fix purple face
		temp[0+6] = current_orientation[18+8];
		temp[0+7] = current_orientation[18+5];
		temp[0+8] = current_orientation[18+2];
		// fix blue face 
		temp[9] = current_orientation[0+6];
		temp[9+3] = current_orientation[0+7];
		temp[9+6] = current_orientation[0+8];
		// fix yellow face
		temp[27] = current_orientation[9+6];
		temp[27+1] = current_orientation[9+3];
		temp[27+2] = current_orientation[9];
		// fix green face
		temp[18+2] = current_orientation[27];
		temp[18+5] = current_orientation[27+1];
		temp[18+8] = current_orientation[27+2];
	}
	else if (color == 0x1) { // orange
		// fix orange face
		temp[36] = current_orientation[36+6];
		temp[37] = current_orientation[36+3];
		temp[38] = current_orientation[36];
		temp[39] = current_orientation[36+7];
		temp[41] = current_orientation[36+1];
		temp[42] = current_orientation[36+8];
		temp[43] = current_orientation[36+5];
		temp[44] = current_orientation[36+2]; 
		// fix purple face
		temp[0] = current_orientation[9+2];
		temp[1] = current_orientation[9+5];
		temp[2] = current_orientation[9+8];
		// fix blue face 
		temp[9+2] = current_orientation[27+8];
		temp[9+5] = current_orientation[27+7];
		temp[9+8] = current_orientation[27+6];
		// fix yellow face
		temp[27+6] = current_orientation[18];
		temp[27+7] = current_orientation[18+3];
		temp[27+8] = current_orientation[18+6];
		// fix green face
		temp[18+0] = current_orientation[2];
		temp[18+3] = current_orientation[1];
		temp[18+6] = current_orientation[0];
	}
	else if (color == 0x2) { // yellow
		// fix yellow face
		temp[27] = current_orientation[27+6];
		temp[28] = current_orientation[27+3];
		temp[29] = current_orientation[27];
		temp[30] = current_orientation[27+7];
		temp[32] = current_orientation[27+1];
		temp[33] = current_orientation[27+8];
		temp[34] = current_orientation[27+5];
		temp[35] = current_orientation[27+2];
		// fix red face
		temp[45+6] = current_orientation[18+6];
		temp[45+7] = current_orientation[18+7];
		temp[45+8] = current_orientation[18+8];
		// fix blue face 
		temp[9+6] = current_orientation[45+6];
		temp[9+7] = current_orientation[45+7];
		temp[9+8] = current_orientation[45+8];
		// fix orange face
		temp[36+6] = current_orientation[9+6];
		temp[36+7] = current_orientation[9+7];
		temp[36+8] = current_orientation[9+8];
		// fix green face
		temp[18+6] = current_orientation[36+6];
		temp[18+7] = current_orientation[36+7];
		temp[18+8] = current_orientation[36+8];
	}
	else if (color == 0x3) { // green
		// fix green face
		temp[18] = current_orientation[18+6];
		temp[19] = current_orientation[18+3];
		temp[20] = current_orientation[18];
		temp[21] = current_orientation[18+7];
		temp[23] = current_orientation[18+1];
		temp[24] = current_orientation[18+8];
		temp[25] = current_orientation[18+5];
		temp[26] = current_orientation[18+2];
		// fix yellow face
		temp[27+0] = current_orientation[45+0];
		temp[27+3] = current_orientation[45+3];
		temp[27+6] = current_orientation[45+6];
		// fix red face 
		temp[45+0] = current_orientation[0];
		temp[45+3] = current_orientation[3];
		temp[45+6] = current_orientation[6];
		// fix purple face
		temp[0] = current_orientation[36+8];
		temp[3] = current_orientation[36+5];
		temp[6] = current_orientation[36+2];
		// fix orange face
		temp[36+2] = current_orientation[27+6];
		temp[36+5] = current_orientation[27+3];
		temp[36+8] = current_orientation[27+0];
	}
	else if (color == 0x4) { // blue
		// fix blue face
		temp[9] = current_orientation[9+6];
		temp[10] = current_orientation[9+3];
		temp[11] = current_orientation[9];
		temp[12] = current_orientation[9+7];
		temp[14] = current_orientation[9+1];
		temp[15] = current_orientation[9+8];
		temp[16] = current_orientation[9+5];
		temp[17] = current_orientation[9+2];
		// fix yellow face
		temp[27+2] = current_orientation[36+6];
		temp[27+5] = current_orientation[36+3];
		temp[27+8] = current_orientation[36+0];
		// fix red face 
		temp[45+2] = current_orientation[27+2];
		temp[45+5] = current_orientation[27+5];
		temp[45+8] = current_orientation[27+8];
		// fix purple face
		temp[2] = current_orientation[45+2];
		temp[5] = current_orientation[45+5];
		temp[8] = current_orientation[45+8];
		// fix orange face
		temp[36+0] = current_orientation[8];
		temp[36+3] = current_orientation[5];
		temp[36+6] = current_orientation[2];

	}
	else if (color == 0x5) { // purple
		// fix purple face
		temp[0] = current_orientation[6];
		temp[1] = current_orientation[3];
		temp[2] = current_orientation[0];
		temp[3] = current_orientation[7];
		temp[5] = current_orientation[1];
		temp[6] = current_orientation[8];
		temp[7] = current_orientation[5];
		temp[8] = current_orientation[2];
		// fix red face
		temp[45+0] = current_orientation[9+0];
		temp[45+1] = current_orientation[9+1];
		temp[45+2] = current_orientation[9+2];
		// fix blue face 
		temp[9+0] = current_orientation[36+0];
		temp[9+1] = current_orientation[36+1];
		temp[9+2] = current_orientation[36+2];
		// fix orange face
		temp[36+0] = current_orientation[18+0];
		temp[36+1] = current_orientation[18+1];
		temp[36+2] = current_orientation[18+2];
		// fix green face
		temp[18+0] = current_orientation[45+0];
		temp[18+1] = current_orientation[45+1];
		temp[18+2] = current_orientation[45+2];
	}
	for (int i = 0; i < 54; i++) {
	  current_orientation[i] = temp[i];
	}
}

void counter_clockwise_turn(char* current_orientation, char color){
	char temp[54];
	for (int i = 0; i < 54; i++) {
	  temp[i] = current_orientation[i];
	}
	if (color == 0x0) { // red
		// fix red face
		temp[45] = current_orientation[45+2];
		temp[46] = current_orientation[45+5];
		temp[47] = current_orientation[45+8];
		temp[48] = current_orientation[45+1];
		temp[50] = current_orientation[45+7];
		temp[51] = current_orientation[45+0];
		temp[52] = current_orientation[45+3];
		temp[53] = current_orientation[45+6];
		// fix purple face
		temp[0+6] = current_orientation[9+0];
		temp[0+7] = current_orientation[9+3];
		temp[0+8] = current_orientation[9+6];
		// fix blue face 
		temp[9] = current_orientation[27+2];
		temp[9+3] = current_orientation[27+1];
		temp[9+6] = current_orientation[27+0];
		// fix yellow face
		temp[27] = current_orientation[18+2];
		temp[27+1] = current_orientation[18+5];
		temp[27+2] = current_orientation[18+8];
		// fix green face
		temp[18+2] = current_orientation[8];
		temp[18+5] = current_orientation[7];
		temp[18+8] = current_orientation[6];
	}
	else if (color == 0x1) { // orange
		// fix orange face
		temp[36] = current_orientation[36+2];
		temp[37] = current_orientation[36+5];
		temp[38] = current_orientation[36+8];
		temp[39] = current_orientation[36+1];
		temp[41] = current_orientation[36+7];
		temp[42] = current_orientation[36+0];
		temp[43] = current_orientation[36+3];
		temp[44] = current_orientation[36+6];
		// fix purple face
		temp[0] = current_orientation[18+6];
		temp[1] = current_orientation[18+3];
		temp[2] = current_orientation[18+0];
		// fix blue face 
		temp[9+2] = current_orientation[0];
		temp[9+5] = current_orientation[1];
		temp[9+8] = current_orientation[2];
		// fix yellow face
		temp[27+6] = current_orientation[9+8];
		temp[27+7] = current_orientation[9+5];
		temp[27+8] = current_orientation[9+2];
		// fix green face
		temp[18+0] = current_orientation[27+6];
		temp[18+3] = current_orientation[27+7];
		temp[18+6] = current_orientation[27+8];
	}
	else if (color == 0x2) { // yellow
		// fix yellow face
		temp[27] = current_orientation[27+2];
		temp[28] = current_orientation[27+5];
		temp[29] = current_orientation[27+8];
		temp[30] = current_orientation[27+1];
		temp[32] = current_orientation[27+7];
		temp[33] = current_orientation[27+0];
		temp[34] = current_orientation[27+3];
		temp[35] = current_orientation[27+6];
		// fix red face
		temp[45+6] = current_orientation[9+6];
		temp[45+7] = current_orientation[9+7];
		temp[45+8] = current_orientation[9+8];
		// fix blue face 
		temp[9+6] = current_orientation[36+6];
		temp[9+7] = current_orientation[36+7];
		temp[9+8] = current_orientation[36+8];
		// fix orange face
		temp[36+6] = current_orientation[18+6];
		temp[36+7] = current_orientation[18+7];
		temp[36+8] = current_orientation[18+8];
		// fix green face
		temp[18+6] = current_orientation[45+6];
		temp[18+7] = current_orientation[45+7];
		temp[18+8] = current_orientation[45+8];
	}
	else if (color == 0x3) { // green
		// fix green face
		temp[18] = current_orientation[18+2];
		temp[19] = current_orientation[18+5];
		temp[20] = current_orientation[18+8];
		temp[21] = current_orientation[18+1];
		temp[23] = current_orientation[18+7];
		temp[24] = current_orientation[18+0];
		temp[25] = current_orientation[18+3];
		temp[26] = current_orientation[18+6];
		// fix yellow face
		temp[27+0] = current_orientation[36+8];
		temp[27+3] = current_orientation[36+5];
		temp[27+6] = current_orientation[36+2];
		// fix red face 
		temp[45+0] = current_orientation[27+0];
		temp[45+3] = current_orientation[27+3];
		temp[45+6] = current_orientation[27+6];
		// fix purple face
		temp[0] = current_orientation[45+0];
		temp[3] = current_orientation[45+3];
		temp[6] = current_orientation[45+6];
		// fix orange face
		temp[36+2] = current_orientation[6];
		temp[36+5] = current_orientation[3];
		temp[36+8] = current_orientation[0];
	}
	else if (color == 0x4) { // blue
		// fix blue face
		temp[9] = current_orientation[9+2];
		temp[10] = current_orientation[9+5];
		temp[11] = current_orientation[9+8];
		temp[12] = current_orientation[9+1];
		temp[14] = current_orientation[9+7];
		temp[15] = current_orientation[9+0];
		temp[16] = current_orientation[9+3];
		temp[17] = current_orientation[9+6];
		// fix yellow face
		temp[27+2] = current_orientation[45+2];
		temp[27+5] = current_orientation[45+5];
		temp[27+8] = current_orientation[45+8];
		// fix red face 
		temp[45+2] = current_orientation[2];
		temp[45+5] = current_orientation[5];
		temp[45+8] = current_orientation[8];
		// fix purple face
		temp[2] = current_orientation[36+6];
		temp[5] = current_orientation[36+3];
		temp[8] = current_orientation[36+0];
		// fix orange face
		temp[36+0] = current_orientation[27+8];
		temp[36+3] = current_orientation[27+5];
		temp[36+6] = current_orientation[27+2];
	}
	else if (color == 0x5) { // purple
		// fix purple face
		temp[0] = current_orientation[2];
		temp[1] = current_orientation[5];
		temp[2] = current_orientation[8];
		temp[3] = current_orientation[1];
		temp[5] = current_orientation[7];
		temp[6] = current_orientation[0];
		temp[7] = current_orientation[3];
		temp[8] = current_orientation[6];
		// fix red face
		temp[45+0] = current_orientation[18+0];
		temp[45+1] = current_orientation[18+1];
		temp[45+2] = current_orientation[18+2];
		// fix blue face 
		temp[9+0] = current_orientation[45+0];
		temp[9+1] = current_orientation[45+1];
		temp[9+2] = current_orientation[45+2];
		// fix orange face
		temp[36+0] = current_orientation[9+0];
		temp[36+1] = current_orientation[9+1];
		temp[36+2] = current_orientation[9+2];
		// fix green face
		temp[18+0] = current_orientation[36+0];
		temp[18+1] = current_orientation[36+1];
		temp[18+2] = current_orientation[36+2];
	}
	for (int i = 0; i < 54; i++) {
	  current_orientation[i] = temp[i];
	}
}
