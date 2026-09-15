#include "DHT.h"

// Definisi Pin Sesuai Instruksi
#define DHTPIN D7          // Sensor DHT pada pin D7
#define DHTTYPE DHT11      // Ganti DHT22 jika menggunakan DHT22
const int ldrPin   = A0;   // Sensor LDR pada pin A0
const int relayPin = D6;   // Modul Relay pada pin D6
const int ledPin   = D1;   // LED Indikator pada pin D1

DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
const long interval = 2000; // Baca data sensor setiap 2 detik

void setup() {
  Serial.begin(115200);
  delay(300);

  dht.begin();

  // Konfigurasi pin sebagai OUTPUT
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Perintah awal untuk mematikan perangkat (keamanan sistem)
  digitalWrite(relayPin, HIGH); // Relay Active LOW: HIGH = Mati
  digitalWrite(ledPin, LOW);    // LED Mati

  Serial.println("--- SISTEM OTOMASI DIMULAI ---");
}

void loop() {
  unsigned long currentMillis = millis();

  // Eksekusi pembacaan dan evaluasi aturan berkala
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Pembacaan data sensor
    float suhu = dht.readTemperature();
    float kelembapan = dht.readHumidity();
    int nilaiLDR = analogRead(ldrPin);

    // Validasi pembacaan DHT
    if (isnan(suhu)) {
      Serial.println("[Peringatan] Gagal membaca sensor DHT di D7!");
      return;
    }

    // Tampilkan data ke Serial Monitor
    Serial.println("----------------------------------------");
    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.print(" °C | Nilai LDR: ");
    Serial.println(nilaiLDR);

    // Rule Engine: Suhu > 34 C ATAU LDR < 300
    if (suhu > 34.0 || nilaiLDR < 300) {
      // Kondisi Buruk: Aktifkan Relay dan LED
      digitalWrite(relayPin, LOW); // Relay ON (Active LOW)
      digitalWrite(ledPin, HIGH);  // LED ON
      Serial.println("STATUS: Peringatan: Aktuator Aktif! (Kondisi Buruk)");
    } else {
      // Kondisi Aman: Matikan Relay dan LED
      digitalWrite(relayPin, HIGH); // Relay OFF (Active LOW)
      digitalWrite(ledPin, LOW);    // LED OFF
      Serial.println("STATUS: Kondisi Aman");
    }
  }
}