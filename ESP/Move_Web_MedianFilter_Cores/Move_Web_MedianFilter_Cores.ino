///////////////////////////////////////↓↓↓↓Настройки веба↓↓↓↓/////////////////////////////////////
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Настройка Wi-Fi точки доступа
const char *ssid = "ESP32";
const char *password = "8467239547495";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);  // Асинхронный веб-сервер

int RvalueWeb = 0, LvalueWeb = 0, UvalueWeb = 0, DvalueWeb = 0;  // Данные для веб-сервера

// Таймер для обновления значений в JSON (сервер)
unsigned long previousMillisWeb = 0;
const unsigned long interval = 150;  // Интервал обновления для веба
///////////////////////////////////////↑↑↑↑Настройки веба↑↑↑↑/////////////////////////////////////

#include <Arduino.h>
#include <AccelStepper.h>
#include <RunningMedian.h>

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

// Для фильтра
RunningMedian filterR(10);
RunningMedian filterL(10);
RunningMedian filterU(10);
RunningMedian filterD(10);

// Значения сигналов с антенн (до фильтра)
volatile int RvalueBeforeFilter = 0, LvalueBeforeFilter = 0, UvalueBeforeFilter = 0, DvalueBeforeFilter = 0;

// Значения сигналов после фильтра
volatile int Rvalue = 0, Lvalue = 0, Uvalue = 0, Dvalue = 0;

// Разница между парами антенн
int DX = 0;
int DY = 0;

int DeadBandX = 100;
int DeadBandY = 100;

int btwnMeasure = 30;              // Периодичность считывания (мс)
unsigned long previousMillis = 0;  // Для периодического обновления логики
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
float maxSpeedY = 2500.0;
float accelerationY = 2000.0;
AccelStepper stepperY(AccelStepper::DRIVER, Y_PulPlus, Y_DirPlus);


// -------------------------------------------------------------------------------------
// Функции расчёта направления
// -------------------------------------------------------------------------------------
void CalculateDirectionX() {

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

  int diffY = abs(Uvalue - Dvalue);

  if (diffY <= DeadBandY) {
    moveLogicY = 0;
  } else if ((Uvalue > Dvalue) && (diffY > DeadBandY)) {
    moveLogicY = 2;
  } else if ((Uvalue < Dvalue) && (diffY > DeadBandY)) {
    moveLogicY = 1;
  }
}

