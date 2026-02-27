/*
 * MKR Zero – Hamamatsu Spectrometer I2C
 */

#include <Wire.h>

#define I2C_ADDR         0x11 // sun, sky, ocean

#define SPEC_ST          3
#define SPEC_CLK         4
#define SPEC_VIDEO       A0

#define SPEC_CHANNELS    288
#define CHUNK_SIZE       16   // 16 channels = 32 bytes (I2C-safe)

uint16_t data[SPEC_CHANNELS];

// --- NEW: double-buffer control ---
volatile bool dataReady = false;
volatile bool busy = false;   // prevents overwrite during I2C transfer

// ------------------ SETUP ------------------
void setup() {
  Wire.begin(I2C_ADDR);
  Wire.onRequest(sendData);

  pinMode(SPEC_CLK, OUTPUT);
  pinMode(SPEC_ST, OUTPUT);

  digitalWrite(SPEC_CLK, HIGH);
  digitalWrite(SPEC_ST, LOW);

  analogReadResolution(12);   // MKR Zero ADC is 12-bit
}

// ------------------ READ SPECTROMETER ------------------
void readSpectrometer() {

  if (busy) return;   // <<< NEW: do not read while I2C is transmitting
  busy = true;

  int delayTime = 1;  // µs clock timing

  digitalWrite(SPEC_CLK, LOW);
  delayMicroseconds(delayTime);
  digitalWrite(SPEC_CLK, HIGH);
  delayMicroseconds(delayTime);
  digitalWrite(SPEC_CLK, LOW);

  digitalWrite(SPEC_ST, HIGH);
  delayMicroseconds(delayTime);

  // Dummy clocks (Hamamatsu timing)
  for (int i = 0; i < 15; i++) {
    digitalWrite(SPEC_CLK, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(SPEC_CLK, LOW);
    delayMicroseconds(delayTime);
  }

  digitalWrite(SPEC_ST, LOW);

  for (int i = 0; i < 85; i++) {
    digitalWrite(SPEC_CLK, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(SPEC_CLK, LOW);
    delayMicroseconds(delayTime);
  }

  // Final pre-read clock
  digitalWrite(SPEC_CLK, HIGH);
  delayMicroseconds(delayTime);
  digitalWrite(SPEC_CLK, LOW);
  delayMicroseconds(delayTime);

  // -------- READ PIXELS --------
  for (int i = 0; i < SPEC_CHANNELS; i++) {
    data[i] = analogRead(SPEC_VIDEO);

    digitalWrite(SPEC_CLK, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(SPEC_CLK, LOW);
    delayMicroseconds(delayTime); 
  }

  digitalWrite(SPEC_ST, HIGH);

  for (int i = 0; i < 7; i++) {
    digitalWrite(SPEC_CLK, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(SPEC_CLK, LOW);
    delayMicroseconds(delayTime);
  }

  digitalWrite(SPEC_CLK, HIGH);

  dataReady = true;
  busy = false;
}

// ------------------ I2C SEND HANDLER ------------------
void sendData() {

  if (!dataReady) return;

  static uint16_t idx = 0;

  for (uint8_t i = 0; i < CHUNK_SIZE && idx < SPEC_CHANNELS; i++) {
    Wire.write(highByte(data[idx]));
    Wire.write(lowByte(data[idx]));
    idx++;
  }

  if (idx >= SPEC_CHANNELS) {
    idx = 0;
    dataReady = false;   // <<< ensures master gets a full frame only
  }
}

// ------------------ LOOP ------------------
void loop() {
  readSpectrometer();
  delay(20);   // <<< reduced rate (~50 Hz) = more stable ADC + I2C
}
