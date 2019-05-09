#include <HID-Project.h>
#include <HID-Settings.h>


#define waterPin A2
#define ledPin 7

int waterLevel = 0;
bool waterSwitch = false;
long waterCounter = 0;

void setup() {
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);

  Keyboard.begin();
  Consumer.begin();

}

void loop() {



  waterLevel = analogRead(waterPin);

  //increase counter while the water level is high enough
  if (waterLevel > 300) {
    waterCounter++;
  } else if (waterLevel <= 300) {
    waterCounter = 0;
  }

  //once the water level has been high enough for long enough, then turn waterSwitch to True
  if (waterCounter >= 5000) {
    waterSwitch = true;
  }

  // if waterSwitch is true, then call the screen off function and turn waterSwitch to false
  if (waterSwitch == true) {
    screenOff();

    waterSwitch == false;

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
