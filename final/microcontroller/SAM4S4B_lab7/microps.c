/*
Aaron Trujillo & Pinky King
atrujillo@g.hmc.edu pking@g.hmc.edu
Fall 2019

'setup_rot_encoder' and rotary encoder functionality was adapted from:
 http://eeshop.unl.edu/pdf/KEYES%20Rotary%20encoder%20module%20KY-040.pdf
*/

#include "SAM4S4B_pio.h"

#DEFINE pinCW = PIOA_PA15; // Connected to CLK on KY­040
#DEFINE pinCCW = PIOA_PA16; // Connected to DT on KY­040

int encoderPosCount = 0;
int pinCWLast;
int aVal;
boolean bCW;

void setup_rot_encoder() {
pinMode(pinCW,INPUT);
pinMode(pinCCW,INPUT);
/* Read Pin A
Whatever state it's in will reflect the last position
*/
pinCWLast = digitalRead(pinCW);
}

int main() {
    setup_rot_encoder();
    for(1){
        rot = digitalRead(pinCW);
        if (rot != pinCWLast){ // Means the knob is rotating
            // if the knob is rotating, we need to determine direction
            // We do that by reading pin B.
            if (digitalRead(pinCCW) != rot) { // Means pin A Changed first ­ We're Rotating Clockwise
            encoderPosCount++;
            rotCW = true;
            } else { // Otherwise B changed first and we're moving CCW
            rotCW = false;
            encoderPosCount­­--;
            }
            Serial.print ("Rotated: ");
            if (rotCW){
            Serial.println ("clockwise");
            }else{
            Serial.println("counterclockwise");
            }
            Serial.print("Encoder Position: ");
            Serial.println(encoderPosCount);
        }
        pinCWLast = rot
    }
return 0
}