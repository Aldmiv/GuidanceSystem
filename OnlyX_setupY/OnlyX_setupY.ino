//check

#include <AccelStepper.h>

// ===== Антенны =====
#define RA A2  // Правая антенна
#define LA A1  // Левая антенна
#define button 12  // кнопка

int Rvalue = 0;  
int Lvalue = 0;  
int DeadBand = 20;

// Переменная логики для оси X:
//   0 - стоп, 1 - LEFT (минус), 2 - RIGHT (плюс)
int moveLogicX = 0;

// Переменная логики для оси Y:
//   0 - стоп, 1 - вперёд (плюс), 2 - назад (минус)
int moveLogicY = 0;

// ===== Потенциометр для отладки =====
int potentY = 0;  // Читаем на A3



unsigned long previousMillis = 0;  
const unsigned long btwnMeasure = 50; // Периодичность считывания (мс)

// ===== Параметры шагового мотора (ось X) =====
const int X_PulPlus      = 8;    // Пин для импульсов (шагов) оси X
const int X_DirPlus      = 9;    // Пин для направления оси X
const float maxSpeedX     = 2500; // Макс. скорость (шаг/с) для X
const float accelerationX = 2000; // Ускорение (шаг/с^2) для X

// Создаём объект класса AccelStepper для оси X
AccelStepper stepperX(AccelStepper::DRIVER, X_PulPlus, X_DirPlus);

// ===== Параметры шагового мотора (ось Y) =====
const int Y_PulPlus      = 6;    // Пин для импульсов (шагов) оси Y
const int Y_DirPlus      = 7;    // Пин для направления оси Y
const float maxSpeedY     = 2500; // Макс. скорость (шаг/с) для Y
const float accelerationY = 2000;  // Ускорение (шаг/с^2) для Y

// Создаём объект класса AccelStepper для оси Y
AccelStepper stepperY(AccelStepper::DRIVER, Y_PulPlus, Y_DirPlus);

// ----------------------------------------------------------------------------
// Логика определения moveLogicX на основе антенн RA и LA
// ----------------------------------------------------------------------------
void CalculateDirectionX() {
  Rvalue = analogRead(RA);
  Lvalue = analogRead(LA);

  int diff = abs(Rvalue - Lvalue);
  Serial.print("diff X = ");
  Serial.print(diff);

  if (diff <= DeadBand) {
    Serial.println(" => STOP");
    moveLogicX = 0;
  }
  else if ((Rvalue > Lvalue) && (diff > DeadBand)) {
    Serial.println(" => RIGHT");
    moveLogicX = 2;
  }
  else if ((Rvalue < Lvalue) && (diff > DeadBand)) {
    Serial.println(" => LEFT");
    moveLogicX = 1;
  }
}

// ----------------------------------------------------------------------------
// Временная логика определения moveLogicY на основе потенциометра A3
// ----------------------------------------------------------------------------
void CalculateDirectionY() {
  potentY = analogRead(A3);
  Serial.print("potentY = ");
  Serial.print(potentY);

  if (potentY < 100) {
    Serial.println(" => Y UP (plus)");
    moveLogicY = 1; 
  }
  else if (potentY > 1000) {
    Serial.println(" => Y DOWN (minus)");
    moveLogicY = 2;
  }
  else {
    Serial.println(" => Y STOP");
    moveLogicY = 0; // Стоп
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(button, INPUT);

  // === Ось X ===
  stepperX.setMaxSpeed(maxSpeedX);
  stepperX.setAcceleration(accelerationX);
  stepperX.setCurrentPosition(0);
  stepperX.moveTo(stepperX.currentPosition());

  // === Ось Y ===
  stepperY.setMaxSpeed(maxSpeedY);
  // Используем глобальную переменную accelerationY
  stepperY.setAcceleration(accelerationY);
  stepperY.setCurrentPosition(0);

  // ---------- Начальный прогон мотора Y ----------
  // Двигаем "вперёд" (-999999), пока кнопка не нажата (HIGH)
  stepperY.setAcceleration(300); 
  stepperY.moveTo(-999999);
  while (digitalRead(button) == LOW) {
    stepperY.run();
  }

  // Останавливаемся
  stepperY.setAcceleration(999999.0f);  
  stepperY.moveTo(stepperY.currentPosition());
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }

  //Возвращаем ускорение
  stepperY.setAcceleration(300); 

  // Двигаем в обратную сторону
  unsigned long startBack = millis();
  stepperY.moveTo(stepperY.currentPosition() + 100000);
  while (millis() - startBack < 1500) {
    stepperY.run();
  }

  // Останавливаемся
  stepperY.setAcceleration(999999.0f);  
  stepperY.moveTo(stepperY.currentPosition());
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }

  // Возвращаем глобальное ускорение
  stepperY.setAcceleration(accelerationY);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Каждые btwnMeasure выполняем сканирование
  if (currentMillis - previousMillis >= btwnMeasure) {
    previousMillis = currentMillis;
    CalculateDirectionX();  // Определяем moveLogicX
    CalculateDirectionY();  // Определяем moveLogicY
  }

  // ===== Управление осью X =====
  switch (moveLogicX) {
    case 0:
      stepperX.moveTo(stepperX.currentPosition());
      break;
    case 1:
      stepperX.moveTo(1000000);
      break;
    case 2:
      stepperX.moveTo(-1000000);
      break;
  }
  stepperX.run();

  // ===== Управление осью Y =====
  switch (moveLogicY) {
    case 0:
      stepperY.moveTo(stepperY.currentPosition());
      break;
    case 1:
      stepperY.moveTo(1000000);
      break;
    case 2:
      stepperY.moveTo(-1000000);
      break;
  }
  stepperY.run();
}
