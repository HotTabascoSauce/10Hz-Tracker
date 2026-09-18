#include <Arduino.h>
#include <FT6336.h>
#include <FS.h>
#include <SD_MMC.h>
#include <SPIFFS.h>
#include <TinyGPSPlus.h>
#include <TFT_eSPI.h>
#include "esp_sleep.h"

namespace {
constexpr uint8_t TOUCH_SDA = 16;
constexpr uint8_t TOUCH_SCL = 15;
constexpr uint8_t TOUCH_INT = 17;
constexpr uint8_t TOUCH_RST = 18;

constexpr uint8_t SD_CLK = 38;
constexpr uint8_t SD_CMD = 40;
constexpr uint8_t SD_D0 = 39;
constexpr uint8_t SD_D1 = 41;
constexpr uint8_t SD_D2 = 48;
constexpr uint8_t SD_D3 = 47;
// UART connector pinout on this ESP32-S3 board:
// GPS TXD -> ESP32 RX (GPIO44)
// GPS RXD -> ESP32 TX (GPIO43)
constexpr uint8_t GPS_RX = 44;
constexpr uint8_t GPS_TX = 43;
constexpr uint32_t GPS_BAUD = 9600;
constexpr uint32_t GPS_LOG_INTERVAL_MS = 100;
constexpr uint32_t GPS_FIX_MAX_AGE_MS = 3000;
constexpr uint32_t GPS_DIAGNOSTIC_INTERVAL_MS = 2000;
constexpr int16_t RECORD_BUTTON_X = 20;
constexpr int16_t RECORD_BUTTON_Y = 255;
constexpr uint16_t RECORD_BUTTON_WIDTH = 200;
constexpr uint16_t RECORD_BUTTON_HEIGHT = 45;
constexpr uint16_t IMAGE_WIDTH = 240;
constexpr uint16_t IMAGE_HEIGHT = 320;

constexpr uint16_t WHITE = TFT_WHITE;
constexpr uint16_t BLUE = TFT_BLUE;
constexpr uint16_t GREEN = TFT_GREEN;
constexpr uint16_t RED = TFT_RED;
constexpr uint8_t TOUCH_MARKER_RADIUS = 8;
constexpr uint32_t BACKLIGHT_TIMEOUT_MS = 10000;
constexpr uint8_t BATTERY_ADC_PIN = 9;
constexpr uint32_t BATTERY_UPDATE_INTERVAL_MS = 5000;
constexpr uint8_t BOOT_BUTTON_PIN = 0;
constexpr uint32_t DEEP_SLEEP_HOLD_MS = 3000;

TFT_eSPI display;
FT6336 touch(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, 240, 320);
HardwareSerial gpsSerial(1);
TinyGPSPlus gps;
File gpsLogFile;
bool tfCardDetected = false;
bool cardImageLoaded = false;
bool gpsHasFix = false;
bool recording = false;
bool touchWasPressed = false;
uint32_t nextGpsLogTime = 0;
uint32_t lastGpsDiagnosticTime = 0;
uint32_t gpsLocationUpdates = 0;
bool backlightOn = true;
uint32_t lastTouchTime = 0;
uint32_t lastBatteryUpdate = 0;
uint16_t batteryMillivolts = 0;
uint16_t previousBatteryMillivolts = 0;
uint8_t batteryPercent = 0;
bool batteryCharging = false;
bool bootButtonArmed = true;
uint32_t bootButtonPressTime = 0;
int16_t previousX = -1;
int16_t previousY = -1;

void setBacklight(bool enabled) {
  digitalWrite(TFT_BL, enabled ? TFT_BACKLIGHT_ON : !TFT_BACKLIGHT_ON);
  backlightOn = enabled;
}

bool drawCardImage() {
  if (!tfCardDetected || !SD_MMC.exists("/image.bin")) {
    return false;
  }

  display.setSwapBytes(true);
  File imageFile = SD_MMC.open("/image.bin", FILE_READ);
  if (!imageFile || imageFile.size() < IMAGE_WIDTH * IMAGE_HEIGHT * 2) {
    return false;
  }

  uint16_t row[IMAGE_WIDTH];
  for (uint16_t y = 0; y < IMAGE_HEIGHT; ++y) {
    if (imageFile.read(reinterpret_cast<uint8_t *>(row), sizeof(row)) !=
        sizeof(row)) {
      imageFile.close();
      return false;
    }
    display.pushImage(0, y, IMAGE_WIDTH, 1, row);
  }

  imageFile.close();
  return true;
}

void drawChargingPlug(int16_t x, int16_t y) {
  display.drawRect(x + 3, y + 5, 10, 9, BLUE);
  display.drawFastVLine(x + 6, y + 1, 4, BLUE);
  display.drawFastVLine(x + 10, y + 1, 4, BLUE);
  display.drawLine(x + 13, y + 9, x + 17, y + 9, BLUE);
  display.drawLine(x + 17, y + 9, x + 17, y + 15, BLUE);
  display.drawLine(x + 17, y + 15, x + 13, y + 19, BLUE);
}

void drawBatteryIndicator() {
  const int16_t panelX = display.width() - 120;
  display.fillRect(panelX, 0, 120, 29, WHITE);
  display.drawRoundRect(panelX + 4, 6, 40, 18, 3, BLUE);
  display.fillRect(panelX + 44, 11, 3, 8, BLUE);

  const int16_t fillWidth = 34 * batteryPercent / 100;
  if (fillWidth > 0) {
    display.fillRoundRect(panelX + 7, 9, fillWidth, 12, 2, BLUE);
  }

  display.setTextDatum(TL_DATUM);
  display.setTextColor(BLUE, WHITE);
  display.drawString(String(batteryPercent) + "%", panelX + 51, 7, 2);
  if (batteryCharging) {
    drawChargingPlug(panelX + 91, 3);
  }
}

uint8_t batteryPercentFromMillivolts(uint16_t millivolts) {
  if (millivolts <= 3300) return 0;
  if (millivolts <= 3500) return map(millivolts, 3300, 3500, 0, 10);
  if (millivolts <= 3700) return map(millivolts, 3500, 3700, 10, 35);
  if (millivolts <= 3800) return map(millivolts, 3700, 3800, 35, 60);
  if (millivolts <= 3900) return map(millivolts, 3800, 3900, 60, 80);
  if (millivolts <= 4100) return map(millivolts, 3900, 4100, 80, 97);
  if (millivolts < 4200) return map(millivolts, 4100, 4200, 97, 100);
  return 100;
}

void updateBatteryIndicator() {
  const uint32_t now = millis();
  if (now - lastBatteryUpdate < BATTERY_UPDATE_INTERVAL_MS) {
    return;
  }
  lastBatteryUpdate = now;

  const uint16_t measuredMillivolts = analogReadMilliVolts(BATTERY_ADC_PIN) * 2;
  previousBatteryMillivolts = batteryMillivolts;
  batteryMillivolts = measuredMillivolts;
  batteryPercent = batteryPercentFromMillivolts(batteryMillivolts);

  if (previousBatteryMillivolts > 0) {
    batteryCharging = batteryMillivolts > previousBatteryMillivolts + 8;
  }
  drawBatteryIndicator();
}

void drawStatus() {
  display.setTextDatum(TL_DATUM);
  display.setTextColor(BLUE, WHITE);
  display.drawString("Touch:", 6, 6, 2);

  display.setTextDatum(BR_DATUM);
  display.drawString(tfCardDetected ? "TF: DETECTED" : "TF: NOT DETECTED",
                     display.width() - 4, display.height() - 4, 2);

  if (gpsHasFix) {
    display.fillRoundRect(RECORD_BUTTON_X, RECORD_BUTTON_Y,
                          RECORD_BUTTON_WIDTH, RECORD_BUTTON_HEIGHT, 6,
                          recording ? RED : GREEN);
    display.drawRoundRect(RECORD_BUTTON_X, RECORD_BUTTON_Y,
                          RECORD_BUTTON_WIDTH, RECORD_BUTTON_HEIGHT, 6, BLUE);
    display.setTextDatum(MC_DATUM);
    display.setTextColor(WHITE);
    display.drawString(recording ? "STOP RECORDING" : "START RECORDING",
                       RECORD_BUTTON_X + RECORD_BUTTON_WIDTH / 2,
                       RECORD_BUTTON_Y + RECORD_BUTTON_HEIGHT / 2, 2);
  }
}

void sendGpsUartCommand() {
  const uint8_t command[] = {
      0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x08, 0x00, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x07,
      0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD2, 0xD8};
  gpsSerial.write(command, sizeof(command));
  gpsSerial.flush();
}

void sendGpsRateCommand() {
  const uint8_t command[] = {
      0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00,
      0x01, 0x00, 0x7A, 0x12};
  gpsSerial.write(command, sizeof(command));
  gpsSerial.flush();
}

void updateGps() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated()) {
      ++gpsLocationUpdates;
      gps.location.lat();
    }
  }

  const bool newFix = gps.location.isValid() &&
                      gps.location.age() <= GPS_FIX_MAX_AGE_MS;
  if (newFix != gpsHasFix) {
    gpsHasFix = newFix;
    if (!gpsHasFix && recording) {
      recording = false;
      if (gpsLogFile) {
        gpsLogFile.flush();
        gpsLogFile.close();
      }
    }
    drawStatus();
  }
}

