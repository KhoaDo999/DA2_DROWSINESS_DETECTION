
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =========================
// WiFi credentials
// =========================
const char* ssid = "Iphone";
const char* password = "11111111";

// =========================
// Firebase config
// =========================
#define FIREBASE_HOST ""
#define FIREBASE_SECRET ""

// =========================
// Telegram config
// =========================
#define BOTtoken "" 
#define CHAT_ID "" 

// =========================
// Pin definitions
// =========================
#define BUZZER_PIN 17
#define LED_PIN 26

// =========================
// Firebase objects
// =========================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// =========================
// LCD I2C
// =========================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =========================
// NTP Client for time
// =========================
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);

// =========================
// Web server
// =========================
WebServer server(80);

// // =========================
// // Variables for alert timeout & LED blinking
// // =========================
 unsigned long lastAlertTime = 0;
// unsigned long lastBlinkToggle = 0;
bool alertActive = false;
// bool blinkState = false;
// const unsigned long BLINK_INTERVAL = 200; // 500ms

// =========================
// Function declarations
// =========================
void connectToWiFi();
void handleHttpAlert();
void triggerAlert(bool activate);
void updateFirebaseAlert(String message, bool active);
void sendTelegramMessage(String text);
String getCurrentTime();

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  connectToWiFi();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());

  timeClient.begin();
  timeClient.update();

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase connected");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Trang thai: OK");
  lcd.setCursor(0, 1);
  lcd.print("Binh Thuong ");

  server.on("/alert", HTTP_POST, handleHttpAlert);
  server.begin();
  Serial.println("ESP32 Ready - Listening for HTTP alerts...");
}

void loop() {
  server.handleClient();
  timeClient.update();

  if (alertActive && millis() - lastAlertTime > 4000) {
    Serial.println("Mất tín hiệu từ Python - Reset về bình thường!");
    alertActive = false;
    triggerAlert(false);
    updateFirebaseAlert("Tài xế bình thường", false);
  }

  delay(10);
}


// =========================
// Handle HTTP Alert
// =========================
void handleHttpAlert() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"missing body\"}");
    return;
  }

  String jsonData = server.arg("plain");
  Serial.println("JSON Received: " + jsonData);

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, jsonData);
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.f_str());
    server.send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  bool active = doc["active"];
  lastAlertTime = millis();

  Serial.println("Alert received - active = " + String(active));

  if (active && !alertActive)  {
    triggerAlert(true);
    alertActive = true;
    updateFirebaseAlert("Phát hiện tài xế buồn ngủ!", true);
    sendTelegramMessage("*CẢNH BÁO BUỒN NGỦ!* \nTài xế có dấu hiệu buồn ngủ lúc: " + getCurrentTime());
  }else if (!active && alertActive) {
    triggerAlert(false);
    updateFirebaseAlert("Tài xế bình thường", false);
    alertActive = false;
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }

  server.send(200, "application/json", "{\"status\":\"received\"}");
}

// =========================
// Trigger buzzer & LCD
// =========================
void triggerAlert(bool activate) {
  lcd.clear();
  if (activate) {
    Serial.println("Cảnh báo: BUZZER ON");
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1000);
    lcd.setCursor(3, 0);
    lcd.print("CANH BAO!");
    lcd.setCursor(0, 1);
    lcd.print("TAI XE BUON NGU!");
  } else {
    Serial.println("Trạng thái bình thường: BUZZER OFF");
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    lcd.setCursor(0, 0);
    lcd.print("Trang thai: OK     ");
    lcd.setCursor(0, 1);
    lcd.print("Binh Thuong ");
  }
}

// =========================
// Update Firebase
// =========================
void updateFirebaseAlert(String message, bool active) {
  String currentTime = getCurrentTime();
  FirebaseJson data;
  data.set("message", message);
  data.set("time", currentTime);
  data.set("active", active);

  if (Firebase.setJSON(fbdo, "/alerts", data)) {
    Serial.println("Updated /alerts successfully");
  } else {
    Serial.println("Failed to update /alerts: " + fbdo.errorReason());
  }
  if (active == true) {
  if (Firebase.pushJSON(fbdo, "/history", data)) {
    Serial.println("Added new record to /history");
  } else {
    Serial.println("Failed to push to /history: " + fbdo.errorReason());
  }
  }
}

// =========================
// Send Telegram message
// =========================
void sendTelegramMessage(String text) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;    
  client.setInsecure();        
  HTTPClient http;

  String url = "https://api.telegram.org/bot" + String(BOTtoken) + "/sendMessage";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String payload = "chat_id=" + String(CHAT_ID) + "&parse_mode=Markdown&text=" + text;
  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.printf("Telegram sent! HTTP code: %d\n", httpCode);
  } else {
    Serial.printf("Telegram send failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

// =========================
// Get current time
// =========================
String getCurrentTime() {
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  String date = String(ptm->tm_mday) + "-" + 
                String(ptm->tm_mon + 1) + "-" + 
                String(ptm->tm_year + 1900);
  return date + " " + formattedTime;
}

// =========================
// WiFi connection
// =========================
void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());
}


