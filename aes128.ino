#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <AESLib.h>
#include <DHT.h>
#include <base64.h> // Pastikan Anda memiliki library Base64

#define DHTPIN D1
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "mn";
const char* password ="manan1234";
const char* server = "http://192.168.137.87:5000/upload";
// const char* server = "http://192.168.43.21:5000/upload";
AESLib aes;

byte aes_key[16] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6'};
byte aes_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

String encryptData(String data) {
  // Tambahkan padding
  int dataLen = data.length();
  int paddedLen = ((dataLen + 15) / 16) * 16; // Panjang kelipatan 16
  char dataBuffer[paddedLen + 1]; // +1 untuk null terminator
  data.toCharArray(dataBuffer, dataLen + 1);
  
  // Tambahkan padding dengan spasi
  for (int i = dataLen; i < paddedLen; i++) {
    dataBuffer[i] = ' ';
  }
  dataBuffer[paddedLen] = '\0'; // Null terminator

  byte encryptedData[32];
  int bits = 128; 
  int encryptedLen = aes.encrypt((byte*)dataBuffer, paddedLen, encryptedData, aes_key, bits, aes_iv);

  // Encode ke Base64
  String base64Encoded = base64::encode(encryptedData, encryptedLen);
  return base64Encoded;
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
}

void loop() {
  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Gagal Membaca Sensor");
    delay(2000);
    return;
  }

  unsigned long startTime = millis();
  String rawData = String(temperature) + "," + String(humidity);
  String encryptedData = encryptData(rawData);
  unsigned long endTime = millis();

  // Tampilkan hanya data terenkripsi dan waktu enkripsi di Serial Monitor
  Serial.println("Enkripsi Data: " + encryptedData);
  Serial.println("Waktu Enkripsi: " + String(endTime - startTime) + " ms");

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, server);
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{\"data\":\"" + encryptedData + "\"}";
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      // Anda bisa menambahkan logika di sini jika perlu
    } else {
      Serial.println("Gagal Mengirim Data: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected!");
  }

  delay(10000); // Delay sebelum pengulangan berikutnya
}