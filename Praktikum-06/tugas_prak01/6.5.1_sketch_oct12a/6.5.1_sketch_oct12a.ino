#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Kredensial Jaringan Wi-Fi Hotspot
const char* ssid = "Ilhamzzz";
const char* password = "abcdefghij";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

// IDENTITAS KLIEN
const char* clientId = "ESP8266_LabIoT_25782039";

// Definisi Topik Publikasi Telemetri
const char* topicPubTelemetry = "kampus/labiot/25782039/telemetri";

// Konfigurasi Pin Sensor DHT
const byte dhtPin = 2;   // Pin D4 (GPIO 2)
#define DHTTYPE DHT11

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long previousPublishTime = 0;
const unsigned long publishInterval = 5000;

void connectToWiFi() {
  Serial.println("\n--- Memulai Diagnostik Wi-Fi ---");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // 1. Pindai jaringan di sekitar
  Serial.println("Memindai jaringan...");
  int n = WiFi.scanNetworks();
  bool ditemukan = false;
  
  if (n == 0) {
    Serial.println("Tidak ada jaringan terdeteksi sama sekali!");
  } else {
    for (int i = 0; i < n; ++i) {
      Serial.print("Ditemukan: ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (RSSI: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm)");
      if (WiFi.SSID(i) == ssid) {
        ditemukan = true;
      }
    }
  }

  if (!ditemukan) {
    Serial.println("\n[ERROR]: Nama Wi-Fi tidak ditemukan dalam daftar pindai!");
    Serial.println("Kemungkinan: Frekuensi hotspot bukan 2.4 GHz atau hotspot tersembunyi (hidden).");
    return;
  }

  // 2. Hubungkan jika SSID tertangkap
  Serial.print("\nSSID ditemukan! Mencoba koneksi ke: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 30) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Terkoneksi!");
    Serial.print("Alamat IP Lokal: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("\nGagal terhubung! Kode status WL: ");
    Serial.println(WiFi.status());
    // Keterangan kode status:
    // 1 = WL_NO_SSID_AVAIL
    // 4 = WL_CONNECT_FAILED (Masalah kata sandi / enkripsi keamanan)
    // 6 = WL_DISCONNECTED
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    if (WiFi.status() != WL_CONNECTED) {
      return; // Tunggu koneksi Wi-Fi siap
    }
    Serial.print("Menghubungkan ke MQTT Broker...");
    if (client.connect(clientId)) {
      Serial.println(" Sukses Terhubung!");
    } else {
      Serial.print(" Gagal, kode status rc=");
      Serial.print(client.state());
      Serial.println(". Mencoba kembali dalam 5 detik...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  connectToWiFi();

  client.setServer(mqttServer, mqttPort);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
    return;
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousPublishTime >= publishInterval) {
    previousPublishTime = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("Peringatan: Gagal membaca data DHT!");
      return;
    }

    StaticJsonDocument<256> doc;
    doc["node_id"] = clientId;
    doc["suhu"] = serialized(String(t, 2));
    doc["kelembapan"] = serialized(String(h, 2));

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    bool isSuccess = client.publish(topicPubTelemetry, jsonBuffer);
    if (isSuccess) {
      Serial.print("Telemetri Terkirim [");
      Serial.print(topicPubTelemetry);
      Serial.print("]: ");
      Serial.println(jsonBuffer);
    } else {
      Serial.println("Peringatan: Gagal memublikasikan paket!");
    }
  }
}