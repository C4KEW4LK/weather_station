# ESP32-C3 Anemometer + BME280 Logger

This project implements a **Wi‑Fi connected wind + environment logger** using an **ESP32‑C3**, a **pulse-based anemometer**, a **BME280** (temperature, humidity, pressure), and an **SD card** for long-term storage.

It provides:

* Real-time wind speed calculation (1-second sampling)
* Default 1-minute aggregated logging (adjustable), aligned to real time (NTP)
* Daily CSV files with configurable retention (or keep forever)
* A built-in web UI with interactive graphing and zoom
* A REST API with full precision sensor data
* CSV and ZIP download endpoints
* Password-protected file management

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

Wind speed is calculated linearly from pulse rate over a 1-second window.

---

## Time handling

* **Requires WiFi connection** for timestamped logging
* Uses **WiFiManager** for Wi‑Fi configuration
* Uses **NTP** for time sync
* Timezone configured via `TZ` string (default: Australia/Sydney)
* All logging aligned to:
  * **Default 1-minute boundaries** (adjustable via `BUCKET_SECONDS`)
  * **Local midnight rollovers**

---

## Data logging

### Logging interval

* **Every 1 minute** (configurable via `BUCKET_SECONDS` in `weather_station.ino`)
* Wind speed sampled once per second into each bucket

### Logged fields

Each row contains:

| Field         | Description                      |
| ------------- | -------------------------------- |
| `datetime`    | Local time (`YYYY-MM-DD HH:MM`)  |
| `epoch`       | Unix epoch (seconds)             |
| `wind_avg_ms` | Average wind over bucket (m/s)   |
| `wind_max_ms` | Max wind over bucket (m/s)       |
| `temp_c`      | Temperature (°C)                 |
| `hum_rh`      | Relative humidity (%)            |
| `press_hpa`   | Pressure (hPa)                   |
| `samples`     | Number of wind samples in bucket |

---

## SD card layout

```
/
└── data/
    ├── YYYYMMDD.csv          # One per day (1-min rows)
    └── ...
```

### Daily files (`/data`)

* One CSV per day
* Automatically deleted after `RETENTION_DAYS` (default 3600 days, set to 0 to never delete)
* See Configuration options below for details

---

## Configuration options

Key constants in `weather_station.ino`:

```cpp
static const char* API_PASSWORD = "ChangeMe";        // Password for delete operations
static constexpr int BUCKET_SECONDS = 60;            // Logging interval (1 minute)
static constexpr int RETENTION_DAYS = 3600;          // Days to keep CSV files (0 = never delete)
static constexpr int FILES_PER_PAGE = 30;            // Files shown per page in UI
static constexpr int MAX_PLOT_POINTS = 500;          // Max points rendered on plots (for performance)
```

* `API_PASSWORD`: Password for file deletion and clear data operations (default: "ChangeMe")
* `BUCKET_SECONDS`: How often data is logged (default 1 minute = 60 seconds)
* `RETENTION_DAYS`: Auto-delete CSV files older than this many days (set to 0 to never delete)
* `FILES_PER_PAGE`: Number of files shown per page in the CSV download section
* `MAX_PLOT_POINTS`: Maximum number of points rendered on plots to prevent performance issues with large datasets. When zooming, this limit applies only to the visible region, revealing more detail.

---

## Storage usage (typical)

Assuming ~80 bytes per row:

* 1440 rows/day ≈ **100 KB/day**
* 3600 days (≈ 10 years) ≈ **~400 MB** (with default retention setting)

---

## Web UI

Open in browser:

```
http://<device-ip>/
```

### UI features

* **Current readings:**
  * Wind speed (km/h), pulses per second
  * Temperature, humidity, pressure
  * Uptime and RAM usage

* **Last 24h graphs:**
  * Wind speed (average and max lines with dual hover dots)
  * Temperature, humidity, and pressure
  * **Interactive zoom:** Click and drag to zoom, double-click to reset
  * Auto-scaling y-axis when zoomed to show detail in visible range
  * Smart tooltip positioning (auto-adjusts near edges)
  * Performance optimization (max 500 points, configurable)

