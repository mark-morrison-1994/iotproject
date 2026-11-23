#include <Wire.h>
#include "rgb_lcd.h"
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <math.h>
#include "arduino_secrets.h"

rgb_lcd lcd;

// Wi-Fi credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASSWORD;

// ThingSpeak settings
const char server[] = "api.thingspeak.com";
const int port = 80;

// Enter your NEW ThingSpeak Write API Key here
const String writeAPIKey = "J2TLGHNYPDOTK75T";

// HTTP client
WiFiClient wifi;
HttpClient client(wifi, server, port);

// Grove sensors
const int LIGHT_SENSOR = A0;
const int SOUND_SENSOR = A1;
const int TEMP_SENSOR = A2;

// Environment thresholds
int lightThreshold = 300;
int soundThreshold = 250;
float temperatureThreshold = 28.0;

// Upload interval
unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 20000;

// Connect to Wi-Fi
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");

  WiFi.begin(ssid, pass);

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 30000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed.");
  }
}

// Read Grove temperature sensor
float readTemperatureC(int pin) {
  int sensorValue = analogRead(pin);

  if (sensorValue <= 0 || sensorValue >= 1023) {
    return NAN;
  }

  float resistance =
    (1023.0 - sensorValue) * 10000.0 / sensorValue;

  const float B = 3975.0;
  const float R0 = 10000.0;
  const float T0 = 298.15;

  float temperatureK =
    1.0 / ((1.0 / T0) +
    (1.0 / B) * log(resistance / R0));

  return temperatureK - 273.15;
}

// Upload sensor data to ThingSpeak using POST
void postToThingSpeak(
  int lightValue,
  int soundValue,
  float temperature,
  bool tempError
) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Upload skipped.");
    return;
  }

  // Create POST request body
  String body = "api_key=" + writeAPIKey;

  body += "&field1=" + String(lightValue);
  body += "&field2=" + String(soundValue);

  if (!tempError) {
    body += "&field3=" + String(temperature, 1);
  }

  Serial.println();
  Serial.println("Posting sensor data to ThingSpeak...");

  // Send POST request
  client.beginRequest();
  client.post("/update");

  client.sendHeader(
    "Content-Type",
    "application/x-www-form-urlencoded"
  );

  client.sendHeader(
    "Content-Length",
    body.length()
  );

  client.beginBody();
  client.print(body);
  client.endRequest();

  // Read server response
  int statusCode = client.responseStatusCode();

  String response = client.responseBody();
  response.trim();

  Serial.print("HTTP Status Code: ");
  Serial.println(statusCode);

  Serial.print("ThingSpeak Response: ");
  Serial.println(response);

  if (statusCode == 200 && response.toInt() > 0) {
    Serial.println("UPLOAD SUCCESSFUL!");
    Serial.print("Entry ID: ");
    Serial.println(response);
  } else {
    Serial.println("UPLOAD FAILED.");
    Serial.println("Check your API key and channel settings.");
  }

  Serial.println("-----------------------------");

  client.stop();
}

// Setup
void setup() {
  Serial.begin(9600);
  delay(2000);

  analogReadResolution(10);

  Serial.println("Sensory Environment Monitor");
  Serial.println("ThingSpeak POST Connection");

  // Start LCD
  lcd.begin(16, 2);
  lcd.setRGB(0, 255, 0);

  lcd.setCursor(0, 0);
  lcd.print("Room Monitor");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(2000);
  lcd.clear();

  // Connect to Wi-Fi
  connectWiFi();

  lcd.setCursor(0, 0);

  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("Wi-Fi connected");
    lcd.setCursor(0, 1);
    lcd.print("Cloud ready");
  } else {
    lcd.print("Wi-Fi failed");
  }

  delay(2000);
  lcd.clear();
}

// Main loop
void loop() {

  // Read sensors
  int lightValue = analogRead(LIGHT_SENSOR);
  int soundValue = analogRead(SOUND_SENSOR);

  float temperature = readTemperatureC(TEMP_SENSOR);

  bool tempError = isnan(temperature);

  // Check environment
  bool tooDark = lightValue < lightThreshold;
  bool tooNoisy = soundValue > soundThreshold;

  bool tooHot = false;

  if (!tempError) {
    tooHot = temperature > temperatureThreshold;
  }

  bool alert =
    tempError || tooDark || tooNoisy || tooHot;

  // Print readings to Serial Monitor
  Serial.print("Light: ");
  Serial.print(lightValue);

  Serial.print(" | Sound: ");
  Serial.print(soundValue);

  Serial.print(" | Temperature: ");

  if (tempError) {
    Serial.print("Sensor error");
  } else {
    Serial.print(temperature, 1);
    Serial.print(" C");
  }

  Serial.print(" | Status: ");

  Serial.println(
    alert ? "CHECK ENVIRONMENT" : "ROOM LOOKS OK"
  );

  // Upload every 20 seconds
  if (millis() - lastUploadTime >= uploadInterval) {

    lastUploadTime = millis();

    postToThingSpeak(
      lightValue,
      soundValue,
      temperature,
      tempError
    );
  }

  // LCD alert system
  lcd.clear();

  if (alert) {

    lcd.setRGB(255, 60, 0);

    lcd.setCursor(0, 0);
    lcd.print("Check environment");

    lcd.setCursor(0, 1);

    if (tempError) {
      lcd.print("Check temp sensor");
    }
    else if (tooDark) {
      lcd.print("Room too dark");
    }
    else if (tooNoisy) {
      lcd.print("Noise is high");
    }
    else if (tooHot) {
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

  delay(1000);
}
