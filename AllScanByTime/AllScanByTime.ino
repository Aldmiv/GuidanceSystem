#include <AccelStepper.h>

// ===== Антенны =====
#define RA A2  // Правая антенна
#define LA A1  // Левая антенна
#define button 12  // кнопка
#define buzzer 2   // пищалка

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
const unsigned long btwnMeasure = 60; // Периодичность считывания (мс)

// ===== Параметры шагового мотора (ось X) =====
const int X_PulPlus      = 8;    
const int X_DirPlus      = 9;    
const float maxSpeedX     = 3000; 
const float accelerationX = 2900; 
AccelStepper stepperX(AccelStepper::DRIVER, X_PulPlus, X_DirPlus);

// ===== Параметры шагового мотора (ось Y) =====
const int Y_PulPlus      = 6;    
const int Y_DirPlus      = 7;    
const float maxSpeedY     = 2500; 
const float accelerationY = 2000; 
AccelStepper stepperY(AccelStepper::DRIVER, Y_PulPlus, Y_DirPlus);

// ----------------------------------------------------------------------------
// Логика определения moveLogicX на основе антенн RA и LA
// ----------------------------------------------------------------------------
void CalculateDirectionX() {
  Rvalue = analogRead(RA);
  Lvalue = analogRead(LA);

  int diff = abs(Rvalue - Lvalue);
  //Serial.print("diff X = ");
  //Serial.print(diff);

  if (diff <= DeadBand) {
    //Serial.println(" => STOP");
    moveLogicX = 0;
  }
  else if ((Rvalue > Lvalue) && (diff > DeadBand)) {
    //Serial.println(" => RIGHT");
    moveLogicX = 2;
  }
  else if ((Rvalue < Lvalue) && (diff > DeadBand)) {
    //Serial.println(" => LEFT");
    moveLogicX = 1;
  }
}

// ----------------------------------------------------------------------------
// Временная логика определения moveLogicY на основе потенциометра A3
// (пока закомментирована - оставляем как есть)
// ----------------------------------------------------------------------------
void CalculateDirectionY() {
/*
  potentY = analogRead(A3);
  // Debug:
  Serial.print("DEBUG: potentY = ");
  Serial.println(potentY);

  if (potentY < 100) {
    moveLogicY = 1; 
  }
  else if (potentY > 1000) {
    moveLogicY = 2;
  }
  else {
    moveLogicY = 0; 
  }
*/
}

// =============================
// ===== НОВЫЙ ФУНКЦИОНАЛ =====
// =============================

// Глобальные переменные для "сканирования" по Y (каждые 20 сек для примера)
const unsigned long verticalScanPeriod = 20000; // Каждые 20 секунд
unsigned long lastScanTime = 0;                 // когда была последняя попытка сканирования

// Параметры «вертикального» шага
const long totalScanSteps  = 800;   // всего шагов "вперёд"
const unsigned long signalMeasureInterval = 100; // Интервал времени для замера сигнала (мс)
unsigned long lastSignalMeasureTime = 0;         // Последнее время замера сигнала
long bestSignal = -9999;           // Лучший сигнал
long bestPosition = 0;             // Позиция с лучшим сигналом
bool isVerticalScanActive = false; // Идёт ли процесс сканирования
unsigned long scanStartTime = 0;   // Время начала сканирования
unsigned long scanDuration = 0;    // Длительность сканирования
bool isReturningToStart = false;   // Флаг возврата к кнопке

void startVerticalScan() {
  if (isVerticalScanActive || isReturningToStart) return; // Не дублировать

  Serial.println("DEBUG: Returning to start position");
  isReturningToStart = true;

  // Двигаемся к кнопке (вниз)
  stepperY.setSpeed(-1000);
  stepperY.moveTo(-999999); // Двигаемся до упора
}

