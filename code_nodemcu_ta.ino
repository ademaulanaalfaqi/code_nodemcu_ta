#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// Ganti dengan SSID dan Password jaringan WiFi Anda
const char* ssid = "Bidang_Informatika_2.4G";
const char* password = "informatika2022";

// URL skrip PHP di server
const char* serverFlow = "http://10.10.176.157/php_esp_to_database/insert_flow_rate.php";
const char* serverPressure = "http://10.10.176.157/php_esp_to_database/insert_pressure_rate.php";
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

// Define the analog pin
#define SENSOR_PIN A0
int sensorOffset = 0;

// OLED display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void ICACHE_RAM_ATTR pulseCounter() {
  pulseCount++;
}

void setup() {
  // Inisialisasi Serial Monitor
  Serial.begin(115200);
  delay(10);

  // Inisialisasi OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

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

  // Calibrate the sensor (read the sensor value with no pressure)
  sensorOffset = analogRead(SENSOR_PIN);
  Serial.print("Sensor Offset (No Pressure): ");
  Serial.println(sensorOffset);

  // Tampilkan informasi awal di OLED
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("WiFi connected");
  display.setCursor(0,10);
  display.print("IP: ");
  display.print(WiFi.localIP());
  display.display();
  delay(2000); // Pause for 2 seconds
}

void loop() {
  // Hitung laju aliran air setiap detik
  if ((millis() - oldTime) > 10000) {
    // Nonaktifkan interupsi saat menghitung aliran
    detachInterrupt(digitalPinToInterrupt(flowSensorPin));
    
    // Hitung laju aliran (liter per menit)
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / 4.5;

    // Calculate the total litres
    totalLitres += (flowRate / 60); // LPM to litres per second
    
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

    // Tampilkan data di OLED display
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Flow rate:");
    display.setCursor(0,10);
    display.print(flowRate);
    display.print(" L/min");
    display.setCursor(0, 20);
    display.print("Total liters:");
    display.setCursor(0, 30);
    display.print(totalLitres);
    display.setCursor(0, 40);
    display.print("Tekanan:");
    display.setCursor(0, 50);
    display.print(actualPressureValue);
    display.display();

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
