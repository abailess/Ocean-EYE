#include <WiFi.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// ---------- Wi-Fi ----------
const char* ssid = "Wifi_Tara_Scientist";
const char* password = "Password"; //your password here 
WiFiServer server(4070);

// ---------- NTP ----------
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 3600000);

// ---------- I2C ----------
#define MKR1 0x10
#define MKR2 0x11
#define MKR3 0x12

#define SPEC_CHANNELS 288
#define CHUNK 16

uint16_t data1[SPEC_CHANNELS];
uint16_t data2[SPEC_CHANNELS];
uint16_t data3[SPEC_CHANNELS];

unsigned long lastLog = 0;
const unsigned long LOG_INTERVAL = 10000; // 10 seconds

// ---------- I2C timeout (ms) ----------
#define I2C_TIMEOUT_MS 20

// ------------------ I2C Reset ------------------
void resetI2CBus() {
  Serial.println("Resetting I2C bus");
  Wire.end();
  delay(10);
  Wire.begin();
}

// ------------------ I2C Read ------------------
bool readFromMKR(uint8_t addr, uint16_t* dest) {
  int idx = 0;
  const int TOTAL_WORDS = SPEC_CHANNELS;

  while (idx < TOTAL_WORDS) {
    int toRead = min(CHUNK, TOTAL_WORDS - idx);
    Wire.requestFrom(addr, toRead * 2);

    unsigned long start = millis();

    for (int i = 0; i < toRead; i++) {
      while (Wire.available() < 2) {
        if (millis() - start > I2C_TIMEOUT_MS) {
          Serial.printf("I2C timeout from MKR 0x%02X\n", addr);
          return false;
        }
        delayMicroseconds(50);
      }

      uint8_t msb = Wire.read();
      uint8_t lsb = Wire.read();
      uint16_t val = (msb << 8) | lsb;

      if (idx < SPEC_CHANNELS) {
        dest[idx] = val;           // pixel
      }

      idx++;
    }
  }
  return true;
}

// ------------------ Print Spectrum ------------------
void printSpectrum(WiFiClient& client, uint16_t* data) {
  for (int i = 0; i < SPEC_CHANNELS; i++) {
    client.print(",");
    client.print(data[i]);
  }
}

// ------------------ Setup ------------------
void setup() {
  Serial.begin(115200);
  Wire.begin(); // ESP32 = I2C master

  // Wi-Fi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();

  timeClient.begin();
  timeClient.update();
}

// ------------------ Main Loop ------------------
void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  Serial.println("Client connected");

  while (client.connected()) {

    if (millis() - lastLog >= LOG_INTERVAL) {
      lastLog = millis();
      timeClient.update();

      // Timestamp
      client.print(timeClient.getEpochTime());

      // ---- MKR 1 ----
      if (!readFromMKR(MKR1, data1)) {
        resetI2CBus();
        client.println(",MKR1_FAIL");
        continue;
      }
      printSpectrum(client, data1);

      // ---- MKR 2 ----
      if (!readFromMKR(MKR2, data2)) {
        resetI2CBus();
        client.println(",MKR2_FAIL");
        continue;
      }
      printSpectrum(client, data2);


      // ---- MKR 3 ----
      if (!readFromMKR(MKR3, data3)) {
        resetI2CBus();
        client.println(",MKR3_FAIL");
        continue;
      }
      printSpectrum(client, data3);


      client.println();
      Serial.println("Frame sent (all MKRs)");
    }

    delay(10); // yield to Wi-Fi stack
  }

  client.stop();
  Serial.println("Client disconnected");
}
