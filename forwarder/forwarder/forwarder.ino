#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Defines constants
#include "/Users/alper/Documents/MyProj/PiDuinoController/config.h"

RF24 radio(9,10);
// This must be same accross all nodes!!!
const uint64_t rAddress[] = {0x7878787878LL, 0xB3B4B5B6F1LL, 0x34BA33DDF2LL};
const uint64_t masterAddress = rAddress[MASTER_ADDRESS_POS];
const uint64_t heaterAddress = rAddress[HEATER_ADDRESS_POS];
const uint64_t forwarderAddress = rAddress[FORWARDER_ADDRESS_POS];
const uint64_t selfAddress = forwarderAddress;

// This must be same accross all nodes!!!
typedef enum hr{ NO_SIG, ON, OFF, SEND, NONE } HeaterRequest;
const long timeoutPeriod = 10000000;

void setup() {
  Serial.begin(9600);
  radio.begin();
  delay(500);  
  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel( CHANNEL );
  radio.enableAckPayload();
  //radio.openWritingPipe(heaterAddress);
  radio.setRetries(15,15); // delay, count
  // Open a single pipe for PRX to receive data
  radio.openReadingPipe(1, selfAddress);
  radio.startListening();
}

void loop() {
  /**  
   *  TODO: Add sleep to reduce power consumption
   *  TODO: If forwarder is going to be used for more than one node it is
   *        require to listen forwardee and get the address to forward data
   */
 
  // Listen incoming request from master
  // Forward it to the heater node
  // Forward incoming Ack Result to the master node
  HeaterRequest incomingRequest = getIncomingRequest();
  HeaterRequest ackedRequest = getAckedRequest(incomingRequest);
  forwardAckedRequestToMaster(ackedRequest);
  delay(500);

}

void forwardAckedRequestToMaster(HeaterRequest ackedRequest) {
  int numOfTries = 5;
  if(isSignalValid(ackedRequest)) {
    Serial.print("INFO: getAckedRequest is successful - ");
    Serial.println(ackedRequest);
    while(!isSignalValid(forwardIncomingRequest(ackedRequest, masterAddress)) && numOfTries--) {
      delay(500);
    }
  }
}

HeaterRequest forwardIncomingRequest(HeaterRequest incomingRequest, uint64_t nodeAddress) {
  radio.openWritingPipe(nodeAddress);
  Serial.print("Forwarding request to ");
  Serial.println(nodeAddress == masterAddress ? "Master node": "Heater node");  
  return sendRequest(incomingRequest) ? incomingRequest : NONE;
}

bool sendRequest(HeaterRequest command) {
  radio.stopListening();
  bool timeout = false;
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
  bool isSent = false;
  while(!timeout && !isSent) {
    timeout = micros() - waitStart > timeoutPeriod;
    if(radio.write( &command, sizeof(HeaterRequest) )) {
      if(radio.isAckPayloadAvailable()){ // clears the internal flag which indicates a payload is available
        HeaterRequest temp = NONE; // we don't need ack response from this transmission so getting rid of it
        radio.read(&temp, sizeof(HeaterRequest));
      }
      Serial.print("Request is sent! ");
      Serial.println(command);
      isSent = true;
    }
  }
  radio.startListening();
  return !timeout;
}

HeaterRequest forwardIncomingRequestToHeater(HeaterRequest incomingRequest) {
  radio.openWritingPipe(heaterAddress);
  bool isRequestSent = sendRequest(incomingRequest);
  return isRequestSent ? getIncomingRequest() : NONE;
}

HeaterRequest getAckedRequest(HeaterRequest incomingRequest) {
  if(incomingRequest == NO_SIG) {
    return NO_SIG;
  }
  return forwardIncomingRequestToHeater(incomingRequest);
}

HeaterRequest getIncomingRequest() {
  HeaterRequest incomingRequest = NO_SIG;
  if(isRadioAvailable()){
    radio.read( &incomingRequest, sizeof(HeaterRequest) );
    if(isSignalValid(incomingRequest)) {
      Serial.print("INFO: Received incoming request - ");
      Serial.println(incomingRequest);
    } else {
      Serial.println("ERROR: Couldn't read incoming request!");
    }
    return incomingRequest;
  }
}

bool isRadioAvailable() {
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
  Serial.println("Listening for radio...");
  while ( ! radio.available() ){        // While nothing is received
    if (micros() - waitStart > timeoutPeriod ){
        return false;
    }    
 }
 return true;
}

bool isSignalValid(HeaterRequest statusSignal) {
  return statusSignal != NONE && statusSignal != NO_SIG;
}
