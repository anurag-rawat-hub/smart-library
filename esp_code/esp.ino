#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#define SS_PIN  4
#define RST_PIN 5
#define BUZZER  16

MFRC522 rfid(SS_PIN, RST_PIN);

// ===== WIFI =====
const char* ssid = "iqoo";
const char* password = "pankaj90";

// ===== GOOGLE SCRIPT URL =====
String googleURL = "https://script.google.com/macros/s/AKfycbyu5YWkWlxIz83FGoUBu57yGXiK5tK-tTqp2MJ2BiKaSONNLXzYrRNZL_IaUWC7AQfaxQ/exec";

// ===== BOOK DATABASE =====
byte book1[4] = {0x9D, 0xEE, 0x7A, 0x05};
String book1Name = "ELECTRONICS";
bool book1Issued = false;

byte book2[4] = {0x0E, 0xB3, 0x2D, 0x06};
String book2Name = "CENGAGE";
bool book2Issued = false;

void setup() {
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Serial.println("=================================");
  Serial.println("   SMART LIBRARY SYSTEM (RFID)   ");
  Serial.println("=================================");

  Serial.print("[■] Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\n[■■] WiFi Connected");
  Serial.println("[■■■] System Ready");
  Serial.println("[■] Waiting for Scan...");
}

void loop() {

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.println("\n[■■] RFID Detected");

  // ===== READ UID =====
  String uidStr = "";
  for (byte i = 0; i < 4; i++) {
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();

  // ===== CHECK BOOK =====
  if (compareUID(rfid.uid.uidByte, book1)) {
    handleBook(book1Name, book1Issued, uidStr);
  }
  else if (compareUID(rfid.uid.uidByte, book2)) {
    handleBook(book2Name, book2Issued, uidStr);
  }
  else {
    Serial.println("[ERROR] Unknown Book");
    rfid.PICC_HaltA();
    return;
  }

  Serial.println("[■] Waiting for Scan...");
  rfid.PICC_HaltA();
  delay(1500);
}

// ============ FUNCTIONS ============

void handleBook(String bookName, bool &issuedStatus, String uid) {

  if (!issuedStatus) {
    Serial.print("[TAKEN] Book Issued: ");
    Serial.println(bookName);
    issuedStatus = true;
    beepShort();
    sendToGoogleSheet(uid, bookName + " - TAKEN");
  }
  else {
    Serial.print("[RECEIVED] Book Returned: ");
    Serial.println(bookName);
    issuedStatus = false;
    beepLong();
    sendToGoogleSheet(uid, bookName + " - RECEIVED");
  }
}

bool compareUID(byte *a, byte *b) {
  for (byte i = 0; i < 4; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void beepShort() {
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
}

void beepLong() {
  digitalWrite(BUZZER, HIGH);
  delay(800);
  digitalWrite(BUZZER, LOW);
}

void sendToGoogleSheet(String uid, String bookStatus) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi Disconnected");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  String url = googleURL + "?uid=" + uid + "&book=" + bookStatus;

  https.begin(client, url);
  https.GET();
  https.end();

  Serial.println("[✓] Google Sheet Updated");
}
