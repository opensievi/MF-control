// OpenSievi MF Control
// https://github.com/opensievi/MF-control
//
// Copyright (c) Tapio Salonsaari <take@nerd.fi> 2016
//
// See README.md and LICENSE.txt for more info
//

// Libraries
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Own setup
#include "pinout.h"

Adafruit_PCD8544 lcd1 = Adafruit_PCD8544(LCD_SCLK, LCD_SDIN, LCD_DC, LCD1_SCE, LCD_RESET);

// Reset pin hardcoded to 13. See https://forums.adafruit.com/viewtopic.php?f=25&t=38893&sid=2dc5a085b125b0591e1bd50b2cb527ea&start=15
// and DO NOT use pin 13 unless you move this out first!
Adafruit_PCD8544 lcd2 = Adafruit_PCD8544(LCD_SCLK, LCD_SDIN, LCD_DC, LCD2_SCE, 13);

// Software version and writing time
#define SWVERSION "0.1a"
#define SWDATE __DATE__

// Rotary encoder reader from http://www.instructables.com/id/Improved-Arduino-Rotary-Encoder-Reading/?ALLSTEPS
volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent (opposite direction to when aFlag is set)
volatile byte encoderPos = 0; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte if you want to record a larger range than 0-255
volatile byte oldEncPos = 0; //stores the last encoder position value so we can compare to the current reading and see if it has changed (so we know when to print to the serial monitor)
volatile byte reading = 0; //somewhere to store the direct values we read from our interrupt pins before checking to see if we have moved a whole detent


int MainPWRStatus = 0; // Is the main power on?

void RunTroughRelays() {

	char current[50];
	digitalWrite(MAIN_POWER_PIN, LOW);
	MainPWRStatus=1;

	for(int i=0; i < NUM_RELAYS; i++) {
		
		/*lcd.setCursor(0,1);
		sprintf(current,"Relay %d   ", i);
		lcd.print(current);
		digitalWrite(RELAY_START_PIN+i, LOW);
		delay(1000);
		digitalWrite(RELAY_START_PIN+i, HIGH);*/
	
	}

	digitalWrite(MAIN_POWER_PIN, HIGH);
	MainPWRStatus=0;
}

void PinA(){
	cli(); //stop interrupts happening before we read pin values
	reading = PIND & 0xC; // read all eight pin values then strip away all but pinA and pinB's values
	if(reading == B00001100 && aFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
		encoderPos --; //decrement the encoder's position count
		bFlag = 0; //reset flags for the next turn
		aFlag = 0; //reset flags for the next turn
	}
	else if (reading == B00000100) bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
	Serial.println(reading);
	Serial.println("PinA");
	sei(); //restart interrupts
}

void PinB(){
	cli(); //stop interrupts happening before we read pin values
	reading = PIND & 0xC; //read all eight pin values then strip away all but pinA and pinB's values
	if (reading == B00001100 && bFlag) { //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
		encoderPos ++; //increment the encoder's position count
		bFlag = 0; //reset flags for the next turn
		aFlag = 0; //reset flags for the next turn
	}
	else if (reading == B00001000) aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
	Serial.println("PinB");
	sei(); //restart interrupts
}

