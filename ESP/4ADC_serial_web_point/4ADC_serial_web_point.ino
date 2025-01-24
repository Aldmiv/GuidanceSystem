#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Настройка Wi-Fi точки доступа
const char *ssid = "ESP32";
const char *password = "12345678";

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80); // Асинхронный веб-сервер

// Входы АЦП
const int potPin1 = 34;
const int potPin2 = 35;
const int potPin3 = 36;
const int potPin4 = 39;

// Значения потенциометров
int val1 = 0, val2 = 0, val3 = 0, val4 = 0;

// HTML веб-страницы
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Guidance System</title>
  <script>
    async function refreshValues() {
      const response = await fetch('/values');
      const data = await response.json();
      document.getElementById('p1').innerText = data.p1;
      document.getElementById('p2').innerText = data.p2;
      document.getElementById('p3').innerText = data.p3;
      document.getElementById('p4').innerText = data.p4;
    }
    setInterval(refreshValues, 500); // Обновление каждые 500 мс
  </script>
</head>
<body>
  <h1>ESP32 Potentiometer Values</h1>
  <p><strong>Potentiometer 1:</strong> <span id="p1">0</span></p>
  <p><strong>Potentiometer 2:</strong> <span id="p2">0</span></p>
  <p><strong>Potentiometer 3:</strong> <span id="p3">0</span></p>
  <p><strong>Potentiometer 4:</strong> <span id="p4">0</span></p>
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

  // Обработка запроса значений потенциометров
  server.on("/values", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"p1\":" + String(val1) + ",";
    json += "\"p2\":" + String(val2) + ",";
    json += "\"p3\":" + String(val3) + ",";
    json += "\"p4\":" + String(val4);
    json += "}";
    request->send(200, "application/json", json);
  });

  // Запуск веб-сервера
  server.begin();
  Serial.println("Server started");
}

void loop() {
  // Считываем значения с потенциометров
  val1 = analogRead(potPin1);
  val2 = analogRead(potPin2);
  val3 = analogRead(potPin3);
  val4 = analogRead(potPin4);

  // Выводим результаты в Serial Monitor
  Serial.print("P1: ");
  Serial.print(val1);
  Serial.print("\tP2: ");
  Serial.print(val2);
  Serial.print("\tP3: ");
  Serial.print(val3);
  Serial.print("\tP4: ");
  Serial.println(val4);

 // delay(200); // Небольшая задержка
}
