#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Настройка Wi-Fi точки доступа
const char *ssid = "ESP32";
const char *password = "8467239547495";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);  // Асинхронный веб-сервер

// Входы АЦП для антенн
const int RA = 36;  // Правая антенна
const int LA = 39;  // Левая антенна
const int UA = 34;  // Верхняя антенна
const int DA = 35;  // Нижняя антенна
const int button = 15; // Кнопка digitalRead(button);

// Значения сигналов с антенн
volatile int Rvalue = 0, Lvalue = 0, Uvalue = 0, Dvalue = 0;     // Непрерывные данные
int RvalueWeb = 0, LvalueWeb = 0, UvalueWeb = 0, DvalueWeb = 0;  // Данные для веб-сервера

// Разница между парой антенн
int DX = 0;
int DY = 0;

float maxSpeedX = 3000.0;
float accelerationX = 2900.0;

float maxSpeedY = 2500.0;
float accelerationY = 2000.0;

int DeadBandX = 20;
int DeadBandY = 20;
int btwnMeasure = 60;  // Периодичность считывания (мс)

bool AllowMoving = true;

// Таймер для обновления значений
unsigned long previousMillis = 0;
const unsigned long interval = 100;  // Интервал обновления значений для веба

// HTML веб-страницы
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Guidance System</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, maximum-scale=1.0, minimum-scale=1.0">
  <style>
    body {
      background-color: #202020;
      color: white;
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      display: flex;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      overflow-x: hidden; /* Полностью убираем горизонтальный скролл */
    }
    .container {
      display: flex;
      flex-direction: column;
      background: #303030;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      width: auto; /* Ширина автоматически подстраивается под содержимое */
      max-width: 100%; /* Контейнер не выходит за пределы окна */
      margin: 0 auto; /* Центрируем контейнер */
    }
    .row {
      display: flex;
      justify-content: flex-start; /* Выравнивание по левому краю */
      align-items: center;
      margin-bottom: 15px;
      gap: 10px; /* Пространство между элементами */
      flex-wrap: wrap; /* Перенос элементов на новую строку при необходимости */
    }
    .row strong {
      font-size: 1rem;
      flex: 0 0 auto; /* Текст занимает только необходимое место */
      min-width: 120px; /* Минимальная ширина текста */
    }
    .row input {
      padding: 5px 10px;
      border-radius: 5px;
      border: none;
      outline: none;
      width: 150px; /* Фиксированная ширина для полей ввода */
    }
    .row button {
      padding: 5px 15px; /* Компактный размер кнопок */
      border-radius: 5px;
      border: none;
      background: #007BFF;
      color: white;
      cursor: pointer;
      flex-shrink: 0; /* Кнопки не сжимаются */
    }
    .row button:hover {
      background: #0056b3;
    }
    #AllowMovingSwitch {
      width: 20px;
      height: 20px;
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
      document.getElementById('AllowMoving').innerText = data.AllowMoving ? "Enabled" : "Disabled";

      // Обновляем положение кружочка
      const dot = document.querySelector('.dot');
      const maxOffset = 100; // Половина ширины/высоты перекрестия (визуально 100 пикселей)
      const maxValue = 2000; // Максимальные значения DX и DY

      // Рассчитываем нормализованное значение для X
      const x = Math.min(Math.max((data.DX / maxValue) * maxOffset, -maxOffset), maxOffset);
      // Рассчитываем нормализованное значение для Y (инвертируем направление)
      const y = Math.min(Math.max((-data.DY / maxValue) * maxOffset, -maxOffset), maxOffset);

      // Устанавливаем позиции точки относительно центра перекрестия
      dot.style.left = `${x + maxOffset}px`; // Центрируем точку по X
      dot.style.top = `${y + maxOffset}px`; // Центрируем точку по Y
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
      const input = document.getElementById('AllowMovingSwitch').checked;
      await fetch(`/updateAllowMoving?value=${input ? 1 : 0}`);
    }

    setInterval(refreshValues, 50); // Обновление каждые 50 мс
</script>

