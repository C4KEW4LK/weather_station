/*
  ESP32-C3 Anemometer + BME280 + SD CSV logger (NTP + WiFiManager)
  + Web UI + REST API + CSV download + "last N days ZIP" download (streamed)

  Wind:
  - Pulse input: GPIO0 interrupt
  - Calibration: 20 pps = 1.75 m/s => m/s = pps * (1.75/20)

  Logging:
  - 5-minute aligned buckets (NTP time)
  - /data/YYYYMMDD.csv (daily, retained for RETENTION_DAYS)
  - /chunks/chunk_YYYYMMDD.csv (10-day chunked, indefinite)

  Downloads:
  - List:     /api/files?dir=data   or  /api/files?dir=chunks
  - CSV:      /download?path=/data/20251214.csv
  - ZIP:      /download_zip?days=7  (streams ZIP of last N daily /data files, STORE mode)

  Notes:
  - GPIO0 is a boot strap pin on ESP32-C3; ensure sensor doesn't hold it LOW at boot.
  - SD pin mapping varies by board/module; set SD_CS_PIN and optionally SPI.begin(...) pins.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <time.h>

#include <Wire.h>
#include <Adafruit_BME280.h>

#include <SPI.h>
#include <SD.h>

#include <vector>
#include <algorithm>

// ------------------- CONFIG -------------------

// Retention for /data daily files
static constexpr int RETENTION_DAYS = 30;   // delete daily file older than this (in days)

// Wind
static constexpr int   PULSE_PIN = 0;                 // GPIO0
static constexpr float PPS_TO_MS = 1.75f / 20.0f;     // 0.0875 m/s per pps
static constexpr uint32_t NOW_SAMPLE_MS = 250;
static constexpr uint32_t PPS_WINDOW_MS = 1000;

// Buckets for UI chart (RAM only)
static constexpr int BUCKETS_24H  = 24 * 60 / 5;      // 288
static constexpr int DAYS_HISTORY = 30;               // RAM daily summaries shown in UI

// BME280
static constexpr uint32_t BME_POLL_MS = 2000;
static constexpr bool     BME_ENABLE  = true;
static constexpr uint8_t  BME_I2C_ADDR = 0x76;        // will also try 0x77

// SD
static constexpr bool SD_ENABLE = true;
static constexpr int  SD_CS_PIN = 10;                // <-- CHANGE for your wiring/board

// NTP
static const char* NTP1 = "pool.ntp.org";
static const char* NTP2 = "time.google.com";
static const char* NTP3 = "time.windows.com";

// POSIX TZ string for Australia/Sydney (DST aware)
static const char* TZ_AU_SYDNEY = "AEST-10AEDT,M10.1.0/2,M4.1.0/3";

// ------------------- DATA -------------------

struct Bucket5Min {
  time_t startEpoch;
  float avgWind;
  float maxWind;
  uint32_t samples;
};

struct DaySummary {
  time_t dayStartEpoch; // local midnight
  float avgWind;
  float maxWind;
};

static Bucket5Min gBuckets[BUCKETS_24H];
static int gBucketWrite = 0;

static DaySummary gDays[DAYS_HISTORY];
static int gDayWrite = 0;
static uint32_t gDaysCount = 0;

// wind pulses
static volatile uint32_t gPulseCount = 0;
static float gNowWindMS = 0.0f;
static float gNowWindMaxSinceBoot = 0.0f;

// timing / bucket accumulators
static uint32_t gLastPpsMillis = 0;
static uint32_t gLastWindSampleMillis = 0;
static time_t   gCurrentBucketStart = 0;
static float    gBucketWindSum = 0.0f;
static float    gBucketWindMax = 0.0f;
static uint32_t gBucketSamples = 0;

// daily rollup
static time_t   gTodayMidnightEpoch = 0;
static float    gTodaySumOfBucketAvgs = 0.0f;
static uint32_t gTodayBucketCount = 0;
static float    gTodayMax = 0.0f;

// BME280 latest
static Adafruit_BME280 bme;
static bool  gBmeOk = false;
static float gTempC = NAN;
static float gHumRH = NAN;
static float gPressurePa = NAN;
static uint32_t gLastBmePollMillis = 0;

// SD status
static bool gSdOk = false;

// Web
static WebServer server(80);

// ------------------- ISR -------------------
void IRAM_ATTR onPulse() { gPulseCount++; }

// ------------------- TIME HELPERS -------------------
static inline bool timeIsValid(time_t t) { return t > 1577836800; } // > 2020-01-01
static inline time_t epochNow() { return time(nullptr); }

time_t localMidnight(time_t nowEpoch) {
  struct tm tmLocal;
  localtime_r(&nowEpoch, &tmLocal);
  tmLocal.tm_hour = 0; tmLocal.tm_min = 0; tmLocal.tm_sec = 0;
  return mktime(&tmLocal);
}

time_t floorTo5MinBoundaryLocal(time_t nowEpoch) {
  struct tm tmLocal;
  localtime_r(&nowEpoch, &tmLocal);
  tmLocal.tm_sec = 0;
  tmLocal.tm_min = (tmLocal.tm_min / 5) * 5;
  return mktime(&tmLocal);
}

String fmtLocal(time_t t) {
  struct tm tmLocal;
  localtime_r(&t, &tmLocal);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmLocal);
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
  tmLocal.tm_hour = 0; tmLocal.tm_min = 0; tmLocal.tm_sec = 0;
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

// 10-day chunk filename based on epoch day index (good enough for chunk naming)
String chunkFileForDay(time_t dayMidnightLocal) {
  long dayIndex = (long)(dayMidnightLocal / 86400L);
  int mod = (int)(dayIndex % 10L);
  if (mod < 0) mod += 10;
  time_t chunkStart = subtractDaysLocalMidnight(dayMidnightLocal, mod);
  return String("/chunks/chunk_") + ymdString(chunkStart) + ".csv";
}

void deleteOldDailyFileIfNeeded(time_t todayMidnightLocal) {
  time_t oldMidnight = subtractDaysLocalMidnight(todayMidnightLocal, RETENTION_DAYS + 1);
  String ymd = ymdString(oldMidnight);
  String dataPath = String("/data/") + ymd + ".csv";
  if (gSdOk && SD.exists(dataPath.c_str())) SD.remove(dataPath.c_str());
}

void logBucketToSD(const Bucket5Min& b) {
  if (!gSdOk) return;

  ensureDir("/data");
  ensureDir("/chunks");

  time_t bucketMidnightLocal = localMidnight(b.startEpoch);
  String ymd = ymdString(bucketMidnightLocal);

  String dailyFile = String("/data/") + ymd + ".csv";
  String chunkFile = chunkFileForDay(bucketMidnightLocal);

  float press_hpa = isfinite(gPressurePa) ? (gPressurePa / 100.0f) : NAN;
  String dt = fmtLocal(b.startEpoch);

  String header = "datetime,epoch,wind_avg_ms,wind_max_ms,temp_c,hum_rh,press_hpa,samples";
  String line =
    dt + "," + String((uint32_t)b.startEpoch) + "," +
    String(b.avgWind, 3) + "," + String(b.maxWind, 3) + "," +
    String(gTempC, 2) + "," + String(gHumRH, 2) + "," + String(press_hpa, 2) + "," +
    String(b.samples);

  appendLine(dailyFile, line, header);
  appendLine(chunkFile, line, header);
}

// ------------------- BME280 -------------------
void pollBME() {
  if (!BME_ENABLE || !gBmeOk) return;
  gTempC = bme.readTemperature();
  gPressurePa = bme.readPressure();
  gHumRH = bme.readHumidity();
}

// ------------------- BUCKETS / DAILY -------------------

void startBucketAt(time_t bucketStart) {
  gCurrentBucketStart = bucketStart;
  gBucketWindSum = 0.0f;
  gBucketWindMax = 0.0f;
  gBucketSamples = 0;
}

void push5MinBucket(const Bucket5Min& b) {
  gBuckets[gBucketWrite] = b;
  gBucketWrite = (gBucketWrite + 1) % BUCKETS_24H;
}

void pushDaySummary(time_t dayStart, float avg, float mx) {
  DaySummary d{};
  d.dayStartEpoch = dayStart;
  d.avgWind = avg;
  d.maxWind = mx;

  gDays[gDayWrite] = d;
  gDayWrite = (gDayWrite + 1) % DAYS_HISTORY;
  if (gDaysCount < (uint32_t)DAYS_HISTORY) gDaysCount++;
}

void maybeRolloverDay(time_t nowEpoch) {
  time_t midnight = localMidnight(nowEpoch);

  if (gTodayMidnightEpoch == 0) {
    gTodayMidnightEpoch = midnight;
    return;
  }

  if (midnight != gTodayMidnightEpoch) {
    if (gTodayBucketCount > 0) {
      float avg = gTodaySumOfBucketAvgs / (float)gTodayBucketCount;
      pushDaySummary(gTodayMidnightEpoch, avg, gTodayMax);
    }

    gTodayMidnightEpoch = midnight;
    gTodaySumOfBucketAvgs = 0.0f;
    gTodayBucketCount = 0;
    gTodayMax = 0.0f;

    deleteOldDailyFileIfNeeded(midnight);
  }
}

void finalizeCurrentBucket(time_t bucketStart) {
  Bucket5Min b{};
  b.startEpoch = bucketStart;
  b.samples = gBucketSamples;

  if (gBucketSamples > 0) {
    b.avgWind = gBucketWindSum / (float)gBucketSamples;
    b.maxWind = gBucketWindMax;
  } else {
    b.avgWind = 0.0f;
    b.maxWind = 0.0f;
  }

  push5MinBucket(b);

  gTodaySumOfBucketAvgs += b.avgWind;
  gTodayBucketCount++;
  if (b.maxWind > gTodayMax) gTodayMax = b.maxWind;

  logBucketToSD(b);
}

// ------------------- DOWNLOAD / FILE LIST -------------------

static bool isAllowedPath(const String& p) {
  // Only allow these two directories, and no path traversal.
  if (!p.startsWith("/data/") && !p.startsWith("/chunks/")) return false;
  if (p.indexOf("..") >= 0) return false;
  if (p.indexOf('\\') >= 0) return false;
  if (!p.endsWith(".csv")) return false;
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

  String dir = server.arg("dir"); // "data" or "chunks"
  String base;
  if (dir == "data") base = "/data";
  else if (dir == "chunks") base = "/chunks";
  else {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"dir_must_be_data_or_chunks\"}");
    return;
  }

  if (!SD.exists(base.c_str())) {
    server.send(200, "application/json", String("{\"ok\":true,\"dir\":\"") + dir + "\",\"files\":[]}");
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
      String fullPath;

      if (name.startsWith(base + "/")) fullPath = name;
      else if (name.startsWith("/")) fullPath = name;
      else fullPath = base + "/" + name;

      if (fullPath.endsWith(".csv")) {
        if (!first) out += ",";
        first = false;
        out += "{";
        out += "\"path\":\"" + fullPath + "\",";
        out += "\"size\":" + String((uint32_t)f.size());
        out += "}";
      }
    }
    f.close();
    f = root.openNextFile();
  }
  root.close();

  out += "]}";
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

static void w16(WiFiClient &c, uint16_t v) {
  uint8_t b[2] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF)};
  c.write(b, 2);
}
static void w32(WiFiClient &c, uint32_t v) {
  uint8_t b[4] = {
    (uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF),
    (uint8_t)((v >> 16) & 0xFF), (uint8_t)((v >> 24) & 0xFF)
  };
  c.write(b, 4);
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
  if (days > RETENTION_DAYS) days = RETENTION_DAYS;
  if (days < 1) days = 1;

  auto entries = buildLastNDaysList(days);
  if (entries.empty()) {
    server.send(404, "text/plain", "No daily CSV files found");
    return;
  }

  crc32_init();

  WiFiClient client = server.client();

  server.sendHeader("Content-Type", "application/zip");
  server.sendHeader("Content-Disposition", "attachment; filename=\"last_" + String(days) + "_days.zip\"");
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Connection", "close");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200);

  uint32_t offset = 0;

  const size_t BUF_SZ = 1024;
  uint8_t buf[BUF_SZ];

  // Local headers + data + data descriptors
  for (auto &e : entries) {
    File f = SD.open(e.sdPath.c_str(), FILE_READ);
    if (!f) continue;

    e.lho = offset;

    // Local file header
    w32(client, 0x04034b50); offset += 4;       // signature
    w16(client, 20);         offset += 2;       // version
    w16(client, 0x0008);     offset += 2;       // flags: data descriptor present
    w16(client, 0);          offset += 2;       // method: STORE
    w16(client, e.dosTime);  offset += 2;
    w16(client, e.dosDate);  offset += 2;
    w32(client, 0);          offset += 4;       // crc
    w32(client, 0);          offset += 4;       // comp size
    w32(client, 0);          offset += 4;       // uncomp size
    w16(client, (uint16_t)e.name.length()); offset += 2;  // name len
    w16(client, 0);          offset += 2;       // extra len

    client.write((const uint8_t*)e.name.c_str(), e.name.length());
    offset += (uint32_t)e.name.length();

    // Data stream + CRC32
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t sent = 0;
    while (f.available()) {
      int n = f.read(buf, BUF_SZ);
      if (n <= 0) break;
      client.write(buf, n);
      crc = crc32_update(crc, buf, (size_t)n);
      sent += (uint32_t)n;
      offset += (uint32_t)n;
      delay(0);
    }
    f.close();

    e.crc = crc ^ 0xFFFFFFFFUL;
    e.size = sent;

    // Data descriptor
    w32(client, 0x08074b50); offset += 4;
    w32(client, e.crc);      offset += 4;
    w32(client, e.size);     offset += 4;
    w32(client, e.size);     offset += 4;
  }

  // Central directory
  uint32_t cdStart = offset;

  for (auto &e : entries) {
    w32(client, 0x02014b50); offset += 4;  // signature
    w16(client, 20);         offset += 2;  // version made by
    w16(client, 20);         offset += 2;  // version needed
    w16(client, 0x0008);     offset += 2;  // flags
    w16(client, 0);          offset += 2;  // method
    w16(client, e.dosTime);  offset += 2;
    w16(client, e.dosDate);  offset += 2;
    w32(client, e.crc);      offset += 4;
    w32(client, e.size);     offset += 4;
    w32(client, e.size);     offset += 4;
    w16(client, (uint16_t)e.name.length()); offset += 2; // name len
    w16(client, 0);          offset += 2;  // extra len
    w16(client, 0);          offset += 2;  // comment len
    w16(client, 0);          offset += 2;  // disk start
    w16(client, 0);          offset += 2;  // internal attrs
    w32(client, 0);          offset += 4;  // external attrs
    w32(client, e.lho);      offset += 4;  // local header offset

    client.write((const uint8_t*)e.name.c_str(), e.name.length());
    offset += (uint32_t)e.name.length();
  }

  uint32_t cdSize = offset - cdStart;

  // End of central directory
  w32(client, 0x06054b50);
  w16(client, 0);
  w16(client, 0);
  w16(client, (uint16_t)entries.size());
  w16(client, (uint16_t)entries.size());
  w32(client, cdSize);
  w32(client, cdStart);
  w16(client, 0);

  client.flush();
}

// ------------------- WEB UI + API -------------------

void handleRoot() {
  const char* html = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Anemometer</title>
  <style>
    body { font-family: system-ui, sans-serif; margin: 16px; }
    .row { display: flex; gap: 12px; flex-wrap: wrap; }
    .card { border: 1px solid #ddd; border-radius: 12px; padding: 12px; min-width: 260px; }
    .big { font-size: 42px; font-weight: 700; }
    .muted { color: #666; }
    svg { width: 100%; height: 140px; border: 1px solid #eee; border-radius: 10px; background: #fafafa; }
    table { width: 100%; border-collapse: collapse; }
    td, th { border-bottom: 1px solid #eee; padding: 6px 4px; text-align: left; font-size: 14px; }
    .pill { display:inline-block; padding:2px 8px; border-radius:999px; background:#f2f2f2; margin-right:6px; margin-top:6px; }
    .small { font-size: 12px; }
    a { color: inherit; }
    .files { display:flex; gap:12px; flex-wrap:wrap; }
    .filecol { min-width: 320px; flex:1; }
    .btnrow { display:flex; gap:8px; margin: 8px 0; flex-wrap: wrap; }
    button { padding: 8px 10px; border-radius: 10px; border:1px solid #ddd; background:#fff; cursor:pointer; }
    code { background:#f6f6f6; padding:2px 6px; border-radius: 8px; }
    input { padding: 7px 10px; border-radius: 10px; border:1px solid #ddd; width: 80px; }
  </style>
</head>
<body>
  <h2>Wind + Environment</h2>

  <div class="row">
    <div class="card">
      <div class="muted">Current wind</div>
      <div class="big" id="now">--.-</div>
      <div class="muted">m/s</div>

      <div style="margin-top:10px">
        <span class="pill">PPS: <span id="pps">--</span></span>
        <span class="pill">Max boot: <span id="maxboot">--.-</span> m/s</span>
      </div>

      <div style="margin-top:8px">
        <span class="pill">Temp: <span id="temp">--.-</span> °C</span>
        <span class="pill">RH: <span id="rh">--.-</span> %</span>
        <span class="pill">Pressure: <span id="pres">----.-</span> hPa</span>
      </div>

      <div style="margin-top:8px">
        <span class="pill">BME280: <span id="bmeok">--</span></span>
        <span class="pill">SD: <span id="sdok">--</span></span>
      </div>

      <div class="muted small" style="margin-top:10px">Time: <span id="tlocal">--</span></div>
    </div>

    <div class="card" style="flex:1; min-width:320px;">
      <div class="muted">Last 24h (5-min avg wind)</div>
      <svg viewBox="0 0 600 140" preserveAspectRatio="none">
        <polyline id="line" fill="none" stroke="black" stroke-width="2" points=""></polyline>
      </svg>
      <div class="muted small" style="margin-top:8px">Left = older, Right = now</div>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card" style="flex:1; min-width:320px;">
      <div class="muted">Daily wind summaries (RAM)</div>
      <table>
        <thead><tr><th>Day start</th><th>Avg wind</th><th>Max wind</th></tr></thead>
        <tbody id="days"></tbody>
      </table>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card" style="flex:1; min-width:320px;">
      <div class="muted">CSV downloads</div>
      <div class="small muted">List API: <code>/api/files?dir=data</code> and <code>/api/files?dir=chunks</code></div>

      <div class="btnrow">
        <button onclick="loadFiles('data')">Refresh daily /data</button>
        <button onclick="loadFiles('chunks')">Refresh /chunks</button>
      </div>

      <div class="btnrow">
        <input id="zipdays" type="number" min="1" value="7"/>
        <button onclick="downloadZip()">Download last N days (ZIP)</button>
        <span class="small muted">Endpoint: <code>/download_zip?days=N</code></span>
      </div>

      <div class="files">
        <div class="filecol">
          <div class="muted">Daily files (/data)</div>
          <div id="files_data" class="small"></div>
        </div>
        <div class="filecol">
          <div class="muted">10-day chunks (/chunks)</div>
          <div id="files_chunks" class="small"></div>
        </div>
      </div>
    </div>
  </div>

<script>
async function fetchJSON(url){ const r = await fetch(url, {cache:"no-store"}); return await r.json(); }

function renderLine(values){
  const svgW = 600, svgH = 140, pad = 8;
  const ys = values.map(v => v.avgWind ?? 0);
  const maxY = Math.max(1, ...ys);
  const minY = 0;

  const n = ys.length;
  let pts = [];
  for (let i=0;i<n;i++){
    const x = (i/(n-1))*(svgW);
    const yNorm = (ys[i]-minY)/(maxY-minY);
    const y = (svgH - pad) - yNorm*(svgH - 2*pad);
    pts.push(`${x.toFixed(1)},${y.toFixed(1)}`);
  }
  document.getElementById("line").setAttribute("points", pts.join(" "));
}

function renderDays(days){
  const tb = document.getElementById("days");
  tb.innerHTML = "";
  for (const d of days){
    const tr = document.createElement("tr");
    tr.innerHTML = `<td>${d.dayStartLocal}</td><td>${d.avgWind.toFixed(2)}</td><td>${d.maxWind.toFixed(2)}</td>`;
    tb.appendChild(tr);
  }
}

function numOrDash(v, digits=1){
  if (v === null || v === undefined) return "--";
  if (!isFinite(v)) return "--";
  return Number(v).toFixed(digits);
}

function bytesPretty(n){
  if (!isFinite(n)) return "";
  if (n < 1024) return `${n} B`;
  if (n < 1024*1024) return `${(n/1024).toFixed(1)} KB`;
  return `${(n/(1024*1024)).toFixed(1)} MB`;
}

async function loadFiles(dir){
  const target = document.getElementById(dir === 'data' ? 'files_data' : 'files_chunks');
  target.textContent = "Loading...";
  try{
    const j = await fetchJSON(`/api/files?dir=${encodeURIComponent(dir)}`);
    if (!j.ok){
      target.textContent = `Error: ${j.error || 'unknown'}`;
      return;
    }
    const files = (j.files || []).slice().sort((a,b)=> (b.path||'').localeCompare(a.path||''));
    if (!files.length){
      target.textContent = "(none)";
      return;
    }
    let html = "<ul style='margin:8px 0; padding-left:18px'>";
    for (const f of files){
      const p = f.path;
      const s = bytesPretty(f.size);
      const dl = `/download?path=${encodeURIComponent(p)}`;
      html += `<li><a href="${dl}">${p}</a> <span class="muted">(${s})</span></li>`;
    }
    html += "</ul>";
    target.innerHTML = html;
  }catch(e){
    target.textContent = "Error loading list";
  }
}

function downloadZip(){
  const n = parseInt(document.getElementById("zipdays").value || "7", 10);
  const days = (isFinite(n) && n > 0) ? n : 7;
  window.location = `/download_zip?days=${encodeURIComponent(days)}`;
}

async function tick(){
  try{
    const now = await fetchJSON("/api/now");
    document.getElementById("now").textContent = numOrDash(now.wind_ms, 2);
    document.getElementById("pps").textContent = numOrDash(now.wind_pps, 1);
    document.getElementById("maxboot").textContent = numOrDash(now.wind_max_since_boot, 2);
    document.getElementById("tlocal").textContent = now.local_time || "--";

    document.getElementById("temp").textContent = numOrDash(now.temp_c, 1);
    document.getElementById("rh").textContent = numOrDash(now.hum_rh, 1);
    document.getElementById("pres").textContent = numOrDash(now.press_hpa, 1);

    document.getElementById("bmeok").textContent = now.bme280_ok ? "OK" : "ERR";
    document.getElementById("sdok").textContent  = now.sd_ok ? "OK" : "ERR";

    const buckets = await fetchJSON("/api/5min");
    renderLine(buckets.buckets);

    const days = await fetchJSON("/api/days");
    renderDays(days.days);
  }catch(e){}
}
tick();
setInterval(tick, 2000);

loadFiles('data');
loadFiles('chunks');
</script>

</body>
</html>
)HTML";
  server.send(200, "text/html", html);
}

void handleApiNow() {
  float pps = (PPS_TO_MS > 0.0f) ? (gNowWindMS / PPS_TO_MS) : 0.0f;
  time_t nowE = epochNow();
  float press_hpa = isfinite(gPressurePa) ? (gPressurePa / 100.0f) : NAN;

  String out = "{";
  out += "\"epoch\":" + String((uint32_t)nowE) + ",";
  out += "\"local_time\":\"" + fmtLocal(nowE) + "\",";

  out += "\"wind_pps\":" + String(pps, 3) + ",";
  out += "\"wind_ms\":" + String(gNowWindMS, 3) + ",";
  out += "\"wind_max_since_boot\":" + String(gNowWindMaxSinceBoot, 3) + ",";

  out += "\"bme280_ok\":" + String(gBmeOk ? "true" : "false") + ",";
  out += "\"temp_c\":" + String(gTempC, 2) + ",";
  out += "\"hum_rh\":" + String(gHumRH, 2) + ",";
  out += "\"press_hpa\":" + String(press_hpa, 2) + ",";

  out += "\"sd_ok\":" + String(gSdOk ? "true" : "false") + ",";
  out += "\"retention_days\":" + String(RETENTION_DAYS);
  out += "}";
  server.send(200, "application/json", out);
}

void handleApi5Min() {
  String out;
  out.reserve(16 * 1024);
  out += "{";
  out += "\"now_epoch\":" + String((uint32_t)epochNow()) + ",";
  out += "\"buckets\":[";
  for (int i = 0; i < BUCKETS_24H; i++) {
    int idx = (gBucketWrite + i) % BUCKETS_24H;
    const Bucket5Min& b = gBuckets[idx];
    if (i) out += ",";
    out += "{";
    out += "\"startEpoch\":" + String((uint32_t)b.startEpoch) + ",";
    out += "\"avgWind\":" + String(b.avgWind, 3) + ",";
    out += "\"maxWind\":" + String(b.maxWind, 3) + ",";
    out += "\"samples\":" + String(b.samples);
    out += "}";
  }
  out += "]}";
  server.send(200, "application/json", out);
}

void handleApiDays() {
  String out;
  out.reserve(8 * 1024);
  out += "{";
  out += "\"days\":[";
  uint32_t count = gDaysCount;
  for (uint32_t i = 0; i < count; i++) {
    int idx = (gDayWrite + (int)i) % DAYS_HISTORY;
    const DaySummary& d = gDays[idx];
    if (i) out += ",";
    out += "{";
    out += "\"dayStartEpoch\":" + String((uint32_t)d.dayStartEpoch) + ",";
    out += "\"dayStartLocal\":\"" + fmtLocal(d.dayStartEpoch) + "\",";
    out += "\"avgWind\":" + String(d.avgWind, 3) + ",";
    out += "\"maxWind\":" + String(d.maxWind, 3);
    out += "}";
  }
  out += "]}";
  server.send(200, "application/json", out);
}

// ------------------- SETUP HELPERS -------------------

bool syncTimeWithNTP(uint32_t timeoutMs = 20000) {
  setenv("TZ", TZ_AU_SYDNEY, 1);
  tzset();
  configTime(0, 0, NTP1, NTP2, NTP3);

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
  if (!SD_ENABLE) return;

  // If your board needs explicit SPI pins, do it here, e.g.:
  // SPI.begin(SCK, MISO, MOSI, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN)) {
    gSdOk = false;
    return;
  }
  gSdOk = true;

  ensureDir("/data");
  ensureDir("/chunks");
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

  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), onPulse, RISING);

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
  if (BME_ENABLE) {
    gBmeOk = bme.begin(BME_I2C_ADDR);
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

  initSD();

  time_t nowE = epochNow();
  gTodayMidnightEpoch = localMidnight(nowE);
  deleteOldDailyFileIfNeeded(gTodayMidnightEpoch);

  time_t aligned = floorTo5MinBoundaryLocal(nowE);
  startBucketAt(aligned);

  gLastPpsMillis = millis();
  gLastWindSampleMillis = millis();
  gLastBmePollMillis = millis();

  // routes
  server.on("/", handleRoot);
  server.on("/api/now", handleApiNow);
  server.on("/api/5min", handleApi5Min);
  server.on("/api/days", handleApiDays);

  server.on("/api/files", handleApiFiles);
  server.on("/download", handleDownload);
  server.on("/download_zip", handleDownloadZip);

  server.begin();
}

void loop() {
  server.handleClient();
  uint32_t msNow = millis();

  // Wind PPS window (elapsed-time scaled, read-and-zero)
  if (msNow - gLastPpsMillis >= PPS_WINDOW_MS) {
    uint32_t elapsedMs = msNow - gLastPpsMillis;

    uint32_t delta;
    noInterrupts();
    delta = gPulseCount;
    gPulseCount = 0;
    interrupts();

    float pps = (elapsedMs > 0) ? (delta * 1000.0f / (float)elapsedMs) : 0.0f;
    float ms = pps * PPS_TO_MS;

    gNowWindMS = ms;
    if (gNowWindMS > gNowWindMaxSinceBoot) gNowWindMaxSinceBoot = gNowWindMS;

    // advance by actual elapsed so we don't accumulate drift
    gLastPpsMillis = msNow;
  }


  // Sample wind into bucket
  if (msNow - gLastWindSampleMillis >= NOW_SAMPLE_MS) {
    gBucketWindSum += gNowWindMS;
    if (gNowWindMS > gBucketWindMax) gBucketWindMax = gNowWindMS;
    gBucketSamples++;
    gLastWindSampleMillis += NOW_SAMPLE_MS;
  }

  // Poll BME
  if (BME_ENABLE && gBmeOk && (msNow - gLastBmePollMillis >= BME_POLL_MS)) {
    pollBME();
    gLastBmePollMillis += BME_POLL_MS;
  }

  // Rollover day + finalize/catch-up to 5-min boundary
  time_t nowE = epochNow();
  maybeRolloverDay(nowE);

  time_t aligned = floorTo5MinBoundaryLocal(nowE);
  if (!timeIsValid(gCurrentBucketStart)) startBucketAt(aligned);

  while (gCurrentBucketStart < aligned) {
    finalizeCurrentBucket(gCurrentBucketStart);

    gCurrentBucketStart += 5 * 60;
    gBucketWindSum = 0.0f;
    gBucketWindMax = 0.0f;
    gBucketSamples = 0;
  }
}
