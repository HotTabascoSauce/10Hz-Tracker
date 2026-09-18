# 10Hz GPS Tracker

An ESP32-S3 GPS data logger designed as a simple replacement for a Garmin Edge 520 when the primary requirement is high-rate GPS track recording. The tracker configures the GPS receiver for a 100 ms measurement interval, processes the UART stream continuously, and records GPX trackpoints at 10 Hz.

This project is focused on GPS position logging. It does not reproduce the Garmin 520's cycling sensors, navigation, mapping, training metrics, or wireless ecosystem.

## Hardware

- ESP32-S3 2.8-inch development board
- ILI9341 TFT display, 240 x 320
- FT6336 capacitive touch controller
- u-blox Neo-M8N-compatible GPS receiver
- microSD card for GPX recordings
- USB-C connection for power, firmware upload, and serial diagnostics

The board's USB-C port provides USB serial rather than a separate USB-to-UART bridge. Windows 10 and newer can use it directly.

## Pin Map

### GPS UART

| GPS signal | ESP32-S3 GPIO | Notes |
| --- | ---: | --- |
| GPS TXD | 44 | Connect to ESP32 RX |
| GPS RXD | 43 | Connect to ESP32 TX |
| GPS power | Board supply | Use the receiver's required voltage |
| GPS ground | GND | Common ground required |

The firmware starts UART1 at `9600 8N1` and sends the receiver configuration commands during boot. The measurement rate command requests one solution every 100 ms, or 10 Hz.

### Touch controller

| Signal | ESP32-S3 GPIO |
| --- | ---: |
| SDA | 16 |
| SCL | 15 |
| INT | 17 |
| RST | 18 |

### microSD, 4-bit SD_MMC

| Signal | ESP32-S3 GPIO |
| --- | ---: |
| CLK | 38 |
| CMD | 40 |
| D0 | 39 |
| D1 | 41 |
| D2 | 48 |
| D3 | 47 |

### Display and board controls

| Signal | ESP32-S3 GPIO |
| --- | ---: |
| TFT MISO | 13 |
| TFT MOSI | 11 |
| TFT SCLK | 12 |
| TFT CS | 10 |
| TFT DC | 46 |
| TFT RST | Board reset / not driven by firmware |
| Backlight | 45 |
| Boot button | 0 |
| Battery ADC | 9 |

## GPS Tracking Rate

The firmware uses these intervals:

- GPS receiver measurement request: 100 ms, nominal 10 Hz
- GPX logging interval: 100 ms, nominal 10 trackpoints per second
- GPS fix validity timeout: 3 seconds

The serial monitor prints a measured rate every two seconds. A healthy 10 Hz receiver should report approximately:

```text
location_updates=20 rate_hz=10.00
```

The reported rate counts completed location updates, not UART characters. Reception quality, satellite visibility, receiver configuration, and serial bandwidth can reduce the observed rate.

## Recording a Track

1. Insert a FAT-formatted microSD card.
2. Connect the GPS and power the ESP32-S3.
3. Wait for a valid GPS fix. The display then shows the recording control.
4. Touch **START RECORDING**.
5. Touch **STOP RECORDING** when finished.

GPX files are written to the card with names similar to:

```text
/gps_2026-09-18_12-34-56_123.gpx
```

A new recording writes a complete GPX header immediately, followed by the track points. Each point contains latitude, longitude, optional elevation, and a UTC timestamp with centisecond precision. The output uses GPX 1.1 fields and is intended to be accepted by Strava's GPX upload service.

## Serial Monitor

Use 115200 baud:

```text
pio device monitor --baud 115200
```

Example diagnostic output:

```text
Neo-M8N UART ready: RX=44 TX=43
GPS: chars=... sentences=... checksum_errors=0 fix=yes sats=... location_updates=20 rate_hz=10.00
```

## Build and Upload

Install PlatformIO, connect the ESP32-S3, and run:

```text
pio run
pio run --target upload
```

The project environment is `esp32-s3-devkitc-1` and uses the Arduino framework.

## Strava Upload

The generated files use the `.gpx` extension and include:

- XML declaration with UTF-8 encoding
- GPX version 1.1
- GPX 1.1 namespace and schema location
- Creator and metadata timestamp
- A track with one track segment
- A timestamp on every trackpoint

Upload the completed GPX file to Strava as a GPX activity. Existing files made by earlier firmware versions are not rewritten; create a new recording after updating the firmware.
