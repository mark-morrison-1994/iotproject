const int LIGHT_SENSOR = A0;
const int SOUND_SENSOR = A1;

void setup() {
  Serial.begin(9600);
  Serial.println("Sensory Room Sensor Monitor");
}

void loop() {
  int lightValue = analogRead(LIGHT_SENSOR);
  int soundValue = analogRead(SOUND_SENSOR);

  Serial.print("Light: ");
  Serial.print(lightValue);

  Serial.print(" | Sound: ");
  Serial.println(soundValue);

  delay(1000);
}
