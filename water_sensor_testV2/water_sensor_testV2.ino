//#include <Keyboard.h>

#include <HID-Project.h>
#include <HID-Settings.h>



#define waterPin A2
#define ledPin 7

//#define SYSTEM_SLEEP  0x82
int waterLevel = 0;
bool waterSwitch = false;
long waterCounter = 0;
bool compOn = true;

void setup() {
  pinMode(ledPin, OUTPUT);

  //  Serial.begin(9600);

  //  System.begin();
  Keyboard.begin();
  Consumer.begin();

}

void loop() {


  //increase counter while the sensor is submerged enough
  if (compOn == true) {
    waterLevel = analogRead(waterPin);

    if (waterLevel > 300) {
      waterCounter++;
    } else if (waterLevel <= 300) {
      waterCounter = 0;
    }

    if (waterCounter >= 5000) {
      waterSwitch = true;
    }
  }
  if (waterSwitch == true) {
    screenOff();

    waterSwitch == false;
    compOn = false;

  }




  Serial.print("Water Level: ");
  Serial.print(waterLevel);
  Serial.print(" . | . ");
  Serial.print("Switch Activated:");
  Serial.println(waterSwitch);
}

void screenOff() {
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(KEY_LEFT_GUI);
  Consumer.press(HID_CONSUMER_EJECT);
  delay(10);
  Consumer.releaseAll();
  Keyboard.releaseAll();
}
