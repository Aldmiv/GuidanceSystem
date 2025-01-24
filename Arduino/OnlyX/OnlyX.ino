#include <AccelStepper.h>

// ===== Антенны =====
#define RA A2  // Правая антенна
#define LA A1  // Левая антенна

// ===== Глобальные переменные для антенн =====
int Rvalue = 0;  
int Lvalue = 0;  
int DeadBand = 20;

int potent = 0; //для отладки потенциометром

// Переменная, определяющая логику движения:
// 0 - стоп, 1 - направо (вперёд), 2 - налево (назад)
int moveLogic = 0;

// Для периодического чтения антенн
unsigned long previousMillis = 0;  
const unsigned long btwnMeasure = 50; // интервал (мс) между измерениями

// ===== Параметры шагового мотора =====
const int X_PulPlus      = 8;    // Пин для импульсов (шагов)
const int X_DirPlus      = 9;    // Пин для направления
const float maxSpeed     = 2500; // Максимальная скорость (шаг/с)
const float acceleration = 2000; // Ускорение (шаг/с^2)

// Объект класса AccelStepper в режиме DRIVER:
//   1-й параметр — тип драйвера (DRIVER),
//   2-й — пин для импульсов,
//   3-й — пин для направления.
AccelStepper stepper(AccelStepper::DRIVER, X_PulPlus, X_DirPlus);

// ----------------------------------------------------------------------------
// Функция определения направления (moveLogic), исходя из разницы сигналов
// ----------------------------------------------------------------------------
void CalculateDirection() {
 
  Rvalue = analogRead(RA);
  Lvalue = analogRead(LA);

  int diff = abs(Rvalue - Lvalue); 
  Serial.println(diff);

  // Если в "мертвой зоне", останавливаемся
  if (diff <= DeadBand) {
    Serial.println("STOP");
    moveLogic = 0;
  }
  // Если сигнал правой антенны больше левой — движение в одну сторону
  else if ((Rvalue > Lvalue) && (diff > DeadBand)) {
    Serial.println("RIGHT");
    moveLogic = 2;
  }
  // Если сигнал левой антенны больше правой — движение в другую сторону
  else if ((Rvalue < Lvalue) && (diff > DeadBand)) {
    Serial.println("LEFT");
    moveLogic = 1;
  }

/*
potent = analogRead(A3);
Serial.println(moveLogic);

if (potent<100) {moveLogic = 1;}
if (potent>1000) {moveLogic = 2;}
if (potent>=100 && potent<=1000) {moveLogic = 0;}
*/
}

void setup() {
  // Инициализация Serial для отладки
  Serial.begin(9600);

  // Настройки шагового мотора: макс. скорость и ускорение
  stepper.setMaxSpeed(maxSpeed);
  stepper.setAcceleration(acceleration);

  stepper.stop(); 
}

void loop() {
  unsigned long currentMillis = millis();

  // Периодически считываем данные с антенн
  if (currentMillis - previousMillis >= btwnMeasure) {
    previousMillis = currentMillis;
    CalculateDirection();
  }

  // В зависимости от moveLogic определяем, куда двигать мотор
  switch (moveLogic) {
    case 0:
      // Остановка с учётом плавного замедления
      stepper.moveTo(stepper.currentPosition());
      break;
    case 1:
      stepper.moveTo(1000000);
      //stepper.stop();
      break;
    case 2:
      stepper.moveTo(-1000000);
      //stepper.stop();
      break;
  }

  // Запускаем работу шагового двигателя (важно вызывать в каждом проходе loop)
  stepper.run();
}

