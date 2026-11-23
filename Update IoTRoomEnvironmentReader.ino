
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
const char thingSpeakHost[] = "api.thingspeak.com";
const int thingSpeakPort = 80;

const char writeAPIKey[] =
  SECRET_THINGSPEAK_WRITE_API_KEY;

WiFiClient thingSpeakWifi;

HttpClient thingSpeakClient(
  thingSpeakWifi,
  thingSpeakHost,
  thingSpeakPort
);

// Google Sheets settings
const char googleHost[] = "script.google.com";
const int googlePort = 443;

const char googlePath[] =
  "/macros/s/AKfycbwxGlZNQGISPRpwux2sY0BIKWO4m18ndTj-x6mMo00Zfh-4vw2rrGuiVuhMpZ6-eDxa/exec";

WiFiSSLClient googleWifi;

HttpClient googleClient(
  googleWifi,
  googleHost,
  googlePort
);

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

  Serial.print("Connecting to Wi-Fi...");

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

// Upload sensor data to ThingSpeak
void postToThingSpeak(
  int lightValue,
  int soundValue,
  float temperature,
  bool tempError
) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("ThingSpeak upload skipped: Wi-Fi disconnected.");
    return;
  }

  String body = "api_key=" + String(writeAPIKey);

  body += "&field1=" + String(lightValue);
  body += "&field2=" + String(soundValue);

  if (!tempError) {
    body += "&field3=" + String(temperature, 1);
  }

  Serial.println();
  Serial.println("Posting sensor data to ThingSpeak...");

  thingSpeakClient.beginRequest();
  thingSpeakClient.post("/update");

  thingSpeakClient.sendHeader(
    "Content-Type",
    "application/x-www-form-urlencoded"
  );

  thingSpeakClient.sendHeader(
    "Content-Length",
    body.length()
  );

  thingSpeakClient.beginBody();
  thingSpeakClient.print(body);
  thingSpeakClient.endRequest();

  int statusCode =
    thingSpeakClient.responseStatusCode();

  String response =
    thingSpeakClient.responseBody();

  response.trim();

  Serial.print("ThingSpeak HTTP Status: ");
  Serial.println(statusCode);

  Serial.print("ThingSpeak Response: ");
  Serial.println(response);

  if (statusCode == 200 && response.toInt() > 0) {

    Serial.println("ThingSpeak upload successful.");

    Serial.print("Entry ID: ");
    Serial.println(response);

  } else {

    Serial.println("ThingSpeak upload failed.");
    Serial.println("Check your API key and channel.");
  }

  thingSpeakClient.stop();
}

// Upload sensor data to Google Sheets
void postToGoogleSheets(
  int lightValue,
  int soundValue,
  float temperature,
  bool tempError
) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("Google Sheets upload skipped: Wi-Fi disconnected.");
    return;
  }

  String temperatureJson = tempError
    ? "null"
    : String(temperature, 1);

  // Create JSON data
  String body = "{";

  body += "\"light\":" + String(lightValue) + ",";
  body += "\"sound\":" + String(soundValue) + ",";
  body += "\"temperature\":" + temperatureJson;

  body += "}";

  Serial.println();
  Serial.println("Posting sensor data to Google Sheets...");

  googleClient.beginRequest();
  googleClient.post(googlePath);

  googleClient.sendHeader(
    "Content-Type",
    "application/json"
  );

  googleClient.sendHeader(
    "Content-Length",
    body.length()
  );

  googleClient.beginBody();
  googleClient.print(body);
  googleClient.endRequest();

  int statusCode =
    googleClient.responseStatusCode();

  String response =
    googleClient.responseBody();

  response.trim();

  Serial.print("Google Sheets HTTP Status: ");
  Serial.println(statusCode);

  Serial.print("Google response: ");
  Serial.println(response);

  if (statusCode == 200) {

    Serial.println("Google Sheets returned HTTP 200.");
    Serial.println("Check spreadsheet for the new row.");

  } else if (statusCode == 302 || statusCode == 303) {

    Serial.println("Google Apps Script redirected its response.");
    Serial.println("Check spreadsheet for the new row.");

  } else {

    Serial.println("Google Sheets request may have failed.");
    Serial.println("Check Web App URL and deployment settings.");
  }

  googleClient.stop();
}

// Setup
void setup() {

  Serial.begin(9600);
  delay(2000);

  analogReadResolution(10);

  Serial.println("Sensory Environment Monitor");
  Serial.println("ThingSpeak + Google Sheets");

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

  lastUploadTime = millis();
}

// Main loop
void loop() {

  // Reconnect Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("Wi-Fi disconnected. Reconnecting...");

    connectWiFi();
  }

  // Read sensors
  int lightValue = analogRead(LIGHT_SENSOR);

  int soundValue = analogRead(SOUND_SENSOR);

  float temperature =
    readTemperatureC(TEMP_SENSOR);

  bool tempError = isnan(temperature);

  // Check environment
  bool tooDark =
    lightValue < lightThreshold;

  bool tooNoisy =
    soundValue > soundThreshold;

  bool tooHot =
    (!tempError && temperature > temperatureThreshold);

  bool alert =
    tempError || tooDark || tooNoisy || tooHot;

  // Print sensor readings
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

    // Upload to ThingSpeak
    postToThingSpeak(
      lightValue,
      soundValue,
      temperature,
      tempError
    );

    // Upload to Google Sheets
    postToGoogleSheets(
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

  delay(1000);
}
