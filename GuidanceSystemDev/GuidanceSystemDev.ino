#include <ParadigmaScreens.h>
#include <AccelStepper.h>

// ===== Антенны =====
#define RA A2            // Правая антенна
#define LA A1            // Левая антенна
#define borderbutton 12  // кнопка
#define buzzer 2         // пищалка

// Определяем пины для Bluetooth модуля
int txPin = 4;  // TX пин на Arduino / TX pin on Arduino
int rxPin = 3;  // RX пин на Arduino / RX pin on Arduino

// Создаем объект ParadigmaScreens
ParadigmaScreens ParadigmaScreens(txPin, rxPin);

int Rvalue = 0;
int Lvalue = 0;
int DeadBand = 20;

// Переменная логики для оси X:
//   0 - стоп, 1 - LEFT (минус), 2 - RIGHT (плюс)
int moveLogicX = 0;

// Переменная логики для оси Y:
//   0 - стоп, 1 - вперёд (плюс), 2 - назад (минус)
int moveLogicY = 0;

//Переменная разрешающая сканировать по x
bool AllowScanY = true;

// ===== Потенциометр для отладки =====
int potentY = 0;  // Читаем на A3

unsigned long previousMillis = 0;
const unsigned long btwnMeasure = 60;  // Периодичность считывания (мс)

// ===== Параметры шагового мотора (ось X) =====
const int X_PulPlus = 8;
const int X_DirPlus = 9;
int maxSpeedX = 3000;
const float accelerationX = 2900;
AccelStepper stepperX(AccelStepper::DRIVER, X_PulPlus, X_DirPlus);

// ===== Параметры шагового мотора (ось Y) =====
const int Y_PulPlus = 6;
const int Y_DirPlus = 7;
const float maxSpeedY = 2500;
const float accelerationY = 2000;
AccelStepper stepperY(AccelStepper::DRIVER, Y_PulPlus, Y_DirPlus);

// ===== Параметры сканирования =====
const unsigned long verticalScanPeriod = 10000;  // Интервал автосканирования
const unsigned long idleScanThreshold = 1500;    // Порог ожидания для запуска сканирования (мс)
unsigned long lastScanTime = 0;                  // Последнее время сканирования
unsigned long idleStartTime = 0;                 // Время начала состояния покоя
bool scanCondition = false;                      // Условие сканирования (true - интервал времени, false - покой)

// Параметры "вертикального" сканирования
const long totalScanSteps = 700;                 // Всего шагов "вперёд"
const unsigned long signalMeasureInterval = 60;  // Интервал времени для замера сигнала (мс)
unsigned long lastSignalMeasureTime = 0;         // Последнее время замера сигнала
long bestSignal = -9999;                         // Лучший сигнал
long bestPosition = 0;                           // Позиция с лучшим сигналом
bool isVerticalScanActive = false;               // Идёт ли процесс сканирования
unsigned long scanStartTime = 0;                 // Время начала сканирования
unsigned long scanDuration = 0;                  // Длительность сканирования
bool isReturningToStart = false;                 // Флаг возврата к кнопке

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
  //Serial.println((Rvalue + Lvalue) / 2);
  if (isVerticalScanActive || isReturningToStart) return;

  // Проверяем условие по среднему сигналу
  if (!AllowScanY) {
    //Serial.println("DEBUG: Signal too low, skipping vertical scan");
    return;
  }

  //Serial.println("DEBUG: Returning to start position");
  isReturningToStart = true;

  stepperY.setSpeed(-1300);
  stepperY.moveTo(-999999);  // Двигаемся до упора
}

void handleVerticalScan() {
  if (isReturningToStart) {
    if (digitalRead(borderbutton) == HIGH) {
      //Serial.println("DEBUG: Reached start position, beginning scan");
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
    //Serial.print("DEBUG: Time ");
    //Serial.print(currentMillis - scanStartTime);
    //Serial.print(" ms, Signal ");
    //Serial.println(currentSignal);

    if (currentSignal > bestSignal) {
      bestSignal = currentSignal;
      bestPosition = stepperY.currentPosition();
    }
  }

  if (stepperY.distanceToGo() == 0) {
    scanDuration = currentMillis - scanStartTime;
    //Serial.println("DEBUG: Scan complete");
    //Serial.print("Best Signal: ");
    //Serial.println(bestSignal);
    //Serial.print("Best Position: ");
    //Serial.println(bestPosition);
    //Serial.print("Scan Duration: ");
    //Serial.println(scanDuration);

    unsigned long returnTime = (scanDuration * abs(bestPosition)) / totalScanSteps;
    //Serial.print("Estimated Return Time: ");
    //Serial.println(returnTime);

    stepperY.moveTo(bestPosition);
    isVerticalScanActive = false;
  }

  stepperY.run();
}

void startVerticalScanProcess() {
  //Serial.println("DEBUG: startVerticalScanProcess()");
  isVerticalScanActive = true;
  bestSignal = -9999;
  bestPosition = 0;
  scanStartTime = millis();

  long endPosition = totalScanSteps;

  stepperY.setSpeed(1000);
  stepperY.moveTo(endPosition);
}

void setup() {
  // Инициализация соединения с модулем Bluetooth
  ParadigmaScreens.begin(9600);

  Serial.begin(115200);
  //Serial.println("Start");

  analogWrite(buzzer, 0);
  delay(100);
  analogWrite(buzzer, 255);
  delay(100);
  analogWrite(buzzer, 0);
  delay(100);
  analogWrite(buzzer, 255);
  delay(100);

  pinMode(borderbutton, INPUT);

  stepperX.setMaxSpeed(maxSpeedX);
  stepperX.setAcceleration(accelerationX);
  stepperX.setCurrentPosition(0);
  stepperX.moveTo(stepperX.currentPosition());

  stepperY.setMaxSpeed(maxSpeedY);
  stepperY.setAcceleration(accelerationY);
  stepperY.setCurrentPosition(0);

  stepperY.setAcceleration(300);
  stepperY.moveTo(-999999);
  while (digitalRead(borderbutton) == LOW) {
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

  lastScanTime = millis();
  //Serial.println("DEBUG: setup() done, entering loop()");
}

void loop() {

  // Эта строка всегда должна быть в коде для обработки событий
  ParadigmaScreens.update();
  bluetoothControl();

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
      //Serial.println("DEBUG: Idle threshold reached, starting scan");
      startVerticalScan();
    }
  } else {
    idleStartTime = 0;
  }

  if (!isVerticalScanActive && !isReturningToStart && scanCondition && (currentMillis - lastScanTime >= verticalScanPeriod)) {
    lastScanTime = currentMillis;
    //Serial.println("DEBUG: Time-based scan trigger");
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
  } else {
    stepperX.moveTo(stepperX.currentPosition());
  }

  handleVerticalScan();

  stepperX.run();
  stepperY.run();
}

void bluetoothControl() {

//исправить
  if (ParadigmaScreens.button[1]) {
maxSpeedX = maxSpeedX - 10;
ParadigmaScreens.sendText(2, maxSpeedX);
  }

  if (ParadigmaScreens.switchState[1]) {
    AllowScanY = true;
  } else {
    AllowScanY = false;
  }
}