// -------------------------------------------------------------------------------------
// Первичная калибровка оси Y по кнопке
// -------------------------------------------------------------------------------------
void FirstYSetup() {
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

// -------------------------------------------------------------------------------------
// HTML веб-страницы
// -------------------------------------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Tracking System</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <style>
    body {
      background-color: #202020; color: white; font-family: Arial, sans-serif;
      margin: 0; padding: 0; display: flex; align-items: center; justify-content: center; 
      min-height: 100vh; overflow-x: hidden;
    }
    .container {
      display: flex; flex-direction: column; background: #303030; padding: 20px;
      border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2); max-width: 100%;
    }
    .row {
      display: flex; justify-content: flex-start; align-items: center; margin-bottom: 15px;
      gap: 10px; flex-wrap: wrap;
    }
    .row strong { font-size: 1rem; min-width: 120px; }
    .row input {
      padding: 5px 10px; border-radius: 5px; border: none; outline: none; width: 150px;
    }
    .row button {
      padding: 5px 15px; border-radius: 5px; border: none; background: #007BFF; 
      color: white; cursor: pointer; flex-shrink: 0;
    }
    .row button:hover { background: #0056b3; }
    .emergency-container {
      display: flex; flex-direction: column; align-items: center; margin: 20px 0;
    }
    .emergency-container h2 { font-size: 1.5rem; margin: 0 0 10px 0; }
    #AllowMovingSwitch { width: 40px; height: 40px; cursor: pointer; }
    .crosshair-container {
      position: relative; width: 200px; height: 200px; margin: 20px auto; background: #444;
    }
    .crosshair {
      position: absolute; width: 100%; height: 100%; top: 0; left: 0;
    }
    .horizontal-line, .vertical-line {
      position: absolute; background-color: #ffffff; opacity: 0.6;
    }
    .horizontal-line {
      top: 50%; left: 0; right: 0; height: 1px;
    }
    .vertical-line {
      left: 50%; top: 0; bottom: 0; width: 1px;
    }
    .dot {
      position: absolute; left: 100px; top: 100px; margin: -5px 0 0 -5px; 
      width: 10px; height: 10px; background-color: red; border-radius: 50%;
      transform: none;
    }
  </style>
  <script>
    async function refreshValues() {
      const response = await fetch('/values');
      const data = await response.json();
      document.getElementById('RA').innerText = data.RA;
      document.getElementById('LA').innerText = data.LA;
      document.getElementById('UA').innerText = data.UA;
      document.getElementById('DA').innerText = data.DA;
      document.getElementById('DX').innerText = data.DX;
      document.getElementById('DY').innerText = data.DY;
      document.getElementById('maxSpeedX').innerText = data.maxSpeedX;
      document.getElementById('accelerationX').innerText = data.accelerationX;
      document.getElementById('maxSpeedY').innerText = data.maxSpeedY;
      document.getElementById('accelerationY').innerText = data.accelerationY;
      document.getElementById('DeadBandX').innerText = data.DeadBandX;
      document.getElementById('DeadBandY').innerText = data.DeadBandY;
      document.getElementById('btwnMeasure').innerText = data.btwnMeasure;
      
      // Обновляем положение точки (для наглядности)
      const dot = document.querySelector('.dot');
      if (dot) {
        const maxOffset = 100.0;
        const maxValue = 4095.0;
        const k = 4.0;
        let xOffset = (data.DX / maxValue) * maxOffset * k;
        let yOffset = -(data.DY / maxValue) * maxOffset * k;

        if (xOffset >  100) xOffset =  100;
        if (xOffset < -100) xOffset = -100;
        if (yOffset >  100) yOffset =  100;
        if (yOffset < -100) yOffset = -100;

        dot.style.transform = `translate(${xOffset}px, ${yOffset}px)`;
      }
    }

    async function updateMaxSpeedX() {
      const input = document.getElementById('maxSpeedXInput').value;
      await fetch(`/updateMaxSpeedX?value=${input}`);
    }
    async function updateAccelerationX() {
      const input = document.getElementById('accelerationXInput').value;
      await fetch(`/updateAccelerationX?value=${input}`);
    }
    async function updateMaxSpeedY() {
      const input = document.getElementById('maxSpeedYInput').value;
      await fetch(`/updateMaxSpeedY?value=${input}`);
    }
    async function updateAccelerationY() {
      const input = document.getElementById('accelerationYInput').value;
      await fetch(`/updateAccelerationY?value=${input}`);
    }
    async function updateDeadBandX() {
      const input = document.getElementById('DeadBandXInput').value;
      await fetch(`/updateDeadBandX?value=${input}`);
    }
    async function updateDeadBandY() {
      const input = document.getElementById('DeadBandYInput').value;
      await fetch(`/updateDeadBandY?value=${input}`);
    }
    async function updateBtwnMeasure() {
      const input = document.getElementById('btwnMeasureInput').value;
      await fetch(`/updateBtwnMeasure?value=${input}`);
    }
    async function updateAllowMoving() {
      // Галочка = EmergencyStop -> AllowMoving = false (value=0)
      // Нет галочки -> AllowMoving = true (value=1)
      const input = document.getElementById('AllowMovingSwitch').checked;
      await fetch(`/updateAllowMoving?value=${input ? 0 : 1}`);
    }

    // Обновление каждые 150 мс
    setInterval(refreshValues, 150);
  </script>
</head>
<body>
  <div class="container">
    <h1>Tracking System</h1>

    <div class="row"><strong>Right Antenna:</strong> <span id="RA">0</span></div>
    <div class="row"><strong>Left Antenna:</strong> <span id="LA">0</span></div>
    <div class="row"><strong>Upper Antenna:</strong> <span id="UA">0</span></div>
    <div class="row"><strong>Down Antenna:</strong> <span id="DA">0</span></div>

    <div class="row"><strong>DX (R-L):</strong> <span id="DX">0</span></div>
    <div class="row"><strong>DY (U-D):</strong> <span id="DY">0</span></div>

    <div class="crosshair-container">
      <div class="crosshair">
        <div class="horizontal-line"></div>
        <div class="vertical-line"></div>
        <div class="dot"></div>
      </div>
    </div>

    <div class="emergency-container">
      <h2>Emergency Stop</h2>
      <input type="checkbox" id="AllowMovingSwitch" onchange="updateAllowMoving()">
    </div>

    <div class="row"><strong>Max Speed X:</strong> <span id="maxSpeedX">0.0</span></div>
    <div class="row">
      <input type="text" id="maxSpeedXInput" placeholder="Enter value"> 
      <button onclick="updateMaxSpeedX()">Update</button>
    </div>

    <div class="row"><strong>Acceleration X:</strong> <span id="accelerationX">0.0</span></div>
    <div class="row">
      <input type="text" id="accelerationXInput" placeholder="Enter value"> 
      <button onclick="updateAccelerationX()">Update</button>
    </div>

    <div class="row"><strong>Max Speed Y:</strong> <span id="maxSpeedY">0.0</span></div>
    <div class="row">
      <input type="text" id="maxSpeedYInput" placeholder="Enter value"> 
      <button onclick="updateMaxSpeedY()">Update</button>
    </div>

    <div class="row"><strong>Acceleration Y:</strong> <span id="accelerationY">0.0</span></div>
    <div class="row">
      <input type="text" id="accelerationYInput" placeholder="Enter value"> 
      <button onclick="updateAccelerationY()">Update</button>
    </div>

    <div class="row"><strong>Dead Band X:</strong> <span id="DeadBandX">0</span></div>
    <div class="row">
      <input type="text" id="DeadBandXInput" placeholder="Enter value"> 
      <button onclick="updateDeadBandX()">Update</button>
    </div>

    <div class="row"><strong>Dead Band Y:</strong> <span id="DeadBandY">0</span></div>
    <div class="row">
      <input type="text" id="DeadBandYInput" placeholder="Enter value"> 
      <button onclick="updateDeadBandY()">Update</button>
    </div>

    <div class="row"><strong>Between Measure:</strong> <span id="btwnMeasure">0</span></div>
    <div class="row">
      <input type="text" id="btwnMeasureInput" placeholder="Enter value"> 
      <button onclick="updateBtwnMeasure()">Update</button>
    </div>
  </div>
</body>
</html>
)rawliteral";