void handleVerticalScan() {
  if (isReturningToStart) {
    if (digitalRead(button) == HIGH) {
      Serial.println("DEBUG: Reached start position, beginning scan");
      isReturningToStart = false;

      // Устанавливаем начальную точку для сканирования
      stepperY.setCurrentPosition(0);
      startVerticalScanProcess();
    }
    stepperY.run();
    return;
  }

  if (!isVerticalScanActive) return;

  unsigned long currentMillis = millis();

  // Фиксация сигнала через определённые промежутки времени
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

  // Проверка завершения сканирования
  if (stepperY.distanceToGo() == 0) {
    scanDuration = currentMillis - scanStartTime;
    Serial.println("DEBUG: Scan complete");
    Serial.print("Best Signal: ");
    Serial.println(bestSignal);
    Serial.print("Best Position: ");
    Serial.println(bestPosition);
    Serial.print("Scan Duration: ");
    Serial.println(scanDuration);

    // Вычисляем время возврата к лучшему сигналу
    unsigned long returnTime = (scanDuration * abs(bestPosition)) / totalScanSteps;
    Serial.print("Estimated Return Time: ");
    Serial.println(returnTime);

    // Возврат к лучшему положению
    stepperY.moveTo(bestPosition);
    isVerticalScanActive = false;
  }

  // Продолжаем движение
  stepperY.run();
}

void startVerticalScanProcess() {
  Serial.println("DEBUG: startVerticalScanProcess()");
  isVerticalScanActive = true;
  bestSignal = -9999;
  bestPosition = 0;
  scanStartTime = millis();

  // Устанавливаем начальное и конечное положение сканирования
  long endPosition = totalScanSteps;

  // Устанавливаем равномерное движение
  stepperY.setSpeed(1000);
  stepperY.moveTo(endPosition);
}

// ----------------------------------------------------------------------------
// setup()
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  Serial.println("Start");

  analogWrite(buzzer,0);
  delay(100);
  analogWrite(buzzer,255);
  delay(100);
  analogWrite(buzzer,0);
  delay(100);
  analogWrite(buzzer,255);
  delay(100);

  pinMode(button, INPUT);

  // === Ось X ===
  stepperX.setMaxSpeed(maxSpeedX);
  stepperX.setAcceleration(accelerationX);
  stepperX.setCurrentPosition(0);
  stepperX.moveTo(stepperX.currentPosition());

  // === Ось Y ===
  stepperY.setMaxSpeed(maxSpeedY);
  stepperY.setAcceleration(accelerationY);
  stepperY.setCurrentPosition(0);

  // ---------- Начальный прогон мотора Y ----------
  stepperY.setAcceleration(300); 
  stepperY.moveTo(-999999);
  while (digitalRead(button) == LOW) {
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

  // Возвращаем глобальное ускорение
  stepperY.setAcceleration(accelerationY);

  // Сохраняем время, чтобы через verticalScanPeriod запустить первый скан
  lastScanTime = millis();
  Serial.println("DEBUG: setup() done, entering loop()");
}

// ----------------------------------------------------------------------------
// loop()
// ----------------------------------------------------------------------------
void loop() {
  unsigned long currentMillis = millis();

  // Каждые btwnMeasure выполняем сканирование
  if (currentMillis - previousMillis >= btwnMeasure) {
    previousMillis = currentMillis;
    CalculateDirectionX();
    CalculateDirectionY();
  }

  // ===== Управление осью X =====
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
    // Останавливаем мотор X, удерживая текущую позицию
    stepperX.moveTo(stepperX.currentPosition());
    stepperX.run();
  }

  stepperY.run();

  // --- Проверяем, не пора ли запускать сканирование (каждые verticalScanPeriod мс) ---
  if (!isVerticalScanActive && !isReturningToStart && (currentMillis - lastScanTime >= verticalScanPeriod)) {
    lastScanTime = currentMillis; 
    Serial.println("DEBUG: time passed, calling startVerticalScan()");
    startVerticalScan();
  }

  // --- Обрабатываем процесс сканирования ---
  handleVerticalScan();
}