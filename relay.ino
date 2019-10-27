#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#define relay 2 // relay pin

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);
typedef enum hr{ ON, OFF, SEND, NONE } HeaterRequests;
HeaterRequests heaterReq;

const uint64_t rAddress[] = {0x7878787878LL, 0xB3B4B5B6F1LL, 0xB3B4B5B6CDLL, 0xB3B4B5B6A3LL, 0xB3B4B5B60FLL, 0xB3B4B5B605LL };
bool newData = false;
// Is the heater connected to the Normally close port?
bool connectedToNC = true;

//1 ON 0 OFF
char currentStat = '1';

void setup() {
  Serial.begin(9600);
  radio.begin();  
  delay(100);
  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_MAX);  // "short range setting" - increase if you want more range AND have a good power supply
  radio.setChannel(108);          // the higher channels tend to be more "open"
  radio.setRetries(15, 15);
  // Open a pipe for PRX to receive data
  radio.openReadingPipe(1, rAddress[1]);
  radio.enableAckPayload();
  radio.startListening();
  pinMode(relay, OUTPUT); //Set relay as output
  
  // if NC is used HIGH turns on else if NO is used HIGH turns off vice versa
  if(connectedToNC)
    digitalWrite(relay, HIGH); // Turn on
  else
    digitalWrite(relay, LOW);  
}

void loop() {
  // listen for the incoming message
  // set the switch state accordingly
  heaterReq = getIncomingState();
  if (newData) {
      switch (heaterReq) {
        case ON:
            Serial.println("ON");
            if(connectedToNC)
              digitalWrite(relay, HIGH); //Turn on
            else
              digitalWrite(relay, LOW); //Turn on
            currentStat = '1';
            break;
        case OFF:
            Serial.println("OFF");
            if(connectedToNC)
              digitalWrite(relay, LOW); //Turn off
            else
              digitalWrite(relay, HIGH); //Turn off
            currentStat = '0';
            break;
        case SEND:
            Serial.println("SEND");
            sendStatusToPiduino();
            break;
        default:
            break;
        }
        newData = false;        
  }
}

HeaterRequests getIncomingState() {
  int newState = (int) NONE; 
  char gotByte = '0'; //used to store payload from transmit module

  unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
 
  bool timeout = false;                                   // Set up a variable to indicate if a response was received or not
  while ( ! radio.available() ){        // While nothing is received
    if (micros() - started_waiting_at > 10000000 ){            // If waited longer than 10sec, indicate timeout and exit while loop
        timeout = true;
        break;
    }     
 }
  if ( timeout ){                                             // Describe the results
    Serial.println("Radio available and getting nothing!");
  } else {
      Serial.println("Receiving radio signal");
      updateAckPayload();
      radio.read( &gotByte, sizeof(char) );  
      newData = true;   
      newState = gotByte - '0';
      Serial.println((HeaterRequests) newState); 
  }
  return ( HeaterRequests ) newState; 
}

void updateAckPayload() {
  radio.writeAckPayload(1, &currentStat, sizeof(char));
}

void sendStatusToPiduino() {
  // send the status of the heater to the arduino attached to the rasPi
  Serial.println("SEND command is received!");
  Serial.print("Sending back the heater stat: ");
  String statusOfHeater = currentStat == '1' ? "ON" : "OFF";
  Serial.println(statusOfHeater);  
}
