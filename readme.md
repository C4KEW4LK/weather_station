# ESP32-C3 Anemometer + BME280 Logger

This project implements a **Wi‑Fi connected wind + environment logger** using an **ESP32‑C3**, a **pulse-based anemometer**, a **BME280** (temperature, humidity, pressure), and an **SD card** for long-term storage.

It provides:

* Real-time wind speed calculation
* 5-minute aggregated logging aligned to real time (NTP)
* Daily CSV files with automatic retention cleanup
* A built-in web UI with enhanced graphing and pagination
* A REST API
* CSV download endpoints
* A streamed **ZIP download of the last N days** (no temp files, low RAM)

---

## Recent improvements

**Latest changes:**

* **Web UI enhancements:**
  * Added pagination for daily files (30 files per page) with previous/next buttons and page selector
  * Dual hover dots on wind graph (shows both average and max wind)
  * Smart tooltip positioning (auto-switches to left when near right edge)
  * All sensor graphs now show temperature, humidity, and pressure data
  * Added file deletion functionality with password protection
  * Added "Clear all SD data" button with password protection
  * Added reboot button

* **Code improvements:**
  * Added 2ms debounce filter on pulse input to prevent spurious triggers
  * Changed default bucket interval from 1 minute to 5 minutes (`BUCKET_SECONDS = 300`)
  * Wind max calculation now uses actual maximum reading (not second-highest)
  * Daily summaries now include temperature and humidity min/max/avg
  * Removed stale cache logic - always loads fresh from SD on boot
  * Daily summaries now sorted from most recent to oldest in API response

* **API additions:**
  * `/api/buckets` now returns temperature, humidity, and pressure data
  * `/api/delete` (POST) - Delete individual files (password protected)
  * `/api/clear_data` (POST) - Clear all SD data (password protected)
  * `/api/reboot` (POST) - Reboot the device
  * `/api/now` now includes `uptime_ms` field

---

## Hardware requirements

* **ESP32-C3**
* **Pulse anemometer**
  * Connected to **GPIO 3** (`PULSE_PIN` in `weather_station.ino`)
  * 20 pulses per second = **1.75 m/s**
  * 2ms debounce filter implemented in ISR to prevent spurious triggers
* **BME280** (I2C)
  * Default address `0x76` (fallback `0x77`)
  * Uses the board's default SDA/SCL pins (`Wire.begin()`)
* **Micro SD card module** (SPI)
  * CS -> **GPIO 20** (`SD_CS_PIN`), with your board's SPI pins for SCK/MISO/MOSI
* Pull-up on anemometer signal recommended

**Note:** If you change the pulse input to GPIO0, remember it is a boot-strap pin on ESP32-C3 and must not be held low during boot.

---

## Wind speed calculation

```
20 pulses / second = 1.75 m/s
1 pulse / second  = 0.0875 m/s
```

Wind speed is calculated linearly from pulse rate.

---

## Time handling

* Uses **WiFiManager** for Wi‑Fi configuration
* Uses **NTP** for time sync
* Timezone configured via `TZ` string (default: Australia/Sydney)
* All logging aligned to:

  * **5-minute boundaries**
  * **Local midnight rollovers**

---

## Data logging

### Logging interval

* **Every 5 minutes** (configurable via `BUCKET_SECONDS` in `weather_station.ino`)

### Logged fields

Each row contains:

| Field         | Description                      |
| ------------- | -------------------------------- |
| `datetime`    | Local time (`YYYY-MM-DD HH:MM`)  |
| `epoch`       | Unix epoch (seconds)             |
| `wind_avg_ms` | Average wind over 5 min (m/s)    |
| `wind_max_ms` | Max wind over 5 min (m/s)        |
| `temp_c`      | Temperature (°C)                 |
| `hum_rh`      | Relative humidity (%)            |
| `press_hpa`   | Pressure (hPa)                   |
| `samples`     | Number of wind samples in bucket |

---

## SD card layout

```
/
└── data/
    ├── YYYYMMDD.csv          # One per day (5-min rows)
    └── ...
```

### Daily files (`/data`)

* One CSV per day
* Automatically deleted after **RETENTION_DAYS**
* Controlled by:

```cpp
static constexpr int RETENTION_DAYS = 360;
```

---

## Storage usage (typical)

Assuming ~80 bytes per row:

* 288 rows/day ≈ **23 KB/day**
* 360 days ≈ **~8 MB** (with current retention setting)

---

## Web UI

Open in browser:

```
http://<device-ip>/
```

### UI features

* Current wind speed
* PPS and max wind since boot
* Temperature / humidity / pressure
* Last 24h graphs (5-min resolution):
  * Wind speed (with average and max lines, dual hover dots)
  * Temperature
  * Humidity
  * Pressure
