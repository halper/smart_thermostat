#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Defines constants
// #include "/Users/alper/Documents/MyProj/PiDuinoController/config.h"

RF24 radio(9,10);
// This must be same accross all nodes!!!
const uint64_t rAddress[] = {0x7878787878LL, 0xB3B4B5B6F1LL, 0x34BA33DDF2LL};
const uint64_t masterAddress = rAddress[0];
const uint64_t heaterAddress = rAddress[1];
const uint64_t forwarderAddress = rAddress[2];
const uint64_t selfAddress = forwarderAddress;

// This must be same accross all nodes!!!
typedef enum hr{ NO_SIG, ON, OFF, SEND, NONE } HeaterRequest;

void setup() {
  Serial.begin(9600);
  radio.begin();
  delay(100);  
  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel( 108 );
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
  if (isRadioAvailable()) {
    HeaterRequest incomingRequest = getIncomingRequest();
    HeaterRequest ackedRequest = getAckedRequest(incomingRequest);
    forwardAckedRequestToMaster(ackedRequest);
  }
  delay(500);

}

void forwardAckedRequestToMaster(HeaterRequest ackedRequest) {
  int numOfTries = 5;
  if(isSignalValid(ackedRequest)) {
    Serial.print("INFO: getAckedRequest is successfull - ");
    Serial.println(ackedRequest);
    while(!isSignalValid(forwardIncomingRequest(ackedRequest, masterAddress)) && numOfTries--) {
      delay(500);
    }
  }
}

HeaterRequest forwardIncomingRequest(HeaterRequest incomingRequest, uint64_t nodeAddress) {
  HeaterRequest ackedRequest = NO_SIG;
  radio.stopListening();
  radio.openWritingPipe(nodeAddress);
  Serial.print("Forwarding request to ");
  Serial.println(nodeAddress == masterAddress ? "Master node": "Heater node");
  if(radio.write( &incomingRequest, sizeof(HeaterRequest) )) {
    Serial.println("INFO: Forward is successfull!");
    if(radio.isAckPayloadAvailable()){
      radio.read(&ackedRequest, sizeof(HeaterRequest) );
      if(isSignalValid(ackedRequest)){
        Serial.println("INFO: Forward and ack is successfull!");
      }
    }
  }
  radio.startListening();
  return ackedRequest;
}

HeaterRequest forwardIncomingRequestToHeater(HeaterRequest incomingRequest) {
  int numOfSuccess = 0;
  HeaterRequest incomingAck = NONE;
  bool timeout = false;
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
  long timeoutPeriod = 10000000;
 
  while(numOfSuccess < 5 && ! timeout) {      
    // There is buffering so we need to probe it min 3 times to get updated data
    incomingAck = forwardIncomingRequest(incomingRequest, heaterAddress);
    if(isSignalValid(incomingAck)) {
      numOfSuccess++;      
      delay(30);
    }
    timeout = micros() - waitStart > timeoutPeriod;
  }
  if(timeout) {
    Serial.println("ERROR: Timeout during forward to the heater!");
  }
  return timeout ? NONE : incomingAck;
}

HeaterRequest getAckedRequest(HeaterRequest incomingRequest) {
  if(incomingRequest == NO_SIG) {
    return NO_SIG;
  }
  return forwardIncomingRequestToHeater(incomingRequest);
}

HeaterRequest getIncomingRequest() {
  HeaterRequest incomingRequest = NO_SIG;
  radio.read( &incomingRequest, sizeof(HeaterRequest) );
  if(isSignalValid(incomingRequest)) {
    Serial.print("INFO: Received incoming request - ");
    Serial.println(incomingRequest);
  } else {
    Serial.println("ERROR: Couldn't read incoming request!");
  }
  return incomingRequest;
}

bool isRadioAvailable() {
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
  long timeOutPeriod = 10000000;
  Serial.println("Listening for radio...");
  while ( ! radio.available() ){        // While nothing is received
    if (micros() - waitStart > timeOutPeriod ){
        return false;
    }    
 }
 return true;
}

bool isSignalValid(HeaterRequest statusSignal) {
  return statusSignal != NONE && statusSignal != NO_SIG;
}
