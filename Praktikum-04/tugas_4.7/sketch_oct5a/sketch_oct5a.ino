#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// --- KONFIGURASI WIFI ---
const char* ssid     = "Infinix GT 30 Pro";
const char* password = "murahcpz";

// --- KONFIGURASI PIN ---
const byte dhtPin        = 0;   // D3 (GPIO 0)
const byte buttonPin     = 4;   // D2 (GPIO 4)
const byte ledDigitalPin = 12;  // D6 (GPIO 12) - ON/OFF LED
const byte ledPwmPin     = 5;   // D1 (GPIO 5)  - Dimmer PWM LED

DHT dht(dhtPin, DHT11);

// --- VARIABEL GLOBAL ---
bool ledState = false;
int pwmValue = 0;
String currentTemp = "--";
String currentHum  = "--";

int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastTime = 0;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------------- FRONT-END (HTML, CSS, JS) ----------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Room - PWM Dimmer</title>
  <style>
    body { font-family: Arial; text-align: center; background-color: #f4f4f9; margin: 0; padding: 20px; }
    .card { background: white; margin: 15px auto; padding: 20px; max-width: 320px; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
    button { padding: 12px 28px; font-size: 18px; border-radius: 8px; cursor: pointer; color: white; border: none; }  
    .btn-on { background-color: #4CAF50; }  
    .btn-off { background-color: #f44336; }
    .slider { width: 100%; height: 10px; border-radius: 5px; background: #d3d3d3; outline: none; margin-top: 15px; }
    .val-text { font-size: 20px; font-weight: bold; color: #333; }
  </style>
</head>
<body>
  <h1>Smart Lighting Dashboard</h1>

  <div class="card">
    <h3>Telemetri Room</h3>
    <p>Suhu: <b><span id="tempValue">--</span> &deg;C</b></p>
    <p>Kelembapan: <b><span id="humValue">--</span> %</b></p>
  </div>

  <div class="card">
    <h3>Lampu Utama (Digital)</h3>
    <p>Status: <b><span id="ledStatus">OFF</span></b></p>
    <button id="toggleBtn" class="btn-off" onclick="toggleLed()">Turn ON</button>
  </div>

  <div class="card">
    <h3>Lampu Dimmer (PWM)</h3>
    <p>Intensitas: <span id="pwmValText" class="val-text">0</span> / 1023</p>
    <input type="range" min="0" max="1023" value="0" class="slider" id="pwmSlider" oninput="sendPWM(this.value)">
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    window.addEventListener('load', onLoad);
    function onLoad(event) { initWebSocket(); }

    function initWebSocket() {
      websocket = new WebSocket(gateway);
      websocket.onopen    = function(e) { console.log('WebSocket Terkoneksi'); };
      websocket.onclose   = function(e) { setTimeout(initWebSocket, 2000); };
      websocket.onmessage = onMessage;
    }

    function toggleLed(){  
      websocket.send('toggle');
    }

    function sendPWM(value){
      document.getElementById('pwmValText').innerHTML = value;
      websocket.send('pwm,' + value);
    }

    function onMessage(event) {
      var dataObj = JSON.parse(event.data);  
        
      if(dataObj.suhu !== undefined) {  
         document.getElementById('tempValue').innerHTML = dataObj.suhu;  
      }  
      if(dataObj.hum !== undefined) {  
         document.getElementById('humValue').innerHTML = dataObj.hum;  
      }  
      if(dataObj.led !== undefined) {  
         var btn = document.getElementById('toggleBtn');  
         var status = document.getElementById('ledStatus');  
         if(dataObj.led == "1"){  
           status.innerHTML = "ON";  
           btn.innerHTML = "Turn OFF";  
           btn.className = "btn-on";  
         } else {  
           status.innerHTML = "OFF";  
           btn.innerHTML = "Turn ON";  
           btn.className = "btn-off";  
         }  
      }
      if(dataObj.pwm !== undefined) {
         document.getElementById('pwmSlider').value = dataObj.pwm;
         document.getElementById('pwmValText').innerHTML = dataObj.pwm;
      }
    }  
  </script>  
</body>  
</html>  
)rawliteral";

// ---------------- BACK-END (C++) ----------------

void notifyClients() {
  String jsonString = "{\"led\":\"" + String(ledState ? 1 : 0) + "\", ";
  jsonString += "\"pwm\":\"" + String(pwmValue) + "\", ";
  jsonString += "\"suhu\":\"" + currentTemp + "\", ";
  jsonString += "\"hum\":\"" + currentHum + "\"}";
  ws.textAll(jsonString);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = String((char*)data);

    // Parsing Perintah Toggle Digital
    if (message == "toggle") {
      ledState = !ledState;
      digitalWrite(ledDigitalPin, ledState ? HIGH : LOW);
      notifyClients();
    } 
    // Parsing Perintah Slider PWM (Format: "pwm,VAL")
    else if (message.startsWith("pwm,")) {
      int commaIndex = message.indexOf(',');
      if (commaIndex != -1) {
        String valStr = message.substring(commaIndex + 1);
        pwmValue = valStr.toInt();
        analogWrite(ledPwmPin, pwmValue); // Terapkan nilai PWM ke D1
        notifyClients();
      }
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      notifyClients();
      break;
    case WS_EVT_DISCONNECT:
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  pinMode(ledDigitalPin, OUTPUT);
  pinMode(ledPwmPin, OUTPUT);

  digitalWrite(ledDigitalPin, LOW);
  analogWrite(ledPwmPin, 0);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nIP Address: " + WiFi.localIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients();

  // 1. Debounce Tombol Fisik (Digital LED)
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        ledState = !ledState;
        digitalWrite(ledDigitalPin, ledState ? HIGH : LOW);
        notifyClients();
      }
    }
  }
  lastButtonState = reading;

  // 2. Timer Non-Blocking Pembacaan DHT (3 detik)
  if ((millis() - lastTime) > 3000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if(!isnan(t) && !isnan(h)) {
      currentTemp = String(t);
      currentHum  = String(h);
      notifyClients();
    }
    lastTime = millis();
  }
}