
// ===== Антенны (аналоги) =====
#define RA A2  // Правая антенна
#define LA A1  // Левая антенна

// ===== Глобальные переменные для антенн =====
int Rvalue = 0;  
int Lvalue = 0;  
int DeadBand = 20;

// Для периодического чтения антенн
unsigned long previousMillis = 0;  
const unsigned long btwnMeasure = 50; // интервал (мс) между измерениями

// ----------------------------------------------------------------------------
// Функция определения направления (moveLogic), исходя из разницы сигналов
// ----------------------------------------------------------------------------
void CalculateDirection() {
 
  Rvalue = analogRead(RA);
  Lvalue = analogRead(LA);

  int Y = (Rvalue + Lvalue)/2; 
  Serial.println(Y);
}

// ----------------------------------------------------------------------------
// Стандартные функции setup() и loop()
// ----------------------------------------------------------------------------
void setup() {
  // Инициализация Serial для отладки
  Serial.begin(9600);
}

void loop() {
  unsigned long currentMillis = millis();

  // Периодически считываем данные с антенн
  if (currentMillis - previousMillis >= btwnMeasure) {
    previousMillis = currentMillis;
    CalculateDirection();
  }
}