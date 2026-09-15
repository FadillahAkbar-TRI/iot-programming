const int pinAO = A0;
const int pinDO = D1;

void setup() {
  Serial.begin(115200);
  pinMode(pinDO, INPUT);
}

void loop() {
  int nilaiAnalog = analogRead(pinAO);
  int statusDigital = digitalRead(pinDO);

  Serial.print("Analog: ");
  Serial.print(nilaiAnalog);
  Serial.print(" | Digital: ");
  Serial.println(statusDigital);

  delay(1000);
}