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
int nSegments              = 50;    // на сколько диапазонов делим 600 шагов
// => шагов в каждом диапазоне = totalScanSteps / nSegments

// Вспомогательные переменные для процесса сканирования
bool isVerticalScanActive = false;    // идёт ли сейчас цикл сканирования
int  currentSegment = 0;             // какой из n сегментов проходим
long startScanPosition = 0;          // откуда начинаем движение вперёд
long bestPositionOffset = 0;         // где сигнал был максимум
int  bestSignal = -9999;             // максимум сигнала
long stepsPerSegment = 2;            // totalScanSteps / nSegments (вычисляется)

// === Новый глобальный: какой именно SEGMENT был лучшим ===
int bestSegmentIndex = 0;            // 0..nSegments

// Состояния внутреннего автомата
enum VScanState {
  VS_IDLE = 0,
  VS_MOVE_NEG,
  VS_STOP_NEG,
  VS_MOVE_SEGMENTS,
  VS_RETURN_BEST
};
VScanState vscanState = VS_IDLE;

// ----------------------------------------------------------------------------
// Функция запуска вертикального сканирования
// ----------------------------------------------------------------------------
void startVerticalScan() {
  if (isVerticalScanActive) return; // Не дублировать

  Serial.println("DEBUG: startVerticalScan()");
  isVerticalScanActive = true;
  vscanState = VS_MOVE_NEG;

  Serial.println("DEBUG: vscanState -> VS_MOVE_NEG");
  stepperY.setAcceleration(300);  
  stepperY.moveTo(-999999); // идём в минус, пока не нажмут кнопку
}

// ----------------------------------------------------------------------------
// Процесс сканирования — вызывается в loop()
// ----------------------------------------------------------------------------
void handleVerticalScan() {
  if (!isVerticalScanActive) return;  // не запущено
  
  switch (vscanState) {

    case VS_MOVE_NEG:
      if (digitalRead(button) == HIGH) {
        Serial.println("DEBUG: Button is HIGH, go STOP_NEG");
        vscanState = VS_STOP_NEG;
        stepperY.setAcceleration(999999.0f);
        stepperY.moveTo(stepperY.currentPosition());
      }
      break;
      
    case VS_STOP_NEG:
      if (stepperY.distanceToGo() == 0) {
        Serial.println("DEBUG: Reached STOP_NEG, go MOVE_SEGMENTS");
        
        // Готовимся двинуться на +600
        stepperY.setAcceleration(900);  
        startScanPosition = stepperY.currentPosition(); 
        stepsPerSegment = totalScanSteps / nSegments;
        currentSegment = 0;
        bestSignal = -9999;
        bestPositionOffset = 0;
        bestSegmentIndex = 0;  // Сброс лучшего сегмента
        
        vscanState = VS_MOVE_SEGMENTS;
        long nextPos = startScanPosition + stepsPerSegment;
        stepperY.moveTo(nextPos);
      }
      break;
      
    case VS_MOVE_SEGMENTS:
      if (stepperY.distanceToGo() == 0) {
        // Закончили очередной сегмент
        currentSegment++;

        // Считываем "вертикальный" сигнал
        int verticalSignal = ( (int)Rvalue + (int)Lvalue ) / 2; 
        Serial.print("DEBUG: Segment ");
        Serial.print(currentSegment);
        Serial.print("/");
        Serial.print(nSegments);
        Serial.print(", offset=");
        Serial.print(currentSegment*stepsPerSegment);
        Serial.print(", signal=");
        Serial.println(verticalSignal);
        
        // Проверяем максимум
        if (verticalSignal > bestSignal) {
          bestSignal = verticalSignal;
          bestPositionOffset = currentSegment * stepsPerSegment;
          bestSegmentIndex = currentSegment;  // Запомним сегмент
        }
        
        // Если ещё не дошли до конца:
        if (currentSegment < nSegments) {
          long nextPos = startScanPosition + (currentSegment+1) * stepsPerSegment;
          stepperY.moveTo(nextPos);
        } else {
          // Закончили все 600 шагов -> переходим к VS_RETURN_BEST
          vscanState = VS_RETURN_BEST;
          long bestPos = startScanPosition + bestPositionOffset;
          Serial.print("DEBUG: done segments, bestSignal=");
          Serial.print(bestSignal);
          Serial.print(", bestOffset=");
          Serial.print(bestPositionOffset);
          Serial.print(", bestSegmentIndex=");
          Serial.println(bestSegmentIndex);
          
          stepperY.moveTo(bestPos);
        }
      }
      break;
      
    case VS_RETURN_BEST:
      if (stepperY.distanceToGo() == 0) {
        Serial.println("DEBUG: Reached best signal position, done!");
        Serial.print("BEST segment index was: ");
        Serial.println(bestSegmentIndex);

        // Сканирование завершено
        stepperY.setAcceleration(accelerationY);
        vscanState = VS_IDLE;
        isVerticalScanActive = false;
      }
      break;

    case VS_IDLE:
    default:
      // ничего
      break;
  }
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
if (!isVerticalScanActive) {
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

/*
  // ===== Управление осью Y =====
  // (Закомментировано, чтобы не мешать автомату сканирования.
  //  Если нужно, можно оставить.)
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
*/
  stepperY.run();

  // --- Проверяем, не пора ли запускать сканирование (каждые verticalScanPeriod мс) ---
  if (!isVerticalScanActive && (currentMillis - lastScanTime >= verticalScanPeriod)) {
    lastScanTime = currentMillis; 
    Serial.println("DEBUG: time passed, calling startVerticalScan()");
    startVerticalScan();
  }

  // --- Обрабатываем процесс сканирования (автомат) ---
  handleVerticalScan();
}