// Function: Setup
//
// Initialize subsystems and start main loop
//
void setup() {

	Serial.begin(9600);

	Serial.print("Version ");
	Serial.println(SWVERSION);
	Serial.print("Build ");
	Serial.println(SWDATE);
	Serial.println("Initializing");

	// Set up relay pins
	for (int i=0; i < NUM_RELAYS;i++) {
		pinMode(RELAY_START_PIN+i, OUTPUT);
		digitalWrite(RELAY_START_PIN+i, HIGH);
	}

	// Set up other pins
	pinMode(MAIN_POWER_PIN, OUTPUT);
	digitalWrite(MAIN_POWER_PIN, HIGH);

	pinMode(LCD_RESET,OUTPUT);
	pinMode(LCD_DC,OUTPUT);
	pinMode(LCD_SDIN,OUTPUT);
	pinMode(LCD_SCLK,OUTPUT);

	pinMode(LCD1_SCE,OUTPUT);
	pinMode(LCD2_SCE,OUTPUT);

	pinMode(BTN_CANCEL, INPUT_PULLUP);
	pinMode(BTN_ROTARY1, INPUT_PULLUP);
	pinMode(BTN_ROTARY2, INPUT_PULLUP);

	// Interrupts
	attachInterrupt(digitalPinToInterrupt(BTN_ROTARY1),PinA,RISING); // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
	attachInterrupt(digitalPinToInterrupt(BTN_ROTARY2),PinB,RISING); // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)

	lcd1.begin();
	lcd2.begin();

	lcd1.setContrast(60);
	lcd2.setContrast(60);

	lcd1.clearDisplay();

	lcd1.setTextSize(5);
	lcd1.setCursor(10,1);
	lcd1.print("M");

	lcd1.setTextSize(1);
	lcd1.setCursor(1,40);
	lcd1.print(SWVERSION);
	lcd1.display();

	lcd2.clearDisplay();
	lcd2.setTextSize(5);
	lcd2.setCursor(10,1);
	lcd2.print("F");
	
	lcd2.setTextSize(1);
	lcd2.setCursor(1,40);
	lcd2.print(SWDATE);

	lcd2.display();

	delay(5000);
	Serial.println("Going to main loop");

	//DO NOT ENABLE THESE IN PRODUCTION! YOU WILL KILL YOURSELF!
	//RunTroughRelays();
	//delay(2000);
	//RunTroughRelays();

	return;
}

// Function: FuelCut
//
// Turns fuel cut either on or off depending on parameter
//
// Parameters: 1 - Fuel cut on
//	       0 - Fuel cut off
//
// TODO: Move this out from main file
void FuelCut(int cut) {

	if(cut == 1) {
		
		// Turn polarity (2 relays, could do better, really)
		digitalWrite(RELAY_START_PIN+1, LOW);
		digitalWrite( (RELAY_START_PIN+3) , LOW);

	} else {

		digitalWrite(RELAY_START_PIN+1, HIGH);
		digitalWrite( (RELAY_START_PIN+3), HIGH);
	}

	delay(100); // We really need to get rid of these...

	// Turn power on to solenoid, wait 0.2 sec and turn power off
	digitalWrite( (RELAY_START_PIN+5) , LOW);
	delay(2000);

	digitalWrite(RELAY_START_PIN+1, HIGH);
	digitalWrite( (RELAY_START_PIN+3), HIGH);
	digitalWrite( (RELAY_START_PIN+5), HIGH);

	return;
}

// Function: Loop
//
// Main loop
//
unsigned long count = 0;
int fuelcut_status;
int btnStatus=0;

int encoder0Pos = 0;
int encoder0PinALast = LOW;
int n = LOW;

void loop() {

	int btn;
	btn = digitalRead(BTN_CANCEL);

	lcd1.clearDisplay();
	lcd1.setTextSize(3);
	lcd1.print(btn);
	lcd1.display();

	if(btn == HIGH && btnStatus==0) {
		btnStatus=1;
		digitalWrite(MAIN_POWER_PIN, LOW);
	} 
	
	if (btn == LOW && btnStatus == 1) {
		btnStatus=0;
		digitalWrite(MAIN_POWER_PIN, HIGH);
	}

	lcd2.clearDisplay();
	lcd2.setTextSize(3);
	lcd2.print(encoderPos);

	int btn2, btn3;
	btn2 = digitalRead(2);
	btn3 = digitalRead(3);

	lcd2.setCursor(1,15);
	lcd2.print(btn2);
	lcd2.setCursor(20,15);
	lcd2.print(btn3);
	lcd2.display();
	/*if(MainPWRStatus == 0) {
		// Turn main power on
		digitalWrite(MAIN_POWER_PIN, LOW);
		MainPWRStatus = 1;
		fuelcut_status = 0;
	}


	if(millis() - count > 10000) {
		FuelCut(1);
		count = millis();
		fuelcut_status=0;
	}

	if(millis() - count > 5000 && fuelcut_status == 0) {
		FuelCut(0);
		fuelcut_status=1;
	}*/

}
