// Входы АЦП
const int potPin1 = 34;
const int potPin2 = 35;
const int potPin3 = 36;
const int potPin4 = 39;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Считываем значения с четырёх потенциометров
  int val1 = analogRead(potPin1);
  int val2 = analogRead(potPin2);
  int val3 = analogRead(potPin3);
  int val4 = analogRead(potPin4);

  // Выводим результаты в монитор порта
  Serial.print("P1: ");
  Serial.print(val1);
  Serial.print("\tP2: ");
  Serial.print(val2);
  Serial.print("\tP3: ");
  Serial.print(val3);
  Serial.print("\tP4: ");
  Serial.println(val4);

  // Делаем небольшую задержку для удобства наблюдения
  delay(200);
}
