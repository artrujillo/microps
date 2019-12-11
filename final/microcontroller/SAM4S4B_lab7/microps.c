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
#define pinCCW PIO_PA17 // Connected to DT on KY­040
#define pinCW PIO_PA18 // Connected to CLK on KY­040
// SPI Pins
#define LOAD_PIN PIO_PA29
// Buttin Pins
#define RED_FACE PIO_PA19
#define ORANGE_FACE PIO_PA20
#define YELLOW_FACE PIO_PA21
#define GREEN_FACE PIO_PA22
#define BLUE_FACE PIO_PA23
#define WHITE_FACE PIO_PA24
#define RESET_BUTTON PIO_PA25

#define SCRAMBLE_NUM 15

char starting_orientation[54] = {0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5, 0x5,
																 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4,
								                 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3,
								                 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
								                 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
								                 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
char shifted[21];


///////////////////////////
// function prototypes
///////////////////////////

// these functions do basic PIO setup for the rotary encoder and buttons
// and read the user input
void user_interface_setup();
int read_input(char*);
void determine_direction(char* );

// spi communication for microcontroller
// takes in an array of bytes that are then sent over 
// using spiSendRecieve and then waits for a DONE signal
void send_orientation(char*);

// these functions are used to change the orientation of the cube 
void scramble_cube(char*, int);
void rotate_cube(char*, char*);

// these functions are used to determine the new orientation
// of the cube based off the rotation of the rotary encoder
void counter_clockwise_turn(char*, char);
void clockwise_turn(char*, char);

// these functions are used to perform the shifting necessary to
// be able to send our orientation over spi
void shift_helper(char*, int, int, int, int, int, int, int, int, int, int, int);
void final_shift_helper(char*);
void shift_orientation(char*);

// we use this function to create a psuedorandom seed to get a shuffled cube.
void seed_generator();

//////////////////////////
// main
//////////////////////////

	
int main(void) {

	int pinCWLast; // used to track the most recent rot. encoder position
	int rot; // used to read a current rot position 
	int rand_seed;
	samInit();
	pioInit();
	tcDelayInit();
	spiInit(MCK_FREQ/244000, 0, 1); // 244000
	pioPinMode(LOAD_PIN, PIO_OUTPUT);
	pioPinMode(PIO_PA28, PIO_OUTPUT);
	pioPinMode(PIO_PA3, PIO_OUTPUT);

	char user_input[2];
	char rotations[2];
	rotations[0] = 0x3;
	rotations[1] = 0x3;
	user_input[0] = 0x7;// = {1,1}; // first char is the color, second char is the rotation direction
	//user_input[0] = 0x7;

	char orientation[54];

	for (int i = 0; i < 54; i++) {
			orientation[i] = starting_orientation[i];
	}
	shift_orientation(orientation);
	send_orientation(shifted);

	//send_orientation(starting_orientation);
	user_interface_setup();
	int turned_once = 0;
	int counter = 0;
	pinCWLast = pioDigitalRead(pinCW);
	while (1) { 
		//pinCWLast = pioDigitalRead(pinCW);
		rand_seed = read_input(user_input);
		if (user_input[0] == 0x7) { // user has never picked a face
			continue;
		} else if (user_input[0] == 0x6) {
			scramble_cube(orientation, rand_seed); // RESET
			shift_orientation(orientation);
			send_orientation(shifted);
		} else { // user has chosen a face
			rot = pioDigitalRead(pinCW);
			if (rot != pinCWLast) {
				if (pioDigitalRead(pinCCW) != rot) {
					if (rotations[0] != 0x3) rotations[1] = 0x0;
					else rotations[0] = 0x0;
				} else {
					if (rotations[0] != 0x3) rotations[1] = 0x1;
					else rotations[0] = 0x1;
				}
				if ((rotations[0] == 0x0) & (rotations[1] == 0x0)) {
					user_input[1] = 0x0;
					rotate_cube(orientation, user_input); // ROTATE
					shift_orientation(orientation);
					send_orientation(shifted);
					pioDigitalWrite(PIO_PA3,0);
					pioDigitalWrite(PIO_PA28,1);
					rotations[1] = 0x3;
					rotations[0] = 0x3;
					tcDelayMillis(10);
					pinCWLast = pioDigitalRead(pinCW);
				}
				else if ((rotations[0] == 0x1) & (rotations[1] == 0x1)) {
					user_input[1] = 0x1;
					rotate_cube(orientation, user_input); // ROTATE
					shift_orientation(orientation);
					send_orientation(shifted);
					pioDigitalWrite(PIO_PA3,1);
					pioDigitalWrite(PIO_PA28,0);
					rotations[1] = 0x3;
					rotations[0] = 0x3;
					tcDelayMillis(10);
					pinCWLast = pioDigitalRead(pinCW);
				}
				else if ((rotations[0] == 0x0) & (rotations[1] == 0x1)) {
					user_input[1] = 0x1;
					rotate_cube(orientation, user_input); // ROTATE
					shift_orientation(orientation);
					send_orientation(shifted);
					//pioDigitalWrite(PIO_PA3,1);
					//pioDigitalWrite(PIO_PA28,0);
					rotations[1] = 0x3;
					rotations[0] = 0x1;
					tcDelayMillis(10);
					pinCWLast = pioDigitalRead(pinCW);
				}
				else if ((rotations[0] == 0x1) & (rotations[1] == 0x0)) {
					user_input[1] = 0x0;
					rotate_cube(orientation, user_input); // ROTATE
					shift_orientation(orientation);
					send_orientation(shifted);
					//pioDigitalWrite(PIO_PA3,0);
					//pioDigitalWrite(PIO_PA28,1);
					rotations[1] = 0x3;
					rotations[0] = 0x0;
					tcDelayMillis(10);
					pinCWLast = pioDigitalRead(pinCW);
				}
			}
			pinCWLast = rot;
		} 
	} 
}
///////////////////////////
// helper functions
///////////////////////////


void user_interface_setup() {
	// Setup Buttons
	pioPinMode(RED_FACE, PIO_INPUT);
	pioPinMode(ORANGE_FACE, PIO_INPUT);
	pioPinMode(YELLOW_FACE, PIO_INPUT);
	pioPinMode(GREEN_FACE, PIO_INPUT);
	pioPinMode(BLUE_FACE, PIO_INPUT);
	pioPinMode(WHITE_FACE, PIO_INPUT);
	pioPinMode(RESET_BUTTON, PIO_INPUT);
	
	// Rotary Encoder Setup
	pioPinMode(pinCW, PIO_INPUT);
	pioPinMode(pinCCW, PIO_INPUT);
}

// scrambles the cube
void scramble_cube(char* orientation, int rand_seed) {
		char user_input[2];
	  char range = rand_seed % 10;
		for (int i = 0; i < 54; i++) {
			orientation[i] = starting_orientation[i];
		}
		for (int i = 0; i < SCRAMBLE_NUM + range; i++) {
			user_input[0] = rand()%6;
			user_input[0] = (user_input[0]+rand_seed)%6;
			user_input[1] = rand()%2;
			user_input[1] = (user_input[1]+rand_seed)%2;
			rotate_cube(orientation, user_input);
			rand_seed = rand_seed*13;
		}
}


// sends a new orientation over SPI
void send_orientation(char* current_orientation){
	int i;
	pioDigitalWrite(LOAD_PIN, 1);
	for (i = 0; i < 21; i++){
		spiSendReceive(current_orientation[i]);
	}
	pioDigitalWrite(LOAD_PIN, 0);
	tcDelayMillis(13);
	spiSendReceive(0);
}


// reads the user input and outputs an array of two chars
int read_input(char* user_input) {
	
	char red, orange, yellow, green, blue, white, reset;
	int rand_seed = 0;
	// read buttons
	red = pioDigitalRead(RED_FACE);
	orange = pioDigitalRead(ORANGE_FACE);
	yellow = pioDigitalRead(YELLOW_FACE);
	green = pioDigitalRead(GREEN_FACE);
	blue = pioDigitalRead(BLUE_FACE);
	white = pioDigitalRead(WHITE_FACE);
	reset = pioDigitalRead(RESET_BUTTON);
	if      (red)     user_input[0] = 0x0;
	else if (orange)  user_input[0] = 0x1;
	else if (yellow)  user_input[0] = 0x2;
	else if (green)   user_input[0] = 0x3;
	else if (blue)    user_input[0] = 0x4;
	else if (white)  user_input[0] = 0x5;
	else if (reset)   user_input[0] = 0x6;
	while (pioDigitalRead(RESET_BUTTON)) {
		rand_seed++;
		//tcDelayMicroseconds(100);
	}
	
	return rand_seed;
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
	// white face starts at 0
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
		// fix white face
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
		// fix white face
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
		// fix white face
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
		// fix white face
		temp[2] = current_orientation[45+2];
		temp[5] = current_orientation[45+5];
		temp[8] = current_orientation[45+8];
		// fix orange face
		temp[36+0] = current_orientation[8];
		temp[36+3] = current_orientation[5];
		temp[36+6] = current_orientation[2];

	}
	else if (color == 0x5) { // white
		// fix white face
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
		// fix white face
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
		// fix white face
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
		// fix white face
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
		// fix white face
		temp[2] = current_orientation[36+6];
		temp[5] = current_orientation[36+3];
		temp[8] = current_orientation[36+0];
		// fix orange face
		temp[36+0] = current_orientation[27+8];
		temp[36+3] = current_orientation[27+5];
		temp[36+6] = current_orientation[27+2];
	}
	else if (color == 0x5) { // white
		// fix white face
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

// takes in a 54-byte array and condenses it down to 21 bytes by removing the 5 most significant bits
void shift_orientation(char* current_orientation){
	shift_helper(current_orientation, 15, 16, 17, 47,46,45,44,43,42,41,40);
	shift_helper(current_orientation, 12, 13, 14 ,39,38,37,36,35,34,33,32);
	shift_helper(current_orientation, 9, 10, 11, 31,30,29,28,27,26,25,24);
	shift_helper(current_orientation, 6, 7, 8, 23,22,21,20,19,18,17,16);
	shift_helper(current_orientation, 3, 4, 5, 15,14,13,12,11,10,9,8);
	shift_helper(current_orientation, 0, 1, 2, 7,6,5,4,3,2,1,0);
	final_shift_helper(current_orientation);
}

// takes in 8 bytes and condenses them into 3 bytes by removing the 5 most significant bits and 
// shifting and or'ing the remaining bits together.
void shift_helper(char* current_orientation, int shift_byte0, int shift_byte1, int shift_byte2, int ori_byte0, int ori_byte1, int ori_byte2, int ori_byte3, int ori_byte4, int ori_byte5, int ori_byte6, int ori_byte7) {
    char byte0;
    char byte1;
    char byte2;
  
    byte2 = (current_orientation[ori_byte2] << 6) | (current_orientation[ori_byte1] << 3) | current_orientation[ori_byte0];
    byte1 = (current_orientation[ori_byte5] << 7) | (current_orientation[ori_byte4] << 4) |  (current_orientation[ori_byte3] << 1) | (current_orientation[ori_byte2] >> 2);
    byte0 = (current_orientation[ori_byte7] << 5) | (current_orientation[ori_byte6] << 2) | (current_orientation[ori_byte5] >> 1);
  
    shifted[shift_byte0] = byte0;
    shifted[shift_byte1] = byte1;
    shifted[shift_byte2] = byte2;
}

// Since 54 is not divisible by 8, we needed a special helper function for the final elements of the array
void final_shift_helper(char* current_orientation){
    shifted[18] = (current_orientation[48] << 5) | (current_orientation[49] << 2) | (current_orientation[50] >> 1);
    shifted[19] = (current_orientation[50] << 7) | (current_orientation[51] << 4) |  (current_orientation[52] << 1) | (current_orientation[53] >> 2);
    shifted[20] = (current_orientation[53] << 6) | (0 << 3) | 0;

}