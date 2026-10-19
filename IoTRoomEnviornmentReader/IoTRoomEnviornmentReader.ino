#include <math.h>
const int LIGHT_SENSOR = A0;
const int SOUND_SENSOR = A1;
const int TEMP_SENSOR = A2;
void setup() {
Serial.begin(9600);
Serial.println("Sensory Room Environment Monitor");
}
void loop() {
int lightValue = analogRead(LIGHT_SENSOR);
int soundValue = analogRead(SOUND_SENSOR);
int tempReading = analogRead(TEMP_SENSOR);
// Convert Grove thermistor reading to Celsius
float temperature = -999;
if (tempReading > 0 && tempReading < 1023) {
float resistance =
10000.0 * (1023.0 / tempReading - 1.0);
temperature = 1.0 /
(log(resistance / 10000.0) / 3975.0
1.0 / 298.15) - 273.15;
}
Serial.print("Light: ");
Serial.print(lightValue);
Serial.print(" | Sound: ");
Serial.print(soundValue);
Serial.print(" | Temperature: ");
if (temperature == -999) {
Serial.println("Sensor error");
} else {
Serial.print(temperature, 1);
Serial.println(" C");
}
delay(1000);
}
