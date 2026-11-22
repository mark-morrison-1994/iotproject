
#include <Wire.h>
#include "rgb_lcd.h"
#include <math.h>
#include <WiFiS3.h>

// Wi-Fi settings
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ThingSpeak settings
const char* API_KEY = "YOUR_NEW_WRITE_API_KEY";
const char* HOST = "api.thingspeak.com";

WiFiSSLClient client;
rgb_lcd lcd;

const int LIGHT_SENSOR = A0;
const int SOUND_SENSOR = A1;
const int TEMP_SENSOR = A2;

int lightThreshold = 300;
int soundThreshold = 250;
float temperatureThreshold = 28.0;

unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 20000;

// Wi-Fi connection
void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");

  lcd.clear();
  lcd.setRGB(0, 0, 255);
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi connected!");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setRGB(0, 255, 0);
  lcd.print("WiFi Connected");
  delay(1500);
}

// Send readings to ThingSpeak
void sendToThingSpeak(int lightValue,
                      int soundValue,
                      float temperature,
                      bool tempError) {

  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  Serial.println("Sending readings to ThingSpeak...");

  if (client.connect(HOST, 443)) {

    String path = "/update?api_key=" + String(API_KEY)
      + "&field1=" + String(lightValue)
      + "&field2=" + String(soundValue);

    if (!tempError) {
      path += "&field3=" + String(temperature, 1);
    }

    client.print("GET ");
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(HOST);
    client.println("Connection: close");
    client.println();

    client.setTimeout(10000);

    String response = client.readString();

    int bodyPosition = response.indexOf("\r\n\r\n");

    if (bodyPosition >= 0) {
      String body = response.substring(bodyPosition + 4);
      body.trim();

      Serial.print("ThingSpeak entry response: ");
      Serial.println(body);

      if (body.toInt() > 0) {
        Serial.println("API upload successful!");
      } else {
        Serial.println("API upload rejected. Check API key or update interval.");
      }
    } else {
      Serial.println("Could not read server response.");
      Serial.println(response);
    }

    client.stop();

  } else {
    Serial.println("ThingSpeak connection failed.");
  }
}

void setup() {
  Serial.begin(9600);
  analogReadResolution(10);

  lcd.begin(16, 2);
  lcd.setRGB(0, 255, 0);
  lcd.print("Room Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);

  Serial.println("Sensory Room Environment Monitor");
  connectToWiFi();
}

void loop() {
  // Read sensors
  int lightValue = analogRead(LIGHT_SENSOR);
  int soundValue = analogRead(SOUND_SENSOR);
  int tempReading = analogRead(TEMP_SENSOR);

  // Temperature calculation
  float temperature = -999;

  if (tempReading > 0 && tempReading < 1023) {
    float resistance =
      10000.0 * (1023.0 / tempReading - 1.0);

    temperature = 1.0 /
      (log(resistance / 10000.0) / 3975.0
      + 1.0 / 298.15) - 273.15;
  }

  bool tempError = (temperature == -999);
  bool tooDark = lightValue < lightThreshold;
  bool tooNoisy = soundValue > soundThreshold;
  bool tooHot = !tempError && temperature > temperatureThreshold;

  bool alert = tempError || tooDark || tooNoisy || tooHot;

  // Serial monitor
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
  Serial.println(alert ? "CHECK ENVIRONMENT" : "ROOM LOOKS OK");

  // LCD display
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

  // Upload every 20 seconds
  if (millis() - lastUploadTime >= uploadInterval) {
    sendToThingSpeak(
      lightValue,
      soundValue,
      temperature,
      tempError
    );

    lastUploadTime = millis();
  }

  delay(1000);
}
