#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Definisi pin NodeMCU ESP8266
#define BUTTON_PIN D3      // Push button ke D3 (GPIO 0)
#define BUZZER_PIN D5      // Buzzer ke D5 (GPIO 14)
#define LED_PIN    D6      // LED ke D6 (GPIO 12)

// Inisialisasi LCD (alamat 0x27, 16 kolom, 2 baris)
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool statusSistem = false; // Status ON/OFF
int totalTekan = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Jalur I2C NodeMCU: SDA = D2, SCL = D1
  Wire.begin(D2, D1);

  lcd.init();
  lcd.backlight();
  
  // Tampilan awal
  perbaruiLayar();
}

void loop() {
  // Tombol ditekan (logika LOW)
  if (digitalRead(BUTTON_PIN) == LOW) {
    statusSistem = !statusSistem; // Balik status (ON jadi OFF, OFF jadi ON)
    totalTekan++;

    // Kendali LED
    digitalWrite(LED_PIN, statusSistem ? HIGH : LOW);

    // Nada konfirmasi buzzer sesaat
    if (statusSistem) {
      tone(BUZZER_PIN, 1200, 150); // Nada tinggi (ON)
    } else {
      tone(BUZZER_PIN, 600, 150);  // Nada rendah (OFF)
    }

    perbaruiLayar();

    // Debounce tombol agar tidak terbaca berulang kali saat ditekan lama
    delay(300);
  }
}

void perbaruiLayar() {
  lcd.setCursor(0, 0);
  lcd.print("Status: ");
  if (statusSistem) {
    lcd.print("MENYALA ");
  } else {
    lcd.print("MATI    ");
  }

  lcd.setCursor(0, 1);
  lcd.print("Ditekan: ");
  lcd.print(totalTekan);
  lcd.print(" kali   ");
}