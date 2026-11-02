
#include <Wire.h>
#include "rgb_lcd.h"
#include <math.h>

// This project checks the light, noise and temperature
// in a sensory room and lets us know if anything is too high.


// --------------------------------------------------
// Sensor connections
// --------------------------------------------------

// Each sensor is connected to a different Arduino pin.

const int lightSensorPin = A0;       // Light sensor
const int soundSensorPin = A1;       // Sound sensor
const int temperatureSensorPin = A2; // Temperature sensor


// --------------------------------------------------
// Temperature settings
// --------------------------------------------------

// These numbers help us work out the temperature
// from the readings we get from the temperature sensor.

const float nominalResistance = 10000.0;
const float nominalTemperature = 25.0;
const float betaValue = 3975.0;


// --------------------------------------------------
// Alert limits
// --------------------------------------------------

// These are the limits we have chosen for each sensor.
// If a reading goes above these numbers, an alert appears.

int lightThreshold = 300;
int soundThreshold = 250;
float temperatureThreshold = 28.0;


// --------------------------------------------------
// Reading settings
// --------------------------------------------------

// We take 5 readings from each sensor and work out
// the average to make the results more reliable.

const int numberOfSamples = 5;
const int sampleDelay = 10;

// The system waits 1 second before checking again.

const int monitoringDelay = 1000;


// --------------------------------------------------
// LCD screen
// --------------------------------------------------

// This allows us to control the Grove LCD screen.

rgb_lcd lcd;

// These remember what was last shown on the LCD.
// This stops the screen from updating when it doesn't need to.

String previousLine1 = "";
String previousLine2 = "";


// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {

  // Starts communication with the computer so we
  // can see the sensor readings in the Serial Monitor.

  Serial.begin(9600);

  // Turns on the LCD screen and sets it to 16 columns
  // and 2 rows.

  lcd.begin(16, 2);

  // Makes the LCD green when the system starts.

  lcd.setRGB(0, 255, 0);

  // Shows the project name when the system starts.

  lcd.print("Sensory Room");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // Prints the project name in the Serial Monitor.

  Serial.println("Sensory Room Environment Reader");
  Serial.println("--------------------------------");

  // Shows the alert limits we have chosen.

  printThresholds();

  // Waits 2 seconds before starting the readings.

  delay(2000);

  // Clears the LCD ready for the sensor information.

  lcd.clear();
}


// --------------------------------------------------
// Main monitoring loop
// --------------------------------------------------

void loop() {

  // Gets the average light reading from the sensor.

  int lightReading = readAverage(lightSensorPin);

  // Gets the average sound reading from the sensor.

  int soundReading = readAverage(soundSensorPin);

  // Creates a variable to store the temperature.

  float temperature = 0.0;

  // Reads the temperature and checks if the result is valid.

  bool temperatureValid = readTemperature(temperature);

  // Checks if the light reading is above our chosen limit.

  bool lightAlert = lightReading > lightThreshold;

  // Checks if the sound reading is above our chosen limit.

  bool soundAlert = soundReading > soundThreshold;

  // Checks if the temperature is above 28 degrees.
  // It only checks this if the temperature reading is valid.

  bool temperatureAlert =
    temperatureValid && temperature > temperatureThreshold;

  // Sends all the sensor readings to the Serial Monitor.

  displaySensorReadings(
    lightReading,
    soundReading,
    temperature,
    temperatureValid
  );

  // Checks if anything is above its limit and
  // displays a message if there is a problem.

  displayEnvironmentalStatus(
    lightAlert,
    soundAlert,
    temperatureAlert,
    temperatureValid
  );

  // Updates the LCD to show whether the room is
  // normal or needs attention.

  updateLCD(
    lightAlert,
    soundAlert,
    temperatureAlert,
    temperatureValid
  );

  // Waits 1 second before starting the loop again.

  delay(monitoringDelay);
}


// --------------------------------------------------
// Read and average sensor values
// --------------------------------------------------

int readAverage(int sensorPin) {

  // This function takes 5 readings from a sensor
  // and works out the average.

  long total = 0;

  // Repeats the reading 5 times.

  for (int i = 0; i < numberOfSamples; i++) {

    // Reads the sensor and adds the value to the total.

    total += analogRead(sensorPin);

    // Waits a little before taking the next reading.

    delay(sampleDelay);
  }

  // Divides the total by 5 to get the average.

  return total / numberOfSamples;
}


// --------------------------------------------------
// Read and calculate temperature
// --------------------------------------------------

