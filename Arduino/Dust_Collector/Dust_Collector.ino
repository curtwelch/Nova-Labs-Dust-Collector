//
// Curt Welch <curt@kcwc.com> . 12-4-2017
// For Nova-Labs.com -- custom dust collector full sensor
//
// Supports IR dust full sensor and door open sensor for dust cabinet.
//

#include <Adafruit_SSD1306.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define LEDPIN 13
  // Pin 13: Arduino has an LED connected on pin 13
  // Pin 11: Teensy 2.0 has the LED on pin 11
  // Pin  6: Teensy++ 2.0 has the LED on pin 6
  // Pin 13: Teensy 3.0 has the LED on pin 13
 
#define SENSORPIN 2                       // IR Receiver
#define RELAYPIN 3                        // Relay Driver LOW Turns relay on and Trips FULL STOP
#define DOORPIN 5                         // Magnetic reed switch on door  NC               

int sensorBlocked = 0, lastState=0;       // variable for reading the pushbutton status
unsigned long Dust_start_time = 0;        // Millisonds time when sensor blocked
int Dust_seconds = 0;                     // Dust seconds last display value
int Collector_full = 0;                   // Collector in FULL state -- dust collector locked off
#define FULL_TIME   30000                 // ms of dust before FULL state activates
int Blink_on = 0;                         // 0 text off; 1 text on
unsigned long Full_on_start = 0;          // mills() text on time
int lastDoorState = 0;
int displayNeeded = 0;
int doorOpen = 0;

void setup() {
  // initialize the LED pin as an output:
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW); 
  // And the Relay Control PIN
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);

  pinMode(DOORPIN, INPUT);     
  digitalWrite(DOORPIN, HIGH); // turn on the pullup
  
  sensor_setup();
  display_setup();
  
  Serial.begin(9600);
}

void sensor_setup() {     
  // initialize the sensor pin as an input:
  pinMode(SENSORPIN, INPUT);     
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup
}

void display_setup() {
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
    // Clear the buffer.
  ready();
}

void showtext(int x, int y, int s, char *msg) {
  display.clearDisplay();
  display.setTextSize(s);
  display.setTextColor(WHITE);
  display.setCursor(x,y);
  display.println(msg);
  display.display();
}

void loop() {
  
  doorOpen = !digitalRead(DOORPIN);
  
  sensorBlocked = !digitalRead(SENSORPIN); // Dust sensor

  // check if the sensor beam is broken
  // if it is, the sensorState is LOW:
  if (sensorBlocked == LOW) {     
    // turn LED off:
    digitalWrite(LEDPIN, LOW);  
  } else {
    // turn LED off:
    digitalWrite(LEDPIN, HIGH); 
  }

  unsigned long now = millis();
  
  if (sensorBlocked && !lastState) {
    // Beam just now blocked
    Dust_start_time = now;
    Dust_seconds = -1;
  }
  
  if (sensorBlocked && lastState) {
     // Still blocked
     if (!Collector_full && (now - Dust_start_time > FULL_TIME)) {
        Collector_full = 1;
        Full_on_start = 0; // 0 means start new On Off cycle
     }
  }

  if (sensorBlocked && !Collector_full && !doorOpen) {
    // Display Dust message countdown
    int sec = (FULL_TIME - now + Dust_start_time)/1000 + 1;
    if (Dust_seconds != sec || displayNeeded) {
      char buf[] = "DUST  X";
      int tens = sec / 10;
      int units = sec % 10;
      if (tens > 0) {
        buf[5] = '0' + tens;
      }
      buf[6] = '0' + units;             // only works for single digits of course
      showtext(0, 40, 3, buf);
      Dust_seconds = sec;
      displayNeeded = 0;
    }
  }

  if (Collector_full || doorOpen) {
    // Blink the FULL or DOOR OPEN message!
    if (Full_on_start && now-Full_on_start > 400) {
      Full_on_start = 0;
    }
    if (Blink_on && Full_on_start && now-Full_on_start > 200) {
      showtext(0, 0, 5, "");
      Blink_on = 0;               // Blink Text is OFF now
    }
    if (Full_on_start == 0 || displayNeeded) {
      Full_on_start = now;
      collectorOff();
      if (doorOpen) {
        showtext(0, 5, 4, "DOOR\nOPEN");
      } else {
        showtext(5, 17, 5, "FULL");
      }
      displayNeeded = 0;

      //Serial.println("FULL");
      Blink_on = 1;               // Blink Text is OFF now
    }
  }
  
  if ((!sensorBlocked && lastState)) {
    // Senosr changed to no dust
    Collector_full = 0;
    if (!doorOpen)
      ready();
  }

  if (doorOpen && !lastDoorState) {
    // Door just now opened
    displayNeeded = 1; // force display update now
  }

  if (!doorOpen && lastDoorState) {
    // Door changed from open to closed
    if (!sensorBlocked && !Collector_full)
      ready();      // display and turn collector on
    if (sensorBlocked && !Collector_full) {
      collectorOn(); // Don't display READY becuase we are in dust mode but allow collector on()
      displayNeeded = 1;
    }
  }

  lastState = sensorBlocked;
  lastDoorState = doorOpen;
}

void ready() {
    showtext(5,5,4,"READY");
    collectorOn();
}

void collectorOff() {
  digitalWrite(RELAYPIN, LOW);
  // Causes collector to go into full state and turn off instantly
  // Collector locked out when Relay On like this
}

void collectorOn() {
  digitalWrite(RELAYPIN, HIGH);
  // Remove dust collector lock out
  // Allows collector to be turned on again but requires a manual reset
  // with the STOP button to do so.
}


