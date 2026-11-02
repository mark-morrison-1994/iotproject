
#include <Wire.h>       // Allows the Arduino to communicate with the LCD
#include "rgb_lcd.h"    // Library used to control the RGB LCD screen
#include <math.h>       // Allows us to use maths functions for temperature

rgb_lcd lcd;             // Creates the LCD object

// These are the pins where the three sensors are connected
const int lightSensorPin = A0;
const int soundSensorPin = A1;
const int tempSensorPin = A2;

// These are the alert limits for the room
// If a reading goes above its limit, an alert is triggered
const int lightThreshold = 300;
const int soundThreshold = 250;
const float tempThreshold = 28.0;

// These values are used to calculate the temperature
// They are based on the Grove 10K thermistor with Beta 3975
const float nominalResistance = 10000.0;
const float nominalTemperature = 25.0;
const float beta = 3975.0;
const float seriesResistance = 10000.0;

// These values control the LCD backlight colours
// Green means the room is within the set limits
const int GREEN_R = 0;
const int GREEN_G = 255;
const int GREEN_B = 0;

// Amber means one sensor has detected an alert
const int AMBER_R = 255;
const int AMBER_G = 120;
const int AMBER_B = 0;

// Red means there is a high alert
const int RED_R = 255;
const int RED_G = 0;
const int RED_B = 0;

// Variables used to store the sensor readings
int lightValue;
int soundValue;
float temperature;

void setup() {
  // Starts communication with the Serial Monitor
  Serial.begin(9600);

  // Starts the LCD screen and sets its size
  lcd.begin(16, 2);

  // Sets the starting LCD colour to green
  lcd.setRGB(GREEN_R, GREEN_G, GREEN_B);

  // Displays a welcome message when the system starts
  lcd.setCursor(0, 0);
  lcd.print("Room Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(2000);
}

void loop() {
  // Reads the light, sound and temperature sensors
  // Each sensor is read five times to get an average reading
  lightValue = readAverage(lightSensorPin);
  soundValue = readAverage(soundSensorPin);
  temperature = readTemperature();

  // Sends all sensor readings to the Serial Monitor
  // This allows us to see the values while testing the system
  Serial.print("Light: ");
  Serial.print(lightValue);

  Serial.print(" | Sound: ");
  Serial.print(soundValue);

  Serial.print(" | Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  // Checks whether any of the sensor readings are above their limits
  bool lightAlert = lightValue > lightThreshold;
  bool soundAlert = soundValue > soundThreshold;
  bool tempAlert = temperature > tempThreshold;

  // Counts how many alerts have been detected
  int alertCount = lightAlert + soundAlert + tempAlert;

  // RED ALERT:
  // Red is displayed if the temperature reaches 35 degrees
  // or if two or more sensors trigger an alert
  if (isnan(temperature)) {
    // Shows red if the temperature sensor gives an invalid reading
    lcd.setRGB(RED_R, RED_G, RED_B);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp Sensor");
    lcd.setCursor(0, 1);
    lcd.print("Check wiring");
  }
  else if (temperature >= 35.0 || alertCount >= 2) {
    lcd.setRGB(RED_R, RED_G, RED_B);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("HIGH ALERT!");
    lcd.setCursor(0, 1);
    lcd.print("Check room");
  }

  // AMBER ALERT:
  // Amber is displayed when only one sensor detects a problem
  else if (alertCount == 1) {
    lcd.setRGB(AMBER_R, AMBER_G, AMBER_B);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Room Alert!");

    lcd.setCursor(0, 1);

    // Displays which sensor has detected the alert
    if (lightAlert) {
      lcd.print("High Light");
    }
    else if (soundAlert) {
      lcd.print("High Sound");
    }
    else if (tempAlert) {
      lcd.print("High Temp");
    }
  }

  // GREEN STATUS:
  // Green is displayed when all sensor readings are within their limits
  else {
    lcd.setRGB(GREEN_R, GREEN_G, GREEN_B);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Room: Normal");
    lcd.setCursor(0, 1);
    lcd.print("No alerts");
  }

  // Waits one second before taking the next set of readings
  delay(1000);
}

// This function reads a sensor five times and calculates the average
// This helps reduce small changes in the sensor readings
int readAverage(int pin) {
  long total = 0;

  for (int i = 0; i < 5; i++) {
    total += analogRead(pin);
    delay(10);
  }

  return total / 5;
}

// This function calculates the room temperature
// It uses the thermistor resistance and the Beta formula
float readTemperature() {

  // Reads the temperature sensor five times and gets the average
  int rawReading = readAverage(tempSensorPin);

  // Checks that the sensor reading is valid
  if (rawReading <= 0 || rawReading >= 1023) {
    return NAN;
  }

  // Calculates the thermistor resistance
  float resistance =
    (1023.0 - rawReading) * seriesResistance / rawReading;

  // Uses the Beta formula to calculate the temperature
  float steinhart = resistance / nominalResistance;
  steinhart = log(steinhart);
  steinhart /= beta;
  steinhart += 1.0 / (nominalTemperature + 273.15);
  steinhart = 1.0 / steinhart;

  // Changes the temperature from Kelvin to Celsius
  steinhart -= 273.15;

  return steinhart;
}
