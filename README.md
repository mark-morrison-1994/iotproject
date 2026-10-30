
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

String previousLine1 = "";
String previousLine2 = "";
bool previousAlert = false;
bool firstDisplay = true;

int readAverage(int pin) {
  long total = 0;

  for (int i = 0; i < 5; i++) {
    total += analogRead(pin);
    delay(10);
  }

  return total / 5;
}

void updateLCD(String line1, String line2, bool alert) {
  if (firstDisplay || alert != previousAlert) {
    if (alert) {
      lcd.setRGB(255, 60, 0);
    } else {
      lcd.setRGB(0, 255, 0);
    }
  }

  if (firstDisplay || line1 != previousLine1 ||
      line2 != previousLine2) {

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);

    previousLine1 = line1;
    previousLine2 = line2;
  }

  previousAlert = alert;
  firstDisplay = false;
}

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
  int lightValue = readAverage(LIGHT_SENSOR);
  int soundValue = readAverage(SOUND_SENSOR);
  int tempReading = readAverage(TEMP_SENSOR);

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

  String line1 = "";
  String line2 = "";

  if (alert) {
    line1 = "Check environment";

    if (tempError) {
      line2 = "Check temp sensor";
    } else if (tooDark) {
      line2 = "Room too dark";
    } else if (tooNoisy) {
      line2 = "Noise is high";
    } else if (tooHot) {
      line2 = "Room too hot";
    }
  } else {
    line1 = "Room looks OK";
    line2 = "Temp: " + String(temperature, 1) + " C";
  }

  updateLCD(line1, line2, alert);

  Serial.println("--------------------------------");

  delay(1000);
}