void updateGpsDiagnostics() {
  const uint32_t now = millis();
  if (now - lastGpsDiagnosticTime < GPS_DIAGNOSTIC_INTERVAL_MS) {
    return;
  }
  lastGpsDiagnosticTime = now;

  Serial.printf("GPS: chars=%lu sentences=%lu checksum_errors=%lu fix=%s sats=%lu location_updates=%lu rate_hz=%.2f\n",
                static_cast<unsigned long>(gps.charsProcessed()),
                static_cast<unsigned long>(gps.sentencesWithFix()),
                static_cast<unsigned long>(gps.failedChecksum()),
                gpsHasFix ? "yes" : "no",
                static_cast<unsigned long>(gps.satellites.value()),
                static_cast<unsigned long>(gpsLocationUpdates),
                static_cast<double>(gpsLocationUpdates) * 1000.0 /
                    GPS_DIAGNOSTIC_INTERVAL_MS);
  gpsLocationUpdates = 0;
}

String makeGpsIsoTime() {
  if (!gps.date.isValid() || !gps.time.isValid()) {
    return String("1970-01-01T00:00:00Z");
  }

  char isoTime[28];
  snprintf(isoTime, sizeof(isoTime), "%04lu-%02u-%02uT%02u:%02u:%02u.%02uZ",
           static_cast<unsigned long>(gps.date.year()), gps.date.month(),
           gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second(),
           gps.time.centisecond());
  return String(isoTime);
}

