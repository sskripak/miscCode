#include <WiFiNINA.h>
#include "SEEECRETS.h"
#include <Arduino_LSM6DS3.h>

/*
  based on code created 13 July 2010
  by dlf (Metodo2 srl)
  modified 31 May 2012
  by Tom Igoe

  based on code created 10 Jul 2019
  by Riccardo Rizzo

  This example code is in the public domain.

  Joystick client

  This program reads a joystick on A0 and A1, with
  the pushbutton on pin 5. It uses those values to connect
  to a game server.
  Written and tested on a MKR1000

  created 20 Jun 2012
  modified 20 Feb 2016
  by Tom Igoe
*/

//-----------WIFI Variables------------//
int status = WL_IDLE_STATUS;
WiFiClient client;
char server[] = "game server here";

//----------sensor variables------//
float x, y, z, xRaw, yRaw, zRaw;
long uTimer, dTimer, lTimer, rTimer;

float xL = 0;
float yL = 0;
float zL = 0;
float xH = 0;
float yH = 0;
float zH = 0;


float xOff = 1;
float yOff = 0;
float zOff = 0;

float u = -1; //low z
float d = 1; //high z
float l = -0.8; //low y
float r = 0.75; // high y

long buttonDelay = 250;
long buttonTimer = 0;

String myCommand = "off";
float latestRead = 0;


//---Button Variables---//
const int buttonPin = 4;
boolean buttonState = false;
boolean serverState = false;

int currentButton = HIGH;
int formerButton = HIGH;

const int ledPin = 6;



//------smoothing variables-----//
const int numVals = 15; //number of readings
float smoothersY[numVals]; //array of values to be smoothed
float smoothersZ[numVals]; //array of values to be smoothed
int readIndexY = 0; //current index
int readIndexZ = 0; //current index
float smoothedY = 0; //final smoothed Y value
float smoothedZ = 0; //final smoothed Z value

float addedY = 0;
float addedZ = 0;


void setup() {
  Serial.begin(9600);
  //--------WIFI Setup---------//

  while (WiFi.status() != WL_CONNECTED) { //hang in loop until connected
    // Connect to WPA/WPA2 network.
    status = WiFi.begin(SECRET_SSID, SECRET_PASS);
    // wait 5 seconds for connection:
    delay(5000);
  }
  Serial.println("Connected to wifi"); // indicate that connection has happened
  printWifiStatus();

  //-------Sensor Setup-----//
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");

    while (1);
  }

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Acceleration in G's");
  Serial.println("X\tY\tZ");


  //--------Smoothing Setup--------//
  // initialize all the readings to 0 in order to smooth
  for (int thisReading = 0; thisReading < numVals; thisReading++) {
    smoothersY[thisReading] = 0;
    smoothersZ[thisReading];
  }

  //----Pins Setup----//
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

}

void loop() {
  //---Button Loop---//
  buttonState = buttonRead(buttonPin);
  //Serial.println(buttonState);

  if (client.connected()) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }

  if (buttonState == true) { // if button is pressed, either attempt to connect or disconnect
    if (client.connected()) {
      Serial.println("disconnecting");
      client.print("x");
      client.stop();
    } else {
      Serial.println("connecting");
      client.connect(server, 8080);
    }
  }


  //-----Sensor Loop---//
  if (IMU.accelerationAvailable()) { // read raw accelerometer values and smooth
    IMU.readAcceleration(xRaw, yRaw, zRaw);
    y = smoothingY(yRaw) - yOff;
    z = smoothingZ(zRaw) - zOff;

    if (z > d) { //send d for down
      myCommand = "d";
    } else if (z < u) { //send u for up
      myCommand = "u";
    } else if (y > r) { //send l for left (b/c of the way the arduino is mounted in object
      myCommand = "l";
    } else if (y < l) { //send r for right
      myCommand = "r";
    }else{
      myCommand = "off";
    }
    if (millis() - buttonTimer >= buttonDelay && myCommand != "off") {
      client.print(myCommand);
      Serial.println(myCommand);
      buttonTimer = millis();
    }
  }
}

//----------WIFI Functions-------------//

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

//-------Smoothing Functions----//
float smoothingY(float accVal) {
  addedY = addedY - smoothersY[readIndexY];
  smoothersY[readIndexY] = accVal;
  addedY = addedY + smoothersY[readIndexY];
  readIndexY++;

  // if we're at the end of the array...
  if (readIndexY >= numVals) {
    // ...wrap around to the beginning:
    readIndexY = 0;
  }

  smoothedY = addedY / numVals;
  return smoothedY;

}


float smoothingZ(float accVal) {
  addedZ = addedZ - smoothersZ[readIndexZ];
  smoothersZ[readIndexZ] = accVal;
  addedZ = addedZ + smoothersZ[readIndexZ];
  readIndexZ++;

  // if we're at the end of the array...
  if (readIndexZ >= numVals) {
    // ...wrap around to the beginning:
    readIndexZ = 0;
  }

  smoothedZ = addedZ / numVals;
  return smoothedZ;

}

boolean buttonRead(int daButton) {
  boolean buttonOut = false;
  int currentButton = digitalRead(daButton);
  //  Serial.println(currentButton);
  if (currentButton != formerButton && currentButton == LOW) {
    buttonOut = true;
  }
  //  delay(10);
  formerButton = currentButton;
  return buttonOut;

}
