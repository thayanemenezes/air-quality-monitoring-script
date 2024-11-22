#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <MQ135.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define DHTPIN 32
#define MQ135PIN 33
#define DHTTYPE DHT11

const char* ssid = "Menezes";
const char* password = "Saturn@19#01";
const char* serverUrl = "http://192.168.15.8:3000/sensor-data";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

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
  timeClient.begin(); 
  dht.begin();
  delay(2000);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int co2 = gasSensor.getPPM();
  int nh3 = random(5, 15);
  int nox = random(10, 25);
  int aqi = random(50, 100);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Falha na leitura do DHT!");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String payload = String("{\"sensor_id\": \"esp32-001\", \"timestamp\": \"") +
                     getTimestamp() +
                     String("\", \"data\": {\"temperature\": ") + String(temperature) +
                     String(", \"humidity\": ") + String(humidity) +
                     String(", \"gases\": {\"co2\": ") + String(co2) +
                     String(", \"nh3\": ") + String(nh3) +
                     String(", \"nox\": ") + String(nox) +
                     String("}, \"aqi\": ") + String(aqi) +
                     String("}, \"alerts\": {\"status\": \"ok\", \"messages\": []}}");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("Dados enviados! Código de resposta: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Erro ao enviar os dados. Código de erro: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi desconectado! Dados não enviados.");
  }

  delay(1800000);
}

String getTimestamp() {
  timeClient.update();
  String formattedDate = timeClient.getFormattedDate(); 
  return formattedDate;
}
