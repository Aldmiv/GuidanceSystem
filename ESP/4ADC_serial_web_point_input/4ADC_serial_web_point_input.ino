#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Настройка Wi-Fi точки доступа
const char *ssid = "ESP32";
const char *password = "12345678";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80); // Асинхронный веб-сервер

// Входы АЦП для антенн
const int RA = 36; // Правая антенна
const int LA = 39; // Левая антенна
const int UA = 34; // Верхняя антенна
const int DA = 35; // Нижняя антенна

// Значения сигналов с антенн
volatile int Rvalue = 0, Lvalue = 0, Uvalue = 0, Dvalue = 0; // Непрерывные данные
int RvalueWeb = 0, LvalueWeb = 0, UvalueWeb = 0, DvalueWeb = 0; // Данные для веб-сервера

float maxSpeedX = 3000.0;
float accelerationX = 2900.0;

float maxSpeedY = 2500.0;
float accelerationY = 2000.0;

int DeadBandX = 20;
int DeadBandY = 20;
int btwnMeasure = 60; // Периодичность считывания (мс)

// Таймер для обновления значений
unsigned long previousMillis = 0;
const unsigned long interval = 50; // Интервал вывода значений в веб

// HTML веб-страницы
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>GuidanceSystem</title>
  <script>
    async function refreshValues() {
      const response = await fetch('/values');
      const data = await response.json();
      document.getElementById('RA').innerText = data.RA;
      document.getElementById('LA').innerText = data.LA;
      document.getElementById('UA').innerText = data.UA;
      document.getElementById('DA').innerText = data.DA;
      document.getElementById('maxSpeedX').innerText = data.maxSpeedX;
      document.getElementById('accelerationX').innerText = data.accelerationX;
      document.getElementById('maxSpeedY').innerText = data.maxSpeedY;
      document.getElementById('accelerationY').innerText = data.accelerationY;
      document.getElementById('DeadBandX').innerText = data.DeadBandX;
      document.getElementById('DeadBandY').innerText = data.DeadBandY;
      document.getElementById('btwnMeasure').innerText = data.btwnMeasure;
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

    setInterval(refreshValues, 50); // Обновление каждые 50 мс
  </script>
</head>
<body>
  <h1>Guidance System</h1>

  <h2>Antenna Signals</h2>
  <p><strong>Right Antenna:</strong> <span id="RA">0</span></p>
  <p><strong>Left Antenna:</strong> <span id="LA">0</span></p>
  <p><strong>Upper Antenna:</strong> <span id="UA">0</span></p>
  <p><strong>Down Antenna:</strong> <span id="DA">0</span></p>

  <p><strong>Max Speed X:</strong> <span id="maxSpeedX">0.0</span></p>
  <p>Set Max Speed X: <input type="text" id="maxSpeedXInput" placeholder="Enter value"> <button onclick="updateMaxSpeedX()">Update</button></p>

  <p><strong>Acceleration X:</strong> <span id="accelerationX">0.0</span></p>
  <p>Set Acceleration X: <input type="text" id="accelerationXInput" placeholder="Enter value"> <button onclick="updateAccelerationX()">Update</button></p>

  <p><strong>Max Speed Y:</strong> <span id="maxSpeedY">0.0</span></p>
  <p>Set Max Speed Y: <input type="text" id="maxSpeedYInput" placeholder="Enter value"> <button onclick="updateMaxSpeedY()">Update</button></p>

  <p><strong>Acceleration Y:</strong> <span id="accelerationY">0.0</span></p>
  <p>Set Acceleration Y: <input type="text" id="accelerationYInput" placeholder="Enter value"> <button onclick="updateAccelerationY()">Update</button></p>

  <p><strong>Dead Band X:</strong> <span id="DeadBandX">0</span></p>
  <p>Set Dead Band X: <input type="text" id="DeadBandXInput" placeholder="Enter value"> <button onclick="updateDeadBandX()">Update</button></p>

  <p><strong>Dead Band Y:</strong> <span id="DeadBandY">0</span></p>
  <p>Set Dead Band Y: <input type="text" id="DeadBandYInput" placeholder="Enter value"> <button onclick="updateDeadBandY()">Update</button></p>

  <p><strong>Between Measure:</strong> <span id="btwnMeasure">0</span></p>
  <p>Set Between Measure: <input type="text" id="btwnMeasureInput" placeholder="Enter value"> <button onclick="updateBtwnMeasure()">Update</button></p>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(9600);

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
    json += "\"maxSpeedX\":" + String(maxSpeedX) + ",";
    json += "\"accelerationX\":" + String(accelerationX) + ",";
    json += "\"maxSpeedY\":" + String(maxSpeedY) + ",";
    json += "\"accelerationY\":" + String(accelerationY) + ",";
    json += "\"DeadBandX\":" + String(DeadBandX) + ",";
    json += "\"DeadBandY\":" + String(DeadBandY) + ",";
    json += "\"btwnMeasure\":" + String(btwnMeasure);
    json += "}";
    request->send(200, "application/json", json);
  });

  // Обработка обновления maxSpeedX
  server.on("/updateMaxSpeedX", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      maxSpeedX = request->getParam("value")->value().toFloat();
      Serial.print("Updated maxSpeedX to: ");
      Serial.println(maxSpeedX);
    }
    request->send(200, "text/plain", "OK");
  });

  // Обработка обновления accelerationX
  server.on("/updateAccelerationX", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      accelerationX = request->getParam("value")->value().toFloat();
      Serial.print("Updated accelerationX to: ");
      Serial.println(accelerationX);
    }
    request->send(200, "text/plain", "OK");
  });

  // Обработка обновления maxSpeedY
  server.on("/updateMaxSpeedY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      maxSpeedY = request->getParam("value")->value().toFloat();
      Serial.print("Updated maxSpeedY to: ");
      Serial.println(maxSpeedY);
    }
    request->send(200, "text/plain", "OK");
  });

  // Обработка обновления accelerationY
  server.on("/updateAccelerationY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      accelerationY = request->getParam("value")->value().toFloat();
      Serial.print("Updated accelerationY to: ");
      Serial.println(accelerationY);
    }
    request->send(200, "text/plain", "OK");
  });

  // Обработка обновления DeadBandX
  server.on("/updateDeadBandX", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      DeadBandX = request->getParam("value")->value().toInt();
      Serial.print("Updated DeadBandX to: ");
      Serial.println(DeadBandX);
    }
    request->send(200, "text/plain", "OK");
  });

  // Обработка обновления DeadBandY
  server.on("/updateDeadBandY", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      DeadBandY = request->getParam("value")->value().toInt();
      Serial.print("Updated DeadBandY to: ");
      Serial.println(DeadBandY);
    }
    request->send(200, "text/plain", "OK");
  });

  // Обработка обновления btwnMeasure
  server.on("/updateBtwnMeasure", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      btwnMeasure = request->getParam("value")->value().toInt();
      Serial.print("Updated btwnMeasure to: ");
      Serial.println(btwnMeasure);
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

  // Обновление значений для вывода каждые 50 мс
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
