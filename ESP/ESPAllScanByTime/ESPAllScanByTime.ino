#include <AccelStepper.h>
#include <Arduino.h>

// ===== Антенны =====
#define RA 34  // Правая антенна
#define LA 39  // Левая антенна
#define button 15  // кнопка
#define buzzer 2   // пищалка

int Rvalue = 0;  
int Lvalue = 0;  
int DeadBand = 3000;

// Переменная логики для оси X:
//   0 - стоп, 1 - LEFT (минус), 2 - RIGHT (плюс)
int moveLogicX = 0;

// Переменная логики для оси Y:
//   0 - стоп, 1 - вперёд (плюс), 2 - назад (минус)
int moveLogicY = 0;

// ===== Потенциометр для отладки =====
int potentY = 0;  // Читаем на A3

unsigned long previousMillis = 0;  // Для периодического обновления логики работы моторов
const unsigned long btwnMeasure = 60; // Периодичность считывания (мс)

// ===== Параметры шагового мотора (ось X) =====
const int X_PulPlus      = 21;    
const int X_DirPlus      = 22;    
const float maxSpeedX     = 3000; 
const float accelerationX = 2900; 
AccelStepper stepperX(AccelStepper::DRIVER, X_PulPlus, X_DirPlus);

// ===== Параметры шагового мотора (ось Y) =====
const int Y_PulPlus      = 23;    
const int Y_DirPlus      = 19;    
const float maxSpeedY     = 500; 
const float accelerationY = 500; 
AccelStepper stepperY(AccelStepper::DRIVER, Y_PulPlus, Y_DirPlus);

// ===== Параметры сканирования =====
const unsigned long verticalScanPeriod = 10000; // Интервал автосканирования
const unsigned long idleScanThreshold = 1500;   // Порог ожидания для запуска сканирования (мс)
unsigned long lastScanTime = 0;                 // Последнее время сканирования
unsigned long idleStartTime = 0;                // Время начала состояния покоя
bool scanCondition = false;                     // Условие сканирования (true - интервал времени, false - покой)

// Параметры "вертикального" сканирования
const long totalScanSteps  = 700;   // Всего шагов "вперёд"
const unsigned long signalMeasureInterval = 60; // Интервал времени для замера сигнала (мс)
unsigned long lastSignalMeasureTime = 0;         // Последнее время замера сигнала
long bestSignal = -9999;           // Лучший сигнал
long bestPosition = 0;             // Позиция с лучшим сигналом
bool isVerticalScanActive = false; // Идёт ли процесс сканирования
unsigned long scanStartTime = 0;   // Время начала сканирования
unsigned long scanDuration = 0;    // Длительность сканирования
bool isReturningToStart = false;   // Флаг возврата к кнопке

void CalculateDirectionX() {
  Rvalue = analogRead(RA);
  Lvalue = analogRead(LA);

  int diff = abs(Rvalue - Lvalue);

  if (diff <= DeadBand) {
    moveLogicX = 0;
  } else if ((Rvalue > Lvalue) && (diff > DeadBand)) {
    moveLogicX = 2;
  } else if ((Rvalue < Lvalue) && (diff > DeadBand)) {
    moveLogicX = 1;
  }
}

void CalculateDirectionY() {
  // Логика оси Y не реализована, оставлено для отладки
}

void startVerticalScan() {
  Serial.println((Rvalue + Lvalue) / 2);
  if (isVerticalScanActive || isReturningToStart) return;

  // Проверяем условие по среднему сигналу
  if ((Rvalue + Lvalue) / 2 < 4) {
    Serial.println("DEBUG: Signal too low, skipping vertical scan");
    return;
  }

  Serial.println("DEBUG: Returning to start position");
  isReturningToStart = true;

  stepperY.setSpeed(-1300);
  stepperY.moveTo(-999999); // Двигаемся до упора
}

