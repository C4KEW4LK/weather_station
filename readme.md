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
  * Added RAM usage pill showing free memory and percentage used
  * Streamlined plot layout (removed card borders to save space)
  * Daily summaries table now has visual fade indicator when scrollable
  * Sticky date column in daily summaries with clear border separator
  * Interactive plot zooming:
    * Click and drag to zoom into a time region
    * Double-click to reset zoom
    * Auto-scales y-axis when zoomed to show detail in visible range
    * Configurable point limit (default 500) for performance with large datasets
    * Zoom reveals more detail by downsampling only the visible region
  * Plot rendering improvements:
    * Clip paths prevent plot lines from overlapping y-axis labels
    * Fixed x-axis font stretching on viewport resize
    * 10 x-axis ticks for cleaner time labels
    * Plots limited to 330px minimum width with responsive layout

* **Code improvements:**
  * Added 2ms debounce filter on pulse input to prevent spurious triggers
  * Changed default bucket interval from 1 minute to 5 minutes (`BUCKET_SECONDS = 300`)
  * Wind max calculation now uses actual maximum reading (not second-highest)
  * Daily summaries now include temperature and humidity min/max/avg
  * Removed stale cache logic - always loads fresh from SD on boot
  * Daily summaries now sorted from most recent to oldest in API response
  * Fixed invalid JSON generation in `/api/buckets` endpoint (empty bucket validation)
  * Frontend JSON parsing now handles malformed responses gracefully
  * Optimized RAM usage with compact data storage:
    * Wind: stored as m/s * 2 (uint8_t, 0-127 m/s range) - saves 6 bytes per bucket
    * Temperature: stored as int8_t in °C - saves 3 bytes per bucket
    * Humidity: stored as uint8_t (0-100) - saves 3 bytes per bucket
    * Pressure: stored as int8_t offset from 1000 hPa - saves 3 bytes per bucket
    * **Total savings: ~4.3 KB RAM** for 24h of 5-minute buckets
  * Separated public API (`/api/buckets`) and internal UI API (`/api/buckets_ui`)
    * Public API returns full precision floats
    * Internal UI API uses compact format to save bandwidth

* **API additions:**
  * `/api/buckets` now returns temperature, humidity, and pressure data with full precision
  * `/api/buckets_ui` - Compact format endpoint for internal UI use (saves bandwidth)
  * `/api/delete` (POST) - Delete individual files (password protected)
  * `/api/clear_data` (POST) - Clear all SD data (password protected)
  * `/api/reboot` (POST) - Reboot the device
  * `/api/now` now includes `uptime_ms`, `free_heap`, and `heap_size` fields for RAM monitoring

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
* Automatically deleted after `RETENTION_DAYS` (default 360 days)
* See Configuration options below for details

---

## Configuration options

Key constants in `weather_station.ino`:

```cpp
static constexpr int BUCKET_SECONDS = 300;      // Logging interval (5 minutes)
static constexpr int RETENTION_DAYS = 360;      // Days to keep CSV files
static constexpr int FILES_PER_PAGE = 30;       // Files shown per page in UI
static constexpr int MAX_PLOT_POINTS = 500;     // Max points rendered on plots (for performance)
```

* `BUCKET_SECONDS`: How often data is logged (default 5 minutes = 300 seconds)
* `RETENTION_DAYS`: Auto-delete CSV files older than this many days
* `FILES_PER_PAGE`: Number of files shown per page in the CSV download section
* `MAX_PLOT_POINTS`: Maximum number of points rendered on plots to prevent performance issues with large datasets. When zooming, this limit applies only to the visible region, revealing more detail.

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
  * Interactive zoom (click and drag, double-click to reset)
  * Auto-scaling y-axis when zoomed
  * Performance optimization (configurable via `MAX_PLOT_POINTS` constant, default 500)
* Smart tooltip positioning (auto-adjusts to avoid going off-screen)
* Daily summaries table:
  * Scrollable with sticky date column and visual fade indicator
  * Shows wind, temperature, humidity, and pressure data
  * Sorted from most recent to oldest
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
* Returns full precision floats for all values
* Uses compact JSON keys to reduce bandwidth
* Chunked transfer encoding to handle large responses

Example:

```json
{
  "now_epoch": 1734158023,
  "bucket_seconds": 300,
  "buckets": [
    {
      "t": 1734140200,
      "w": {"a": 1.12, "m": 2.34, "s": 120},
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
```

---

### 3) Last 24h sensor buckets (compact format)

**GET** `/api/buckets_ui`

* Internal endpoint used by the web UI
* Array format (no keys) for bandwidth savings
* Each bucket is an array of 7 full-precision numbers
* Chunked transfer encoding with batching for performance

Example:

```json
{
  "now_epoch": 1734158023,
  "bucket_seconds": 300,
  "buckets": [
    [1734140200, 1.12, 2.34, 120, 23.5, 54.2, 1012.6]
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

**Example bucket:** `[1734140200, 1.12, 2.34, 120, 23.5, 54.2, 1012.6]`
= 2023-12-14 09:30, 1.12 m/s avg, 2.34 m/s max, 120 samples, 23.5°C, 54.2%, 1012.6 hPa
```

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
* `days` is clamped to `1 .. RETENTION_DAYS`

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
  -d "path=/data/20251214.csv&pw=yesplease"
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
  -d "pw=yesplease"
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

