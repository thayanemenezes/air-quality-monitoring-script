#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <MQ135.h>
#include <time.h>

#define DHT_PIN 32
#define MQ135_PIN 33
#define DHT_TYPE DHT11

const char* ssid = "REDE";
const char* password = "SENHA";
const String serverName = "API-SERVER";
const String sensorId = "esp32-001";

DHT dht(DHT_PIN, DHT_TYPE);
MQ135 gasSensor(MQ135_PIN);

const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSec = 0;
const int daylightOffsetSec = 0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi Connected!");
  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
  dht.begin();
  delay(2000);

  Serial.println("Syncing with NTP Server...");
  while (!time(nullptr)) {
    delay(1000);
    Serial.println(".");
  }
  Serial.println("Time Synced!");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int co2 = gasSensor.getPPM();

    int nh3 = random(10, 50); 
    int nox = random(5, 30); 
    int aqi = random(50, 150); 

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("DHT Reading Failed!");
      return;
    }

    bool smokeDetected = detectSmoke(co2);
    if (smokeDetected) {
      Serial.println("⚠️ Cigarette smoke detected!");
    } else {
      Serial.println("No cigarette smoke detected.");
    }

    String timestamp = getTimestamp();
    String alertJson = generateAlert(co2, nh3, nox);
    String jsonPayload = createJsonPayload(temperature, humidity, co2, nh3, nox, aqi, timestamp, alertJson, smokeDetected);

    sendPostRequest(jsonPayload);
  } else {
    Serial.println("WiFi Disconnected!");
  }
  delay(300000); 
}

String getTimestamp() {
  time_t now = time(nullptr);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
  return String(buffer);
}

String generateAlert(int co2, int nh3, int nox) {
  String status = "clear";
  String message = "Low levels, nothing to worry about here...";

  if ((co2 > 800 || nh3 > 50 || nox > 40) && (co2 <= 1200 && nh3 <= 100 && nox <= 80)) {
    status = "medium";
    message = "Medium gas levels alert.";
  } else if (co2 > 1200 || nh3 > 100 || nox > 80) {
    status = "danger";
    message = "Dangerous gas levels!";
  }

  return "{\"status\":\"" + status + "\",\"messages\":[\"" + message + "\"]}";
}

String createJsonPayload(float temperature, float humidity, int co2, int nh3, int nox, int aqi, String timestamp, String alertJson, bool smokeDetected) {
  String payload = "{";
  payload += "\"sensor_id\":\"" + sensorIdentifier + "\",";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"data\":{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"humidity\":" + String(humidity) + ",";
  payload += "\"gases\":{";
  payload += "\"co2\":" + String(co2) + ",";
  payload += "\"nh3\":" + String(nh3) + ",";
  payload += "\"nox\":" + String(nox) + "},";
  payload += "\"aqi\":" + String(aqi) + ",";
  payload += "\"smoke_detected\":" + String(smokeDetected ? "true" : "false");
  payload += "},";
  payload += "\"alerts\":" + alertJson;
  payload += "}";

  return payload;
}

bool checkSensorExists(String id) {
  HTTPClient http;
  String url = serverName + "?sensor_id=" + id; 
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    http.end();
    return true;
  } else {
    http.end();
    return false;
  }
}

void sendPostRequest(String jsonPayload) {
  HTTPClient http;
  http.begin(apiServer);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(20000); 

  int httpCode = http.POST(jsonPayload);
  Serial.println("POST Response Code: " + String(httpCode));
  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.println("Falha ao enviar POST: " + http.errorToString(httpCode));
  }

  http.end();
}

