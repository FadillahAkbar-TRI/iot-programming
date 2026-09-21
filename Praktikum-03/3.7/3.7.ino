#include <ESP8266WiFi.h>  
#include <ESP8266WebServer.h>  
#include <DHT.h>

const char* ssid = "ayam1";        
const char* password = "konfirmasi"; 

ESP8266WebServer server(80);

const byte dhtPin = 2;        
const byte relayPin = 12;      
DHT dht(dhtPin, DHT11);

// Modifikasi pada index_html: penambahan meta refresh & data kelembapan
const char index_html[] PROGMEM = R"rawliteral(  
<!DOCTYPE html>  
<html>  
<head>  
  <meta name="viewport" content="width=device-width, initial-scale=1">  
  <!-- META REFRESH: Memaksa browser reload otomatis setiap 5 detik -->
  <meta http-equiv="refresh" content="5">
  <title>IoT Dashboard</title>  
  <style>  
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 40px; background-color: #f4f6f9; }  
    .card { background: white; padding: 20px; margin: 20px auto; max-width: 350px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    button { padding: 12px 28px; font-size: 18px; border-radius: 8px; margin: 8px; cursor: pointer; border: none; font-weight: bold; }  
    .btn-on { background-color: #4CAF50; color: white; }  
    .btn-off { background-color: #f44336; color: white; }  
    .sensor-val { font-size: 20px; margin: 10px 0; }  
  </style>  
</head>  
<body>  
  <h1>ESP8266 IoT Web Server</h1>  
  
  <div class="card">
    <h2>Data Sensor DHT11</h2>
    <p class="sensor-val">Suhu: <strong>%TEMPERATURE%</strong> &deg;C</p>  
    <p class="sensor-val">Kelembapan: <strong>%HUMIDITY%</strong> %</p>
  </div>

  <div class="card">
    <h2>Kendali Beban / LED</h2>  
    <a href="/relay/on"><button class="btn-on">ON</button></a>  
    <a href="/relay/off"><button class="btn-off">OFF</button></a>  
  </div>
</body>  
</html>  
)rawliteral";

void handleRoot() {  
  String html = index_html;
  float t = dht.readTemperature();  
  float h = dht.readHumidity();
  
  // Penanganan nilai suhu
  if (isnan(t)) {
    html.replace("%TEMPERATURE%", "--"); 
  } else {
    html.replace("%TEMPERATURE%", String(t, 1)); 
  }

  // Penanganan nilai kelembapan
  if (isnan(h)) {
    html.replace("%HUMIDITY%", "--"); 
  } else {
    html.replace("%HUMIDITY%", String(h, 1)); 
  }
  
  server.send(200, "text/html", html);  
}

void handleRelayOn() {  
  digitalWrite(relayPin, HIGH); 
  server.sendHeader("Location", "/");   
  server.send(303);  
}

void handleRelayOff() {  
  digitalWrite(relayPin, LOW);   
  server.sendHeader("Location", "/");   
  server.send(303);  
}

void setup() {  
  Serial.begin(115200);  
  pinMode(relayPin, OUTPUT);  
  digitalWrite(relayPin, LOW);  
  dht.begin();  
    
  WiFi.mode(WIFI_STA);   
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }  
  
  Serial.println("\nNodeMCU Web Server Siap!");  
  Serial.print("Buka browser di alamat IP: http://");  
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);  
  server.on("/relay/on", handleRelayOn);  
  server.on("/relay/off", handleRelayOff);  
  server.begin();  
}

void loop() {  
  server.handleClient();  
}