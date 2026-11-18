#include <WiFi.h>
#include <DHT.h>
#include <NewPing.h>

// --- 1. KONFIGURASI WI-FI ---
const char* WIFI_SSID     = "ocy";
const char* WIFI_PASSWORD = "kamuudahtau";
const unsigned long WIFI_TIMEOUT = 20000;

// --- 2. DEFINISI PIN SENSOR & INDIKATOR ---
#define DHTPIN      4       // Pin Data DHT22 (Suhu & Kelembapan Garasi)
#define DHTTYPE     DHT22
#define TRIG_PIN    5       // Pin Trigger HC-SR04 (Jarak Parkir)
#define ECHO_PIN    18      // Pin Echo HC-SR04
#define MAX_DIST    200     // Jarak maksimal pembacaan (cm)
#define LED_BIRU    2       // LED Biru Built-in ESP32 (Indikator Peringatan)

// --- 3. AMBANG BATAS (THRESHOLD) SMART GARAGE ---
const int   BATAS_PARKIR_CM   = 20;   // Jarak aman berhenti dari dinding (<= 20 cm)
const float BATAS_SUHU_PANAS  = 32.0; // Batas suhu garasi pengap (> 32°C)
const float BATAS_LEMBAP_TINGGI = 75.0; // Batas kelembapan berisiko karat (> 75%)

DHT dht(DHTPIN, DHTTYPE);
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DIST);

unsigned long waktuSebelumnya = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  dht.begin();

  Serial.println("\n=== SISTEM SMART GARAGE MONITORING (ESP32) ===");
  WiFi.mode(WIFI_STA);
  Serial.print("[WIFI] MAC Address : ");
  Serial.println(WiFi.macAddress());
  Serial.print("[WIFI] Menghubungkan ke : ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < WIFI_TIMEOUT)) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] Terhubung! IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] Mode Offline (Gagal terhubung ke Wi-Fi).");
  }

  Serial.println("-------------------------------------------------------------------------");
  Serial.println("WAKTU (ms) | WIFI   | JARAK DINDING | SUHU   | LEMBAP | STATUS GARASI");
  Serial.println("-------------------------------------------------------------------------");
}

void loop() {
  unsigned long sekarang = millis();

  // Pembacaan setiap 2 detik (2000 ms)
  if (sekarang - waktuSebelumnya >= 2000) {
    waktuSebelumnya = sekarang;

    // A. Cek Koneksi Wi-Fi
    String statusWifi = "ONLINE";
    if (WiFi.status() != WL_CONNECTED) {
      statusWifi = "OFFLINE";
      WiFi.reconnect();
    }

    // B. Baca Sensor DHT22 (Sirkulasi Udara Garasi)
    float kelembapan = dht.readHumidity();
    float suhu = dht.readTemperature();

    if (isnan(kelembapan) || isnan(suhu)) {
      Serial.println("[ERROR] Sensor DHT22 gagal terbaca!");
      return;
    }

    // C. Baca Sensor Ultrasonic (Jarak Kendaraan ke Dinding)
    unsigned int jarak = sonar.ping_cm();
    if (jarak == 0) jarak = MAX_DIST;

    // D. Evaluasi Kondisi Parkir & Sirkulasi Udara
    bool bahayaParkir = (jarak <= BATAS_PARKIR_CM);
    bool udaraBuruk   = (suhu > BATAS_SUHU_PANAS || kelembapan > BATAS_LEMBAP_TINGGI);
    String statusGarasi = "[AMAN] Parkir & Udara Normal";

    if (bahayaParkir && udaraBuruk) {
      statusGarasi = "[STOP!] Jarak <=20cm & Udara Pengap!";
      digitalWrite(LED_BIRU, HIGH);
    } else if (bahayaParkir) {
      statusGarasi = "[STOP!] Jarak Terlalu Dekat Dinding!";
      digitalWrite(LED_BIRU, HIGH);
    } else if (udaraBuruk) {
      statusGarasi = "[SIRKULASI] Garasi Panas/Lembap!";
      digitalWrite(LED_BIRU, HIGH);
    } else {
      digitalWrite(LED_BIRU, LOW);
    }

    // E. Tampilkan ke Serial Monitor
    Serial.print(sekarang);
    Serial.print(" ms\t| ");
    Serial.print(statusWifi);
    Serial.print("\t| ");
    Serial.print(jarak);
    Serial.print(" cm\t| ");
    Serial.print(suhu, 1);
    Serial.print("°C | ");
    Serial.print(kelembapan, 1);
    Serial.print("% | ");
    Serial.println(statusGarasi);
  }
}