
/*Credits:
   Marshall Taylor @ SparkFun Electronics
   Nathan Seidle @ SparkFun Electronics
  April 4, 2017
  https://github.com/sparkfun/CCS811_Air_Quality_Breakout
  https://github.com/sparkfun/SparkFun_CCS811_Arduino_Library


*/
//-------------CO2 VARIABLES-----------------------//
#include <Wire.h>
#include <SparkFunCCS811.h> //Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#include "SEEECRETS.h"
#include <SPI.h>
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>

#define CCS811_ADDR 0x5B //Default I2C Address
//#define CCS811_ADDR 0x5A //Alternate I2C Address

#define PIN_NOT_WAKE 5
#define PIN_NOT_INT 6

CCS811 qualitySensor(CCS811_ADDR);

uint16_t co2Val = 0;
uint16_t tvocVal = 0;

//--------------------------//

//------------TEMP VARIABLES-----------------------//
const int tempPin = A7;
float tempRaw = 0.0;
float tempVar = 0.0;

float humidityVar = 50.0;



//---------------------------//

//WIFI VARIABLES------------------------------------//
int status = WL_IDLE_STATUS;
char serverIP[] = "tigoe.io";

WiFiSSLClient wifi;
HttpClient https = HttpClient(wifi, serverIP, 443);
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

const unsigned long postInterval = 900000; //15 min
unsigned long lastPost = 0;

//-------------------------//

//-------LED VARIABLES------------------------------//
// 501-1000 1 LED
//1001-1500 2 LEDS
//1501-2000 3 LEDS
//2001+ 4 LEDS
const int ledPin1 = 12;
const int ledPin2 = 11;
const int ledPin3 = 10;
const int ledPin4 = 9;

const int connectLED = 2;

void setup() {
  Serial.begin(9600);
//  while (!Serial);

  //---------------C02 SETUP------------------------//
  Wire.begin();

  //Begins the sensor and prints the possible error
  CCS811Core::CCS811_Status_e returnCode = qualitySensor.beginWithStatus();
  Serial.print("CCS811 begin error: ");
  //Pass the error code to a function to print the results
  Serial.println(qualitySensor.statusString(returnCode));

  //Set reads to 60 seconds
  returnCode = qualitySensor.setDriveMode(3); //conflicting documentation, may need "2"
  Serial.print("Mode request error: ");
  Serial.println(qualitySensor.statusString(returnCode));

  //Configure and enable the interrupt line,
  //then print error status
  pinMode(PIN_NOT_INT, INPUT_PULLUP);
  returnCode = qualitySensor.enableInterrupts();
  Serial.print("Interrupt configuation error: ");
  Serial.println(qualitySensor.statusString(returnCode));

  //Configure the wake line
  pinMode(PIN_NOT_WAKE, OUTPUT);
  digitalWrite(PIN_NOT_WAKE, 1); //Start asleep

  //----------------------------//

  //---------WIFI SETUP--------------------------//
   while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    delay(500);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network IP = ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  //--------------------------//

  //-------LED SETUP---------------------------//
  pinMode(ledPin1,OUTPUT);
pinMode(ledPin2,OUTPUT);
pinMode(ledPin3,OUTPUT);
pinMode(ledPin4,OUTPUT);
pinMode(connectLED,OUTPUT);
  
  //--------------------------//
}

void loop() {
  if (digitalRead(PIN_NOT_INT) == 0) { //all loop code goes it here so it only runs when interrupt triggers
    //---------------C02 SENSOR LOOP---------------//

    //Wake up the CCS811 logic engine
    digitalWrite(PIN_NOT_WAKE, 0);
    //Need to wait at least 50 us
    delay(1);

    //set the environment data for CO2 calibration
    qualitySensor.setEnvironmentalData(humidityVar, getTemperature());

    //get quality readings and print to serial
    qualitySensor.readAlgorithmResults();

  co2Val = qualitySensor.getCO2();
  tvocVal = qualitySensor.getTVOC();

    
    Serial.print("CO2[");
    //Returns calculated CO2 reading
    Serial.print(co2Val);
    Serial.print("] tVOC[");
    //Returns calculated TVOC reading
    Serial.print(tvocVal);
    Serial.print("] millis[");
    //Display the time since program start
    Serial.print(millis());
    Serial.print("]");
    Serial.println();


    //Turn on appropriate LEDs
    if(co2Val>500 && co2Val<=1000){
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin2, LOW);
      digitalWrite(ledPin3, LOW);
      digitalWrite(ledPin4, LOW);
    }else if(co2Val>1000 && co2Val<=1500){
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin2, HIGH);
      digitalWrite(ledPin3, LOW);
      digitalWrite(ledPin4, LOW);
    }else if(co2Val>1500 && co2Val<=2000){
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin2, HIGH);
      digitalWrite(ledPin3, HIGH);
      digitalWrite(ledPin4, LOW);
    }else if(co2Val>2000){
      digitalWrite(ledPin1, HIGH);
      digitalWrite(ledPin2, HIGH);
      digitalWrite(ledPin3, HIGH);
      digitalWrite(ledPin4, HIGH);
    }else{
      digitalWrite(ledPin1, LOW);
      digitalWrite(ledPin2, LOW);
      digitalWrite(ledPin3, LOW);
      digitalWrite(ledPin4, LOW);
    }

    if ( status != WL_CONNECTED){
      digitalWrite(connectLED,LOW);
    }else{
      digitalWrite(connectLED,HIGH);
    }


    if(millis()-lastPost>=postInterval) {
      postData();
      lastPost = millis();
    }





    //Now put the CCS811's logic engine to sleep
    digitalWrite(PIN_NOT_WAKE, 1);
    //Need to be asleep for at least 20 us
    delay(1);

  }

}


float getTemperature() { //calculate temp based on TM36 sensor input
  //get temp reading and send to quality sensor
  tempRaw = analogRead(tempPin);
  tempVar = tempRaw / 1024;   //find percentage of input reading
  tempVar = tempVar * 3.3;                     //multiply by 5V to get voltage
  tempVar = tempVar - 0.5;                   //Subtract the offset
  tempVar = tempVar * 100;

//  Serial.println(tempVar);
  return tempVar;
}


void postData() {

  // make a String for the sensor data:
  String sensorData = "{\"CO2\":";
  sensorData += String(co2Val);
  sensorData += ",\"tVOC\":";
  sensorData += String(tvocVal);
  sensorData += "}";

  Serial.println(sensorData);

  //add extra "\" because of the data file inside of the JSON message to the server
  sensorData.replace("\"", "\\\"");

  String httpMsg = "{\"macAddress\": \"3C:71:BF:87:C2:6C\", \"sessionKey\": \"3ACD94E4-D87F-4702-8411-A42D76986891\", \"data\": \"DATA\"}";
  httpMsg.replace("DATA", sensorData);

  //{"macAddress":"3C:71:BF:87:C2:6C","sessionKey":"3ACD94E4-D87F-4702-8411-A42D76986891","data":"{\"CO2\":46.04,\"tVOC\": 300 }"}

  String contentType = "application/json";

  // make the POST request to the hub:
  https.post("/data", contentType, httpMsg);

  // read the status code and body of the response
  int statusCode = https.responseStatusCode();
  String response = https.responseBody();

  Serial.println(httpMsg);
  Serial.print("Status code from server: ");
  Serial.println(statusCode);
  Serial.print("Server response: ");
  Serial.println(response);
  Serial.println();
}
