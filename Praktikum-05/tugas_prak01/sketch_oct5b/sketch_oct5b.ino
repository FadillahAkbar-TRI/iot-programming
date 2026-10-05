#include <ESP8266WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(1000); // Penambahan delay singkat agar Serial Monitor sempat siap
  Serial.println();
  
  // Mengaktifkan mode Station agar MAC address antarmuka STA aktif
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.println("==========================================");
  Serial.print("ESP8266 MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("==========================================");
}

void loop() {
  // Tidak ada proses berulang
}