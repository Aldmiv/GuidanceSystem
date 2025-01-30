#include <Arduino.h>
#include <AccelStepper.h>

// Входы АЦП для антенн
const int RA = 36;  // Правая антенна
const int LA = 39;  // Левая антенна
const int UA = 34;  // Верхняя антенна
const int DA = 35;  // Нижняя антенна

// Пины управления моторами
const int X_PulPlus = 21;
const int X_DirPlus = 22;
const int Y_PulPlus = 23;
const int Y_DirPlus = 19;

const int button = 15;  // Кнопка

// Значения сигналов с антенн
volatile int Rvalue = 0, Lvalue = 0, Uvalue = 0, Dvalue = 0;  // Непрерывные данные

// Разница между парами антенн
int DX = 0;
int DY = 0;

int DeadBandX = 150;
int DeadBandY = 150;

int btwnMeasure = 60;              // Периодичность считывания (мс)
unsigned long previousMillis = 0;  // Для периодического обновления логики работы моторов
bool AllowMoving = true;

// Переменная логики для оси X:
//   0 - стоп, 1 - LEFT (минус), 2 - RIGHT (плюс)
int moveLogicX = 0;

// Переменная логики для оси Y:
//   0 - стоп, 1 - вперёд (плюс), 2 - назад (минус)
int moveLogicY = 0;



// ===== Параметры шагового мотора (ось X) =====
float maxSpeedX = 3000.0;
float accelerationX = 2900.0;
AccelStepper stepperX(AccelStepper::DRIVER, X_PulPlus, X_DirPlus);

// ===== Параметры шагового мотора (ось Y) =====
float maxSpeedY = 2000.0;
float accelerationY = 1800.0;
AccelStepper stepperY(AccelStepper::DRIVER, Y_PulPlus, Y_DirPlus);

void CalculateDirectionX() {
  Rvalue = analogRead(RA);
  Lvalue = analogRead(LA);

  int diffX = abs(Rvalue - Lvalue);

  if (diffX <= DeadBandX) {
    moveLogicX = 0;
  } else if ((Rvalue > Lvalue) && (diffX > DeadBandX)) {
    moveLogicX = 2;
  } else if ((Rvalue < Lvalue) && (diffX > DeadBandX)) {
    moveLogicX = 1;
  }
}

void CalculateDirectionY() {
  Uvalue = analogRead(UA);
  Dvalue = analogRead(DA);

  int diffY = abs(Uvalue - Dvalue);

  if (diffY <= DeadBandY) {
    moveLogicY = 0;
  } else if ((Uvalue > Dvalue) && (diffY > DeadBandY)) {
    moveLogicY = 2;
  } else if ((Uvalue < Dvalue) && (diffY > DeadBandY)) {
    moveLogicY = 1;
  }
}

void FirstYSetup() {
  stepperY.setMaxSpeed(maxSpeedY);
  stepperY.setAcceleration(accelerationY);
  stepperY.setCurrentPosition(0);

  stepperY.setAcceleration(300);
  stepperY.moveTo(-999999);
  while (digitalRead(button) == HIGH) {
    stepperY.run();
  }

  stepperY.setAcceleration(999999.0f);
  stepperY.moveTo(stepperY.currentPosition());
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }

  stepperY.setAcceleration(300);
  long stepsToMove = 400;
  stepperY.moveTo(stepperY.currentPosition() + stepsToMove);
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }

  stepperY.setAcceleration(999999.0f);
  stepperY.moveTo(stepperY.currentPosition());
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }

  stepperY.setAcceleration(accelerationY);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Start");
  pinMode(button, INPUT_PULLUP);

  stepperX.setMaxSpeed(maxSpeedX);
  stepperX.setAcceleration(accelerationX);
  stepperX.setCurrentPosition(0);
  stepperX.moveTo(stepperX.currentPosition());

  FirstYSetup();
}


void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= btwnMeasure) {
    previousMillis = currentMillis;
    CalculateDirectionX();
    CalculateDirectionY();

    // Вывод в Serial (для отладки)
    Serial.print("RA: ");
    Serial.print(Rvalue);
    Serial.print("\tLA: ");
    Serial.print(Lvalue);
    Serial.print("\tUA: ");
    Serial.print(Uvalue);
    Serial.print("\tDA: ");
    Serial.println(Dvalue);
  }

  if (AllowMoving) { // Условие при котором мотор не движется
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
  } else {
    stepperX.moveTo(stepperX.currentPosition());
    stepperX.run();
  }

  if (digitalRead(button) && AllowMoving) { // Условие при котором мотор не движется
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
  } else {
    stepperY.moveTo(stepperY.currentPosition());
    stepperY.run();
  }

}
