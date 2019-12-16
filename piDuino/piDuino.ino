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

struct tempHumRecord {
  uint64_t sensAddr;
  float temp;
  float hum;
  float heatInd;
};
typedef struct tempHumRecord Record;
typedef enum hr{ NO_SIG, ON, OFF, NONE } HeaterRequest;
typedef enum pc{ GET_DATA, HEATER_ON, HEATER_OFF, TEST_HEATER, SEND_ALIVE } PiCommand;
PiCommand piCommand;

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
  radio.openWritingPipe(isForwarding ? forwarderAddress: heaterAddress);
  radio.stopListening(); // receive incoming requests from master node
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  setupRadio();
  Serial.println("Arduino is ready!");
  Serial.println("0. Get data");
  Serial.println("1. Turn on the heater");
  Serial.println("2. Turn off the heater");
  Serial.println("3. Test the connection to the heater");
  Serial.println("4. Send alive signal");
}

void loop() {
  // wait for a command from rasPi 
  while (!Serial.available());
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
    case SEND_ALIVE:
      sendAlive();
      break;
    default:
      // nothing
      break;
  }
}

bool passCommandToReceiver(HeaterRequest command) {
  // this function sends heaterRequest to the arduino attached to the heater
  return radio.write( &command, sizeof(HeaterRequest)); 
}

bool turnOnTheHeater() {
  return passCommandToReceiver(ON);
}

bool turnOffTheHeater() {
  return passCommandToReceiver(OFF);
}

void sendAlive() {
  if(passCommandToReceiver(NONE)){
    Serial.println("SUCCESS: Alive signal is sent!");
  } else {
    Serial.println("ERROR: Alive signal is not sent!");
  }
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
      // wait for reading
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