String makeDatestampFilename() {
  char ts[28];
  if (gps.date.isValid() && gps.time.isValid()) {
    snprintf(ts, sizeof(ts), "%04lu-%02u-%02u_%02u-%02u-%02u_%03lu",
             static_cast<unsigned long>(gps.date.year()), gps.date.month(),
             gps.date.day(), gps.time.hour(), gps.time.minute(), gps.time.second(),
             static_cast<unsigned long>(millis() % 1000));
  } else {
    snprintf(ts, sizeof(ts), "%04lu-%02u-%02u_%02u-%02u-%02u_%03lu",
             1970UL, 1U, 1U, 0U, 0U, 0U,
             static_cast<unsigned long>(millis() % 1000));
  }
  return String("/gps_") + String(ts) + ".gpx";
}

bool startRecording() {
  if (!tfCardDetected || !gpsHasFix) {
    return false;
  }

  const String fileName = makeDatestampFilename();
  SD_MMC.remove(fileName);
  gpsLogFile = SD_MMC.open(fileName, FILE_WRITE);
  if (!gpsLogFile) {
    Serial.println("Unable to open GPX file");
    return false;
  }

  gpsLogFile.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  gpsLogFile.println("<gpx version=\"1.1\" creator=\"10hz GPS Tracker\" xmlns=\"http://www.topografix.com/GPX/1/1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">");
  gpsLogFile.println("  <metadata>");
  gpsLogFile.println("    <name>GPS Track</name>");
  gpsLogFile.println("    <desc>Track recorded by 10hz GPS Tracker</desc>");
  gpsLogFile.println((String("    <time>") + makeGpsIsoTime() + "</time>"));
  gpsLogFile.println("  </metadata>");
  gpsLogFile.println("  <trk>");
  gpsLogFile.println("    <name>GPS Track</name>");
  gpsLogFile.println("    <trkseg>");
  gpsLogFile.flush();

  recording = true;
  nextGpsLogTime = millis();
  drawStatus();
  return true;
}

void stopRecording() {
  recording = false;
  if (gpsLogFile) {
    gpsLogFile.println("    </trkseg>");
    gpsLogFile.println("  </trk>");
    gpsLogFile.println("</gpx>");
    gpsLogFile.flush();
    gpsLogFile.close();
  }
  drawStatus();
}

void updateGpsLog() {
  if (!recording || !gpsLogFile || !gpsHasFix) {
    return;
  }

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - nextGpsLogTime) < 0) {
    return;
  }
  nextGpsLogTime += GPS_LOG_INTERVAL_MS;

  char isoTime[28] = "";
  if (gps.date.isValid() && gps.time.isValid()) {
    snprintf(isoTime, sizeof(isoTime), "%04lu-%02u-%02uT%02u:%02u:%02u.%02uZ",
             static_cast<unsigned long>(gps.date.year()), gps.date.month(),
             gps.date.day(), gps.time.hour(), gps.time.minute(),
             gps.time.second(), gps.time.centisecond());
  } else {
    snprintf(isoTime, sizeof(isoTime), "1970-01-01T00:00:00Z");
  }

  gpsLogFile.printf("      <trkpt lat=\"%.6f\" lon=\"%.6f\">",
                    gps.location.lat(), gps.location.lng());
  if (gps.altitude.isValid()) {
    gpsLogFile.printf("<ele>%.2f</ele>", gps.altitude.meters());
  }
  gpsLogFile.printf("<time>%s</time>", isoTime);
  gpsLogFile.println("</trkpt>");
  gpsLogFile.flush();
}