// -------------------------------------------------------------------------------------
// Задача для core 0:
// Cбор данных с антенн, фильтрация, вычисления, работа с веб-сервером и т.д.
// -------------------------------------------------------------------------------------
void everythingElseTask(void *pvParameters) {
  for (;;)  // Бесконечный цикл задачи на core 0
  {
    unsigned long currentMillis = millis();

    // 1) Считывание и фильтрация значений
    RvalueBeforeFilter = analogRead(RA);
    LvalueBeforeFilter = analogRead(LA);
    filterR.add(RvalueBeforeFilter);
    filterL.add(LvalueBeforeFilter);
    Rvalue = filterR.getMedian();
    Lvalue = filterL.getMedian();

    UvalueBeforeFilter = analogRead(UA);
    DvalueBeforeFilter = analogRead(DA);
    filterU.add(UvalueBeforeFilter);
    filterD.add(DvalueBeforeFilter);
    Uvalue = filterU.getMedian();
    Dvalue = filterD.getMedian();

    // 2) Интервал обновления значений для веб-сервера
    if (currentMillis - previousMillisWeb >= interval) {
      previousMillisWeb = currentMillis;

      RvalueWeb = Rvalue;
      LvalueWeb = Lvalue;
      DX = Rvalue - Lvalue;

      UvalueWeb = Uvalue;
      DvalueWeb = Dvalue;
      DY = Uvalue - Dvalue;
    }

    // 3) Каждые btwnMeasure мс обновляем логику движения
    if (currentMillis - previousMillis >= btwnMeasure) {
      previousMillis = currentMillis;
      CalculateDirectionX();
      CalculateDirectionY();

      // Для отладки
      // Serial.print("RA: "); Serial.print(Rvalue);
      // Serial.print("\tLA: "); Serial.print(Lvalue);
      // Serial.print("\tUA: "); Serial.print(Uvalue);
      // Serial.print("\tDA: "); Serial.println(Dvalue);
    }

    vTaskDelay(1);  // Дадим немного «отдыха» RTOS
  }
}

