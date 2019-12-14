#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "/Users/alper/Documents/MyProj/PiDuinoController/config.h"

#define relay 5 // relay pin

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);
// This must be same accross all nodes!!!
typedef enum hr{ NO_SIG, ON, OFF, SEND, NONE } HeaterRequest;
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

void setup() {
  Serial.begin(9600);
  Serial.println("Ready");
  radio.begin();  
  delay(100);
  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_MAX);  // "short range setting" - increase if you want more range AND have a good power supply
  radio.setChannel(CHANNEL); 
  radio.enableAckPayload();         
  radio.setRetries(15, 15);
  // Open a single pipe for PRX to receive data
  radio.openReadingPipe(1, selfAddress);
  radio.openWritingPipe(isForwarding ? forwarderAddress: masterAddress);
  pinMode(relay, OUTPUT); //Set relay as output
  currentStat = ON;
  // if NC is used HIGH turns on else if NO is used HIGH turns off
  digitalWrite(relay, connectedToNC ? HIGH : LOW);
  radio.startListening();  
}

void loop() {
  // listen for the incoming message
  // set the switch state accordingly
  // delay(500);  
  heaterReq = getIncomingState();
  switch (heaterReq) {
    case ON:
    case OFF:
        switchHeater(heaterReq);
        break;
    case SEND:
        Serial.println("SEND");
        sendStatusToPiduino();
        break;
    default:
        break;       
  }
}

bool isRadioAvailable() {
  unsigned long started_waiting_at = micros(); // Set up a timeout period, get the current microseconds

  while ( ! radio.available() ){        // While nothing is received
    if (micros() - started_waiting_at > 10000000 ){   // If waited longer than 10sec, indicate timeout and exit while loop
        return false;
    }     
 }
 return true;
}

HeaterRequest getIncomingState() {
  HeaterRequest newState = NONE;
  if ( isRadioAvailable() ){
      Serial.println("Receiving radio signal");      
      radio.read( &newState, sizeof(HeaterRequest) ); 
      Serial.println(newState); 
  }  
  return newState; 
}

bool replyBack(HeaterRequest stat) {
  //radio.writeAckPayload(1, &stat, sizeof(HeaterRequest));
  radio.stopListening();
  bool timeout = false;
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
  bool isSent = false;
  while(!timeout && !isSent) {
    delay(500);
    timeout = micros() - waitStart > timeoutPeriod;
    if(radio.write( &stat, sizeof(HeaterRequest) )) {
      if(radio.isAckPayloadAvailable()){ // clears the internal flag which indicates a payload is available
        HeaterRequest temp = NONE; // we don't need ack response from this transmission so getting rid of it
        radio.read(&temp, sizeof(HeaterRequest));
      }
      isSent = true;
    }    
  }
  radio.startListening();
  return !timeout;
}

void switchHeater(HeaterRequest incomingHR) {
  currentStat = incomingHR;
  bool requestOn = incomingHR == ON;
  Serial.print("Switching the heater ");
  Serial.println(requestOn ? "ON" : "OFF");
  digitalWrite(relay, connectedToNC && requestOn ? HIGH : LOW);
  replyBack(currentStat); 
}

void sendStatusToPiduino() {
  // send the status of the heater to the arduino attached to the rasPi
  Serial.println("SEND command is received!");
  Serial.print("Sending back the heater stat: ");
  String statusOfHeater = currentStat == ON ? "ON" : "OFF";
  Serial.println(statusOfHeater); 
  replyBack(currentStat);  
}