</head>
<body>
  <div class="container">
    <h1>Guidance System</h1>

    <div class="row">
      <strong>Right Antenna:</strong> <span id="RA">0</span>
    </div>
    <div class="row">
      <strong>Left Antenna:</strong> <span id="LA">0</span>
    </div>
    <div class="row">
      <strong>Upper Antenna:</strong> <span id="UA">0</span>
    </div>
    <div class="row">
      <strong>Down Antenna:</strong> <span id="DA">0</span>
    </div>

    <div class="row">
      <strong>DX:</strong> <span id="DX">0</span>
    </div>
    <div class="row">
      <strong>DY:</strong> <span id="DY">0</span>
    </div>


    <div class="row">
      <strong>Emergency Stop:</strong> <input type="checkbox" id="AllowMovingSwitch" onchange="updateAllowMoving()">
    </div>

    <div class="row">
      <strong>Max Speed X:</strong> <span id="maxSpeedX">0.0</span>
    </div>
    <div class="row">
      <input type="text" id="maxSpeedXInput" placeholder="Enter value"> <button onclick="updateMaxSpeedX()">Update</button>
    </div>

    <div class="row">
      <strong>Acceleration X:</strong> <span id="accelerationX">0.0</span>
    </div>
    <div class="row">
      <input type="text" id="accelerationXInput" placeholder="Enter value"> <button onclick="updateAccelerationX()">Update</button>
    </div>

    <div class="row">
      <strong>Max Speed Y:</strong> <span id="maxSpeedY">0.0</span>
    </div>
    <div class="row">
      <input type="text" id="maxSpeedYInput" placeholder="Enter value"> <button onclick="updateMaxSpeedY()">Update</button>
    </div>

    <div class="row">
      <strong>Acceleration Y:</strong> <span id="accelerationY">0.0</span>
    </div>
    <div class="row">
      <input type="text" id="accelerationYInput" placeholder="Enter value"> <button onclick="updateAccelerationY()">Update</button>
    </div>

    <div class="row">
      <strong>Dead Band X:</strong> <span id="DeadBandX">0</span>
    </div>
    <div class="row">
      <input type="text" id="DeadBandXInput" placeholder="Enter value"> <button onclick="updateDeadBandX()">Update</button>
    </div>

    <div class="row">
      <strong>Dead Band Y:</strong> <span id="DeadBandY">0</span>
    </div>
    <div class="row">
      <input type="text" id="DeadBandYInput" placeholder="Enter value"> <button onclick="updateDeadBandY()">Update</button>
    </div>

    <div class="row">
      <strong>Between Measure:</strong> <span id="btwnMeasure">0</span>
    </div>
    <div class="row">
      <input type="text" id="btwnMeasureInput" placeholder="Enter value"> <button onclick="updateBtwnMeasure()">Update</button>
    </div>
  </div>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(9600);

  pinMode(button, INPUT_PULLUP);

  // Настройка Wi-Fi точки доступа
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Главная страница
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Обработка запроса значений сигналов антенн
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
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/updateAccelerationX", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      accelerationX = request->getParam("value")->value().toFloat();
      Serial.print("Updated accelerationX to: ");
      Serial.println(accelerationX);
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/updateMaxSpeedY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      maxSpeedY = request->getParam("value")->value().toFloat();
      Serial.print("Updated maxSpeedY to: ");
      Serial.println(maxSpeedY);
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/updateAccelerationY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      accelerationY = request->getParam("value")->value().toFloat();
      Serial.print("Updated accelerationY to: ");
      Serial.println(accelerationY);
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

  // Запуск веб-сервера
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Непрерывное считывание сигналов с антенн
  Rvalue = analogRead(RA);
  Lvalue = analogRead(LA);
  Uvalue = analogRead(UA);
  Dvalue = analogRead(DA);

  DX = Rvalue - Lvalue;
  DY = Uvalue - Dvalue;


  // Обновление значений для вывода каждые interval мс
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Копируем значения в переменные для вывода
    RvalueWeb = Rvalue;
    LvalueWeb = Lvalue;
    UvalueWeb = Uvalue;
    DvalueWeb = Dvalue;

    // Выводим результаты в Serial Monitor (для отладки)
    Serial.print("RA: ");
    Serial.print(RvalueWeb);
    Serial.print("\tLA: ");
    Serial.print(LvalueWeb);
    Serial.print("\tUA: ");
    Serial.print(UvalueWeb);
    Serial.print("\tDA: ");
    Serial.println(DvalueWeb);
  }
}