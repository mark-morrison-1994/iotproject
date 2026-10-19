
const int LIGHT_SENSOR = A0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int lightValue = analogRead(LIGHT_SENSOR);

  Serial.print("Light Sensor Value: ");
  Serial.println(lightValue);

  delay(1000);
}
