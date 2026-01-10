# Adding a New Sensor

A simplified guide on how to add a new sensor.

## The Pattern

Adding a sensor involves 4 main areas:

1. **Configure** - Add settings to `config.h`
2. **Read** - Poll the sensor and store the value
3. **Log** - Add to data structures and CSV
4. **Display** - Add to API and UI

## Example: CO2 Sensor

Let's add a CO2 sensor step by step.

---

## 1. Configure (config.h)

Add your sensor's settings:

```cpp
// In config.h
namespace CO2Config {
  static constexpr bool ENABLE = true;
  static constexpr uint8_t I2C_ADDR = 0x61;
}
```

---

## 2. Read (weather_station.ino)

### Add globals and initialization

```cpp
// At top of file
#include <Adafruit_SCD30.h>
Adafruit_SCD30 scd30;
static bool gCO2Ok = false;
static float gLastCO2 = NAN;
static uint32_t gLastCO2ReadMs = 0;

// In setup()
if (CO2Config::ENABLE && scd30.begin(CO2Config::I2C_ADDR)) {
  gCO2Ok = true;
  Serial.println("SCD30 initialized");
}
```

### Add polling function

```cpp
void pollCO2Sensor() {
  if (!CO2Config::ENABLE || !gCO2Ok) return;

  uint32_t nowMs = millis();
  if (nowMs - gLastCO2ReadMs < 2000) return;  // Every 2 seconds
  gLastCO2ReadMs = nowMs;

  if (scd30.dataReady() && scd30.read()) {
    gLastCO2 = scd30.CO2;
  }
}

// In loop(), call it
void loop() {
  // ... existing code ...
  pollCO2Sensor();
}
```

---

## 3. Log (weather_station.ino)

### Add to BucketSample struct

```cpp
struct BucketSample {
  // ... existing fields ...
  float avgCO2;  // Add this
};
```

### Add accumulator and averaging

```cpp
// At top with other accumulators
static float gAccumCO2 = 0.0f;
static uint32_t gAccumCO2Samples = 0;

// In bucketSample() function
if (isfinite(gLastCO2)) {
  gAccumCO2 += gLastCO2;
  gAccumCO2Samples++;
}

// When finalizing bucket
bucket.avgCO2 = (gAccumCO2Samples > 0) ? (gAccumCO2 / gAccumCO2Samples) : NAN;
gAccumCO2 = 0.0f;
gAccumCO2Samples = 0;
```

### Add to CSV

```cpp
// Update CSV header
"datetime,epoch,...,pm10,co2_ppm,samples"

// In writeBucketToCSV()
f.print(b.avgCO2, 1); f.print(",");
```

---

## 4. Display

### Add to /api/now

```cpp
// In handleApiNow()
out += ",\"co2_ok\":" + String(gCO2Ok ? "true" : "false");
out += ",\"co2_ppm\":" + (isfinite(gLastCO2) ? String(gLastCO2, 1) : "null");
```

### Add to /api/buckets

```cpp
// In buildBucketJson()
out += ",\"co2\":" + numOrNull(b.avgCO2, 1);

// In buildBucketJsonCompact() - add as [10]
out += "," + numOrNull(b.avgCO2, 1);
```

### Add plot to config.h

```cpp
// In PLOTS[] array
{
  "co2",
  "CO2 Level",
  "ppm",
  1.0f,
  {
    {"avgCO2", "#9C27B0", "CO2", true},
    {nullptr, nullptr, nullptr, false}
  },
  1
}
```

### Add table columns to config.h

```cpp
// In TABLE_COLUMNS[] array
{"avgCO2", "CO2 avg", "ppm", 1.0f, 1, "#f8f8f8", "Air Quality"}
```

### Add to DaySummary (optional)

If you want daily min/max/avg:

```cpp
struct DaySummary {
  // ... existing fields ...
  float avgCO2, minCO2, maxCO2;
  uint32_t co2Samples;
};

// Accumulate and output in /api/days
```

---

## Quick Checklist

For any new sensor:

**Backend (weather_station.ino):**
- [ ] Add config namespace to `config.h`
- [ ] Add library include and globals
- [ ] Initialize in `setup()`
- [ ] Create poll function, call in `loop()`
- [ ] Add field to `BucketSample`
- [ ] Add accumulator variables
- [ ] Accumulate in bucket sample function
- [ ] Average when finalizing bucket
- [ ] Add to CSV header and row writing
- [ ] Add to `/api/now`, `/api/buckets`, `/api/buckets_compact`

**Frontend (config.h):**
- [ ] Add plot to `PLOTS[]` array
- [ ] Add columns to `TABLE_COLUMNS[]` array

That's it! The UI automatically generates from the config.

---