void handleVerticalScan() {
  if (isReturningToStart) {
    if (digitalRead(button) == LOW) {
      Serial.println("DEBUG: Reached start position, beginning scan");
      isReturningToStart = false;

      stepperY.setCurrentPosition(0);
      startVerticalScanProcess();
    }
    stepperY.run();
    return;
  }

  if (!isVerticalScanActive) return;

  unsigned long currentMillis = millis();

  if (currentMillis - lastSignalMeasureTime >= signalMeasureInterval) {
    lastSignalMeasureTime = currentMillis;

    int currentSignal = (analogRead(RA) + analogRead(LA)) / 2;
    Serial.print("DEBUG: Time ");
    Serial.print(currentMillis - scanStartTime);
    Serial.print(" ms, Signal ");
    Serial.println(currentSignal);

    if (currentSignal > bestSignal) {
      bestSignal = currentSignal;
      bestPosition = stepperY.currentPosition();
    }
  }

  if (stepperY.distanceToGo() == 0) {
    scanDuration = currentMillis - scanStartTime;
    Serial.println("DEBUG: Scan complete");
    Serial.print("Best Signal: ");
    Serial.println(bestSignal);
    Serial.print("Best Position: ");
    Serial.println(bestPosition);
    Serial.print("Scan Duration: ");
    Serial.println(scanDuration);

    unsigned long returnTime = (scanDuration * abs(bestPosition)) / totalScanSteps;
    Serial.print("Estimated Return Time: ");
    Serial.println(returnTime);

    stepperY.moveTo(bestPosition);
    isVerticalScanActive = false;
  }

  stepperY.run();
}

void startVerticalScanProcess() {
  Serial.println("DEBUG: startVerticalScanProcess()");
  isVerticalScanActive = true;
  bestSignal = -9999;
  bestPosition = 0;
  scanStartTime = millis();

  long endPosition = totalScanSteps;

  stepperY.setSpeed(1000);
  stepperY.moveTo(endPosition);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Start");

  analogWrite(buzzer, 0);
  delay(100);
  analogWrite(buzzer, 255);
  delay(100);
  analogWrite(buzzer, 0);
  delay(100);
  analogWrite(buzzer, 255);
  delay(100);

  pinMode(button, INPUT_PULLUP);

  stepperX.setMaxSpeed(maxSpeedX);
  stepperX.setAcceleration(accelerationX);
  stepperX.setCurrentPosition(0);
  stepperX.moveTo(stepperX.currentPosition());

  stepperY.setMaxSpeed(maxSpeedY);
  stepperY.setAcceleration(accelerationY);
  stepperY.setCurrentPosition(0);

  stepperY.setAcceleration(100);
  stepperY.moveTo(-999999);
  while (digitalRead(button) == HIGH) {
    stepperY.run();
  }

  stepperY.setAcceleration(999999.0f);
  stepperY.moveTo(stepperY.currentPosition());
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }

  stepperY.setAcceleration(100);
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

  lastScanTime = millis();
  Serial.println("DEBUG: setup() done, entering loop()");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= btwnMeasure) {
    previousMillis = currentMillis;
    CalculateDirectionX();
    CalculateDirectionY();
  }

  if (moveLogicX == 0) {
    if (idleStartTime == 0) {
      idleStartTime = currentMillis;
    } else if (currentMillis - idleStartTime >= idleScanThreshold && !isVerticalScanActive && !isReturningToStart && !scanCondition) {
      idleStartTime = 0;
      Serial.println("DEBUG: Idle threshold reached, starting scan");
      startVerticalScan();
    }
  } else {
    idleStartTime = 0;
  }

  if (!isVerticalScanActive && !isReturningToStart && scanCondition && (currentMillis - lastScanTime >= verticalScanPeriod)) {
    lastScanTime = currentMillis;
    Serial.println("DEBUG: Time-based scan trigger");
    startVerticalScan();
  }

  if (!isVerticalScanActive && !isReturningToStart) {
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

  stepperY.run();

  handleVerticalScan();
}