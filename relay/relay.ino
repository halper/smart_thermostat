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
const long timeoutPeriod = 5000000;

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

void setup() {
  Serial.begin(9600);
  Serial.println("Ready");  
  pinMode(relay, OUTPUT); //Set relay as output
  currentStat = ON;
  // if NC is used HIGH turns on else if NO is used HIGH turns off
  digitalWrite(relay, connectedToNC ? HIGH : LOW);
  setupRadio();
}

void loop() {
  // listen for the incoming message
  // set the switch state accordingly
  // delay(500);  
  while ( ! radio.available() );
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

HeaterRequest getIncomingState() {
  HeaterRequest newState = NONE;  
  Serial.println("Receiving radio signal");      
  radio.read( &newState, sizeof(HeaterRequest) ); 
  Serial.println(newState); 
  return newState; 
}

void switchHeater(HeaterRequest incomingHR) {
  bool requestOn = incomingHR == ON;
  if(currentStat != incomingHR){    
    Serial.print("Switching the heater ");
    Serial.println(requestOn ? "ON" : "OFF");
    currentStat = incomingHR;
    digitalWrite(relay, connectedToNC && requestOn ? HIGH : LOW);
  }
}
