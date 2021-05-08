// WiFi библиотека
#include <ESP8266WiFi.h> 

//=======================================================================================
// Библиотека WEB-сервера
#include <ESP8266WebServer.h>
// Инициализация библиотеки WEB-сервера
ESP8266WebServer httpServer(80);
// Определения необходимое для работы ficon
#define ficonHttpServer httpServer
//=======================================================================================

//=======================================================================================
// Если есть необходимость в своём скетче использовать
// библиотеку для работы с SPIFFS
// 1. Можно использовать объект ficonFS и флаг ficonFSOK,
// который содержит результат инициализации;
// 2. Проинициализировать библиотеку в своей программе,
// и раскомментировать следующие строки:
// Библиотека для работы с файловой системой SPIFFS
//#include "FS.h"
// Флаг инициализации файловой системы
//static bool fsOK;
//#define ficonFS SPIFFS
//#define ficonFSOK fsOK

// Инициализация в setup должна выглядеть следующим образом:
//fsOK = SPIFFS.begin();
//=======================================================================================

// Инициализация переменных
// Параметры WiFi сети
String ssid = "beltel"; //для отладки
String password = "dima351845"; //для отладки

// Имя и пароль точки доступа
const char* ssidAP = "ESP8266";
const char* passwordAP = "Pa$$w0rd";

// Пин встроенного светодиода
const byte pinBuiltinLed = 2;


//=======================================================================================
// Функции для работы с WiFi сетью
//=======================================================================================
// Соединение устройства с WiFi в режиме станции
// Выход:
// false - не удалось соединиться в течении 1 минуты
// true - соединение установлено
bool setupWiFiAsStation() {
  const uint32_t timeout = 60000;
  uint32_t maxtime = millis() + timeout;

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(pinBuiltinLed, LOW);
    delay(500);
    digitalWrite(pinBuiltinLed, HIGH);
    Serial.print(".");
    if (millis() >= maxtime) {
      Serial.println(" fail!");

      return false;
    }
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

// Перевод устройства в реежим точки доступа
void setupWiFiAsAP() {
  Serial.print("Configuring access point ");
  Serial.println(ssidAP);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP, passwordAP);

  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

// Установка соединения, если в переменной ssid отсутствует наименование сети
// или не удалось соединиться с WiFi с установленными параметрами,
// то переводим устройство в режим точки доступа
void setupWiFi() {
  if ((! ssid.length()) || (! setupWiFiAsStation()))
    setupWiFiAsAP();

  // Запуск WEB-сервера
  httpServer.begin();
  Serial.println("HTTP web server started");
}

// Функция переключения светодиода по http-запросу
void handleLedSwitch() {
  String on = httpServer.arg("on");

  Serial.print(F("/switch("));
  Serial.print(on);
  Serial.println(F(")"));

  if(on == "true") digitalWrite(pinBuiltinLed, 0);
  else digitalWrite(pinBuiltinLed, 1);

  String message = "OK";
  httpServer.send(200, F("text/html"), message);
}

// Функция формирования json данных, запрашиваемых страницей index.hnm
void handleData() {
  String message = "{";
  message += "\"freeHeap\":" + String(ESP.getFreeHeap());
  message += ",";
  message += "\"wifiMode\":";
  switch (WiFi.getMode()) {
    case WIFI_OFF:
      message += "\"OFF\"";
      break;
    case WIFI_STA:
      message += "\"Station\"";
      break;
    case WIFI_AP:
      message += "\"Access Point\"";
      break;
    case WIFI_AP_STA:
      message += "\"Hybrid_(AP+STA)\"";
      break;
    default:
      message += "\"Unknown!\"";
  }
  message += ",";
  message += "\"led\":" + String(digitalRead(pinBuiltinLed));
  message += ",";
  message += "\"uptime\":" + String(millis() / 1000);
  message += "}";
  httpServer.send(200, "text/json", message);
}



//=======================================================================================
void setup() {
  // Инициализация serial-монитора
  Serial.begin(115200);
  Serial.println();
  pinMode(pinBuiltinLed, OUTPUT);

  setupWiFi();

  ficonSetup();

  // Обработка http запросов
  // Получение json-данных для index.htm
  httpServer.on("/data", HTTP_GET, handleData);
  // Переключение светодиода по http-запросу
  httpServer.on("/switch", HTTP_GET, handleLedSwitch);
}

//=======================================================================================
void loop() {
  // Функция необходимая для работы WEB-сервера
  httpServer.handleClient();
  delay(1);


}
