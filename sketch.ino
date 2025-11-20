#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include "DHT.h"

// ---------------- DHT ----------------
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ---------------- Sensores / Atuadores ----------------
int ldrPin = 34;
int red = 25;
int green = 26;
int blue = 27;
int buzzer = 23;

// ---------------- Wi-Fi (Wokwi) ----------------
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ---------------- HTTP (Wokwi API) ----------------
String httpEndpoint = "https://wokwi-api.herokuapp.com/api/test";

// ---------------- MQTT ----------------
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "neurobloom/user123/telemetry";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSend = 0;
const long sendInterval = 5000; // enviar a cada 5s

// ---------------------- SETUP ----------------------
void setup() {
  Serial.begin(115200);

  dht.begin();

  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // Conectar ao Wi-Fi do Wokwi
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  // Configurar MQTT
  client.setServer(mqtt_server, mqtt_port);
}

// ---------------------- MQTT RECONNECT ----------------------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Reconectando ao MQTT...");
    String clientId = "ESP32-NeuroBloom-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println(" conectado!");
    } else {
      Serial.print(" falhou (rc=");
      Serial.print(client.state());
      Serial.println("). Novo teste em 2s");
      delay(2000);
    }
  }
}

// ---------------------- HTTP SEND ----------------------
void sendHTTP(int luz, float t, float h) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(httpEndpoint);
    http.addHeader("Content-Type", "application/json");

    String json = "{";
    json += "\"luz\":" + String(luz) + ",";
    json += "\"temp\":" + String(t) + ",";
    json += "\"hum\":" + String(h);
    json += "}";

    Serial.println("\n[HTTP] Enviando POST:");
    Serial.println(json);

    int code = http.POST(json);

    Serial.print("Código HTTP: ");
    Serial.println(code);

    if (code > 0) {
      Serial.print("Resposta: ");
      Serial.println(http.getString());
    }

    http.end();
  }
}

// ---------------------- MQTT SEND ----------------------
void sendMQTT(int luz, float t, float h) {
  String payload = "{";
  payload += "\"luz\":" + String(luz) + ",";
  payload += "\"temp\":" + String(t) + ",";
  payload += "\"hum\":" + String(h);
  payload += "}";

  client.publish(mqtt_topic, payload.c_str());

  Serial.println("\n[MQTT] Publicado:");
  Serial.println(payload);
}

// ---------------------- LOOP ----------------------
void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long now = millis();

  if (now - lastSend > sendInterval) {
    lastSend = now;

    int luz = analogRead(ldrPin);
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    // Enviar HTTP
    sendHTTP(luz, t, h);

    // Enviar MQTT
    sendMQTT(luz, t, h);
  }

  // ---------------- Lógica do LED / Buzzer ----------------
  int luz = analogRead(ldrPin);
  float t = dht.readTemperature();

  if (luz < 800) {
    setColor(0, 0, 255); // azul = foco
  } else if (t > 28) {
    setColor(255, 0, 0); // vermelho = alerta sensorial
    tone(buzzer, 500, 300);
  } else {
    setColor(0, 255, 0); // verde = ambiente calmo
  }

  delay(300);
}

// ---------------------- RGB ----------------------
void setColor(int r, int g, int b) {
  analogWrite(red, r);
  analogWrite(green, g);
  analogWrite(blue, b);
}