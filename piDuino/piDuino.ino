#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "DHT.h"

// Defines constants
#include "/Users/alper/Documents/MyProj/PiDuinoController/config.h"

#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

RF24 radio(9,10);
const uint64_t rAddress[] = {0x7878787878LL, 0xB3B4B5B6F1LL, 0x34BA33DDF2LL};
const uint64_t selfAddress = rAddress[MASTER_ADDRESS_POS];
const uint64_t heaterAddress = rAddress[HEATER_ADDRESS_POS];
const uint64_t forwarderAddress = rAddress[FORWARDER_ADDRESS_POS];

const bool isForwarding = true;
const long timeoutPeriod = 15000000;

const float testingTempPoint = 24.0;

struct tempHumRecord {
  uint64_t sensAddr;
  float temp;
  float hum;
  float heatInd;
};
typedef struct tempHumRecord Record;
typedef enum hr{ NO_SIG, ON, OFF, SEND, NONE } HeaterRequest;
typedef enum pc{ GET_DATA, HEATER_ON, HEATER_OFF, HEATER_STATUS, TEST_HEATER, TEST_TEMP } PiCommand;
PiCommand piCommand;

void setup() {
  Serial.begin(9600);
  dht.begin();
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel( CHANNEL );
  radio.setAutoAck(true);
  radio.enableAckPayload();
  if (isForwarding) radio.openReadingPipe(1, selfAddress);
  radio.openWritingPipe(isForwarding ? forwarderAddress: heaterAddress);
  radio.stopListening();
  radio.setRetries(15,15); // delay, count
  Serial.println("Arduino is ready!");
  Serial.println("0. Get data");
  Serial.println("1. Turn on the heater");
  Serial.println("2. Turn off the heater");
  Serial.println("3. Get heater status");
  Serial.println("4. Test the connection to the heater");
}

void loop() {
  // wait for a command from rasPi 
  while (!Serial.available()) { } 
  if (Serial.available() > 0) {  
    piCommand = (int) ((char)Serial.read() - '0');
  }  
  switch (piCommand) {
    case GET_DATA:
      getDataFromSensors();
      break;
    case HEATER_ON:
    case HEATER_OFF:
      switchHeater(piCommand);
      break;
    case TEST_HEATER:
      testHeaterConnection();
      break;      
    case TEST_TEMP:
      testTemp();
      break;
    case HEATER_STATUS:
      HeaterRequest hStat = getStatusOfHeater();
      String sHeat = hStat == ON ? "ON" : hStat == OFF ? "OFF" : "UNKNOWN";
      Serial.println(sHeat);
      break;
    default:
      // nothing
      break;
  }
}

bool isForwarderRadioAvailable() {
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
  while ( ! radio.available() ){        // While nothing is received
    if (micros() - waitStart > timeoutPeriod ){   // If waited longer than 10sec, indicate timeout and exit while loop
        return false;
    }     
 }
 return true;
}

bool sendRequest(HeaterRequest command) {
  radio.stopListening();
  bool timeout = false;
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
  
  while(!timeout) {
    timeout = micros() - waitStart > timeoutPeriod;
    if(radio.write( &command, sizeof(HeaterRequest) )) {
      if(radio.isAckPayloadAvailable()){ // clears the internal flag which indicates a payload is available
        HeaterRequest temp = NONE; // we don't need ack response from this transmission so getting rid of it
        radio.read(&temp, sizeof(HeaterRequest));
      }
      return true;
    }
  }
  Serial.println("ERROR: Could not forward request!");
  return false;
}

HeaterRequest getRequestResponse(){
  HeaterRequest incomingStatus = NONE;
  radio.startListening();
  if(isForwarderRadioAvailable()){
    radio.read(&incomingStatus, sizeof(HeaterRequest));
  } else {
    Serial.println("ERROR: Forwarder is not transmitting!");
  }
  radio.stopListening();
  return incomingStatus;
}

HeaterRequest forwardRequest(HeaterRequest command){  
  bool isRequestSent = sendRequest(command);
  return isRequestSent ? getRequestResponse() : NONE;
}

HeaterRequest sendCommandToHeaterArduino(HeaterRequest command) {
  HeaterRequest incomingAck = NONE;
  int numOfSuccess = 0;
  bool timeout = false;
  unsigned long waitStart = micros(); // Set up a timeout period, get the current microseconds
    
  while(numOfSuccess < 5 && ! timeout) {      
    // There is buffering so we need to probe it min 3 times to get updated data
    if(radio.write( &command, sizeof(HeaterRequest) )) {
      if(radio.isAckPayloadAvailable()){
        numOfSuccess++;
        radio.read(&incomingAck, sizeof(HeaterRequest) );
      }      
      delay(30);
    }
    timeout = micros() - waitStart > timeoutPeriod;
  }
  return timeout ? NONE : incomingAck;
}

