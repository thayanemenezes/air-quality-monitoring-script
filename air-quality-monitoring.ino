#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <MQ135.h>
#include <time.h>

#define DHT_PIN 32
#define MQ135_PIN 33
#define DHT_TYPE DHT11

const char* wifiSSID = "Menezes";
const char* wifiPassword = "Saturn@19#01";
const String apiServer = "http://192.168.15.9:3008/api"; 
const String sensorIdentifier = "esp32-001";

DHT dht(DHT_PIN, DHT_TYPE);
MQ135 gasSensor(MQ135_PIN);

float R0;

const char* ntpServer = "pool.ntp.org";
const long gmtOffsetSec = 0;
const int daylightOffsetSec = 0;

float getNH3() {
  float rSensor = gasSensor.getPPM(); 
  float ratio = rSensor / R0;        
  float nh3 = 100.0 / ratio;         
  return nh3;
}

float getNOx() {
  float rSensor = gasSensor.getPPM(); 
  float ratio = rSensor / R0;     
  float nox = 80.0 / ratio;         
  return nox;
}

int calculateAQI(int co2, float nh3, float nox) {
  int aqi = (co2 * 0.5) + (nh3 * 1.0) + (nox * 1.5);
  return aqi;
}

float calibrateSensor() {
  float rsAir = 0.0;
  for (int i = 0; i < 50; i++) {
    rsAir += gasSensor.getResistance(); 
    delay(100);
  }
  rsAir /= 50.0;
  
  float r0 = rsAir / 9.83; 
  return r0;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(wifiSSID, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi Connected! IP: " + WiFi.localIP().toString());

  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
  dht.begin();

  Serial.println("Calibrando sensor MQ135...");
  R0 = calibrateSensor(); 
  Serial.print("R0 calibrado: ");
  Serial.println(R0);

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

    float nh3 = getNH3(); 
    float nox = getNOx(); 
    int aqi = calculateAQI(co2, nh3, nox); 

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("DHT Reading Failed!");
      return;
    }

    bool smokeDetected = detectSmoke(co2);
    if (smokeDetected) {
      Serial.println("⚠️ Smoke detected!");
    } else {
      Serial.println("No smoke detected.");
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

String generateAlert(int co2, float nh3, float nox) {
  String status = "clear";
  String message = "Low levels, nothing to worry about here...";

  if ((co2 > 800 || nh3 > 50 || nox > 40) && (co2 <= 1200 && nh3 <= 100 && nox <= 80)) {
    status = "warning";
    message = "Medium gas levels alert.";
  } else if (co2 > 1200 || nh3 > 100 || nox > 80) {
    status = "critical";
    message = "Dangerous gas levels!";
  }

  return "{\"status\":\"" + status + "\",\"messages\":[\"" + message + "\"]}";
}

String createJsonPayload(float temperature, float humidity, int co2, float nh3, float nox, int aqi, String timestamp, String alertJson, bool smokeDetected) {
  temperature = isnan(temperature) ? 0 : temperature;
  humidity = isnan(humidity) ? 0 : humidity;
  nh3 = isnan(nh3) ? 0 : nh3;
  nox = isnan(nox) ? 0 : nox;

  String payload = "{";
  payload += "\"sensor_id\":\"" + sensorIdentifier + "\",";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"data\":{";
  payload += "\"temperature\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"gases\":{";
  payload += "\"co2\":" + String(co2) + ",";
  payload += "\"nh3\":" + String(nh3, 2) + ",";
  payload += "\"nox\":" + String(nox, 2) + "},";
  payload += "\"aqi\":" + String(aqi) + ",";
  payload += "\"smoke_detected\":" + String(smokeDetected ? "true" : "false");
  payload += "},";
  payload += "\"alerts\":" + alertJson;
  payload += "}";

  return payload;
}


bool detectSmoke(int co2) {
  int smokeThreshold = 1000;
  return co2 > smokeThreshold;
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
    Serial.println("Falha ao enviar POST: " + String(http.errorToString(httpCode)));
  }

  http.end();
}