void setup() {
  // ----- Настройки последовательного порта -----
  Serial.begin(9600);
  Serial.println("Start");

  pinMode(button, INPUT_PULLUP);

  // ----- Запуск веба -----
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Главная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Обработка запроса значений
  server.on("/values", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"RA\":" + String(RvalueWeb) + ",";
    json += "\"LA\":" + String(LvalueWeb) + ",";
    json += "\"UA\":" + String(UvalueWeb) + ",";
    json += "\"DA\":" + String(DvalueWeb) + ",";
    json += "\"DX\":" + String(DX) + ",";
    json += "\"DY\":" + String(DY) + ",";
    json += "\"maxSpeedX\":" + String(maxSpeedX) + ",";
    json += "\"accelerationX\":" + String(accelerationX) + ",";
    json += "\"maxSpeedY\":" + String(maxSpeedY) + ",";
    json += "\"accelerationY\":" + String(accelerationY) + ",";
    json += "\"DeadBandX\":" + String(DeadBandX) + ",";
    json += "\"DeadBandY\":" + String(DeadBandY) + ",";
    json += "\"btwnMeasure\":" + String(btwnMeasure) + ",";
    json += "\"AllowMoving\":" + String(AllowMoving);
    json += "}";
    request->send(200, "application/json", json);
  });

  // Обработка обновления переменных
  server.on("/updateMaxSpeedX", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      maxSpeedX = request->getParam("value")->value().toFloat();
      Serial.print("Updated maxSpeedX to: ");
      Serial.println(maxSpeedX);
      stepperX.setMaxSpeed(maxSpeedX);
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/updateAccelerationX", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      accelerationX = request->getParam("value")->value().toFloat();
      Serial.print("Updated accelerationX to: ");
      Serial.println(accelerationX);
      stepperX.setAcceleration(accelerationX);
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/updateMaxSpeedY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      maxSpeedY = request->getParam("value")->value().toFloat();
      Serial.print("Updated maxSpeedY to: ");
      Serial.println(maxSpeedY);
      stepperY.setMaxSpeed(maxSpeedY);
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/updateAccelerationY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      accelerationY = request->getParam("value")->value().toFloat();
      Serial.print("Updated accelerationY to: ");
      Serial.println(accelerationY);
      stepperY.setAcceleration(accelerationY);
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/updateDeadBandX", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      DeadBandX = request->getParam("value")->value().toInt();
      Serial.print("Updated DeadBandX to: ");
      Serial.println(DeadBandX);
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/updateDeadBandY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      DeadBandY = request->getParam("value")->value().toInt();
      Serial.print("Updated DeadBandY to: ");
      Serial.println(DeadBandY);
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/updateBtwnMeasure", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      btwnMeasure = request->getParam("value")->value().toInt();
      Serial.print("Updated btwnMeasure to: ");
      Serial.println(btwnMeasure);
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/updateAllowMoving", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      AllowMoving = request->getParam("value")->value().toInt() == 1;
      Serial.print("Updated AllowMoving to: ");
      Serial.println(AllowMoving ? "Enabled" : "Disabled");
    }
    request->send(200, "text/plain", "OK");
  });

  server.begin();
  Serial.println("Server started");

  // ----- Инициализация шаговых -----
  stepperX.setMaxSpeed(maxSpeedX);
  stepperX.setAcceleration(accelerationX);
  stepperX.setCurrentPosition(0);
  stepperX.moveTo(stepperX.currentPosition());

  stepperY.setMaxSpeed(maxSpeedY);
  stepperY.setAcceleration(accelerationY);
  stepperY.setCurrentPosition(0);
  stepperY.moveTo(stepperY.currentPosition());

  // Если нужна первоначальная калибровка
  // FirstYSetup();

  // ----- Создаём задачу на core 0 -----
  xTaskCreatePinnedToCore(
    everythingElseTask,  // Функция задачи
    "Core0Task",         // Имя задачи (для отладки)
    4096,                // Размер стека
    NULL,                // Параметр задачи
    1,                   // Приоритет
    NULL,                // Task Handle (не нужно, если не надо управлять)
    0                    // <- номер ядра (core 0)
  );
}

// -------------------------------------------------------------------------------------
// loop() по умолчанию будет работать на core 1.
// Здесь оставляем только движение шаговых моторов.
// -------------------------------------------------------------------------------------
void loop() {
  // Логика оси X
  if (AllowMoving) {
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
    // Если движению нельзя продолжаться, останавливаем шаговик X
    stepperX.moveTo(stepperX.currentPosition());
  }
  stepperX.run();

  // Логика оси Y + учёт кнопки
  if (digitalRead(button) && AllowMoving) {
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
  } else {
    // Или кнопка нажата, или движение запрещено
    stepperY.moveTo(stepperY.currentPosition());
  }
  stepperY.run();
}