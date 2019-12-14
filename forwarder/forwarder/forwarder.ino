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
typedef enum hr{ NO_SIG, ON, OFF, NONE } HeaterRequest;
const long timeoutPeriod = 10000000;

void setupRadio() {
  radio.begin();  
  radio.setDataRate( RF24_250KBPS );
  delay(100);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel( CHANNEL );
  //radio.enableAckPayload();
  radio.setRetries(3, 15); // delay, count
  // Open a single pipe for PRX to receive data
  radio.setPayloadSize(sizeof(HeaterRequest));
  radio.openReadingPipe(1, selfAddress);
  radio.openWritingPipe(heaterAddress);
  radio.startListening(); // receive incoming requests from master node
}

void setup() {
  Serial.begin(9600);
  setupRadio();
}

void loop() {
  /**  
   *  TODO: Add sleep to reduce power consumption
   *  TODO: If forwarder is going to be used for more than one node it is
   *        require to listen forwardee and get the address to forward data
   */
 
  // Listen incoming request from master
  // Forward it to the heater node
  while ( ! radio.available() );
  HeaterRequest incomingRequest = getIncomingRequest();
  forwardRequestToHeater(incomingRequest);
}

void forwardRequestToHeater(HeaterRequest command) {
  radio.stopListening();
  if(radio.write( &command, sizeof(HeaterRequest) )) {
    Serial.print("Request is sent! ");
    Serial.println(command);
  } else {
    Serial.println("ERROR Request is not sent!");
  }
  radio.startListening();
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

bool isSignalValid(HeaterRequest statusSignal) {
  return statusSignal != NONE && statusSignal != NO_SIG;
}
