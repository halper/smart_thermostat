#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "DHT.h"

#define DHTPIN 2     // what digital pin we're connected to

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

RF24 radio(9,10);
const uint64_t rAddress[] = {0x7878787878LL, 0xB3B4B5B6F1LL};
//, 0xB3B4B5B6CDLL, 0xB3B4B5B6A3LL, 0xB3B4B5B60FLL, 0xB3B4B5B605LL };
const short sizeOfAddr = 2;
const uint64_t selfAddress = rAddress[0];
const uint64_t heaterAddress = rAddress[sizeOfAddr - 1];
const float testingTempPoint = 24.0;

struct tempHumRecord {
  uint64_t sensAddr;
  float temp;
  float hum;
  float heatInd;
};
typedef struct tempHumRecord Record;
typedef enum hs{ ON, OFF, SEND, NONE } HeaterStatus;
typedef enum pc{ GET_DATA, HEATER_ON, HEATER_OFF, HEATER_STATUS, TEST_HEATER, TEST_TEMP } PiCommand;
PiCommand piCommand;

void setup() {
  Serial.begin(9600);
  dht.begin();
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel( 108 );
  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.stopListening();
  radio.openWritingPipe(heaterAddress);
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
  if (piCommand < 6 && piCommand >= 0) {
    
    switch (piCommand) {
      case GET_DATA:
        getDataFromSensors();
        break;
      case HEATER_ON:
        turnOnTheHeater(); // this should be handled!!!
        break;
      case HEATER_OFF:
        turnOffTheHeater(); // this should be handled!!!
        break;
      case TEST_HEATER:
        testHeaterConnection();
        break;      
      case TEST_TEMP:
        testTemp();
        break;
      case HEATER_STATUS:
        HeaterStatus hStat = getStatusOfHeater();
        String sHeat = hStat == ON ? "ON" : hStat == OFF ? "OFF" : "UNKNOWN";
        Serial.println(sHeat);
        break;
      default:
        // nothing
        break;
    }
  }
}

bool turnOnTheHeater() {
  return sendCommandToHeaterArduino(ON);
}

bool turnOffTheHeater() {
  return sendCommandToHeaterArduino(OFF);
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
  // last rAddress is reserved for heater 
  for(int i = 0; i < sizeOfAddr - 1; i++) {
    Record rec = getSensorData(rAddress[i]);
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

bool sendCommandToHeaterArduino(HeaterStatus command) {
  // this function sends heaterRequest to the arduino attached to the heater
    char myCommand = ((int) command) + '0';
    bool rslt = false;
    char incomingAck = '9';
    short retry = 3;
    while(!rslt && retry) {
      rslt = radio.write( &myCommand, sizeof(char) );        
      radio.read(&incomingAck, sizeof(char) );
      if ( command == SEND ) {
        for (int i = 0; i < 2; i++){
        rslt = radio.write( &myCommand, sizeof(char) );        
        radio.read(&incomingAck, sizeof(char) );
        delay(10);
      }
        return incomingAck == '1';
      }
      retry--;
    }    
    return rslt;
}

HeaterStatus getStatusOfHeater() {
  // this function receives the status of the heater from the arduino attached to it
  // 0: Off
  // 1: On  
  return sendCommandToHeaterArduino(SEND) ? ON : OFF;
}

void testHeaterConnection() {
  // this function send turn on signal to heater waits for a predefined secs
  // and then turns it off again
  short delayTime = 10000; 
  Serial.println("Started testing");
  bool rslt = false;
  turnOffTheHeater();
  Serial.println("Heater is off!");
  delay(delayTime);
  turnOnTheHeater();
  Serial.print("End of testing: Heater is on again");
}

void testTemp() {
  unsigned long started_waiting_at = micros();
  bool rslt = false;
  Record rec;
  while (1) { 
    delay(500);
    rec = getSelfSensorData();
    if (micros() - started_waiting_at > 10000000 ){            // If waited longer than 10sec, indicate timeout and exit while loop
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
