# ESP32-C3 Anemometer + BME280 Logger

This project implements a **Wi‑Fi connected wind + environment logger** using an **ESP32‑C3**, a **pulse-based anemometer**, a **BME280** (temperature, humidity, pressure), and an **SD card** for long-term storage.

It provides:

* Real-time wind speed calculation
* 5-minute aggregated logging aligned to real time (NTP)
* Daily CSV files with automatic retention cleanup
* 10-day chunk CSV files for long-term archiving
* A built-in web UI
* A REST API
* CSV download endpoints
* A streamed **ZIP download of the last N days** (no temp files, low RAM)

---

## Hardware requirements

* **ESP32-C3**
* **Pulse anemometer**
  * Connected to **GPIO 1** (`PULSE_PIN` in `weather_station.ino`)
  * 20 pulses per second = **1.75 m/s**
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

* **Every 5 minutes**

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
├── data/
│   ├── YYYYMMDD.csv          # One per day (5-min rows)
│   └── ...
├── chunks/
│   ├── chunk_YYYYMMDD.csv    # 10-day rolling chunks
│   └── ...
```

### Daily files (`/data`)

* One CSV per day
* Automatically deleted after **RETENTION_DAYS**
* Controlled by:

```cpp
static constexpr int RETENTION_DAYS = 30;
```

### Chunk files (`/chunks`)

* Combined CSV
* Split into **10-day blocks**
* Not automatically deleted (by design)

---

## Storage usage (typical)

Assuming ~80 bytes per row:

* 288 rows/day ≈ **23 KB/day**
* 30 days ≈ **< 1 MB**
* Chunk files grow ~0.8 MB/month

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
* Last 24h wind graph (5-min resolution)
* Daily wind summaries (RAM)
* CSV download section:

  * Daily files
  * 10-day chunk files
  * ZIP download (last N days)

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
  "retention_days": 30
}
```

---

### 2) Last 24h wind buckets

**GET** `/api/5min`

* Returns **288** five-minute buckets
* Used by UI graph

Example:

```json
{
  "now_epoch": 1734158023,
  "buckets": [
    {
      "startEpoch": 1734140200,
      "avgWind": 1.12,
      "maxWind": 2.34,
      "samples": 120
    }
  ]
}
```

---

### 3) Daily wind summaries (RAM)

**GET** `/api/days`

Example:

```json
{
  "days": [
    {
      "dayStartEpoch": 1734057600,
      "dayStartLocal": "2025-12-13 00:00",
      "avgWind": 1.45,
      "maxWind": 7.12
    }
  ]
}
```

---

### 4) List CSV files

**GET** `/api/files?dir=data`
**GET** `/api/files?dir=chunks`

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
* Only allows `/data/*.csv` and `/chunks/*.csv`
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

## Security notes

* File downloads are restricted to:

  * `/data/*.csv`
  * `/chunks/*.csv`
* Path traversal (`..`) blocked
* ZIP streaming avoids RAM exhaustion
* No delete endpoints (read-only API)

---

## Known limitations

* No authentication (LAN-trusted device)
* GPIO0 boot-strap constraints
* Chunk files are not auto-expired (intentional)
* ZIP uses STORE (fast, not compressed)

---

## Extending the project

Common additions:

* Auth token for API
* ZIP including chunk files
* Automatic chunk retention
* MQTT publishing
* Gust detection (short-window peak)

---

## License

Use freely for personal, educational, or commercial projects. No warranty provided.