* Smart tooltip positioning (auto-adjusts to avoid going off-screen)
* Daily wind summaries (RAM)
* CSV download section:
  * Daily files (paginated - configurable via `FILES_PER_PAGE` constant, default 30 per page)
  * Page navigation controls (previous/next buttons + page selector)
  * File deletion with password protection
  * ZIP download (last N days)
* System controls:
  * Clear all SD data (password protected)
  * Reboot device

---

## REST API specification

### 1) Current status

**GET** `/api/now`

Returns current sensor state.

Example:

```json
{
  "epoch": 1734158023,
  "local_time": "2025-12-14 14:20",
  "wind_pps": 12.4,
  "wind_ms": 1.09,
  "wind_max_since_boot": 6.23,
  "bme280_ok": true,
  "temp_c": 23.41,
  "hum_rh": 54.2,
  "press_hpa": 1012.6,
  "sd_ok": true,
  "uptime_ms": 3600000,
  "retention_days": 30
}
```

---

### 2) Last 24h sensor buckets

**GET** `/api/buckets`

* Returns five-minute buckets for the last 24 hours
* Includes wind, temperature, humidity, and pressure data
* Used by UI graphs

Example:

```json
{
  "now_epoch": 1734158023,
  "bucket_seconds": 300,
  "buckets": [
    {
      "startEpoch": 1734140200,
      "avgWind": 1.12,
      "maxWind": 2.34,
      "samples": 120,
      "avgTempC": 23.5,
      "avgHumRH": 54.2,
      "avgPressHpa": 1012.6
    }
  ]
}
```

---

### 3) Daily summaries (RAM)

**GET** `/api/days`

Returns daily summaries for wind, temperature, and humidity from RAM (last 30 days + current day).

Example:

```json
{
  "days": [
    {
      "dayStartEpoch": 1734057600,
      "dayStartLocal": "2025-12-13 00:00",
      "avgWind": 1.45,
      "maxWind": 7.12,
      "avgTemp": 23.5,
      "minTemp": 18.2,
      "maxTemp": 28.9,
      "avgHum": 54.2,
      "minHum": 42.0,
      "maxHum": 68.5
    }
  ]
}
```

---

### 4) List CSV files

**GET** `/api/files?dir=data`

Example:

```json
{
  "ok": true,
  "dir": "data",
  "files": [
    {
      "path": "/data/20251214.csv",
      "size": 23456
    }
  ]
}
```

---

### 5) Download a single CSV

**GET** `/download?path=/data/20251214.csv`

* Forces browser download
* Only allows `/data/*.csv`
* Path traversal blocked

---

### 6) Download last N days as ZIP

**GET** `/download_zip?days=N`

Examples:

```
/download_zip           -> defaults to 7 days
/download_zip?days=3
/download_zip?days=30
```

* Streams ZIP directly (no temp files)
* Uses ZIP **STORE** mode (no compression)
* Includes only existing `/data/YYYYMMDD.csv` files
* `days` is clamped to `1 .. RETENTION_DAYS`

ZIP filename:

```
last_<N>_days.zip
```

---

### 7) Delete a single file (password protected)

**POST** `/api/delete`

* Deletes a single CSV file from `/data`
* Requires password authentication
* Rate limited: 10 attempts per hour

Parameters:
- `path`: File path (e.g., `/data/20251214.csv`)
- `pw`: Password

Example:

```bash
curl -X POST http://<device-ip>/api/delete \
  -d "path=/data/20251214.csv&pw=yesplease"
```

Response:

```json
{"ok": true}
```

---

### 8) Clear all SD data (password protected)

**POST** `/api/clear_data`

* Deletes all CSV files from `/data` directory
* Requires password authentication
* Rate limited: 10 attempts per hour

Parameters:
- `pw`: Password

Example:

```bash
curl -X POST http://<device-ip>/api/clear_data \
  -d "pw=yesplease"
```

Response:

```json
{
  "ok": true
}
```

---

### 9) Reboot device

**POST** `/api/reboot`

* Reboots the ESP32 device
* No authentication required

Example:

```bash
curl -X POST http://<device-ip>/api/reboot
```

Response:

```json
{"ok": true}
```

---

## Security notes

* File downloads are restricted to `/data/*.csv`
* Path traversal (`..`) blocked
* ZIP streaming avoids RAM exhaustion
* Delete operations are password-protected:
  * Default password: `yesplease` (change in code before deployment)
  * Rate limiting: 10 attempts per hour
  * Required for:
    * Individual file deletion (`POST /api/delete`)
    * Clear all SD data (`POST /api/clear_data`)

---

## Known limitations

* No authentication (LAN-trusted device)
* GPIO0 boot-strap constraints
* ZIP uses STORE (fast, not compressed)

---

## Extending the project

Common additions:

* Auth token for API
* MQTT publishing
* Gust detection (short-window peak)
* Compressed ZIP downloads

---

## License

Use freely for personal, educational, or commercial projects. No warranty provided.

