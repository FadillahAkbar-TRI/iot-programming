#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Konfigurasi Wi-Fi
const char* ssid = "Ilhamzzz";
const char* password = "abcdefghij";

// Konfigurasi MQTT Broker
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* clientId = "ESP8266_LabIoT_25782012_Tugas";

// Topic MQTT
const char* topicTelemetry = "kampus/labiot/25782039/telemetri";
const char* topicControl = "kampus/labiot/25782039/kendali";
const char* topicAlert = "kampus/labiot/25782039/alert";
const char* topicStatus = "kampus/labiot/25782039/status";

// Pin sensor dan LED
const byte dhtPin = 2;  // D4 / GPIO 2
const byte ledPin = 5;  // D1 / GPIO 5
#define DHTTYPE DHT22

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Timer non-blocking
unsigned long lastTelemetry = 0;
unsigned long lastReconnect = 0;
const unsigned long telemetryInterval = 4000;
const unsigned long reconnectInterval = 5000;

float lastTemp = NAN;
bool alertSent = false;
bool ledState = false;

// Callback perintah MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  char message[256];
  if (length >= sizeof(message)) return;

  memcpy(message, payload, length);
  message[length] = '\0';

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, message)) {
    Serial.println("Payload JSON tidak valid!");
    return;
  }

  if (doc.containsKey("led")) {
    int command = doc["led"];

    // Saat suhu bahaya, LED tetap dikendalikan sistem keselamatan.
    if (!isnan(lastTemp) && lastTemp > 31.0) {
      digitalWrite(ledPin, HIGH);
      ledState = true;
      Serial.println("Suhu bahaya: LED tetap menyala.");
      return;
    }

    if (command == 0 || command == 1) {
      ledState = (command == 1);
      digitalWrite(ledPin, ledState ? HIGH : LOW);
      Serial.printf("Perintah LED: %s\n",
                    ledState ? "MENYALA" : "PADAM");
    }
  }
}

// Koneksi awal Wi-Fi
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Menghubungkan Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi terhubung!");
  Serial.println(WiFi.localIP());
}

// Hubungkan MQTT dan daftarkan LWT
void reconnectMQTT() {
  if (client.connected()) return;

  unsigned long now = millis();
  if (now - lastReconnect < reconnectInterval) return;
  lastReconnect = now;

  const char* willMessage =
      "{\"status\":\"OFFLINE\","
      "\"keterangan\":\"Perangkat Terputus Tak Terduga\"}";

  Serial.print("Menghubungkan MQTT...");

  bool connected = client.connect(
      clientId,
      topicStatus,
      1,       // QoS LWT
      true,    // Retain
      willMessage
  );

  if (connected) {
    Serial.println(" terhubung!");
    client.subscribe(topicControl);

    client.publish(
        topicStatus,
        "{\"status\":\"ONLINE\","
        "\"keterangan\":\"Perangkat Beroperasi Normal\"}",
        true
    );

    Serial.println("Status ONLINE dipublikasikan.");
  } else {
    Serial.printf(" gagal, rc=%d\n", client.state());
  }
}

// Publikasi telemetri dan peringatan suhu
void publishTelemetry() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Gagal membaca sensor DHT!");
    return;
  }

  lastTemp = temperature;
  bool danger = temperature > 31.0;

  // Klasifikasi dan tindakan keselamatan
  if (danger) {
    ledState = true;
    digitalWrite(ledPin, HIGH);

    if (!alertSent) {
      StaticJsonDocument<256> alert;
      alert["status"] = "BAHAYA";
      alert["parameter"] = "OVERHEAT";
      alert["nilai_suhu"] = temperature;

      char alertBuffer[256];
      serializeJson(alert, alertBuffer);
      client.publish(topicAlert, alertBuffer);

      Serial.println("PERINGATAN BAHAYA TERKIRIM!");
      alertSent = true;
    }
  } else {
    alertSent = false;
  }

  // Membuat JSON telemetri
  StaticJsonDocument<384> doc;
  doc["node_id"] = clientId;
  doc["suhu"] = temperature;
  doc["kelembapan"] = humidity;
  doc["status_led"] = ledState ? 1 : 0;
  doc["kondisi_suhu"] = danger ? "PANAS" : "NORMAL";

  char buffer[384];
  serializeJson(doc, buffer);

  if (client.publish(topicTelemetry, buffer)) {
    Serial.print("Telemetri: ");
    Serial.println(buffer);
  } else {
    Serial.println("Gagal mengirim telemetri.");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  dht.begin();

  connectWiFi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    return;
  }

  reconnectMQTT();
  client.loop();

  unsigned long now = millis();
  if (client.connected() &&
      now - lastTelemetry >= telemetryInterval) {
    lastTelemetry = now;
    publishTelemetry();
  }
}