bool readTemperature(float &temperature) {

  // This function takes the temperature sensor reading
  // and works out the temperature in degrees Celsius.

  // Gets the average reading from the temperature sensor.

  int rawReading = readAverage(temperatureSensorPin);

  // Checks that the reading is within the correct range.
  // If it isn't, the temperature reading is not accepted.

  if (rawReading <= 0 || rawReading >= 1023) {
    return false;
  }

  // Uses the sensor reading to work out its resistance.

  float resistance =
    (1023.0 - rawReading) * nominalResistance / rawReading;

  // Checks that the resistance value is valid.

  if (resistance <= 0 || isnan(resistance) || isinf(resistance)) {
    return false;
  }

  // Starts the calculation that changes the resistance
  // into a temperature value.

  float steinhart =
    log(resistance / nominalResistance) / betaValue;

  // Adds the reference temperature to the calculation.

  steinhart += 1.0 / (nominalTemperature + 273.15);

  // Changes the result into a temperature in Kelvin.

  steinhart = 1.0 / steinhart;

  // Converts the temperature from Kelvin into Celsius.

  temperature = steinhart - 273.15;

  // Checks that the final temperature is a valid number.

  if (isnan(temperature) || isinf(temperature)) {
    return false;
  }

  // Returns true if the temperature was calculated correctly.

  return true;
}


// --------------------------------------------------
// Display sensor readings in Serial Monitor
// --------------------------------------------------

void displaySensorReadings(
  int lightReading,
  int soundReading,
  float temperature,
  bool temperatureValid
) {

  // This function shows the readings from all three sensors
  // on the computer using the Serial Monitor.

  Serial.println();
  Serial.println("Environmental Sensor Readings");

  // Shows the light reading.

  Serial.print("Light level: ");
  Serial.println(lightReading);

  // Shows the sound reading.

  Serial.print("Sound level: ");
  Serial.println(soundReading);

  // If the temperature reading is correct, show it.

  if (temperatureValid) {

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");

  } else {

    // If there is a problem, show an error message.

    Serial.println("Temperature: Sensor reading error");
  }
}


// --------------------------------------------------
// Display room status and alerts
// --------------------------------------------------

void displayEnvironmentalStatus(
  bool lightAlert,
  bool soundAlert,
  bool temperatureAlert,
  bool temperatureValid
) {

  // This function checks if any of the sensor readings
  // are too high and tells us what needs attention.

  // Checks if at least one alert is active.
  // An invalid temperature reading also counts as an alert.

  bool anyAlert =
    lightAlert || soundAlert || temperatureAlert || !temperatureValid;

  Serial.println();
  Serial.println("Room Environment Status:");

  // If nothing is wrong, it tells us the room is normal.

  if (!anyAlert) {
    Serial.println("Normal - No alerts detected.");
    return;
  }

  // If something is wrong, it tells us to check the room.

  Serial.println("Attention - Check the following:");

  // Shows a message if the light is too high.

  if (lightAlert) {
    Serial.println("- Light level is high.");
  }

  // Shows a message if the noise is too high.

  if (soundAlert) {
    Serial.println("- Noise level is high.");
  }

  // Shows a message if the temperature is too high.

  if (temperatureAlert) {
    Serial.println("- Temperature is high.");
  }

  // Shows an error if the temperature sensor isn't working properly.

  if (!temperatureValid) {
    Serial.println("- Temperature sensor reading is invalid.");
  }
}


// --------------------------------------------------
// Update the LCD screen
// --------------------------------------------------

void updateLCD(
  bool lightAlert,
  bool soundAlert,
  bool temperatureAlert,
  bool temperatureValid
) {

  // This function controls what appears on the LCD screen.
  // It shows if the room is normal or needs attention.

  // These variables store the messages for the two LCD lines.

  String line1;
  String line2;

  // Checks if any of the sensors have triggered an alert.

  bool anyAlert =
    lightAlert || soundAlert || temperatureAlert || !temperatureValid;

  // If there are no alerts, show that the room is normal.

  if (!anyAlert) {

    line1 = "Room: Normal";
    line2 = "No alerts";

    // Green means everything is within our chosen limits.

    lcd.setRGB(0, 255, 0);

  } else {

    // If there is an alert, show that the room needs attention.

    line1 = "Room: Attention";

    // Orange warns the user that something needs checking.

    lcd.setRGB(255, 120, 0);

    // The LCD only shows one alert at a time.
    // It checks the alerts in order of priority.

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

  // Checks if the new message is different from
  // what was already showing on the LCD.

  if (line1 != previousLine1 || line2 != previousLine2) {

    // Clears the screen before showing the new message.

    lcd.clear();

    // Displays the first line.

    lcd.setCursor(0, 0);
    lcd.print(line1);

    // Displays the second line.

    lcd.setCursor(0, 1);
    lcd.print(line2);

    // Saves the messages so they can be checked next time.

    previousLine1 = line1;
    previousLine2 = line2;
  }
}


// --------------------------------------------------
// Print the alert limits
// --------------------------------------------------

void printThresholds() {

  // This function shows the limits we have set for
  // the light, sound and temperature sensors.

  Serial.println();
  Serial.println("Configured Alert Thresholds:");

  // Shows the light limit.

  Serial.print("Light threshold: ");
  Serial.println(lightThreshold);

  // Shows the sound limit.

  Serial.print("Sound threshold: ");
  Serial.println(soundThreshold);

  // Shows the temperature limit.

  Serial.print("Temperature threshold: ");
  Serial.print(temperatureThreshold);
  Serial.println(" C");

  Serial.println("--------------------------------");
}
