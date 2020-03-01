//TODO: change way o controlling speed of value change so that the larger the tilt
//the greater amount is added to the bri/hue/sat val etc for each update


#include <WiFiNINA.h>
#include <SimpleRotary.h>
#include <LiquidCrystal.h> //Import the LCD library
#include <Arduino_LSM6DS3.h>
#include <SPI.h>
#include <ArduinoHttpClient.h>
#include "SEEECRETS.h"

//code adapted from https://create.arduino.cc/projecthub/microBob/lcd-liquid-crystal-display-e72c74?ref=similar&ref_id=43749&offset=4
//code adapted from ArduinoHueCT by Tom Igoe

//WIFI VARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int status = WL_IDLE_STATUS;
char hueHubIP[] = HUB_ID;
String hueUserName = HUB_USERNAME;

WiFiClient wifi;
HttpClient httpClient = HttpClient(wifi, hueHubIP);
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int bulbNum = 6;

long requestInt = 100;
long requestTime = 0;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//SENSOR VARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
float x, y, z, xRaw, yRaw, zRaw;
long uTimer, dTimer, lTimer, rTimer;

float xL = 0;
float yL = 0;
float zL = 0;
float xH = 0;
float yH = 0;
float zH = 0;


float xOff = 0;
float yOff = 0;
float zOff = 0;

float u = -1; //low z
float d = 1; //high z
float l = -0.8; //low y
float r = 0.75; // high y

//approx range of x is -0.8 to 0.8


String myCommand = "off";
float latestRead = 0;

float highThresh = 0.1;
float lowThresh = -0.15;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//SMOOTHING VARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const int numVals = 15; //number of readings
float smoothersY[numVals]; //array of values to be smoothed
float smoothersZ[numVals]; //array of values to be smoothed
float smoothersX[numVals];
int readIndexY = 0; //current index
int readIndexZ = 0; //current index
int readIndexX = 0;
float smoothedY = 0; //final smoothed Y value
float smoothedZ = 0; //final smoothed Z value
float smoothedX = 0;

float addedY = 0;
float addedZ = 0;
float addedX = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//DISPLAY VARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); /*Initialize the LCD and
                                        tell it which pins is
                                        to be used for communicating*/
//Global Var
#define contra 9 //Define the pin that controls the contrast of the screen
#define bri 10 //Define the pin the controls the brightness of the screen
//Both pins are PWM so you can analogWrite to them and have them output a variable value

//~~~~~~~~~~~~~~~~~~~~~~~~~~~

//SETTINGS VAARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int setPos = 0;
int briVal = 50;
int briMapVal = 0;//0-254
int briMax = 254;
int hueVal = 50; 
int hueMapVal = 0;//0-65535;
int hueMax = 65535;
int satVal = 50; 
int satMapVal = 0;//0-254
int satMax = 254;
boolean lightState = 0;

String settings[3] = {"bri", "hue", "sat"};
int setVal[3] = {briMapVal, hueMapVal, satMapVal};
int setMax[3] = {briMax, hueMax, satMax};

int changeRate = 0;
long lastAdjust = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~

//INPUT VARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SimpleRotary rotary(7, 8, 13);
byte rotaryState = 0;
byte buttonState = 1;
int buttonPin = 6;
int formerButt = 1;
//~~~~~~~~~~~~~~~~~~~~~~~~~


