
#include <Wire.h>
#include "rgb_lcd.h"
#include <math.h>

rgb_lcd lcd;

const int LIGHT_SENSOR = A0;
const int SOUND_SENSOR = A1;
const int TEMP_SENSOR = A2;

int lightThreshold = 300;
int soundThreshold = 250;
float temperatureThreshold = 28.0;

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setRGB(0, 255, 0);

  lcd.setCursor(0, 0);
  lcd.print("Room Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  Serial.println("Sensory Room Environment Monitor");
  Serial.println("--------------------------------");
}

void loop() {
  int lightValue = analogRead(LIGHT_SENSOR);
  int soundValue = analogRead(SOUND_SENSOR);
  int tempReading = analogRead(TEMP_SENSOR);

  float temperature = -999;

  if (tempReading > 0 && tempReading < 1023) {
    float resistance =
      10000.0 * (1023.0 / tempReading - 1.0);

    temperature = 1.0 /
      (log(resistance / 10000.0) / 3975.0
      + 1.0 / 298.15) - 273.15;
  }

  bool tempError = temperature == -999;
  bool tooDark = lightValue < lightThreshold;
  bool tooNoisy = soundValue > soundThreshold;
  bool tooHot = false;

  if (!tempError) {
    tooHot = temperature > temperatureThreshold;
  }

  bool alert = tempError || tooDark || tooNoisy || tooHot;

  // Display sensor readings
  Serial.println();
  Serial.println("ENVIRONMENT READINGS");
  Serial.println("--------------------------------");

  Serial.print("Light Level: ");
  Serial.println(lightValue);

  Serial.print("Sound Level: ");
  Serial.println(soundValue);

  Serial.print("Temperature: ");

  if (tempError) {
    Serial.println("Sensor Error");
  } else {
    Serial.print(temperature, 1);
    Serial.println(" C");
  }

  // Report every active alert
  Serial.println();
  Serial.println("ENVIRONMENT ALERTS:");

  bool alertFound = false;

  if (tempError) {
    Serial.println("- Temperature sensor error");
    alertFound = true;
  }

  if (tooDark) {
    Serial.println("- Low light detected");
    alertFound = true;
  }

  if (tooNoisy) {
    Serial.println("- High sound reading detected");
    alertFound = true;
  }

  if (tooHot) {
    Serial.println("- High temperature detected");
    alertFound = true;
  }

  if (!alertFound) {
    Serial.println("No alerts. Room looks OK.");
  }

  // Update LCD display
  lcd.clear();

  if (alert) {
    lcd.setRGB(255, 60, 0);
    lcd.setCursor(0, 0);
    lcd.print("Check environment");

    lcd.setCursor(0, 1);

    if (tempError) {
      lcd.print("Check temp sensor");
    } else if (tooDark) {
      lcd.print("Room too dark");
    } else if (tooNoisy) {
      lcd.print("Noise is high");
    } else if (tooHot) {
      lcd.print("Room too hot");
    }
  } else {
    lcd.setRGB(0, 255, 0);
    lcd.setCursor(0, 0);
    lcd.print("Room looks OK");

    lcd.setCursor(0, 1);
    lcd.print("Temp: ");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C");
  }

  Serial.println("--------------------------------");

  delay(1000);
}
