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

// Таймер для обновления значений
unsigned long previousMillis = 0;
const unsigned long interval = 50; // Интервал вывода в миллисекундах

// HTML веб-страницы
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Antenna Signals</title>
  <script>
    async function refreshValues() {
      const response = await fetch('/values');
      const data = await response.json();
      document.getElementById('RA').innerText = data.RA;
      document.getElementById('LA').innerText = data.LA;
      document.getElementById('UA').innerText = data.UA;
      document.getElementById('DA').innerText = data.DA;
    }
    setInterval(refreshValues, interval); // Обновление каждые 50 мс
  </script>
</head>
<body>
  <h1>ESP32 Antenna Signals</h1>
  <p><strong>Right Antenna:</strong> <span id="RA">0</span></p>
  <p><strong>Left Antenna:</strong> <span id="LA">0</span></p>
  <p><strong>Upper Antenna:</strong> <span id="UA">0</span></p>
  <p><strong>Down Antenna:</strong> <span id="DA">0</span></p>
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
    json += "\"DA\":" + String(DvalueWeb);
    json += "}";
    request->send(200, "application/json", json);
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