void setup() {//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Serial.begin(9600);
    while (!Serial);

  //WIFI SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    delay(2000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network IP = ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  //~~~~~~~~~~~~~~~~~~~~~~

  //SENSOR SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~

  //SMOOTHING SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // initialize all the readings to 0 in order to smooth
  for (int thisReading = 0; thisReading < numVals; thisReading++) {
    smoothersY[thisReading] = 0;
    smoothersZ[thisReading] = 0;
    smoothersX[thisReading] = 0;
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~

  //DISPLAY SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // put your setup code here, to run once:
  lcd.begin(16, 2); //Tell the LCD that it is a 16x2 LCD
  pinMode(contra, OUTPUT); //set pin 9 to OUTPUT
  pinMode(bri, OUTPUT); //Set pin 10 to OUTPUT
  //pinMode-ing OUTPUT makes the specified pin output power
  digitalWrite(contra, LOW); /*outputs no power to the contrast pin.
                            this lets you see the words*/
  analogWrite(bri, 255); //Outputs full power to the screen brightness LED
  //~~~~~~~~~~~~~~~~~~~~~~~~

  //INPUT SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  pinMode(buttonPin, INPUT_PULLUP);
  //~~~~~~~~~~~~~~~~~~~~~~~

//  sendRequest(bulbNum, "on", "false"); // turn off the light to begin

}//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void loop() {//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  //ROTARY LOOP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  rotaryState = rotary.rotate();
  buttonState = digitalRead(buttonPin);

  //  Serial.println(rotaryState);

  if (rotaryState != 0) changeSet(rotaryState, setPos);   //choose the setting to adjust

  if (buttonState == 0 && formerButt == 1) { //turn light off and on
    offOn();
    delay(20);
  }

  formerButt = buttonState;
  //~~~~~~~~~~~~~~~~~~~~~~~

  //SENSOR LOOP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  if (IMU.accelerationAvailable()) { // read raw accelerometer values and smooth
    IMU.readAcceleration(xRaw, yRaw, zRaw);
    x = smoothingX(xRaw) - xOff;
    y = smoothingY(yRaw) - yOff;
    z = smoothingZ(zRaw) - zOff;
    /*
      Serial.print("X Val: ");
      Serial.print(x);
      Serial.print("  |  Y Val: ");
      Serial.print(y);
      Serial.print("  |  Z Val: ");
      Serial.println(z);
    */
  }

  changeRate = abs(x) * 5; //sets the interval between setting changes based on how much the device is tilted
  //practical range of this val is 20-100, 20 being the fastest change and 100 the slowest

  if (x > highThresh || x < lowThresh) {//if the controller is being tilted, change the settings value and send to hub

    changeVal(); //adjust the value of the current selected setting

    if (millis() - requestTime >= requestInt) { //send request based on current setting at an interval
      sendRequest(bulbNum, settings[setPos], String(setVal[setPos]));
      requestTime = millis();
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~

}//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//SETTINGS FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void changeVal() { //change the settings value
  String daSetting = settings[setPos];

  if (x > highThresh) { //if the controller is tilted to right, then increase the selected setting
    if (daSetting == "bri") {
      briVal += changeRate;
      briVal = constrain(briVal, 1, 100);
      briMapVal = map(briVal, 1, 100, 1, briMax);
      setVal[0] = briMapVal;
    } else if (daSetting == "sat") {
      satVal += changeRate;
      satVal = constrain(satVal, 1, 100);
      satMapVal = map(satVal, 1, 100, 1, satMax);
      setVal[2] = satMapVal;
    } else if (daSetting == "hue") {
      hueVal += changeRate;
      hueVal = constrain(hueVal, 1, 100);
      hueMapVal = map(hueVal, 1, 100, 1, hueMax);
      setVal[1] = hueMapVal;
    }
  }
  if (x < lowThresh) { //if the controller is tilted left, decrease the selected setting
    if (daSetting == "bri") {
      briVal -= changeRate;
      briVal = constrain(briVal, 1, 100);
      briMapVal = map(briVal, 1, 100, 1, briMax);
      setVal[0] = briMapVal;
    } else if (daSetting == "sat") {
      satVal -= changeRate;
      satVal = constrain(satVal, 1, 100);
      satMapVal = map(satVal, 1, 100, 1, satMax);
      setVal[2] = satMapVal;
    } else if (daSetting == "hue") {
      hueVal -= changeRate;
      hueVal = constrain(hueVal, 1, 100);
      hueMapVal = map(hueVal, 1, 100, 1, hueMax);
      setVal[1] = hueMapVal;
    }
  }
  /*
    Serial.print("Bri: ");
    Serial.print(briVal);
    Serial.print("  |  ");
    Serial.print("Hue: ");
    Serial.print(hueVal);
    Serial.print("  |  ");
    Serial.print("Sat: ");
    Serial.println(satVal);
  */

}

void changeSet(int daDir, int formerSetPos) { //change the setting to adjust
  if (daDir == 1) {
    setPos++; //if turning CW then increase setting
  } else if (daDir == 2) setPos--; //turning CCW decrease setting
  if (setPos > 2) setPos = 0; //if you turn past the 3rd setting go back to the first
  if (setPos < 0) setPos = 2; // if you turn down past zero than go to 2

  Serial.print(setPos);
  Serial.println(settings[setPos]);
}

void offOn() { // turn off or on light
  if (lightState == 0) {
    lightState = 1;
    sendRequest(bulbNum, "on", "true");
  }
  else if (lightState == 1) {
    lightState = 0;
    sendRequest(bulbNum, "on", "false");
  }
  Serial.print("lightState: ");
  Serial.println(lightState);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//SMOOTHING FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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


float smoothingX(float accVal) {
  addedX = addedX - smoothersX[readIndexX];
  smoothersX[readIndexX] = accVal;
  addedX = addedX + smoothersX[readIndexX];
  readIndexX++;

  // if we're at the end of the array...
  if (readIndexX >= numVals) {
    // ...wrap around to the beginning:
    readIndexX = 0;
  }

  smoothedX = addedX / numVals;
  return smoothedX;

}


//WIFI FUNCTIONS~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void sendRequest(int light, String cmd, String value) {
  // make a String for the HTTP request path:
  String request = "/api/" + hueUserName;
  request += "/lights/";
  request += light;
  request += "/state/";

  String contentType = "application/json";

  // make a string for the JSON command:
  String hueCmd = "{\"" + cmd;
  hueCmd += "\":";
  hueCmd += value;
  hueCmd += "}";
  // see what you assembled to send:
  Serial.print("PUT request to server: ");
  Serial.println(request);
  Serial.print("JSON command to server: ");
  Serial.println(hueCmd);
  // make the PUT request to the hub:
  httpClient.put(request, contentType, hueCmd);

  // read the status code and body of the response
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  Serial.println(hueCmd);
  Serial.print("Status code from server: ");
  Serial.println(statusCode);
  Serial.print("Server response: ");
  Serial.println(response);
  Serial.println();
}
//~~~~~~~~~~~~~~~~~
