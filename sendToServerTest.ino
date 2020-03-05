/*based on code by Tom Igoe (tigoe)
https://github.com/tigoe/Wifi101_examples/blob/master/SensorPost/SensorPost.ino


*/

#include <SPI.h>
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include "SEEECRETS.h"

//WIFI VARIABLES~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int status = WL_IDLE_STATUS;
char serverIP[] = PLACEHOLDER;

WiFiSSLClient wifi;
HttpClient https = HttpClient(wifi, serverIP, 443);
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

uint16_t co2Val = 0;
uint16_t tvocVal = 0;


void setup() {
  Serial.begin(115200);

  //WIFI SETUP~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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

  sendRequest();
}

void loop() {

}


void sendRequest() {
  
  // make a String for the HTTP request path:
  String postData = "{\"macAddress\":\"3C:71:BF:87:C2:6C\",\"sessionKey\":\"3ACD94E4-D87F-4702-8411-A42D76986891\",\"data\": {\"CO2\":";
  postData = postData + co2Val;
  postData = postData + ",\"tVOC\":";
  postData = postData + tvocVal +  "}}";


  String contentType = "application/json";

  // make the POST request to the hub:
  https.post("/data", contentType, postData);

  // read the status code and body of the response
  int statusCode = https.responseStatusCode();
  String response = https.responseBody();

  Serial.println(postData);
  Serial.print("Status code from server: ");
  Serial.println(statusCode);
  Serial.print("Server response: ");
  Serial.println(response);
  Serial.println();
}
