#include <ESP8266WiFi.h>
#include <espnow.h>

// Array 2D untuk menyimpan 5 MAC Address tujuan
uint8_t targetMacAddresses[5][6] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Target 1 (Broadcast)
  {0x34, 0x94, 0x54, 0x9D, 0xBB, 0xA9}, // Target 2
  {0x8C, 0xAA, 0xB5, 0x62, 0xC3, 0xAC}, // Target 3
  {0xE8, 0xDB, 0x84, 0x89, 0x3B, 0x04}, // Target 4
  {0xC8, 0xC9, 0xA3, 0x11, 0x24, 0xC6}  // Target 5
};

// Identitas unik node
#define BOARD_ID 1

typedef struct struct_sensor {
  int id;
  float pembacaan1;
  float pembacaan2;
} struct_sensor;

struct_sensor dataKirim;

unsigned long prevTime = 0;
const unsigned long sendInterval = 3000; // Kirim data setiap 3 detik

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Pengiriman Node #");
  Serial.print(BOARD_ID);
  Serial.print(" ke MAC ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println(sendStatus == 0 ? " -> Sukses" : " -> Gagal");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("Inisialisasi ESP-NOW gagal");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  // Daftarkan ke-5 peer (penerima)
  for (int i = 0; i < 5; i++) {
    esp_now_add_peer(targetMacAddresses[i], ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  }
}

void loop() {
  unsigned long now = millis();
  if (now - prevTime >= sendInterval) {
    prevTime = now;

    dataKirim.id = BOARD_ID;
    dataKirim.pembacaan1 = random(250, 350) / 10.0; 
    dataKirim.pembacaan2 = random(400, 800) / 10.0; 

    // Looping untuk mengirim data ke semua 5 target
    for (int i = 0; i < 5; i++) {
      esp_now_send(targetMacAddresses[i], (uint8_t *) &dataKirim, sizeof(dataKirim));
      delay(10); // Jeda 10ms agar buffer/antrean ESP-NOW tidak penuh
    }
  }
}