void updateTouchDisplay() {
  touch.read();
  const bool touched = touch.isTouched;
  if (!touched) {
    touchWasPressed = false;
    if (backlightOn && millis() - lastTouchTime >= BACKLIGHT_TIMEOUT_MS) {
      setBacklight(false);
    }
    return;
  }

  lastTouchTime = millis();
  if (!backlightOn) {
    setBacklight(true);
  }

  const int16_t x = constrain(static_cast<int16_t>(touch.points[0].x), 0,
                              display.width() - 1);
  const int16_t y = constrain(static_cast<int16_t>(touch.points[0].y), 0,
                              display.height() - 1);

  if (gpsHasFix && x >= RECORD_BUTTON_X &&
      x < RECORD_BUTTON_X + RECORD_BUTTON_WIDTH && y >= RECORD_BUTTON_Y &&
      y < RECORD_BUTTON_Y + RECORD_BUTTON_HEIGHT) {
    if (!touchWasPressed) {
      if (recording) {
        stopRecording();
      } else if (!startRecording()) {
        Serial.println("Unable to start GPS recording");
      }
    }
    touchWasPressed = true;
    return;
  }
  touchWasPressed = true;

  if (previousX >= 0 && previousY >= 0) {
    if (cardImageLoaded) {
      drawCardImage();
    } else {
      display.fillCircle(previousX, previousY, TOUCH_MARKER_RADIUS, WHITE);
    }
  }

  display.fillCircle(x, y, TOUCH_MARKER_RADIUS, BLUE);
  display.setTextDatum(TL_DATUM);
  display.setTextColor(BLUE, WHITE);
  display.drawString("Touch: " + String(x) + ", " + String(y), 6, 6, 2);
  drawStatus();
  drawBatteryIndicator();

  previousX = x;
  previousY = y;
  Serial.printf("Touch: x=%d, y=%d\n", x, y);
}

void enterDeepSleep() {
  Serial.println("Entering deep sleep");
  setBacklight(false);
  esp_sleep_enable_ext1_wakeup(1ULL << BOOT_BUTTON_PIN,
                               ESP_EXT1_WAKEUP_ANY_HIGH);
  Serial.flush();
  delay(50);
  esp_deep_sleep_start();
}

void waitForWakePress() {
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
    return;
  }

  while (digitalRead(BOOT_BUTTON_PIN) == HIGH) {
    delay(10);
  }
}

void updateBootButton() {
  const bool pressed = digitalRead(BOOT_BUTTON_PIN) == LOW;
  if (!pressed) {
    bootButtonPressTime = 0;
    bootButtonArmed = true;
    return;
  }

  if (!bootButtonArmed) {
    return;
  }

  if (bootButtonPressTime == 0) {
    bootButtonPressTime = millis();
  } else if (millis() - bootButtonPressTime >= DEEP_SLEEP_HOLD_MS) {
    bootButtonArmed = false;
    enterDeepSleep();
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  bootButtonArmed = digitalRead(BOOT_BUTTON_PIN) == HIGH;
  waitForWakePress();

  display.init();
  display.setRotation(0);
  display.fillScreen(WHITE);
  pinMode(TFT_BL, OUTPUT);
  setBacklight(true);
  lastTouchTime = millis();
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
  delay(100);
  sendGpsUartCommand();
  delay(100);
  sendGpsRateCommand();
  Serial.printf("Neo-M8N UART ready: RX=%u TX=%u\n", GPS_RX, GPS_TX);

  touch.begin();
  touch.setRotation(ROTATION_NORMAL);

  if (SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3)) {
    tfCardDetected = SD_MMC.begin("/sdcard", false);
  }

  if (tfCardDetected) {
    cardImageLoaded = drawCardImage();
    Serial.println(cardImageLoaded ? "image.bin loaded" : "image.bin not found or invalid");
  }

  drawStatus();
  updateBatteryIndicator();
  Serial.println(tfCardDetected ? "TF card detected" : "TF card not detected");
}

void loop() {
  updateBootButton();
  updateGps();
  updateGpsDiagnostics();
  updateTouchDisplay();
  updateBatteryIndicator();
  updateGpsLog();
  delay(20);
}