/*
  ESP32-C3 Anemometer + BME280 + SD CSV logger (NTP + WiFiManager)
  + Web UI + REST API + CSV download + "last N days ZIP" download (streamed)

  Wind:
  - Pulse input: GPIO0 interrupt
  - Calibration: 20 pps = 1.75 m/s => m/s = pps * (1.75/20)

  Logging:
  - Bucket-aligned logs (configurable duration, NTP time)
  - /data/YYYYMMDD.csv (daily, retained for LogConfig::RETENTION_DAYS)

  Downloads:
  - List:     /api/files?dir=data
  - CSV:      /download?path=/data/20251214.csv
  - ZIP:      /download_zip?days=7  (streams ZIP of last N daily /data files, STORE mode)

  Notes:
  - GPIO0 is a boot strap pin on ESP32-C3; ensure sensor doesn't hold it LOW at boot.
  - SD pin mapping varies by board/module; set SDConfig::CS_PIN and optionally SPI.begin(...) pins.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <time.h>

#include <Wire.h>
#include <Adafruit_BME280.h>

#include <SPI.h>
#include <SD.h>
#include <ArduinoOTA.h>
#include <pgmspace.h>

#include <vector>
#include <algorithm>
#include "config.h"
#include "upload_page.h"
#include "api_help_page.h"

// ------------------- UART FOR PMS5003 -------------------
// ESP32-C3 doesn't have Serial2 predefined, create custom HardwareSerial
HardwareSerial Serial2(1);  // Use UART1 for PMS5003

// ------------------- FORWARD DECLARATIONS -------------------

struct BucketSample;
struct DaySummary;
void saveDaySummariesCache(const DaySummary* curDay, bool hasCurDay);
bool loadDaySummariesCache();
void pushBucketSample(const BucketSample& b);
void rebuildTodayAggregates();

// ------------------- DATA -------------------

struct BucketSample {
  time_t startEpoch;
  float avgWind;         // m/s (NAN = invalid)
  float maxWind;         // m/s (NAN = invalid)
  uint32_t samples;
  float avgTempC;        // °C (NAN = invalid)
  float avgHumRH;        // % (NAN = invalid)
  float avgPressHpa;     // hPa (NAN = invalid)
  float avgPM1;          // μg/m³ (NAN = invalid)
  float avgPM25;         // μg/m³ (NAN = invalid)
  float avgPM10;         // μg/m³ (NAN = invalid)
};

struct DaySummary {
  time_t dayStartEpoch; // local midnight
  float avgWind;
  float maxWind;
  float avgTemp;
  float minTemp;
  float maxTemp;
  float avgHum;
  float minHum;
  float maxHum;
  float avgPress;
  float minPress;
  float maxPress;
  float avgPM1;
  float maxPM1;
  float avgPM25;
  float maxPM25;
  float avgPM10;
  float maxPM10;
};

static BucketSample gBuckets[LogConfig::BUCKETS_24H];
static int gBucketWrite = 0;

static DaySummary gDays[LogConfig::DAYS_HISTORY];
static int gDayWrite = 0;
static uint32_t gDaysCount = 0;

// wind pulses
static volatile uint32_t gPulseCount = 0;
static volatile uint32_t gLastPulseMicros = 0;
static float gNowWindMS = 0.0f;

// timing / bucket accumulators
static uint32_t gLastPpsMillis = 0;
static time_t   gCurrentBucketStart = 0;
static float    gBucketWindSum = 0.0f;
static float    gBucketWindMax1 = 0.0f; // highest observed in bucket
static float    gBucketWindMax2 = 0.0f; // second-highest observed in bucket
static uint32_t gBucketSamples = 0;
static uint32_t gBucketPulseCount = 0;
static uint32_t gBucketPulseElapsedMs = 0;
static float    gBucketTempSum = 0.0f;
static float    gBucketHumSum = 0.0f;
static float    gBucketPressSumPa = 0.0f;
static uint32_t gBucketEnvSamples = 0;
static float    gBucketPM1Sum = 0.0f;
static float    gBucketPM25Sum = 0.0f;
static float    gBucketPM10Sum = 0.0f;
static uint32_t gBucketPmSamples = 0;

// daily rollup
static time_t   gTodayMidnightEpoch = 0;
static float    gTodaySumOfBucketAvgs = 0.0f;
static uint32_t gTodayBucketCount = 0;
static float    gTodayMax = 0.0f;
static float    gTodayTempSum = 0.0f;
static uint32_t gTodayTempCount = 0;
static float    gTodayTempMin = NAN;
static float    gTodayTempMax = NAN;
static float    gTodayHumSum = 0.0f;
static uint32_t gTodayHumCount = 0;
static float    gTodayHumMin = NAN;
static float    gTodayHumMax = NAN;
static float    gTodayPressSum = 0.0f;
static uint32_t gTodayPressCount = 0;
static float    gTodayPressMin = NAN;
static float    gTodayPressMax = NAN;
static float    gTodayPM1Sum = 0.0f;
static uint32_t gTodayPM1Count = 0;
static float    gTodayPM1Max = NAN;
static float    gTodayPM25Sum = 0.0f;
static uint32_t gTodayPM25Count = 0;
static float    gTodayPM25Max = NAN;
static float    gTodayPM10Sum = 0.0f;
static uint32_t gTodayPM10Count = 0;
static float    gTodayPM10Max = NAN;

// Auth / rate limit for password-protected endpoints
static int      gPwAttempts = 0;
static time_t   gPwWindowStart = 0;

// ------------------- HELPERS -------------------

static void clearTodayAggregates() {
  gTodaySumOfBucketAvgs = 0.0f;
  gTodayBucketCount = 0;
  gTodayMax = 0.0f;
  gTodayTempSum = 0.0f;
  gTodayTempCount = 0;
  gTodayTempMin = NAN;
  gTodayTempMax = NAN;
  gTodayHumSum = 0.0f;
  gTodayHumCount = 0;
  gTodayHumMin = NAN;
  gTodayHumMax = NAN;
  gTodayPressSum = 0.0f;
  gTodayPressCount = 0;
  gTodayPressMin = NAN;
  gTodayPressMax = NAN;
  gTodayPM1Sum = 0.0f;
  gTodayPM1Count = 0;
  gTodayPM1Max = NAN;
  gTodayPM25Sum = 0.0f;
  gTodayPM25Count = 0;
  gTodayPM25Max = NAN;
  gTodayPM10Sum = 0.0f;
  gTodayPM10Count = 0;
  gTodayPM10Max = NAN;
}

// BME280 latest
static Adafruit_BME280 bme;
static bool  gBmeOk = false;
static float gTempC = NAN;
static float gHumRH = NAN;
static float gPressurePa = NAN;
static uint32_t gLastBmePollMillis = 0;

// PMS5003 latest readings
static const int PMS_FRAME_SIZE = 32;
static const uint8_t PMS_HEADER_1 = 0x42;
static const uint8_t PMS_HEADER_2 = 0x4d;
static uint8_t gPmsFrameBuffer[32];
static int gPmsFrameIndex = 0;
static bool gPmsOk = false;
static float gPM1 = NAN;
static float gPM25 = NAN;
static float gPM10 = NAN;
static uint32_t gLastPmsPollMillis = 0;

// SD status
static bool gSdOk = false;
static constexpr uint32_t FILES_CACHE_TTL_MS = 30000; // 30s reuse to avoid slow SD scans on refresh
static String gFilesCacheDataJson;
static uint32_t gFilesCacheDataMs = 0;

// Web
static WebServer server(80);

// ------------------- ISR -------------------
void IRAM_ATTR onPulse() {
  uint32_t now = micros();
  uint32_t elapsed = now - gLastPulseMicros;

  // Debounce: ignore pulses closer than 2ms (2000 microseconds)
  if (elapsed < 2000) return;

  gLastPulseMicros = now;
  gPulseCount++;
}

// ------------------- TIME HELPERS -------------------
static inline bool timeIsValid(time_t t) { return t > 1577836800; } // > 2020-01-01
static inline time_t epochNow() { return time(nullptr); }

time_t localMidnight(time_t nowEpoch) {
  struct tm tmLocal;
  localtime_r(&nowEpoch, &tmLocal);
  tmLocal.tm_hour = 0; tmLocal.tm_min = 0; tmLocal.tm_sec = 0; tmLocal.tm_isdst = -1;
  return mktime(&tmLocal);
}

static void resetPwWindowIfNeeded() {
  time_t now = epochNow();
  if (!timeIsValid(gPwWindowStart) || (now - gPwWindowStart) >= 3600) {
    gPwWindowStart = now;
    gPwAttempts = 0;
  }
}

static bool verifyPassword(const String& pw, bool& rateLimited) {
  resetPwWindowIfNeeded();
  rateLimited = false;
  if (gPwAttempts >= 10) {
    rateLimited = true;
    return false;
  }
  if (pw == API_PASSWORD) {
    gPwAttempts = 0;
    gPwWindowStart = epochNow();
    return true;
  }
  gPwAttempts++;
  if (gPwAttempts >= 10) rateLimited = true;
  return false;
}

time_t floorToBucketBoundaryLocal(time_t nowEpoch) {
  struct tm tmLocal;
  localtime_r(&nowEpoch, &tmLocal);
  tmLocal.tm_sec = 0;
  int bucketMinutes = LogConfig::BUCKET_SECONDS / 60;
  int bucketSeconds = LogConfig::BUCKET_SECONDS % 60;
  if (bucketMinutes > 0) {
    tmLocal.tm_min = (tmLocal.tm_min / bucketMinutes) * bucketMinutes;
    tmLocal.tm_sec = (bucketSeconds > 0) ? (tmLocal.tm_sec / bucketSeconds) * bucketSeconds : 0;
  } else {
    tmLocal.tm_sec = (tmLocal.tm_sec / LogConfig::BUCKET_SECONDS) * LogConfig::BUCKET_SECONDS;
  }
  return mktime(&tmLocal);
}

String fmtLocal(time_t t) {
  struct tm tmLocal;
  localtime_r(&t, &tmLocal);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmLocal);
  return String(buf);
}

String ymdString(time_t t) {
  struct tm tmLocal;
  localtime_r(&t, &tmLocal);
  char buf[16];
  strftime(buf, sizeof(buf), "%Y%m%d", &tmLocal);
  return String(buf);
}

time_t subtractDaysLocalMidnight(time_t tMidnightLocal, int days) {
  struct tm tmLocal;
  localtime_r(&tMidnightLocal, &tmLocal);
  tmLocal.tm_mday -= days;
  tmLocal.tm_hour = 0; tmLocal.tm_min = 0; tmLocal.tm_sec = 0; tmLocal.tm_isdst = -1;
  return mktime(&tmLocal);
}

// ------------------- SD HELPERS -------------------

bool ensureDir(const char* path) {
  if (!gSdOk) return false;
  if (SD.exists(path)) return true;
  return SD.mkdir(path);
}

bool appendLine(const String& path, const String& line, const String& headerIfNew = "") {
  if (!gSdOk) return false;

  bool exists = SD.exists(path.c_str());
  File f = SD.open(path.c_str(), FILE_APPEND);
  if (!f) return false;

  if (!exists && headerIfNew.length()) {
    f.print(headerIfNew);
    if (headerIfNew[headerIfNew.length()-1] != '\n') f.print("\n");
  }
  f.print(line);
  if (line.length() && line[line.length()-1] != '\n') f.print("\n");
  f.close();
  return true;
}

void deleteOldDailyFileIfNeeded(time_t todayMidnightLocal) {
  // If LogConfig::RETENTION_DAYS is 0, never delete files
  if (LogConfig::RETENTION_DAYS == 0) return;

  time_t oldMidnight = subtractDaysLocalMidnight(todayMidnightLocal, LogConfig::RETENTION_DAYS + 1);
  String ymd = ymdString(oldMidnight);
  String dataPath = String("/data/") + ymd + ".csv";
  if (gSdOk && SD.exists(dataPath.c_str())) SD.remove(dataPath.c_str());
}

void logBucketToSD(const BucketSample& b) {
  if (!gSdOk) return;

  ensureDir("/data");

  time_t bucketMidnightLocal = localMidnight(b.startEpoch);
  String ymd = ymdString(bucketMidnightLocal);
  String dailyFile = String("/data/") + ymd + ".csv";

  String header =
    "datetime,epoch,wind_avg_ms,wind_max_ms,temp_c,hum_rh,press_hpa,pm1,pm25,pm10,samples";

  String line =
    fmtLocal(b.startEpoch) + "," +
    String((uint32_t)b.startEpoch) + "," +
    String(b.avgWind, 3) + "," +
    String(b.maxWind, 3) + "," +
    String(b.avgTempC, 2) + "," +
    String(b.avgHumRH, 2) + "," +
    String(b.avgPressHpa, 2) + "," +
    String(isfinite(b.avgPM1) ? String(b.avgPM1, 1) : String("")) + "," +
    String(isfinite(b.avgPM25) ? String(b.avgPM25, 1) : String("")) + "," +
    String(isfinite(b.avgPM10) ? String(b.avgPM10, 1) : String("")) + "," +
    String(b.samples);

  appendLine(dailyFile, line, header);
}

static bool deleteDirFiles(const char* dirPath) {
  File dir = SD.open(dirPath);
  if (!dir) return false;
  bool ok = true;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (entry.isDirectory()) {
      entry.close();
      continue;
    }
    String name = entry.name();
    entry.close();
    if (!SD.remove(name.c_str())) ok = false;
  }
  dir.close();
  return ok;
}

static float parseFloatOrNan(const String& s) {
  if (!s.length()) return NAN;
  String trimmed = s;
  trimmed.trim();
  if (!trimmed.length()) return NAN;
  // Check for common invalid markers
  if (trimmed == "nan" || trimmed == "NaN" || trimmed == "NAN") return NAN;
  if (trimmed == "null" || trimmed == "NULL") return NAN;
  if (trimmed == "-" || trimmed == "--") return NAN;
  // toFloat() returns 0 for invalid input, so check if it's actually a valid number
  // by verifying the string starts with a digit, minus, or decimal point
  char first = trimmed.charAt(0);
  if (first != '-' && first != '.' && !isdigit(first)) return NAN;
  return trimmed.toFloat();
}

static bool splitCSVLine(const String& line, String parts[], int expectedParts) {
  int start = 0;
  int idx = 0;
  while (idx < expectedParts - 1) {
    int comma = line.indexOf(',', start);
    if (comma < 0) break;
    parts[idx++] = line.substring(start, comma);
    start = comma + 1;
  }
  parts[idx++] = line.substring(start);
  return idx == expectedParts;
}

static void invalidateFilesCache() {
  gFilesCacheDataMs = 0;
  gFilesCacheDataJson = "";
}

void loadRecentBucketsFromSD(time_t nowEpoch) {
  if (!gSdOk || !timeIsValid(nowEpoch)) return;

  time_t cutoff = nowEpoch - 86400;
  std::vector<BucketSample> loaded;
  loaded.reserve(LogConfig::BUCKETS_24H);

  auto loadFile = [&](time_t dayMidnightLocal) {
    String path = String("/data/") + ymdString(dayMidnightLocal) + ".csv";
    if (!SD.exists(path.c_str())) return;
    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) return;
    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (!line.length()) continue;
      if (line.startsWith("datetime")) continue;

      // Count actual columns
      int commaCount = 0;
      for (int i = 0; i < line.length(); i++) {
        if (line[i] == ',') commaCount++;
      }
      int numCols = commaCount + 1;

      String parts[11];
      splitCSVLine(line, parts, numCols < 11 ? numCols : 11);

      if (numCols < 8) continue;  // Need at least 8 fields
      BucketSample b{};
      b.startEpoch = (time_t)parts[1].toInt();
      if (!timeIsValid(b.startEpoch) || b.startEpoch < cutoff) continue;
      // Skip buckets that don't align with current LogConfig::BUCKET_SECONDS setting
      if (b.startEpoch % LogConfig::BUCKET_SECONDS != 0) continue;
      // Load floats directly from CSV
      b.avgWind = parseFloatOrNan(parts[2]);
      b.maxWind = parseFloatOrNan(parts[3]);
      b.avgTempC = parseFloatOrNan(parts[4]);
      b.avgHumRH = parseFloatOrNan(parts[5]);
      b.avgPressHpa = parseFloatOrNan(parts[6]);
      // PM fields (optional for backward compatibility)
      b.avgPM1 = (numCols > 7) ? parseFloatOrNan(parts[7]) : NAN;
      b.avgPM25 = (numCols > 8) ? parseFloatOrNan(parts[8]) : NAN;
      b.avgPM10 = (numCols > 9) ? parseFloatOrNan(parts[9]) : NAN;
      b.samples = (uint32_t)std::max<long>(0, parts[numCols > 10 ? 10 : 7].toInt());
      loaded.push_back(b);
    }
    f.close();
  };

  time_t todayMidnight = localMidnight(nowEpoch);
  loadFile(todayMidnight);
  loadFile(subtractDaysLocalMidnight(todayMidnight, 1)); // bring in late-night buckets from previous day

  if (loaded.empty()) return;

  std::sort(loaded.begin(), loaded.end(), [](const BucketSample& a, const BucketSample& b) {
    return a.startEpoch < b.startEpoch;
  });

  if (loaded.size() > (size_t)LogConfig::BUCKETS_24H) {
    loaded.erase(loaded.begin(), loaded.begin() + (loaded.size() - LogConfig::BUCKETS_24H));
  }

  memset(gBuckets, 0, sizeof(gBuckets));
  gBucketWrite = 0;
  for (const auto& b : loaded) {
    pushBucketSample(b);
  }

  rebuildTodayAggregates();
}

// ------------------- BME280 -------------------
// Convert station pressure to Mean Sea Level Pressure (MSLP)
// Using the ICAO Standard Barometric Formula with measured temperature
float convertToMSLP(float stationPressurePa, float tempC, float altitudeM) {
  if (!isfinite(stationPressurePa) || !isfinite(tempC)) return stationPressurePa;
  
  // If altitude is essentially zero, return station pressure
  if (altitudeM > -0.1f && altitudeM < 0.1f) return stationPressurePa;

  const float L = 0.0065f;        // Standard lapse rate (K/m)
  const float T_station = tempC + 273.15f; // Station temperature in Kelvin
  const float exponent = 5.25588f; // g / (R * L)

  // Use the station temperature directly in the ratio
  float ratio = 1.0f + (L * altitudeM / T_station);
  float mslpPa = stationPressurePa * pow(ratio, exponent);

  return mslpPa;
}

void pollBME() {
  if (!BME280Config::ENABLE || !gBmeOk) return;
  gTempC = bme.readTemperature();
  float stationPressurePa = bme.readPressure();
  gHumRH = bme.readHumidity();

  // Convert to Mean Sea Level Pressure using station altitude
  gPressurePa = convertToMSLP(stationPressurePa, gTempC, BME280Config::ALTITUDE_METERS);

  // Accumulate for per-bucket averages
  if (timeIsValid(gCurrentBucketStart)) {
    gBucketTempSum += gTempC;
    gBucketHumSum += gHumRH;
    gBucketPressSumPa += gPressurePa;
    gBucketEnvSamples++;
  }
}

// ------------------- PMS5003 -------------------
void pollPMS() {
  if (!PMS5003Config::ENABLE) return;

  while (Serial2.available()) {
    uint8_t byte = Serial2.read();

    // Frame synchronization - looking for header 0x42 0x4d
    if (gPmsFrameIndex == 0 && byte != PMS_HEADER_1) continue;
    if (gPmsFrameIndex == 1 && byte != PMS_HEADER_2) {
      gPmsFrameIndex = 0;
      continue;
    }

    gPmsFrameBuffer[gPmsFrameIndex++] = byte;

    if (gPmsFrameIndex == PMS_FRAME_SIZE) {
      // Validate checksum (sum of bytes 0-29)
      uint16_t checksum = 0;
      for (int i = 0; i < 30; i++) {
        checksum += gPmsFrameBuffer[i];
      }
      uint16_t frameChecksum = ((uint16_t)gPmsFrameBuffer[30] << 8) | gPmsFrameBuffer[31];

      if (checksum == frameChecksum) {
        // Extract PM values (CF=1 readings, proven working)
        // Bytes 4-5: PM1.0, 6-7: PM2.5, 8-9: PM10
        uint16_t pm1_raw = ((uint16_t)gPmsFrameBuffer[4] << 8) | gPmsFrameBuffer[5];
        uint16_t pm25_raw = ((uint16_t)gPmsFrameBuffer[6] << 8) | gPmsFrameBuffer[7];
        uint16_t pm10_raw = ((uint16_t)gPmsFrameBuffer[8] << 8) | gPmsFrameBuffer[9];

        gPM1 = (float)pm1_raw;
        gPM25 = (float)pm25_raw;
        gPM10 = (float)pm10_raw;

        // Accumulate for per-bucket averages
        if (timeIsValid(gCurrentBucketStart)) {
          gBucketPM1Sum += gPM1;
          gBucketPM25Sum += gPM25;
          gBucketPM10Sum += gPM10;
          gBucketPmSamples++;
        }
      }

      gPmsFrameIndex = 0;
    }
  }
}

// ------------------- BUCKETS / DAILY -------------------

void startBucketAt(time_t bucketStart) {
  gCurrentBucketStart = bucketStart;
  gBucketWindSum = 0.0f;
  gBucketWindMax1 = 0.0f;
  gBucketWindMax2 = 0.0f;
  gBucketSamples = 0;
  gBucketPulseCount = 0;
  gBucketPulseElapsedMs = 0;
  gBucketTempSum = 0.0f;
  gBucketHumSum = 0.0f;
  gBucketPressSumPa = 0.0f;
  gBucketEnvSamples = 0;
  gBucketPM1Sum = 0.0f;
  gBucketPM25Sum = 0.0f;
  gBucketPM10Sum = 0.0f;
  gBucketPmSamples = 0;
}

void pushBucketSample(const BucketSample& b) {
  gBuckets[gBucketWrite] = b;
  gBucketWrite = (gBucketWrite + 1) % LogConfig::BUCKETS_24H;
}

static void accumulateTodayFromBucket(const BucketSample& b) {
  // Use full precision floats for daily summary
  if (isfinite(b.avgWind)) {
    gTodaySumOfBucketAvgs += b.avgWind;
    gTodayBucketCount++;
  }
  if (isfinite(b.maxWind)) {
    if (b.maxWind > gTodayMax) gTodayMax = b.maxWind;
  }
  if (isfinite(b.avgTempC)) {
    if (!isfinite(gTodayTempMin) || b.avgTempC < gTodayTempMin) gTodayTempMin = b.avgTempC;
    if (!isfinite(gTodayTempMax) || b.avgTempC > gTodayTempMax) gTodayTempMax = b.avgTempC;
    gTodayTempSum += b.avgTempC;
    gTodayTempCount++;
  }
  if (isfinite(b.avgHumRH)) {
    if (!isfinite(gTodayHumMin) || b.avgHumRH < gTodayHumMin) gTodayHumMin = b.avgHumRH;
    if (!isfinite(gTodayHumMax) || b.avgHumRH > gTodayHumMax) gTodayHumMax = b.avgHumRH;
    gTodayHumSum += b.avgHumRH;
    gTodayHumCount++;
  }
  if (isfinite(b.avgPressHpa)) {
    if (!isfinite(gTodayPressMin) || b.avgPressHpa < gTodayPressMin) gTodayPressMin = b.avgPressHpa;
    if (!isfinite(gTodayPressMax) || b.avgPressHpa > gTodayPressMax) gTodayPressMax = b.avgPressHpa;
    gTodayPressSum += b.avgPressHpa;
    gTodayPressCount++;
  }
  // PM accumulation (average only, no min/max)
  if (isfinite(b.avgPM1)) {
    gTodayPM1Sum += b.avgPM1;
    gTodayPM1Count++;
    if (!isfinite(gTodayPM1Max) || b.avgPM1 > gTodayPM1Max) gTodayPM1Max = b.avgPM1;
  }
  if (isfinite(b.avgPM25)) {
    gTodayPM25Sum += b.avgPM25;
    gTodayPM25Count++;
    if (!isfinite(gTodayPM25Max) || b.avgPM25 > gTodayPM25Max) gTodayPM25Max = b.avgPM25;
  }
  if (isfinite(b.avgPM10)) {
    gTodayPM10Sum += b.avgPM10;
    gTodayPM10Count++;
    if (!isfinite(gTodayPM10Max) || b.avgPM10 > gTodayPM10Max) gTodayPM10Max = b.avgPM10;
  }
}

void pushDaySummary(time_t dayStart, float avgWind, float maxWind, float avgTemp, float minTemp, float maxTemp, float avgHum, float minHum, float maxHum, float avgPress = NAN, float minPress = NAN, float maxPress = NAN, float avgPM1 = NAN, float maxPM1 = NAN, float avgPM25 = NAN, float maxPM25 = NAN, float avgPM10 = NAN, float maxPM10 = NAN) {
  if (!timeIsValid(dayStart)) return;
  DaySummary d{};
  d.dayStartEpoch = dayStart;
  d.avgWind = avgWind;
  d.maxWind = maxWind;
  d.avgTemp = avgTemp;
  d.minTemp = minTemp;
  d.maxTemp = maxTemp;
  d.avgHum = avgHum;
  d.minHum = minHum;
  d.maxHum = maxHum;
  d.avgPress = avgPress;
  d.minPress = minPress;
  d.maxPress = maxPress;
  d.avgPM1 = avgPM1;
  d.maxPM1 = maxPM1;
  d.avgPM25 = avgPM25;
  d.maxPM25 = maxPM25;
  d.avgPM10 = avgPM10;
  d.maxPM10 = maxPM10;

  gDays[gDayWrite] = d;
  gDayWrite = (gDayWrite + 1) % LogConfig::DAYS_HISTORY;
  if (gDaysCount < (uint32_t)LogConfig::DAYS_HISTORY) gDaysCount++;
}

void maybeRolloverDay(time_t nowEpoch) {
  time_t midnight = localMidnight(nowEpoch);

  if (gTodayMidnightEpoch == 0) {
    gTodayMidnightEpoch = midnight;
    return;
  }

  if (midnight != gTodayMidnightEpoch) {
    if (gTodayBucketCount > 0) {
      float avgWind = gTodaySumOfBucketAvgs / (float)gTodayBucketCount;
      float avgTemp = (gTodayTempCount > 0) ? (gTodayTempSum / (float)gTodayTempCount) : NAN;
      float avgHum  = (gTodayHumCount > 0) ? (gTodayHumSum / (float)gTodayHumCount) : NAN;
      float avgPress = (gTodayPressCount > 0) ? (gTodayPressSum / (float)gTodayPressCount) : NAN;
      float avgPM1 = (gTodayPM1Count > 0) ? (gTodayPM1Sum / (float)gTodayPM1Count) : NAN;
      float avgPM25 = (gTodayPM25Count > 0) ? (gTodayPM25Sum / (float)gTodayPM25Count) : NAN;
      float avgPM10 = (gTodayPM10Count > 0) ? (gTodayPM10Sum / (float)gTodayPM10Count) : NAN;
      pushDaySummary(gTodayMidnightEpoch, avgWind, gTodayMax, avgTemp, gTodayTempMin, gTodayTempMax, avgHum, gTodayHumMin, gTodayHumMax, avgPress, gTodayPressMin, gTodayPressMax, avgPM1, gTodayPM1Max, avgPM25, gTodayPM25Max, avgPM10, gTodayPM10Max);
    }

    gTodayMidnightEpoch = midnight;
    clearTodayAggregates();
    deleteOldDailyFileIfNeeded(midnight);
  }
}

struct DayAgg {
  float sumWind = 0.0f;
  uint32_t countWind = 0;
  float maxWind = NAN;
  float tempSum = 0.0f;
  uint32_t tempCount = 0;
  float tempMin = NAN;
  float tempMax = NAN;
  float humSum = 0.0f;
  uint32_t humCount = 0;
  float humMin = NAN;
  float humMax = NAN;
  float pressSum = 0.0f;
  uint32_t pressCount = 0;
  float pressMin = NAN;
  float pressMax = NAN;
  float pm1Sum = 0.0f;
  uint32_t pm1Count = 0;
  float pm1Max = NAN;
  float pm25Sum = 0.0f;
  uint32_t pm25Count = 0;
  float pm25Max = NAN;
  float pm10Sum = 0.0f;
  uint32_t pm10Count = 0;
  float pm10Max = NAN;
};

static void computeBucketSample(BucketSample& b, time_t bucketStart) {
  b.startEpoch = bucketStart;
  b.samples = gBucketSamples;

  if (gBucketEnvSamples > 0) {
    b.avgTempC = gBucketTempSum / (float)gBucketEnvSamples;
    b.avgHumRH = gBucketHumSum / (float)gBucketEnvSamples;
    b.avgPressHpa = (gBucketPressSumPa / (float)gBucketEnvSamples) / 100.0f;
  } else {
    b.avgTempC = NAN;
    b.avgHumRH = NAN;
    b.avgPressHpa = NAN;
  }

  // Wind
  float avgWindMs = 0.0f;
  if (gBucketPulseElapsedMs > 0) {
    float ppsAvg = (float)gBucketPulseCount * 1000.0f / (float)gBucketPulseElapsedMs;
    avgWindMs = ppsAvg * WindConfig::PPS_TO_MS;
  } else if (gBucketSamples > 0) {
    avgWindMs = gBucketWindSum / (float)gBucketSamples;
  }
  b.avgWind = avgWindMs;

  float maxWindMs = gBucketWindMax1;
  if (maxWindMs < avgWindMs) maxWindMs = avgWindMs;
  b.maxWind = maxWindMs;

  // PM averages
  if (gBucketPmSamples > 0) {
    b.avgPM1 = gBucketPM1Sum / (float)gBucketPmSamples;
    b.avgPM25 = gBucketPM25Sum / (float)gBucketPmSamples;
    b.avgPM10 = gBucketPM10Sum / (float)gBucketPmSamples;
  } else {
    b.avgPM1 = NAN;
    b.avgPM25 = NAN;
    b.avgPM10 = NAN;
  }
}

void loadDaySummariesFromSD(time_t nowEpoch) {
  if (!gSdOk) return;
  // Reset
  memset(gDays, 0, sizeof(gDays));
  gDayWrite = 0;
  gDaysCount = 0;

  time_t todayMid = localMidnight(nowEpoch);
  String todayYmd = ymdString(todayMid);

  File dir = SD.open("/data");
  if (!dir) return;

  // Collect per-day aggregates
  struct Entry { time_t day; DayAgg agg; };
  Entry entries[30];  // Reduced from 64 to avoid stack overflow
  int entryCount = 0;

  while (true) {
    File f = dir.openNextFile();
    if (!f) break;
    if (f.isDirectory()) { f.close(); continue; }
    String name = f.name();
    if (!name.endsWith(".csv")) { f.close(); continue; }
    time_t dayMid = 0;
    if (!parseYmdFromPath(name, dayMid) || !timeIsValid(dayMid)) {
      f.close(); continue;
    }
    // Skip current in-progress day by name as well as by computed midnight to avoid DST skew duplicates.
    if (dayMid == todayMid || name.indexOf(todayYmd) >= 0) { f.close(); continue; }

    // find or add entry
    DayAgg* agg = nullptr;
    for (int i=0;i<entryCount;i++){
      if (entries[i].day == dayMid) { agg = &entries[i].agg; break; }
    }
    if (!agg && entryCount < (int)(sizeof(entries)/sizeof(entries[0]))) {
      entries[entryCount].day = dayMid;
      agg = &entries[entryCount].agg;
      entryCount++;
    }
    if (!agg) { f.close(); continue; }

    // parse file
    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (!line.length()) continue;
      if (line.startsWith("datetime")) continue;

      // Count actual columns
      int commaCount = 0;
      for (int i = 0; i < line.length(); i++) {
        if (line[i] == ',') commaCount++;
      }
      int numCols = commaCount + 1;

      String parts[11];
      splitCSVLine(line, parts, numCols < 11 ? numCols : 11);

      // Parse available columns (old format has 8 cols, new format has 11)
      if (numCols < 7) continue;  // Need at least 7 columns for basic data
      float windAvg = parseFloatOrNan(parts[2]);
      float windMax = parseFloatOrNan(parts[3]);
      float temp    = parseFloatOrNan(parts[4]);
      float hum     = parseFloatOrNan(parts[5]);
      float press   = parseFloatOrNan(parts[6]);
      float pm1     = (numCols > 7) ? parseFloatOrNan(parts[7]) : NAN;
      float pm25    = (numCols > 8) ? parseFloatOrNan(parts[8]) : NAN;
      float pm10    = (numCols > 9) ? parseFloatOrNan(parts[9]) : NAN;

      if (isfinite(windAvg)) { agg->sumWind += windAvg; agg->countWind++; }
      if (isfinite(windMax)) { if (!isfinite(agg->maxWind) || windMax > agg->maxWind) agg->maxWind = windMax; }
      if (isfinite(temp)) {
        if (!isfinite(agg->tempMin) || temp < agg->tempMin) agg->tempMin = temp;
        if (!isfinite(agg->tempMax) || temp > agg->tempMax) agg->tempMax = temp;
        agg->tempSum += temp; agg->tempCount++;
      }
      if (isfinite(hum)) {
        if (!isfinite(agg->humMin) || hum < agg->humMin) agg->humMin = hum;
        if (!isfinite(agg->humMax) || hum > agg->humMax) agg->humMax = hum;
        agg->humSum += hum; agg->humCount++;
      }
      if (isfinite(press)) {
        if (!isfinite(agg->pressMin) || press < agg->pressMin) agg->pressMin = press;
        if (!isfinite(agg->pressMax) || press > agg->pressMax) agg->pressMax = press;
        agg->pressSum += press; agg->pressCount++;
      }
      if (isfinite(pm1)) {
        if (!isfinite(agg->pm1Max) || pm1 > agg->pm1Max) agg->pm1Max = pm1;
        agg->pm1Sum += pm1; agg->pm1Count++;
      }
      if (isfinite(pm25)) {
        if (!isfinite(agg->pm25Max) || pm25 > agg->pm25Max) agg->pm25Max = pm25;
        agg->pm25Sum += pm25; agg->pm25Count++;
      }
      if (isfinite(pm10)) {
        if (!isfinite(agg->pm10Max) || pm10 > agg->pm10Max) agg->pm10Max = pm10;
        agg->pm10Sum += pm10; agg->pm10Count++;
      }
    }
    f.close();
  }
  dir.close();

  // sort by day ascending
  for (int i=0;i<entryCount;i++){
    for (int j=i+1;j<entryCount;j++){
      if (entries[j].day < entries[i].day){
        Entry tmp = entries[i]; entries[i]=entries[j]; entries[j]=tmp;
      }
    }
  }
  // push into ring (keep last LogConfig::DAYS_HISTORY)
  int startIdx = entryCount > LogConfig::DAYS_HISTORY ? entryCount - LogConfig::DAYS_HISTORY : 0;
  for (int i=startIdx; i<entryCount; i++){
    DayAgg& a = entries[i].agg;
    if (!timeIsValid(entries[i].day)) continue;
    float avgWind = (a.countWind > 0) ? (a.sumWind / (float)a.countWind) : NAN;
    float maxWind = a.maxWind;
    float avgTemp = (a.tempCount > 0) ? (a.tempSum / (float)a.tempCount) : NAN;
    float avgPress = (a.pressCount > 0) ? (a.pressSum / (float)a.pressCount) : NAN;
    float avgPM1 = (a.pm1Count > 0) ? (a.pm1Sum / (float)a.pm1Count) : NAN;
    float avgPM25 = (a.pm25Count > 0) ? (a.pm25Sum / (float)a.pm25Count) : NAN;
    float avgPM10 = (a.pm10Count > 0) ? (a.pm10Sum / (float)a.pm10Count) : NAN;
    pushDaySummary(entries[i].day, avgWind, maxWind, avgTemp, a.tempMin, a.tempMax,
                   (a.humCount > 0) ? (a.humSum / (float)a.humCount) : NAN,
                   a.humMin, a.humMax, avgPress, a.pressMin, a.pressMax,
                   avgPM1, a.pm1Max, avgPM25, a.pm25Max, avgPM10, a.pm10Max);
  }
}


void finalizeCurrentBucket(time_t bucketStart) {
  BucketSample b{};
  computeBucketSample(b, bucketStart);
  pushBucketSample(b);
  accumulateTodayFromBucket(b);
  logBucketToSD(b);
}

void rebuildTodayAggregates() {
  clearTodayAggregates();
  if (!timeIsValid(gTodayMidnightEpoch)) return;
  for (int i = 0; i < LogConfig::BUCKETS_24H; i++) {
    const BucketSample& b = gBuckets[i];
    if (!timeIsValid(b.startEpoch)) continue;
    if (b.startEpoch < gTodayMidnightEpoch) continue;
    accumulateTodayFromBucket(b);
  }
}

bool buildCurrentDaySummary(DaySummary& out) {
  if (!timeIsValid(gTodayMidnightEpoch)) return false;

  float sumWind = gTodaySumOfBucketAvgs;
  uint32_t windCount = gTodayBucketCount;
  float maxWind = gTodayMax;

  float tempSum = gTodayTempSum;
  uint32_t tempCount = gTodayTempCount;
  float tempMin = gTodayTempMin;
  float tempMax = gTodayTempMax;

  float humSum = gTodayHumSum;
  uint32_t humCount = gTodayHumCount;
  float humMin = gTodayHumMin;
  float humMax = gTodayHumMax;

  float pressSum = gTodayPressSum;
  uint32_t pressCount = gTodayPressCount;
  float pressMin = gTodayPressMin;
  float pressMax = gTodayPressMax;

  // Do not incluide the in-progress bucket's data in this step.

  bool hasData = (windCount > 0) || (tempCount > 0) || (humCount > 0) || (pressCount > 0);
  if (!hasData) return false;

  out.dayStartEpoch = gTodayMidnightEpoch;
  out.avgWind = (windCount > 0) ? (sumWind / (float)windCount) : NAN;
  out.maxWind = (windCount > 0) ? maxWind : NAN;
  out.avgTemp = (tempCount > 0) ? (tempSum / (float)tempCount) : NAN;
  out.minTemp = tempMin;
  out.maxTemp = tempMax;
  out.avgHum = (humCount > 0) ? (humSum / (float)humCount) : NAN;
  out.minHum = humMin;
  out.maxHum = humMax;
  out.avgPress = (pressCount > 0) ? (pressSum / (float)pressCount) : NAN;
  out.minPress = pressMin;
  out.maxPress = pressMax;

  // PM data
  out.avgPM1 = (gTodayPM1Count > 0) ? (gTodayPM1Sum / (float)gTodayPM1Count) : NAN;
  out.maxPM1 = gTodayPM1Max;
  out.avgPM25 = (gTodayPM25Count > 0) ? (gTodayPM25Sum / (float)gTodayPM25Count) : NAN;
  out.maxPM25 = gTodayPM25Max;
  out.avgPM10 = (gTodayPM10Count > 0) ? (gTodayPM10Sum / (float)gTodayPM10Count) : NAN;
  out.maxPM10 = gTodayPM10Max;

  return true;
}

BucketSample currentBucketSnapshot() {
  BucketSample b{};
  if (!timeIsValid(gCurrentBucketStart)) return b;
  computeBucketSample(b, gCurrentBucketStart);
  return b;
}

// ------------------- DOWNLOAD / FILE LIST -------------------

static bool isAllowedPath(const String& p) {
  // Only allow /data directory, and no path traversal.
  if (!p.startsWith("/data/")) return false;
  if (p.indexOf("..") >= 0) return false;
  if (p.indexOf('\\') >= 0) return false;
  if (!p.endsWith(".csv")) return false;
  return true;
}

static bool parseYmdFromPath(const String& path, time_t& outMidnightLocal) {
  int idx = path.lastIndexOf('/');
  String name = (idx >= 0) ? path.substring(idx + 1) : path;
  if (name.length() < 8) return false;

  // Parse YYYYMMDD.csv format
  String ymd = name.substring(0, 8);
  if (ymd.length() != 8) return false;
  int y = ymd.substring(0,4).toInt();
  int m = ymd.substring(4,6).toInt();
  int d = ymd.substring(6,8).toInt();
  if (y <= 1970 || m < 1 || m > 12 || d < 1 || d > 31) return false;
  struct tm tmLocal = {};
  tmLocal.tm_year = y - 1900;
  tmLocal.tm_mon = m - 1;
  tmLocal.tm_mday = d;
  tmLocal.tm_hour = 0; tmLocal.tm_min = 0; tmLocal.tm_sec = 0; tmLocal.tm_isdst = -1;
  time_t t = mktime(&tmLocal);
  if (!timeIsValid(t)) return false;
  outMidnightLocal = t;
  return true;
}

void handleDownload() {
  if (!gSdOk) {
    server.send(503, "text/plain", "SD not available");
    return;
  }

  String path = server.arg("path");
  if (!isAllowedPath(path)) {
    server.send(400, "text/plain", "Invalid path");
    return;
  }

  if (!SD.exists(path.c_str())) {
    server.send(404, "text/plain", "Not found");
    return;
  }

  File f = SD.open(path.c_str(), FILE_READ);
  if (!f) {
    server.send(500, "text/plain", "Failed to open file");
    return;
  }

  String filename = path;
  int slash = filename.lastIndexOf('/');
  if (slash >= 0) filename = filename.substring(slash + 1);

  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Cache-Control", "no-store");
  server.streamFile(f, "text/csv");
  f.close();
}

void handleApiFiles() {
  if (!gSdOk) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"sd_not_available\"}");
    return;
  }

  String dir = server.arg("dir");
  String base;
  if (dir == "data") {
    base = "/data";
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"dir_must_be_data\"}");
    return;
  }

  if (!SD.exists(base.c_str())) {
    server.send(200, "application/json", String("{\"ok\":true,\"dir\":\"") + dir + "\",\"files\":[]}");
    return;
  }

  uint32_t nowMs = millis();
  if (gFilesCacheDataMs != 0 && (nowMs - gFilesCacheDataMs) < FILES_CACHE_TTL_MS && gFilesCacheDataJson.length() > 0) {
    server.send(200, "application/json", gFilesCacheDataJson);
    return;
  }

  File root = SD.open(base.c_str());
  if (!root || !root.isDirectory()) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"failed_to_open_dir\"}");
    return;
  }

  String out;
  out.reserve(4096);
  out += "{\"ok\":true,\"dir\":\"" + dir + "\",\"files\":[";
  bool first = true;

  File f = root.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String name = f.name();
      // Skip internal cache files
      if (name.indexOf("day_summaries_cache") >= 0) {
        f.close();
        f = root.openNextFile();
        continue;
      }
      if (name.endsWith(".csv")) {
        if (!first) out += ",";
        first = false;
        out += "{";
        // Include full path with /data/ prefix
        out += "\"path\":\"" + base + "/" + name + "\",";
        out += "\"size\":" + String((uint32_t)f.size());
        out += "}";
      }
    }
    f.close();
    f = root.openNextFile();
  }
  root.close();

  out += "]}";
  gFilesCacheDataJson = out;
  gFilesCacheDataMs = nowMs;
  server.send(200, "application/json", out);
}

// ------------------- ZIP (streamed, STORE method) -------------------

static uint32_t crc32_table[256];
static bool crc32_init_done = false;

static void crc32_init() {
  if (crc32_init_done) return;
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t c = i;
    for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
    crc32_table[i] = c;
  }
  crc32_init_done = true;
}

static inline uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len) {
  uint32_t c = crc;
  for (size_t i = 0; i < len; i++) c = crc32_table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
  return c;
}

static void dosDateTime(time_t t, uint16_t &dosDate, uint16_t &dosTime) {
  struct tm tmLocal;
  localtime_r(&t, &tmLocal);
  int year = tmLocal.tm_year + 1900;
  if (year < 1980) year = 1980;
  dosDate = (uint16_t)(((year - 1980) << 9) | ((tmLocal.tm_mon + 1) << 5) | (tmLocal.tm_mday));
  dosTime = (uint16_t)((tmLocal.tm_hour << 11) | (tmLocal.tm_min << 5) | ((tmLocal.tm_sec / 2) & 0x1F));
}

struct ZipEntryInfo {
  String name;          // inside-zip name like "data/20251214.csv"
  String sdPath;        // "/data/20251214.csv"
  uint32_t size = 0;    // uncompressed size
  uint32_t crc = 0;     // computed while streaming
  uint32_t lho = 0;     // local header offset
  uint16_t dosDate = 0;
  uint16_t dosTime = 0;
};

static std::vector<ZipEntryInfo> buildLastNDaysList(int days) {
  std::vector<ZipEntryInfo> out;
  out.reserve(days);

  time_t nowE = epochNow();
  time_t todayMidnight = localMidnight(nowE);

  for (int i = 0; i < days; i++) {
    time_t dayMidnight = subtractDaysLocalMidnight(todayMidnight, i);
    String ymd = ymdString(dayMidnight);
    String sdPath = String("/data/") + ymd + ".csv";

    if (gSdOk && SD.exists(sdPath.c_str())) {
      File f = SD.open(sdPath.c_str(), FILE_READ);
      if (f) {
        ZipEntryInfo e;
        e.sdPath = sdPath;
        e.name = String("data/") + ymd + ".csv";
        e.size = (uint32_t)f.size();
        dosDateTime(dayMidnight, e.dosDate, e.dosTime);
        f.close();
        out.push_back(e);
      }
    }
  }

  // Oldest -> newest
  std::reverse(out.begin(), out.end());
  return out;
}

void handleDownloadZip() {
  if (!gSdOk) {
    server.send(503, "text/plain", "SD not available");
    return;
  }

  int days = server.arg("days").toInt();
  if (days <= 0) days = 7;
  // If LogConfig::RETENTION_DAYS is 0 (never delete), allow any number of days
  if (LogConfig::RETENTION_DAYS > 0 && days > LogConfig::RETENTION_DAYS) days = LogConfig::RETENTION_DAYS;
  if (days < 1) days = 1;

  auto entries = buildLastNDaysList(days);
  if (entries.empty()) {
    server.send(404, "text/plain", "No daily CSV files found");
    return;
  }

  crc32_init();

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Content-Type", "application/zip");
  server.sendHeader("Content-Disposition", "attachment; filename=\"last_" + String(days) + "_days.zip\"");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200);

  uint32_t offset = 0;

  const size_t BUF_SZ = 512;
  uint8_t buf[BUF_SZ];

  // Helper to write data via sendContent
  auto sendBytes = [&](const uint8_t* data, size_t len) {
    server.sendContent((const char*)data, len);
    offset += len;
  };

  auto sendU16 = [&](uint16_t v) {
    uint8_t b[2] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF)};
    sendBytes(b, 2);
  };

  auto sendU32 = [&](uint32_t v) {
    uint8_t b[4] = {
      (uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF),
      (uint8_t)((v >> 16) & 0xFF), (uint8_t)((v >> 24) & 0xFF)
    };
    sendBytes(b, 4);
  };

  // Local headers + data + data descriptors
  for (auto &e : entries) {
    File f = SD.open(e.sdPath.c_str(), FILE_READ);
    if (!f) continue;

    e.lho = offset;

    // Local file header
    sendU32(0x04034b50);                      // signature
    sendU16(20);                              // version
    sendU16(0x0008);                          // flags: data descriptor present
    sendU16(0);                               // method: STORE
    sendU16(e.dosTime);
    sendU16(e.dosDate);
    sendU32(0);                               // crc
    sendU32(0);                               // comp size
    sendU32(0);                               // uncomp size
    sendU16((uint16_t)e.name.length());       // name len
    sendU16(0);                               // extra len

    sendBytes((const uint8_t*)e.name.c_str(), e.name.length());

    // Data stream + CRC32
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t sent = 0;
    while (f.available()) {
      int n = f.read(buf, BUF_SZ);
      if (n <= 0) break;
      sendBytes(buf, n);
      crc = crc32_update(crc, buf, (size_t)n);
      sent += (uint32_t)n;
      yield();
    }
    f.close();

    e.crc = crc ^ 0xFFFFFFFFUL;
    e.size = sent;

    // Data descriptor
    sendU32(0x08074b50);
    sendU32(e.crc);
    sendU32(e.size);
    sendU32(e.size);

    yield();
  }

  // Central directory
  uint32_t cdStart = offset;

  for (auto &e : entries) {
    sendU32(0x02014b50);                    // signature
    sendU16(20);                            // version made by
    sendU16(20);                            // version needed
    sendU16(0x0008);                        // flags
    sendU16(0);                             // method
    sendU16(e.dosTime);
    sendU16(e.dosDate);
    sendU32(e.crc);
    sendU32(e.size);
    sendU32(e.size);
    sendU16((uint16_t)e.name.length());     // name len
    sendU16(0);                             // extra len
    sendU16(0);                             // comment len
    sendU16(0);                             // disk start
    sendU16(0);                             // internal attrs
    sendU32(0);                             // external attrs
    sendU32(e.lho);                         // local header offset

    sendBytes((const uint8_t*)e.name.c_str(), e.name.length());
    yield();
  }

  uint32_t cdSize = offset - cdStart;

  // End of central directory
  sendU32(0x06054b50);
  sendU16(0);
  sendU16(0);
  sendU16((uint16_t)entries.size());
  sendU16((uint16_t)entries.size());
  sendU32(cdSize);
  sendU32(cdStart);
  sendU16(0);

  server.sendContent("");  // End chunked transfer
}

// ------------------- WEB UI + API -------------------

void handleRoot() {
  if (gSdOk && SD.exists("/web/index.html")) {
    File f = SD.open("/web/index.html", FILE_READ);
    if (f) {
      server.streamFile(f, "text/html");
      f.close();
      return;
    }
  }

  const char* fallbackHtml = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Weather Station - Setup Required</title>
  <style>
    body { font-family: system-ui, sans-serif; margin: 20px; max-width: 600px; }
    h1 { color: #333; }
    a { color: #0275d8; text-decoration: none; }
    a:hover { text-decoration: underline; }
    ul { line-height: 1.8; }
  </style>
</head>
<body>
  <h1>Weather Station - Setup Required</h1>
  <p>Web interface files not found on SD card.</p>
  <ul>
    <li><a href="/upload">Upload UI files (index.html, app.js)</a></li>
    <li><a href="/api/now">View current sensor data (JSON)</a></li>
    <li><a href="/api/buckets">View bucket data (JSON)</a></li>
    <li><a href="/api/days">View daily summaries (JSON)</a></li>
    <li><a href="/api/config">View configuration (JSON)</a></li>
    <li><a href="/api_help">API documentation</a></li>
  </ul>
</body>
</html>
)HTML";

  server.send(200, "text/html", fallbackHtml);
}

void handleApiHelp() {
  server.send_P(200, "text/html", API_HELP_HTML);
}

void handleRootJs() {
  if (gSdOk && SD.exists("/web/app.js")) {
    File f = SD.open("/web/app.js", FILE_READ);
    if (f) {
      server.streamFile(f, "application/javascript");
      f.close();
      return;
    }
  }

  server.send(200, "application/javascript", "console.log('app.js not found on SD card');");
}

void handleApiClearData() {
  String pw = server.arg("pw");
  bool rateLimited = false;
  if (!verifyPassword(pw, rateLimited)) {
    server.send(rateLimited ? 429 : 401, "application/json",
                rateLimited ? "{\"ok\":false,\"error\":\"rate_limited\"}"
                            : "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }
  if (!SDConfig::ENABLE || !gSdOk) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"sd_not_available\"}");
    return;
  }
  bool ok = deleteDirFiles("/data");
  invalidateFilesCache();
  String out = String("{\"ok\":") + (ok ? "true" : "false") + "}";
  server.send(ok ? 200 : 500, "application/json", out);
}

void handleApiDelete() {
  String pw = server.arg("pw");
  bool rateLimited = false;
  if (!verifyPassword(pw, rateLimited)) {
    server.send(rateLimited ? 429 : 401, "application/json",
                rateLimited ? "{\"ok\":false,\"error\":\"rate_limited\"}"
                            : "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }
  if (!gSdOk) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"sd_not_available\"}");
    return;
  }
  String pathArg = server.arg("path");
  if (!isAllowedPath(pathArg)) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid_path\"}");
    return;
  }
  if (!SD.exists(pathArg.c_str())) {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"not_found\"}");
    return;
  }
  bool ok = SD.remove(pathArg.c_str());
  if (!ok) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"delete_failed\"}");
    return;
  }
  invalidateFilesCache();
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiReboot() {
  String pw = server.arg("pw");
  bool rateLimited = false;
  if (!verifyPassword(pw, rateLimited)) {
    server.send(rateLimited ? 429 : 401, "application/json",
                rateLimited ? "{\"ok\":false,\"error\":\"rate_limited\"}"
                            : "{\"ok\":false,\"error\":\"unauthorized\"}");
    return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
  delay(100);
  ESP.restart();
}

// ------------------- AQI CALCULATION -------------------
// Calculate EPA AQI for PM2.5 (24-hour average, but we use current reading)
int calculateAQI_PM25(float pm25) {
  if (!isfinite(pm25) || pm25 < 0) return -1;

  // EPA AQI breakpoints for PM2.5 (μg/m³)
  if (pm25 <= 12.0) return map((int)(pm25 * 10), 0, 120, 0, 50);
  else if (pm25 <= 35.4) return map((int)(pm25 * 10), 121, 354, 51, 100);
  else if (pm25 <= 55.4) return map((int)(pm25 * 10), 355, 554, 101, 150);
  else if (pm25 <= 150.4) return map((int)(pm25 * 10), 555, 1504, 151, 200);
  else if (pm25 <= 250.4) return map((int)(pm25 * 10), 1505, 2504, 201, 300);
  else if (pm25 <= 500.4) return map((int)(pm25 * 10), 2505, 5004, 301, 500);
  else return 500;
}

// Calculate EPA AQI for PM10 (24-hour average, but we use current reading)
int calculateAQI_PM10(float pm10) {
  if (!isfinite(pm10) || pm10 < 0) return -1;

  // EPA AQI breakpoints for PM10 (μg/m³)
  if (pm10 <= 54) return map((int)pm10, 0, 54, 0, 50);
  else if (pm10 <= 154) return map((int)pm10, 55, 154, 51, 100);
  else if (pm10 <= 254) return map((int)pm10, 155, 254, 101, 150);
  else if (pm10 <= 354) return map((int)pm10, 255, 354, 151, 200);
  else if (pm10 <= 424) return map((int)pm10, 355, 424, 201, 300);
  else if (pm10 <= 604) return map((int)pm10, 425, 604, 301, 500);
  else return 500;
}

// Get AQI category string
const char* getAQICategory(int aqi) {
  if (aqi < 0) return "N/A";
  else if (aqi <= 50) return "Good";
  else if (aqi <= 100) return "Moderate";
  else if (aqi <= 150) return "Unhealthy for Sensitive";
  else if (aqi <= 200) return "Unhealthy";
  else if (aqi <= 300) return "Very Unhealthy";
  else return "Hazardous";
}

void handleApiNow() {
  float pps = (WindConfig::PPS_TO_MS > 0.0f) ? (gNowWindMS / WindConfig::PPS_TO_MS) : 0.0f;
  time_t nowE = epochNow();
  float press_hpa = isfinite(gPressurePa) ? (gPressurePa / 100.0f) : NAN;

  String out = "{";
  out += "\"epoch\":" + String((uint32_t)nowE) + ",";
  out += "\"local_time\":\"" + fmtLocal(nowE) + "\",";

  out += "\"wind_pps\":" + (isfinite(pps) ? String(pps, 3) : String("null")) + ",";
  out += "\"wind_ms\":" + (isfinite(gNowWindMS) ? String(gNowWindMS, 3) : String("null")) + ",";

  out += "\"bme280_ok\":" + String(gBmeOk ? "true" : "false") + ",";
  out += "\"temp_c\":" + (isfinite(gTempC) ? String(gTempC, 2) : String("null")) + ",";
  out += "\"hum_rh\":" + (isfinite(gHumRH) ? String(gHumRH, 2) : String("null")) + ",";
  out += "\"press_hpa\":" + (isfinite(press_hpa) ? String(press_hpa, 2) : String("null")) + ",";

  out += "\"pms5003_ok\":" + String(gPmsOk ? "true" : "false") + ",";
  out += "\"pm1\":" + (isfinite(gPM1) ? String(gPM1, 1) : String("null")) + ",";
  out += "\"pm25\":" + (isfinite(gPM25) ? String(gPM25, 1) : String("null")) + ",";
  out += "\"pm10\":" + (isfinite(gPM10) ? String(gPM10, 1) : String("null")) + ",";

  // Calculate AQI values
  int aqiPM25 = calculateAQI_PM25(gPM25);
  int aqiPM10 = calculateAQI_PM10(gPM10);
  out += "\"aqi_pm25\":" + (aqiPM25 >= 0 ? String(aqiPM25) : String("null")) + ",";
  out += "\"aqi_pm25_category\":\"" + String(getAQICategory(aqiPM25)) + "\",";
  out += "\"aqi_pm10\":" + (aqiPM10 >= 0 ? String(aqiPM10) : String("null")) + ",";
  out += "\"aqi_pm10_category\":\"" + String(getAQICategory(aqiPM10)) + "\",";

  out += "\"sd_ok\":" + String(gSdOk ? "true" : "false") + ",";
  out += "\"cpu_temp_c\":" + String(temperatureRead(), 1) + ",";
  out += "\"uptime_s\":" + String((uint32_t)(millis() / 1000)) + ",";
  out += "\"retention_days\":" + String(LogConfig::RETENTION_DAYS) + ",";
  out += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
  out += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  out += "\"heap_size\":" + String(ESP.getHeapSize());
  out += "}";
  server.send(200, "application/json", out);
}

void handleApiHomeAssistant() {
  time_t nowE = epochNow();
  float press_hpa = isfinite(gPressurePa) ? (gPressurePa / 100.0f) : NAN;

  // Calculate AQI values
  int aqiPM25 = calculateAQI_PM25(gPM25);
  int aqiPM10 = calculateAQI_PM10(gPM10);

  String out = "{";
  out += "\"temperature\":" + (isfinite(gTempC) ? String(gTempC, 2) : String("null")) + ",";
  out += "\"humidity\":" + (isfinite(gHumRH) ? String(gHumRH, 2) : String("null")) + ",";
  out += "\"pressure\":" + (isfinite(press_hpa) ? String(press_hpa, 2) : String("null")) + ",";
  out += "\"wind_speed\":" + (isfinite(gNowWindMS) ? String(gNowWindMS, 3) : String("null")) + ",";
  out += "\"pm1\":" + (isfinite(gPM1) ? String(gPM1, 1) : String("null")) + ",";
  out += "\"pm25\":" + (isfinite(gPM25) ? String(gPM25, 1) : String("null")) + ",";
  out += "\"pm10\":" + (isfinite(gPM10) ? String(gPM10, 1) : String("null")) + ",";
  out += "\"aqi_pm25\":" + (aqiPM25 >= 0 ? String(aqiPM25) : String("null")) + ",";
  out += "\"aqi_pm25_category\":\"" + String(getAQICategory(aqiPM25)) + "\",";
  out += "\"aqi_pm10\":" + (aqiPM10 >= 0 ? String(aqiPM10) : String("null")) + ",";
  out += "\"aqi_pm10_category\":\"" + String(getAQICategory(aqiPM10)) + "\",";
  out += "\"timestamp\":\"" + fmtLocal(nowE) + "\",";
  out += "\"bme280_ok\":" + String(gBmeOk ? "true" : "false") + ",";
  out += "\"pms5003_ok\":" + String(gPmsOk ? "true" : "false") + ",";
  out += "\"wifi_rssi\":" + String(WiFi.RSSI());
  out += "}";

  server.send(200, "application/json", out);
}

static inline String numOrNull(float v, int digits) {
  return isfinite(v) ? String(v, digits) : String("null");
}

static String buildBucketJson(const BucketSample& b) {
  // Compact JSON format - full precision floats, shorter keys
  float avg_wind = (b.avgWind != 255) ? ((float)b.avgWind / 2.0f) : NAN;
  float max_wind = (b.maxWind != 255) ? ((float)b.maxWind / 2.0f) : NAN;
  float temp_c = (b.avgTempC != -128) ? (float)b.avgTempC : NAN;
  float hum_rh = (b.avgHumRH != 255) ? (float)b.avgHumRH : NAN;
  float press_hpa = (b.avgPressHpa != -128) ? (1000.0f + (float)b.avgPressHpa) : NAN;

  // Skip buckets with no valid sensor data
  if (!isfinite(avg_wind) && !isfinite(max_wind) &&
      !isfinite(temp_c) && !isfinite(hum_rh) && !isfinite(press_hpa)) {
    return "";
  }

  String out = "{\"t\":";
  out += String((uint32_t)b.startEpoch);
  out += ",\"w\":{\"a\":";
  out += numOrNull(avg_wind, 3);
  out += ",\"m\":";
  out += numOrNull(max_wind, 3);
  out += ",\"s\":";
  out += String(b.samples);
  out += "},\"T\":";
  out += numOrNull(temp_c, 2);
  out += ",\"H\":";
  out += numOrNull(hum_rh, 2);
  out += ",\"P\":";
  out += numOrNull(press_hpa, 2);
  out += ",\"pm1\":";
  out += numOrNull(b.avgPM1, 1);
  out += ",\"pm25\":";
  out += numOrNull(b.avgPM25, 1);
  out += ",\"pm10\":";
  out += numOrNull(b.avgPM10, 1);
  out += "}";
  return out;
}

static String buildBucketJsonCompact(const BucketSample& b) {
  // Compact format for internal UI - array format with full precision
  // Format: [epoch, avgWind, maxWind, samples, tempC, humRH, pressHpa, pm1, pm25, pm10]
  if (!timeIsValid(b.startEpoch)) return "";

  // Use full precision floats directly
  float avg_wind = b.avgWind;
  float max_wind = b.maxWind;
  float temp_c = b.avgTempC;
  float hum_rh = b.avgHumRH;
  float press_hpa = b.avgPressHpa;

  // Skip buckets with no valid sensor data
  if (!isfinite(avg_wind) && !isfinite(max_wind) &&
      !isfinite(temp_c) && !isfinite(hum_rh) && !isfinite(press_hpa)) {
    return "";
  }

  String out = "[";
  out += String((uint32_t)b.startEpoch);
  out += ",";
  out += numOrNull(avg_wind, 3);
  out += ",";
  out += numOrNull(max_wind, 3);
  out += ",";
  out += String(b.samples);
  out += ",";
  out += numOrNull(temp_c, 2);
  out += ",";
  out += numOrNull(hum_rh, 2);
  out += ",";
  out += numOrNull(press_hpa, 2);
  out += ",";
  out += numOrNull(b.avgPM1, 1);
  out += ",";
  out += numOrNull(b.avgPM25, 1);
  out += ",";
  out += numOrNull(b.avgPM10, 1);
  out += "]";
  return out;
}

void handleApiBuckets() {
  time_t nowE = epochNow();
  time_t todayMidnight = localMidnight(nowE);

  // Use chunked transfer with batching for performance
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  String header = "{\"now_epoch\":";
  header += String((uint32_t)nowE);
  header += ",\"bucket_seconds\":";
  header += String(LogConfig::BUCKET_SECONDS);
  header += ",\"buckets\":[";
  server.sendContent(header);

  bool first = true;
  String batch = "";
  batch.reserve(12288); // Reserve 12KB for batching

  // Add finalized buckets from today only
  for (int i = 0; i < LogConfig::BUCKETS_24H; i++) {
    int idx = (gBucketWrite + i) % LogConfig::BUCKETS_24H;
    const BucketSample& b = gBuckets[idx];
    if (!timeIsValid(b.startEpoch)) continue;
    if (b.startEpoch < todayMidnight) continue;

    String bucketJson = buildBucketJson(b);
    if (bucketJson.length() > 0) {
      if (!first) batch += ",";
      first = false;
      batch += bucketJson;

      // Send batch when it reaches 10KB to avoid blocking too long
      if (batch.length() > 10000) {
        server.sendContent(batch);
        batch = "";
      }
    }
  }

  // Always append the current in-progress bucket
  BucketSample cur = currentBucketSnapshot();
  if (timeIsValid(cur.startEpoch) && cur.startEpoch >= todayMidnight) {
    bool alreadyFinalized = false;
    int lastIdx = (gBucketWrite - 1 + LogConfig::BUCKETS_24H) % LogConfig::BUCKETS_24H;
    if (timeIsValid(gBuckets[lastIdx].startEpoch) &&
        gBuckets[lastIdx].startEpoch == cur.startEpoch) {
      alreadyFinalized = true;
    }

    if (!alreadyFinalized) {
      String bucketJson = buildBucketJson(cur);
      if (bucketJson.length() > 0) {
        if (!first) batch += ",";
        batch += bucketJson;
      }
    }
  }

  // Send remaining batch
  if (batch.length() > 0) {
    server.sendContent(batch);
  }

  server.sendContent("]}");
}

void handleApiBucketsUi() {
  // Compact format for internal UI use - saves bandwidth
  time_t nowE = epochNow();
  time_t todayMidnight = localMidnight(nowE);

  // Use chunked transfer with batching for performance
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");

  String header = "{\"now_epoch\":";
  header += String((uint32_t)nowE);
  header += ",\"bucket_seconds\":";
  header += String(LogConfig::BUCKET_SECONDS);
  header += ",\"buckets\":[";
  server.sendContent(header);

  bool first = true;
  String batch = "";
  batch.reserve(12288); // Reserve 12KB for batching

  // Add finalized buckets from last 24 hours
  time_t cutoff24h = nowE - 86400;  // 24 hours ago
  for (int i = 0; i < LogConfig::BUCKETS_24H; i++) {
    int idx = (gBucketWrite + i) % LogConfig::BUCKETS_24H;
    const BucketSample& b = gBuckets[idx];
    if (!timeIsValid(b.startEpoch)) continue;
    if (b.startEpoch < cutoff24h) continue;

    String bucketJson = buildBucketJsonCompact(b);
    if (bucketJson.length() > 0) {
      if (!first) batch += ",";
      first = false;
      batch += bucketJson;

      // Send batch when it reaches 10KB to avoid blocking too long
      if (batch.length() > 10000) {
        server.sendContent(batch);
        batch = "";
      }
    }
  }

  // Always append the current in-progress bucket
  BucketSample cur = currentBucketSnapshot();
  if (timeIsValid(cur.startEpoch) && cur.startEpoch >= cutoff24h) {
    bool alreadyFinalized = false;
    int lastIdx = (gBucketWrite - 1 + LogConfig::BUCKETS_24H) % LogConfig::BUCKETS_24H;
    if (timeIsValid(gBuckets[lastIdx].startEpoch) &&
        gBuckets[lastIdx].startEpoch == cur.startEpoch) {
      alreadyFinalized = true;
    }

    if (!alreadyFinalized) {
      String bucketJson = buildBucketJsonCompact(cur);
      if (bucketJson.length() > 0) {
        if (!first) batch += ",";
        batch += bucketJson;
      }
    }
  }

  // Send remaining batch
  if (batch.length() > 0) {
    server.sendContent(batch);
  }

  server.sendContent("]}");
}

void handleApiDays() {
  String out;
  out.reserve(8 * 1024);
  out += "{";
  out += "\"days\":[";
  DaySummary curDay{};
  bool hasCurDay = buildCurrentDaySummary(curDay);
  bool firstOut = true;
  if (hasCurDay) {
    out += "{";
    out += "\"dayStartEpoch\":" + String((uint32_t)curDay.dayStartEpoch) + ",";
    out += "\"dayStartLocal\":\"" + fmtLocal(curDay.dayStartEpoch) + "\",";
    out += "\"avgWind\":" + (isfinite(curDay.avgWind) ? String(curDay.avgWind, 3) : String("null")) + ",";
    out += "\"maxWind\":" + (isfinite(curDay.maxWind) ? String(curDay.maxWind, 3) : String("null")) + ",";
    out += "\"avgTemp\":" + (isfinite(curDay.avgTemp) ? String(curDay.avgTemp, 2) : String("null")) + ",";
    out += "\"minTemp\":" + (isfinite(curDay.minTemp) ? String(curDay.minTemp, 2) : String("null")) + ",";
    out += "\"maxTemp\":" + (isfinite(curDay.maxTemp) ? String(curDay.maxTemp, 2) : String("null")) + ",";
    out += "\"avgHum\":" + (isfinite(curDay.avgHum) ? String(curDay.avgHum, 2) : String("null")) + ",";
    out += "\"minHum\":" + (isfinite(curDay.minHum) ? String(curDay.minHum, 2) : String("null")) + ",";
    out += "\"maxHum\":" + (isfinite(curDay.maxHum) ? String(curDay.maxHum, 2) : String("null")) + ",";
    out += "\"avgPress\":" + (isfinite(curDay.avgPress) ? String(curDay.avgPress, 2) : String("null")) + ",";
    out += "\"minPress\":" + (isfinite(curDay.minPress) ? String(curDay.minPress, 2) : String("null")) + ",";
    out += "\"maxPress\":" + (isfinite(curDay.maxPress) ? String(curDay.maxPress, 2) : String("null")) + ",";
    out += "\"avgPM1\":" + (isfinite(curDay.avgPM1) ? String(curDay.avgPM1, 1) : String("null")) + ",";
    out += "\"maxPM1\":" + (isfinite(curDay.maxPM1) ? String(curDay.maxPM1, 1) : String("null")) + ",";
    out += "\"avgPM25\":" + (isfinite(curDay.avgPM25) ? String(curDay.avgPM25, 1) : String("null")) + ",";
    out += "\"maxPM25\":" + (isfinite(curDay.maxPM25) ? String(curDay.maxPM25, 1) : String("null")) + ",";
    out += "\"avgPM10\":" + (isfinite(curDay.avgPM10) ? String(curDay.avgPM10, 1) : String("null")) + ",";
    out += "\"maxPM10\":" + (isfinite(curDay.maxPM10) ? String(curDay.maxPM10, 1) : String("null"));
    out += "}";
    firstOut = false;
  }
  uint32_t count = gDaysCount;
  // Loop backwards from most recent to oldest
  for (int i = (int)count - 1; i >= 0; i--) {
    int startIdx = (gDayWrite - (int)gDaysCount + LogConfig::DAYS_HISTORY) % LogConfig::DAYS_HISTORY;
    int idx = (startIdx + i) % LogConfig::DAYS_HISTORY;
    const DaySummary& d = gDays[idx];
    if (!timeIsValid(d.dayStartEpoch)) continue;
    if (!firstOut) out += ",";
    firstOut = false;
    out += "{";
    out += "\"dayStartEpoch\":" + String((uint32_t)d.dayStartEpoch) + ",";
    out += "\"dayStartLocal\":\"" + fmtLocal(d.dayStartEpoch) + "\",";
    out += "\"avgWind\":" + (isfinite(d.avgWind) ? String(d.avgWind, 3) : String("null")) + ",";
    out += "\"maxWind\":" + (isfinite(d.maxWind) ? String(d.maxWind, 3) : String("null")) + ",";
    out += "\"avgTemp\":" + (isfinite(d.avgTemp) ? String(d.avgTemp, 2) : String("null")) + ",";
    out += "\"minTemp\":" + (isfinite(d.minTemp) ? String(d.minTemp, 2) : String("null")) + ",";
    out += "\"maxTemp\":" + (isfinite(d.maxTemp) ? String(d.maxTemp, 2) : String("null")) + ",";
    out += "\"avgHum\":" + (isfinite(d.avgHum) ? String(d.avgHum, 2) : String("null")) + ",";
    out += "\"minHum\":" + (isfinite(d.minHum) ? String(d.minHum, 2) : String("null")) + ",";
    out += "\"maxHum\":" + (isfinite(d.maxHum) ? String(d.maxHum, 2) : String("null")) + ",";
    out += "\"avgPress\":" + (isfinite(d.avgPress) ? String(d.avgPress, 2) : String("null")) + ",";
    out += "\"minPress\":" + (isfinite(d.minPress) ? String(d.minPress, 2) : String("null")) + ",";
    out += "\"maxPress\":" + (isfinite(d.maxPress) ? String(d.maxPress, 2) : String("null")) + ",";
    out += "\"avgPM1\":" + (isfinite(d.avgPM1) ? String(d.avgPM1, 1) : String("null")) + ",";
    out += "\"maxPM1\":" + (isfinite(d.maxPM1) ? String(d.maxPM1, 1) : String("null")) + ",";
    out += "\"avgPM25\":" + (isfinite(d.avgPM25) ? String(d.avgPM25, 1) : String("null")) + ",";
    out += "\"maxPM25\":" + (isfinite(d.maxPM25) ? String(d.maxPM25, 1) : String("null")) + ",";
    out += "\"avgPM10\":" + (isfinite(d.avgPM10) ? String(d.avgPM10, 1) : String("null")) + ",";
    out += "\"maxPM10\":" + (isfinite(d.maxPM10) ? String(d.maxPM10, 1) : String("null"));
    out += "}";
  }
  out += "]}";
  server.send(200, "application/json", out);
}

void handleApiConfig() {
  String out;
  out.reserve(4096);
  out = "{";

  // Plots configuration
  out += "\"plots\":[";
  for (int i = 0; i < PLOTS_COUNT; i++) {
    if (i > 0) out += ",";
    const PlotConfig& p = PLOTS[i];
    out += "{";
    out += "\"id\":\"" + String(p.id) + "\",";
    out += "\"title\":\"" + String(p.title) + "\",";
    out += "\"unit\":\"" + String(p.unit) + "\",";
    out += "\"conversionFactor\":" + String(p.conversionFactor, 3) + ",";
    out += "\"series\":[";
    for (int j = 0; j < p.seriesCount; j++) {
      if (j > 0) out += ",";
      const PlotSeries& s = p.series[j];
      if (s.field) {
        out += "{";
        out += "\"field\":\"" + String(s.field) + "\",";
        out += "\"color\":\"" + String(s.color) + "\",";
        out += "\"label\":\"" + String(s.label) + "\"";
        out += "}";
      }
    }
    out += "]";
    out += "}";
  }
  out += "],";

  // Table configuration
  out += "\"tableColumns\":[";
  for (int i = 0; i < TABLE_COLUMNS_COUNT; i++) {
    if (i > 0) out += ",";
    const TableColumn& c = TABLE_COLUMNS[i];
    out += "{";
    out += "\"field\":\"" + String(c.field) + "\",";
    out += "\"label\":\"" + String(c.label) + "\",";
    out += "\"unit\":\"" + String(c.unit) + "\",";
    out += "\"conversionFactor\":" + String(c.conversionFactor, 3) + ",";
    out += "\"decimals\":" + String(c.decimals) + ",";
    out += "\"bgColor\":\"" + String(c.bgColor) + "\",";
    out += "\"group\":\"" + String(c.group) + "\"";
    out += "}";
  }
  out += "],";

  // UI parameters
  out += "\"filesPerPage\":" + String(UIConfig::FILES_PER_PAGE) + ",";
  out += "\"maxPlotPoints\":" + String(UIConfig::MAX_PLOT_POINTS);

  out += "}";
  server.send(200, "application/json", out);
}

void handleApiUiFiles() {
  if (!gSdOk) {
    server.send(503, "application/json", "{\"ok\":false,\"error\":\"sd_not_available\"}");
    return;
  }

  String out = "{\"ok\":true,\"files\":[";
  bool first = true;

  const char* webFiles[] = {"/web/index.html", "/web/app.js"};
  for (int i = 0; i < 2; i++) {
    const char* path = webFiles[i];
    if (SD.exists(path)) {
      File f = SD.open(path, FILE_READ);
      if (f) {
        if (!first) out += ",";
        first = false;
        out += "{";
        out += "\"path\":\"" + String(path) + "\",";
        out += "\"size\":" + String((uint32_t)f.size()) + ",";
        out += "\"lastModified\":" + String((uint32_t)f.getLastWrite());
        out += "}";
        f.close();
      }
    }
  }

  out += "]}";
  server.send(200, "application/json", out);
}

void handleUploadPage() {
  server.send_P(200, "text/html", UPLOAD_HTML);
}

static File gUploadFile;
static bool gUploadPasswordVerified = false;
static bool gUploadError = false;
static String gUploadErrorMsg = "";
static const char* TEMP_UPLOAD_FILE = "/web/.upload.tmp";

void handleUploadData() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    gUploadError = false;
    gUploadErrorMsg = "";
    gUploadPasswordVerified = false;

    if (!gSdOk) {
      gUploadError = true;
      gUploadErrorMsg = "sd_not_available";
      return;
    }

    if (!SD.exists("/web")) {
      SD.mkdir("/web");
    }

    // Delete any previous temp file
    if (SD.exists(TEMP_UPLOAD_FILE)) {
      SD.remove(TEMP_UPLOAD_FILE);
    }

    // Save to temporary file first
    gUploadFile = SD.open(TEMP_UPLOAD_FILE, FILE_WRITE);
    if (!gUploadFile) {
      gUploadError = true;
      gUploadErrorMsg = "failed_to_create_file";
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (gUploadFile && !gUploadError) {
      gUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (gUploadFile) {
      gUploadFile.close();
    }
  }
}

void handleUploadComplete() {
  // Check for upload errors first
  if (gUploadError) {
    // Delete temp file on error
    if (SD.exists(TEMP_UPLOAD_FILE)) {
      SD.remove(TEMP_UPLOAD_FILE);
    }

    int statusCode = 500;
    if (gUploadErrorMsg == "unauthorized") statusCode = 401;
    else if (gUploadErrorMsg == "rate_limited") statusCode = 429;
    else if (gUploadErrorMsg == "forbidden_path") statusCode = 403;
    else if (gUploadErrorMsg == "sd_not_available") statusCode = 503;

    server.send(statusCode, "application/json",
                "{\"ok\":false,\"error\":\"" + gUploadErrorMsg + "\"}");

    gUploadPasswordVerified = false;
    gUploadError = false;
    gUploadErrorMsg = "";
    return;
  }

  // Get path and password (now available after multipart parsing)
  String path = server.arg("path");
  String pw = server.arg("pw");

  // Validate path
  if (path.length() == 0) {
    if (SD.exists(TEMP_UPLOAD_FILE)) SD.remove(TEMP_UPLOAD_FILE);
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing_path\"}");
    return;
  }

  if (!path.startsWith("/web/")) {
    if (SD.exists(TEMP_UPLOAD_FILE)) SD.remove(TEMP_UPLOAD_FILE);
    server.send(403, "application/json", "{\"ok\":false,\"error\":\"forbidden_path\"}");
    return;
  }

  // Verify password
  bool rateLimited = false;
  gUploadPasswordVerified = verifyPassword(pw, rateLimited);

  if (!gUploadPasswordVerified) {
    // Password failed - delete temp file
    if (SD.exists(TEMP_UPLOAD_FILE)) {
      SD.remove(TEMP_UPLOAD_FILE);
    }

    int statusCode = rateLimited ? 429 : 401;
    String errorMsg = rateLimited ? "rate_limited" : "unauthorized";
    server.send(statusCode, "application/json",
                "{\"ok\":false,\"error\":\"" + errorMsg + "\"}");
  } else {
    // Password OK - move temp file to final location
    // Delete existing file if present
    if (SD.exists(path.c_str())) {
      SD.remove(path.c_str());
    }

    // Rename temp file to final name
    if (SD.rename(TEMP_UPLOAD_FILE, path.c_str())) {
      server.send(200, "application/json",
                  "{\"ok\":true,\"bytes\":" + String(server.upload().totalSize) + "}");
    } else {
      // Rename failed - try to delete temp file
      if (SD.exists(TEMP_UPLOAD_FILE)) SD.remove(TEMP_UPLOAD_FILE);
      server.send(500, "application/json", "{\"ok\":false,\"error\":\"rename_failed\"}");
    }
  }

  gUploadPasswordVerified = false;
  gUploadError = false;
  gUploadErrorMsg = "";
}

void saveDaySummariesCache(const DaySummary* curDay, bool hasCurDay) {
  if (!gSdOk) return;
  ensureDir("/data");
  File f = SD.open("/data/day_summaries_cache.csv", FILE_WRITE);
  if (!f) return;
  f.print("dayStartEpoch,avgWind,maxWind,avgTemp,minTemp,maxTemp,avgHum,minHum,maxHum,avgPress,minPress,maxPress,avgPM1,maxPM1,avgPM25,maxPM25,avgPM10,maxPM10\n");
  auto writeRow = [&](const DaySummary& d){
    if (!timeIsValid(d.dayStartEpoch)) return;
    f.print((uint32_t)d.dayStartEpoch); f.print(",");
    f.print(isfinite(d.avgWind) ? String(d.avgWind,3) : String("")); f.print(",");
    f.print(isfinite(d.maxWind) ? String(d.maxWind,3) : String("")); f.print(",");
    f.print(isfinite(d.avgTemp) ? String(d.avgTemp,2) : String("")); f.print(",");
    f.print(isfinite(d.minTemp) ? String(d.minTemp,2) : String("")); f.print(",");
    f.print(isfinite(d.maxTemp) ? String(d.maxTemp,2) : String("")); f.print(",");
    f.print(isfinite(d.avgHum) ? String(d.avgHum,2) : String("")); f.print(",");
    f.print(isfinite(d.minHum) ? String(d.minHum,2) : String("")); f.print(",");
    f.print(isfinite(d.maxHum) ? String(d.maxHum,2) : String("")); f.print(",");
    f.print(isfinite(d.avgPress) ? String(d.avgPress,2) : String("")); f.print(",");
    f.print(isfinite(d.minPress) ? String(d.minPress,2) : String("")); f.print(",");
    f.print(isfinite(d.maxPress) ? String(d.maxPress,2) : String("")); f.print(",");
    f.print(isfinite(d.avgPM1) ? String(d.avgPM1,1) : String("")); f.print(",");
    f.print(isfinite(d.maxPM1) ? String(d.maxPM1,1) : String("")); f.print(",");
    f.print(isfinite(d.avgPM25) ? String(d.avgPM25,1) : String("")); f.print(",");
    f.print(isfinite(d.maxPM25) ? String(d.maxPM25,1) : String("")); f.print(",");
    f.print(isfinite(d.avgPM10) ? String(d.avgPM10,1) : String("")); f.print(",");
    f.print(isfinite(d.maxPM10) ? String(d.maxPM10,1) : String(""));
    f.print("\n");
  };
  if (hasCurDay && curDay) writeRow(*curDay);
  uint32_t count2 = gDaysCount;
  int startIdx = (gDayWrite - (int)gDaysCount + LogConfig::DAYS_HISTORY) % LogConfig::DAYS_HISTORY;
  for (uint32_t i = 0; i < count2; i++) {
    int idx = (startIdx + (int)i) % LogConfig::DAYS_HISTORY;
    writeRow(gDays[idx]);
  }
  f.close();
}

bool loadDaySummariesCache() {
  if (!gSdOk || !SD.exists("/data/day_summaries_cache.csv")) return false;
  File f = SD.open("/data/day_summaries_cache.csv", FILE_READ);
  if (!f) return false;
  memset(gDays, 0, sizeof(gDays));
  gDayWrite = 0;
  gDaysCount = 0;
  bool first = true;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (!line.length()) continue;
    if (first) { first = false; if (line.startsWith("dayStartEpoch")) continue; }
    String parts[18];
    int numParts = splitCSVLine(line, parts, 18) ? 18 : 0;
    if (numParts < 9) continue;  // Need at least 9 fields for backward compatibility
    DaySummary d{};
    d.dayStartEpoch = (time_t)parts[0].toInt();
    if (!timeIsValid(d.dayStartEpoch)) continue;
    d.avgWind = parts[1].length() ? parts[1].toFloat() : NAN;
    d.maxWind = parts[2].length() ? parts[2].toFloat() : NAN;
    d.avgTemp = parts[3].length() ? parts[3].toFloat() : NAN;
    d.minTemp = parts[4].length() ? parts[4].toFloat() : NAN;
    d.maxTemp = parts[5].length() ? parts[5].toFloat() : NAN;
    d.avgHum  = parts[6].length() ? parts[6].toFloat() : NAN;
    d.minHum  = parts[7].length() ? parts[7].toFloat() : NAN;
    d.maxHum  = parts[8].length() ? parts[8].toFloat() : NAN;
    // Pressure fields (optional for backward compatibility)
    d.avgPress = (numParts > 9 && parts[9].length()) ? parts[9].toFloat() : NAN;
    d.minPress = (numParts > 10 && parts[10].length()) ? parts[10].toFloat() : NAN;
    d.maxPress = (numParts > 11 && parts[11].length()) ? parts[11].toFloat() : NAN;
    // PM fields (optional for backward compatibility)
    d.avgPM1 = (numParts > 12 && parts[12].length()) ? parts[12].toFloat() : NAN;
    d.maxPM1 = (numParts > 13 && parts[13].length()) ? parts[13].toFloat() : NAN;
    d.avgPM25 = (numParts > 14 && parts[14].length()) ? parts[14].toFloat() : NAN;
    d.maxPM25 = (numParts > 15 && parts[15].length()) ? parts[15].toFloat() : NAN;
    d.avgPM10 = (numParts > 16 && parts[16].length()) ? parts[16].toFloat() : NAN;
    d.maxPM10 = (numParts > 17 && parts[17].length()) ? parts[17].toFloat() : NAN;
    gDays[gDayWrite] = d;
    gDayWrite = (gDayWrite + 1) % LogConfig::DAYS_HISTORY;
    if (gDaysCount < (uint32_t)LogConfig::DAYS_HISTORY) gDaysCount++;
  }
  f.close();
  return gDaysCount > 0;
}

// ------------------- SETUP HELPERS -------------------

bool syncTimeWithNTP(uint32_t timeoutMs = 20000) {
  setenv("TZ", NetworkConfig::TIMEZONE, 1);
  tzset();
  configTzTime(NetworkConfig::TIMEZONE, NetworkConfig::NTP_SERVER_1, NetworkConfig::NTP_SERVER_2, NetworkConfig::NTP_SERVER_3);

  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    time_t nowE = time(nullptr);
    if (timeIsValid(nowE)) return true;
    delay(250);
  }
  return false;
}

void initSD() {
  gSdOk = false;
  if (!SDConfig::ENABLE) return;

  // If your board needs explicit SPI pins, do it here, e.g.:
  // SPI.begin(SCK, MISO, MOSI, SDConfig::CS_PIN);

  if (!SD.begin(SDConfig::CS_PIN)) return;

  gSdOk = true;
  ensureDir("/data");
}

// ------------------- LOOP HELPERS -------------------

static void updateWindPPS(uint32_t msNow) {
  if (msNow - gLastPpsMillis < WindConfig::PPS_WINDOW_MS) return;

  uint32_t elapsedMs = msNow - gLastPpsMillis;

  uint32_t delta;
  noInterrupts();
  delta = gPulseCount;
  gPulseCount = 0;
  interrupts();

  float pps = (elapsedMs > 0) ? (delta * 1000.0f / (float)elapsedMs) : 0.0f;
  float ms = pps * WindConfig::PPS_TO_MS;

  gNowWindMS = ms;
  if (gNowWindMS > gBucketWindMax1) {
    gBucketWindMax2 = gBucketWindMax1;
    gBucketWindMax1 = gNowWindMS;
  } else if (gNowWindMS > gBucketWindMax2) {
    gBucketWindMax2 = gNowWindMS;
  }

  gBucketPulseCount += delta;
  gBucketPulseElapsedMs += elapsedMs;

  // Sample wind into bucket immediately after calculation
  gBucketWindSum += gNowWindMS;
  gBucketSamples++;

  gLastPpsMillis = msNow;
}

static void pollBMEIfNeeded(uint32_t msNow) {
  if (!BME280Config::ENABLE || !gBmeOk) return;
  if (msNow - gLastBmePollMillis < BME280Config::POLL_INTERVAL_MS) return;
  pollBME();
  gLastBmePollMillis += BME280Config::POLL_INTERVAL_MS;
}

static void pollPMSIfNeeded(uint32_t msNow) {
  if (!PMS5003Config::ENABLE) return;
  if (msNow - gLastPmsPollMillis < PMS5003Config::POLL_INTERVAL_MS) return;
  pollPMS();
  gLastPmsPollMillis += PMS5003Config::POLL_INTERVAL_MS;
}

static void processBucketCatchup(time_t nowE) {
  time_t aligned = floorToBucketBoundaryLocal(nowE);
  if (!timeIsValid(gCurrentBucketStart)) startBucketAt(aligned);

  // Limit catch-up work so we keep serving HTTP/OTA while filling missed buckets
  const int MAX_BUCKETS_PER_LOOP = 6;
  int bucketsProcessed = 0;
  while (gCurrentBucketStart < aligned && bucketsProcessed < MAX_BUCKETS_PER_LOOP) {
    finalizeCurrentBucket(gCurrentBucketStart);

    gCurrentBucketStart += LogConfig::BUCKET_SECONDS;
    gBucketWindSum = 0.0f;
    gBucketWindMax1 = 0.0f;
    gBucketWindMax2 = 0.0f;
    gBucketSamples = 0;
    gBucketPulseCount = 0;
    gBucketPulseElapsedMs = 0;
    gBucketTempSum = 0.0f;
    gBucketHumSum = 0.0f;
    gBucketPressSumPa = 0.0f;
    gBucketEnvSamples = 0;
    gBucketPM1Sum = 0.0f;
    gBucketPM25Sum = 0.0f;
    gBucketPM10Sum = 0.0f;
    gBucketPmSamples = 0;
    bucketsProcessed++;
    if ((bucketsProcessed % 2) == 0) {
      server.handleClient();
      ArduinoOTA.handle();
      delay(0);
    }
  }
}

// ------------------- SETUP / LOOP -------------------

void setup() {
  Serial.begin(115200);
  delay(200);

  memset(gBuckets, 0, sizeof(gBuckets));
  memset(gDays, 0, sizeof(gDays));
  gBucketWrite = 0;
  gDayWrite = 0;
  gDaysCount = 0;

  // Set up pulse pin but don't attach interrupt yet to avoid counting noise during WiFi init
  pinMode(WindConfig::PULSE_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  if (!wm.autoConnect("Anemometer-Setup")) {
    delay(500);
    ESP.restart();
  }

  if (!syncTimeWithNTP()) {
    delay(500);
    ESP.restart();
  }

  Wire.begin();
  if (BME280Config::ENABLE) {
    gBmeOk = bme.begin(BME280Config::I2C_ADDR_PRIMARY);
    if (!gBmeOk) gBmeOk = bme.begin(0x77);
    if (gBmeOk) {
      bme.setSampling(
        Adafruit_BME280::MODE_NORMAL,
        Adafruit_BME280::SAMPLING_X2,
        Adafruit_BME280::SAMPLING_X16,
        Adafruit_BME280::SAMPLING_X1,
        Adafruit_BME280::FILTER_X16,
        Adafruit_BME280::STANDBY_MS_500
      );
      pollBME();
    }
  }

  // Initialize PMS5003 on Serial2
  if (PMS5003Config::ENABLE) {
    Serial2.begin(9600, SERIAL_8N1, PMS5003Config::RX_PIN, PMS5003Config::TX_PIN);
    gPmsOk = true;  // Sensor ready (no handshake needed)
    Serial.println("PMS5003 UART initialized");
  }

  initSD();

  time_t nowE = epochNow();
  gTodayMidnightEpoch = localMidnight(nowE);
  deleteOldDailyFileIfNeeded(gTodayMidnightEpoch);

  // Remove stale cache file if it exists
  if (gSdOk && SD.exists("/data/day_summaries_cache.csv")) {
    SD.remove("/data/day_summaries_cache.csv");
  }

  // Always load fresh from SD to avoid stale cache issues
  loadDaySummariesFromSD(nowE);
  loadRecentBucketsFromSD(nowE);

  time_t aligned = floorToBucketBoundaryLocal(nowE);
  startBucketAt(aligned);

  gLastPpsMillis = millis();
  gLastBmePollMillis = millis();
  gLastPmsPollMillis = millis();

  // routes
  server.on("/", handleRoot);
  server.on("/root.js", handleRootJs);
  server.on("/api/now", handleApiNow);
  server.on("/api/homeassistant", handleApiHomeAssistant);
  server.on("/api/buckets", handleApiBuckets);
  server.on("/api/buckets_ui", handleApiBucketsUi);  // Compact format for internal UI
  server.on("/api/days", handleApiDays);
  server.on("/api/config", handleApiConfig);
  server.on("/api/ui_files", handleApiUiFiles);
  server.on("/api_help", handleApiHelp);
  server.on("/api/clear_data", HTTP_POST, handleApiClearData);
  server.on("/api/delete", HTTP_POST, handleApiDelete);
  server.on("/api/reboot", HTTP_POST, handleApiReboot);

  server.on("/api/files", handleApiFiles);
  server.on("/download", handleDownload);
  server.on("/download_zip", handleDownloadZip);
  server.on("/upload", HTTP_GET, handleUploadPage);
  server.on("/upload", HTTP_POST, handleUploadComplete, handleUploadData);

  server.begin();

  ArduinoOTA.setHostname("anemometer");
  ArduinoOTA.onStart([]() {
  });
  ArduinoOTA.onError([](ota_error_t error) {
    (void)error;
  });
  ArduinoOTA.begin();

  // Attach pulse interrupt after all initialization to avoid counting noise during boot
  gLastPulseMicros = micros();
  gPulseCount = 0;
  attachInterrupt(digitalPinToInterrupt(WindConfig::PULSE_PIN), onPulse, RISING);
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  uint32_t msNow = millis();

  updateWindPPS(msNow);
  pollBMEIfNeeded(msNow);
  pollPMSIfNeeded(msNow);

  time_t nowE = epochNow();
  maybeRolloverDay(nowE);
  processBucketCatchup(nowE);
}
