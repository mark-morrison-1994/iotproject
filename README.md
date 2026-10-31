#include <Wire.h>
#include "rgb_lcd.h"
#include <math.h>

// --------------------------------------------------
// Sensory Room Environment Reader
// Monitors light, sound and temperature.
// Displays the room status on the Grove LCD.
// --------------------------------------------------

// Sensor connections
const int lightSensorPin = A0;
const int soundSensorPin = A1;
const int temperatureSensorPin = A2;

// Temperature sensor settings
// These values assume a common Grove 10K thermistor.
const float nominalResistance = 10000.0;
const float nominalTemperature = 25.0;
const float betaValue = 3975.0;

// Environmental alert thresholds
// These are example values and should be calibrated.
int lightThreshold = 300;
int soundThreshold = 250;
float temperatureThreshold = 28.0;

// Reading settings
const int numberOfSamples = 5;
const int sampleDelay = 10;
const int monitoringDelay = 1000;

// LCD
rgb_lcd lcd;

// Store the previous LCD message to avoid
// unnecessary display updates.
String previousLine1 = "";
String previousLine2 = "";

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setRGB(0, 255, 0);

  lcd.print("Sensory Room");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  Serial.println("Sensory Room Environment Reader");
  Serial.println("--------------------------------");

  printThresholds();

  delay(2000);
  lcd.clear();
}

// --------------------------------------------------
// Main monitoring loop
// --------------------------------------------------

void loop() {
  int lightReading = readAverage(lightSensorPin);
  int soundReading = readAverage(soundSensorPin);

  float temperature = 0.0;
  bool temperatureValid = readTemperature(temperature);

  bool lightAlert = lightReading > lightThreshold;
  bool soundAlert = soundReading > soundThreshold;
  bool temperatureAlert =
    temperatureValid && temperature > temperatureThreshold;

  displaySensorReadings(
    lightReading,
    soundReading,
    temperature,
    temperatureValid
  );

  displayEnvironmentalStatus(
    lightAlert,
    soundAlert,
    temperatureAlert,
    temperatureValid
  );

  updateLCD(
    lightAlert,
    soundAlert,
    temperatureAlert,
    temperatureValid
  );

  delay(monitoringDelay);
}

// --------------------------------------------------
// Read and average sensor values
// --------------------------------------------------

int readAverage(int sensorPin) {
  long total = 0;

  for (int i = 0; i < numberOfSamples; i++) {
    total += analogRead(sensorPin);
    delay(sampleDelay);
  }

  return total / numberOfSamples;
}

// --------------------------------------------------
// Read and calculate temperature
// --------------------------------------------------

bool readTemperature(float &temperature) {
  int rawReading = readAverage(temperatureSensorPin);

  // Reject readings at the limits of the analogue range.
  if (rawReading <= 0 || rawReading >= 1023) {
    return false;
  }

  float resistance =
    (1023.0 - rawReading) * nominalResistance / rawReading;

  if (resistance <= 0 || isnan(resistance) || isinf(resistance)) {
    return false;
  }

  float steinhart =
    log(resistance / nominalResistance) / betaValue;

  steinhart += 1.0 / (nominalTemperature + 273.15);
  steinhart = 1.0 / steinhart;

  temperature = steinhart - 273.15;

  if (isnan(temperature) || isinf(temperature)) {
    return false;
  }

  return true;
}

// --------------------------------------------------
// Print sensor readings to the Serial Monitor
// --------------------------------------------------

void displaySensorReadings(
  int lightReading,
  int soundReading,
  float temperature,
  bool temperatureValid
) {
  Serial.println();
  Serial.println("Environmental Sensor Readings");

  Serial.print("Light level: ");
  Serial.println(lightReading);

  Serial.print("Sound level: ");
  Serial.println(soundReading);

  if (temperatureValid) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");
  } else {
    Serial.println("Temperature: Sensor reading error");
  }
}

// --------------------------------------------------
// Print environmental status and active alerts
// --------------------------------------------------

void displayEnvironmentalStatus(
  bool lightAlert,
  bool soundAlert,
  bool temperatureAlert,
  bool temperatureValid
) {
  bool anyAlert =
    lightAlert || soundAlert || temperatureAlert || !temperatureValid;

  Serial.println();
  Serial.println("Room Environment Status:");

  if (!anyAlert) {
    Serial.println("Normal - No alerts detected.");
    return;
  }

  Serial.println("Attention - Check the following:");

  if (lightAlert) {
    Serial.println("- Light level is high.");
  }

  if (soundAlert) {
    Serial.println("- Noise level is high.");
  }

  if (temperatureAlert) {
    Serial.println("- Temperature is high.");
  }

  if (!temperatureValid) {
    Serial.println("- Temperature sensor reading is invalid.");
  }
}

// --------------------------------------------------
// Update LCD only when the displayed message changes
// --------------------------------------------------

void updateLCD(
  bool lightAlert,
  bool soundAlert,
  bool temperatureAlert,
  bool temperatureValid
) {
  String line1;
  String line2;

  bool anyAlert =
    lightAlert || soundAlert || temperatureAlert || !temperatureValid;

  if (!anyAlert) {
    line1 = "Room: Normal";
    line2 = "No alerts";
    lcd.setRGB(0, 255, 0);
  } else {
    line1 = "Room: Attention";
    lcd.setRGB(255, 120, 0);

    // Display the highest-priority alert.
    if (!temperatureValid) {
      line2 = "Temp sensor error";
    } else if (soundAlert) {
      line2 = "Noise is high";
    } else if (lightAlert) {
      line2 = "Light is high";
    } else if (temperatureAlert) {
      line2 = "Temp is high";
    } else {
      line2 = "Check environment";
    }
  }

  // Only refresh the LCD when the message changes.
  if (line1 != previousLine1 || line2 != previousLine2) {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);

    previousLine1 = line1;
    previousLine2 = line2;
  }
}

// --------------------------------------------------
// Print configured thresholds
// --------------------------------------------------

void printThresholds() {
  Serial.println();
  Serial.println("Configured Alert Thresholds:");

  Serial.print("Light threshold: ");
  Serial.println(lightThreshold);

  Serial.print("Sound threshold: ");
  Serial.println(soundThreshold);

  Serial.print("Temperature threshold: ");
  Serial.print(temperatureThreshold);
  Serial.println(" C");

  Serial.println("--------------------------------");
}
