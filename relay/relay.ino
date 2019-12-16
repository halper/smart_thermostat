#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "/Users/alper/Documents/MyProj/PiDuinoController/config.h"

#define relay 5 // relay pin

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);
// This must be same accross all nodes!!!
typedef enum hr{ NO_SIG, ON, OFF, NONE } HeaterRequest;
HeaterRequest heaterReq;
HeaterRequest currentStat;

// This must be same accross all nodes!!!
const uint64_t rAddress[] = {0x7878787878LL, 0xB3B4B5B6F1LL, 0x34BA33DDF2LL};
const uint64_t selfAddress = rAddress[HEATER_ADDRESS_POS];
const uint64_t masterAddress = rAddress[MASTER_ADDRESS_POS];
const uint64_t forwarderAddress = rAddress[FORWARDER_ADDRESS_POS];

// Is the heater connected to the Normally close port?
bool connectedToNC = true;
const bool isForwarding = true;

// If we don't have radio communication for some hours
// the heater will switch state
const long onTimeoutPeriod = 1000*60*60*2; // 2 hours
const long offTimeoutPeriod = onTimeoutPeriod*2;
long timeoutPeriod = offTimeoutPeriod;
unsigned long timer = millis();
bool communicationError = false;

void setupRadio() {
  radio.begin();  
  radio.setDataRate( RF24_250KBPS );
  delay(100);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(CHANNEL); 
  // radio.enableAckPayload();         
  radio.setRetries(3, 15);
  radio.setPayloadSize(sizeof(HeaterRequest));
  radio.openReadingPipe(1, selfAddress);  
  radio.startListening();  
}
const int ledPin =  LED_BUILTIN;// the number of the LED pin

// Variables will change:
int ledState = LOW;             // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change:
const long interval = 500;           // interval at which to blink (milliseconds)
void setupLED() {
   pinMode(ledPin, OUTPUT);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Ready");  
  pinMode(relay, OUTPUT); //Set relay as output
  currentStat = ON;
  // if NC is used HIGH turns on else if NO is used HIGH turns off
  digitalWrite(relay, connectedToNC ? HIGH : LOW);
  setupRadio();
  setupLED();
}

void loop() {
  // listen for the incoming message
  // set the switch state accordingly
  // delay(500);  
  while ( ! radio.available() ){
    if(millis() - timer >= timeoutPeriod) {
      if (currentStat == OFF) {
        currentStat = ON;
        timeoutPeriod = onTimeoutPeriod;
      } else {
        currentStat = OFF;
        timeoutPeriod = offTimeoutPeriod;
      }
      
      switchHeater(currentStat);
      timer = millis();
      communicationError = true;
    }
    if (communicationError) {
      blinkLED();
    }
  }
  heaterReq = getIncomingState();
  switch (heaterReq) {
    case ON:
    case OFF:
        switchHeater(heaterReq);
        break;
    default:
        break;       
  }
  
}

void blinkLED(){
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
}

HeaterRequest getIncomingState() {
  communicationError = false;
  timer = millis();
  HeaterRequest newState = NONE;  
  Serial.println("Receiving radio signal");      
  radio.read( &newState, sizeof(HeaterRequest) ); 
  Serial.println(newState); 
  return newState; 
}

void switchHeater(HeaterRequest incomingHR) {
  if (incomingHR == ON || incomingHR == OFF) {
    bool requestOn = incomingHR == ON;
    if(currentStat != incomingHR){    
      Serial.print("Switching the heater ");
      Serial.println(requestOn ? "ON" : "OFF");
      currentStat = incomingHR;
      digitalWrite(relay, connectedToNC && requestOn ? HIGH : LOW);
    }
  }
}