* **Daily summaries table:**
  * Wind, temperature, humidity, and pressure min/max/avg
  * Scrollable with sticky date column
  * Sorted from most recent to oldest

* **File management:**
  * Paginated CSV file list (30 per page, configurable)
  * Individual file download and deletion (password protected)
  * ZIP download of last N days
  * Clear all SD data button (password protected)

* **System controls:**
  * Reboot device button
  * RAM usage monitor

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

* Returns sensor buckets for the last 24 hours (default 1-minute intervals)
* Includes wind, temperature, humidity, and pressure data
* Uses compact JSON keys to reduce size
* Chunked transfer encoding to handle large responses

Example:

```json
{
  "now_epoch": 1734158023,
  "bucket_seconds": 60,
  "buckets": [
    {
      "t": 1734140200,
      "w": {"a": 1.12, "m": 2.34, "s": 60},
      "T": 23.5,
      "H": 54.2,
      "P": 1012.6
    }
  ]
}
```

**Field mapping:**
- `t` = timestamp (epoch seconds)
- `w.a` = average wind (m/s)
- `w.m` = max wind (m/s)
- `w.s` = sample count
- `T` = temperature (°C)
- `H` = humidity (%)
- `P` = pressure (hPa)

---

### 3) Last 24h sensor buckets (compact format)

**GET** `/api/buckets_ui`

* Internal endpoint used by the web UI
* Array format (no keys) for size reduction
* Each bucket is an array of 7 full-precision numbers
* Chunked transfer encoding with batching for performance

Example:

```json
{
  "now_epoch": 1734158023,
  "bucket_seconds": 60,
  "buckets": [
    [1734140200, 1.12, 2.34, 60, 23.5, 54.2, 1012.6]
  ]
}
```

**Array indices:**
- `[0]` = timestamp (epoch seconds)
- `[1]` = average wind (m/s)
- `[2]` = max wind (m/s)
- `[3]` = sample count
- `[4]` = temperature (°C)
- `[5]` = humidity (%)
- `[6]` = pressure (hPa)

**Example bucket:** `[1734140200, 1.12, 2.34, 60, 23.5, 54.2, 1012.6]`
= 2023-12-14 09:30, 1.12 m/s avg, 2.34 m/s max, 60 samples, 23.5°C, 54.2%, 1012.6 hPa

---

### 4) Daily summaries (RAM)

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

### 5) List CSV files

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

### 6) Download a single CSV

**GET** `/download?path=/data/20251214.csv`

* Forces browser download
* Only allows `/data/*.csv`
* Path traversal blocked

---

### 7) Download last N days as ZIP

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
* `days` is clamped to `1 .. RETENTION_DAYS` (when `RETENTION_DAYS` > 0), or no upper limit (when `RETENTION_DAYS` = 0)

ZIP filename:

```
last_<N>_days.zip
```

---

### 8) Delete a single file (password protected)

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
  -d "path=/data/20251214.csv&pw=ChangeMe"
```

Response:

```json
{"ok": true}
```

---

### 9) Clear all SD data (password protected)

**POST** `/api/clear_data`

* Deletes all CSV files from `/data` directory
* Requires password authentication
* Rate limited: 10 attempts per hour

Parameters:
- `pw`: Password

Example:

```bash
curl -X POST http://<device-ip>/api/clear_data \
  -d "pw=ChangeMe"
```

Response:

```json
{
  "ok": true
}
```

---

### 10) Reboot device

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
  * Configured via `API_PASSWORD` constant (default: "ChangeMe")
  * Rate limiting: 10 attempts per hour
  * Required for:
    * Individual file deletion (`POST /api/delete`)
    * Clear all SD data (`POST /api/clear_data`)

---

## Known limitations

* No authentication (LAN-trusted device)
* ZIP uses STORE (fast, not compressed)

--