HeaterRequest passCommandToReceiver(HeaterRequest command) {
  // this function sends heaterRequest to the arduino attached to the heater
  // if there is an error NONE signal is sent
  return isForwarding ? forwardRequest(command): sendCommandToHeaterArduino(command);
}

HeaterRequest getStatusOfHeater() {
  // this function receives the status of the heater from the arduino attached to it
  return passCommandToReceiver(SEND);
}

bool turnOnTheHeater() {
  return passCommandToReceiver(ON) == ON;
}

bool turnOffTheHeater() {
  return passCommandToReceiver(OFF) == OFF;
}

void switchHeater(PiCommand switchCommand) {
  if (switchCommand == HEATER_ON){
    if(turnOnTheHeater()){
      Serial.println("SUCCESS: Turn on command is sent!");
    } else {
      Serial.println("ERROR: Turn on command is not sent!");
    }
  } else if(switchCommand == HEATER_OFF) {
     if(turnOffTheHeater()){
      Serial.println("SUCCESS: Turn off command is sent!");
    } else {
      Serial.println("ERROR: Turn off command is not sent!");
    }
  }  
}

Record getSelfSensorData() {
  Record rec;
  rec.sensAddr = selfAddress;
  rec.hum = dht.readHumidity();
  rec.temp = dht.readTemperature();
  // The heat index is a parameter that takes into account temperature and relative humidity, 
  // to determine the apparent temperature or the human perceived equivalent temperature. 
  rec.heatInd = dht.computeHeatIndex(rec.temp, rec.hum, false);
  return rec;
}

Record getSensorData(uint64_t addr){
  Record rec;
  if (addr == selfAddress) {
    rec = getSelfSensorData();
  } else {
    radio.openWritingPipe(addr);
    char transmitting = '1'; // this is an arbitrary char just to receive data
    if( radio.write( &transmitting, sizeof(char) ) ) {
      // set  radio.enableAckPayload(); and
      // struct tempHumRecord on the other nRF
      if ( radio.isAckPayloadAvailable() ) {
        radio.read(&rec, sizeof(Record)); 
      }
    }
  }
  return rec;
}

void getDataFromSensors() {
  // this function reads data from sensors and pass it for printing
  // last rAddress is reserved for heater/forwarder so subtracting one
  /**
   * Pipes 1-5 should share the first 32 bits. Only the least significant byte should be unique, e.g.
   * openReadingPipe(1,0xF0F0F0F0AA);
   * openReadingPipe(2,0xF0F0F0F066);
   */
  size_t n = sizeof(rAddress)/sizeof(selfAddress);
  for(int i = 0; i < n - 1; i++) {
    uint64_t sensorAddress = rAddress[i];
    if (sensorAddress == heaterAddress || sensorAddress == forwarderAddress) {
      continue;
    }
    Record rec = getSensorData(sensorAddress);
    printSensorData(rec);
  }    
}

void printSensorData(Record rec){
  // this function prints sensor data to 
    // sensor address
    Serial.print((long) rec.sensAddr);
    Serial.print("\t");

    // sensor temp
    Serial.print(rec.temp);
    Serial.print("\t");

    // sensor humidity
    Serial.print(rec.hum);
    Serial.print("\t");

    // sensor heatIndex
    Serial.print(rec.heatInd);
    Serial.println("");
}

void testHeaterConnection() {
  // this function send turn on signal to heater waits for a predefined secs
  // and then turns it off again
  short delayTime = 10000; 
  bool testFailed = false;
  Serial.println("INFO: Started testing.");
  if(turnOffTheHeater()){
    Serial.println("INFO: Heater is off!");
    delay(delayTime);
    if(turnOnTheHeater()){
      Serial.println("INFO: End of testing... Heater is on again.");
    } else {
      Serial.println("ERROR: Couldn't turn on the heater!");
      testFailed = true;
    }
  } else {
    Serial.println("ERROR: Couldn't turn off the heater!");
    testFailed = true;
  }
  if(testFailed) {
    Serial.println("ERROR: Test failed!");
  }
  
}

void testTemp() {
  unsigned long started_waiting_at = micros();
  bool rslt = false;
  Record rec;
  while (1) { 
    delay(500);
    rec = getSelfSensorData();
    if (micros() - started_waiting_at > timeoutPeriod ){            // If waited longer than 10sec, indicate timeout and exit while loop
      break;
    }
    if (rec.temp > testingTempPoint) {
      rslt = turnOnTheHeater();
    }
  }
  if (rslt) {
    turnOffTheHeater();
  }
  Serial.println(rec.temp);    
}
