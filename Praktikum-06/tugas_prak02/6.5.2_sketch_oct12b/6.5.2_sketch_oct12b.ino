#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Kredensial Jaringan Wi-Fi Hotspot
const char* ssid = "Ilhamzzz";
const char* password = "abcdefghij";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* clientId = "ESP8266_LabIoT_25782039_Sub";

// Topik Subskripsi Perintah
const char* topicSubCommand = "kampus/labiot/25782039/perintah";

// Konfigurasi Pin Indikator LED
const byte ledPin = 5; // Pin D1 (GPIO 5)

WiFiClient espClient;
PubSubClient client(espClient);

void connectToWiFi() {
  Serial.println("\n--- Memulai Inisialisasi Wi-Fi ---");
  
  // 1. Matikan mode hemat daya Wi-Fi agar radio stabil
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(500);
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // Cegah modem Wi-Fi tertidur
  WiFi.setOutputPower(20.5);          // Set power transmisi maksimal
  
  Serial.print("Menghubungkan ke SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  // Batasi percobaan selama 15 detik, jika gagal reset koneksi otomatis
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[GAGAL] Waktu koneksi habis (Timeout).");
    Serial.print("Status Kode WiFi: ");
    Serial.println(WiFi.status());
    Serial.println("Memulai ulang modul Wi-Fi...");
    return;
  }

  Serial.println("\nWi-Fi Berhasil Terkoneksi!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

// Callback otomatis: Berjalan saat mendeteksi paket masuk
void onMessageReceived(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n========================================");
  Serial.print("Pesan Baru Masuk pada Topik: ");
  Serial.println(topic);

  char payloadStr[length + 1];
  memcpy(payloadStr, payload, length);
  payloadStr[length] = '\0';

  Serial.print("Payload Diterima: ");
  Serial.println(payloadStr);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payloadStr);

  if (error) {
    Serial.print("Gagal parsing JSON: ");
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("led")) {
    int stateLED = doc["led"];
    digitalWrite(ledPin, stateLED == 1 ? HIGH : LOW);
    Serial.printf("--> Aksi: Status LED diubah menjadi %s\n", stateLED == 1 ? "MENYALA (ON)" : "PADAM (OFF)");
  }
  Serial.println("========================================");
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (WiFi.status() != WL_CONNECTED) {
      return;
    }
    Serial.print("Mencoba koneksi MQTT...");
    if (client.connect(clientId)) {
      Serial.println(" Terhubung!");
      client.subscribe(topicSubCommand);
      Serial.print("Subskripsi Topik: ");
      Serial.println(topicSubCommand);
    } else {
      Serial.printf("Gagal (rc=%d). Ulangi dalam 5 detik...\n", client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // LED padam saat mulai

  connectToWiFi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(onMessageReceived);
}

void loop() {
  // Jika Wi-Fi putus di tengah jalan, sambungkan kembali
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
    return;
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}