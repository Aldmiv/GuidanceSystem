// Входы АЦП для антенн
const int RA = 36;  // Правая антенна
const int LA = 39;  // Левая антенна
const int UA = 34;  // Верхняя антенна
const int DA = 35;  // Нижняя антенна

void setup() {
  Serial.begin(9600);
}

void loop() {
  // Считываем значения с четырёх потенциометров
  int val1 = analogRead(RA);
  int val2 = analogRead(LA);
  int val3 = analogRead(UA);
  int val4 = analogRead(DA);

  // Выводим результаты в монитор порта
  Serial.print("RA: ");
  Serial.print(val1);
  Serial.print("\tLA: ");
  Serial.print(val2);
  Serial.print("\tUA: ");
  Serial.print(val3);
  Serial.print("\tDA: ");
  Serial.println(val4);

  // Делаем небольшую задержку для удобства наблюдения
  delay(200);
}
