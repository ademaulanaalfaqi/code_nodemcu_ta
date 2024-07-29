#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// SSID dan Password WiFi
const char* ssid = "anu hehe";
const char* password = "te adalah";

// URL skrip PHP di server
const char* serverFlow = "http://192.168.43.181/php_esp_to_database/insert_flow_rate.php";
const char* serverPressure = "http://192.168.43.181/php_esp_to_database/insert_pressure_rate.php";
const char* id_sensor = "Debit-OaDIw";
const char* id_sensor_tekanan = "Tek-E7ifE";
const char* apiKeyFlow = "WF260204";
const char* apiKeyPressure = "WP260204";

// Deklarasi pin dan variabel untuk sensor water flow
const int flowSensorPin = D5; // Pin digital yang terhubung ke sensor
volatile int pulseCount = 0;
float flowRate = 0.0;
float totalLitres = 0.0;
unsigned long oldTime = 0;

// Analog pin sensor water presssure
#define SENSOR_PIN A0
int sensorOffset = 0;

void ICACHE_RAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  // Inisialisasi Serial Monitor
  Serial.begin(115200);
  delay(10);

  // Memulai koneksi ke WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Menyambungkan ke WiFi
  WiFi.begin(ssid, password);

  // Menunggu hingga tersambung ke WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Jika sudah tersambung
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Inisialisasi sensor water flow
  pinMode(flowSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);

  // Inisialisasi variabel waktu
  oldTime = millis();

  // Kalibrasi sensor water pressure
  sensorOffset = analogRead(SENSOR_PIN);
}

void loop() {
  // Hitung laju aliran air setiap detik
  if ((millis() - oldTime) > 10000) {
    // Nonaktifkan interupsi saat menghitung aliran
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
    
    // Hitung laju aliran (liter per menit)
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / 4.5;

    // Calculate the total litres
    totalLitres += (flowRate / 60 / 1000);
    
    // Reset penghitung pulsa dan waktu
    pulseCount = 0;
    oldTime = millis();

    // Read the analog value from the sensor
    int sensorValue = analogRead(SENSOR_PIN);
    int actualPressureValue = sensorValue - sensorOffset;
    // Convert the actual pressure value to a voltage (assuming a 3.3V reference voltage)
    float voltage = actualPressureValue * (3.3 / 1023.0);

    // Tampilkan hasil di Serial Monitor
    Serial.print("Flow rate: ");
    Serial.print(flowRate);
    Serial.println(" L/min");
    Serial.print("Total liter: ");
    Serial.print(totalLitres);
    Serial.println(" m3");
    Serial.print("Pressure: ");
    Serial.print(actualPressureValue);
    Serial.println(" psi");

    // Aktifkan kembali interupsi
    attachInterrupt(digitalPinToInterrupt(flowSensorPin), pulseCounter, FALLING);

    // Kirim flow ke server
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      WiFiClient client;
      String serverPath = serverFlow + String("?nilai_debit=") + String(flowRate) + String("&total_liter=") + String(totalLitres) + String("&id_sensor=") + String(id_sensor) + "&api_key=" + String(apiKeyFlow);
      
      http.begin(client, serverPath);
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      }
      else {
        Serial.print("Error on sending GET: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("WiFi not connected");
    }

    // Kirim tekanan ke server
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      WiFiClient client;
      String serverPath = serverPressure + String("?nilai_tekanan=") + String(actualPressureValue) + String("&id_sensor=") + String(id_sensor_tekanan) + "&api_key=" + String(apiKeyPressure);
      
      http.begin(client, serverPath);
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      }
      else {
        Serial.print("Error on sending GET: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("WiFi not connected");
    }
  }
}
