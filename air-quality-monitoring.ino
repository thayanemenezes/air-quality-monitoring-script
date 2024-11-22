#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <MQ135.h>

#define DHTPIN 32
#define MQ135PIN 33
#define DHTTYPE DHT11

const char* ssid = "REDE";
const char* password = "SENHA";
const String serverName = "API-SERVER";
const String sensorId = "esp32-001";

DHT dht(DHTPIN, DHTTYPE);
MQ135 gasSensor(MQ135PIN);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");

  dht.begin();
  delay(2000);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    int co2 = gasSensor.getPPM();
    int nh3 = random(10, 50); 
    int nox = random(5, 30); 
    int aqi = random(50, 150); 

    if (isnan(temperatura) || isnan(umidade)) {
      Serial.println("Falha na leitura do DHT!");
      return;
    }

    String timestamp = getTimestamp();
    String jsonPayload = createJsonPayload(temperatura, umidade, co2, nh3, nox, aqi, timestamp);

    if (checkSensorExists(sensorId)) {
      sendPutRequest(sensorId, jsonPayload);
    } else {
      sendPostRequest(jsonPayload);
    }
  } else {
    Serial.println("WiFi desconectado!");
  }
  delay(1800000); 
}

String getTimestamp() {
  time_t now = time(nullptr);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
  return String(buffer);
}

String createJsonPayload(float temperatura, float umidade, int co2, int nh3, int nox, int aqi, String timestamp) {
  String payload = "{";
  payload += "\"sensor_id\":\"" + sensorId + "\",";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"data\":{";
  payload += "\"temperature\":" + String(temperatura) + ",";
  payload += "\"humidity\":" + String(umidade) + ",";
  payload += "\"gases\":{";
  payload += "\"co2\":" + String(co2) + ",";
  payload += "\"nh3\":" + String(nh3) + ",";
  payload += "\"nox\":" + String(nox) + "},";
  payload += "\"aqi\":" + String(aqi) + "},";
  payload += "\"alerts\":{\"status\":\"ok\",\"messages\":[]}";
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
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST(jsonPayload);
  Serial.println("POST Response Code: " + String(httpCode));
  Serial.println("Payload: " + jsonPayload);

  http.end();
}

void sendPutRequest(String id, String jsonPayload) {
  HTTPClient http;
  String url = serverName + "/" + id;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.PUT(jsonPayload);
  Serial.println("PUT Response Code: " + String(httpCode));
  Serial.println("Payload: " + jsonPayload);

  http.end();
}
