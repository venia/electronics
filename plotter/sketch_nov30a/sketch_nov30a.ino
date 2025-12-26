void setup() {
  Serial.begin(115200);
}

float x;

void loop() {
  x += 0.2;
  Serial.println(sin(x));
  delay(40);
}
