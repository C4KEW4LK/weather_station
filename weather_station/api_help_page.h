#pragma once
static const char API_HELP_HTML[] PROGMEM = R"HTML(

<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Weather Station API</title>
  <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><circle cx='50' cy='40' r='18' fill='%23FFB000'/><circle cx='50' cy='40' r='12' fill='%23FFC933'/><path d='M50 10v8M50 62v8M78 40h8M14 40h8M71 19l6-6M23 61l6-6M71 61l6 6M23 19l6 6' stroke='%23FFB000' stroke-width='3' stroke-linecap='round'/><ellipse cx='30' cy='70' rx='18' ry='12' fill='%23E0E0E0'/><ellipse cx='50' cy='75' rx='22' ry='14' fill='%23F0F0F0'/><ellipse cx='70' cy='70' rx='16' ry='11' fill='%23E8E8E8'/></svg>"/>
  <style>
    body { font-family: system-ui, sans-serif; margin: 18px; line-height: 1.5; }
    h2 { margin-bottom: 6px; }
    .card { border: 1px solid #ddd; border-radius: 12px; padding: 14px; margin: 10px 0; }
    code { background:#f6f6f6; padding:2px 6px; border-radius: 8px; }
    .muted { color:#666; }
    a { color: inherit; }
    pre { margin-top: 8px; }
  </style>
</head>
<body>
  <h2>API Reference</h2>
  <div class="muted">Endpoints served by this device.</div>
  <div style="margin-top:10px;"><a href="/"><- Back to dashboard</a></div>
  <div class="muted" style="margin-top:8px;">Password-protected endpoints enforce up to 10 attempts/hour; correct password resets the counter. Failed requests will return 401, and 429 when rate-limited.</div>

  <div class="card">
    <div><code>/api/now</code></div>
    <div class="muted">Current readings and status flags.<br>
    Units: wind_ms (m/s), temp_c (°C), hum_rh (%), press_hpa (hPa), pm1/pm25/pm10 (μg/m³), cpu_temp_c (°C), wifi_rssi (dBm), uptime_s (seconds)</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "epoch": 1734492345,
  "local_time": "2025-12-18 22:25",
  "wind_pps": 0.7,
  "wind_ms": 1.2,
  "bme280_ok": true,
  "temp_c": 23.4,
  "hum_rh": 55.1,
  "press_hpa": 1012.3,
  "pms5003_ok": true,
  "pm1": 5.2,
  "pm25": 12.8,
  "pm10": 18.4,
  "aqi_pm25": 52,
  "aqi_pm25_category": "Moderate",
  "aqi_pm10": 45,
  "aqi_pm10_category": "Good",
  "sd_ok": true,
  "cpu_temp_c": 45.2,
  "uptime_s": 12345,
  "retention_days": 360,
  "wifi_rssi": -65,
  "free_heap": 245000,
  "heap_size": 327680
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/buckets</code></div>
    <div class="muted">Last 24h bucketed data with descriptive property names. Chunked transfer.<br>
    Units: wind_speed (m/s), temperature (°C), humidity (%), pressure (hPa), pm1/pm25/pm10 (μg/m³)</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "now_epoch": 1734492345,
  "bucket_seconds": 60,
  "buckets": [
    {
      "timestamp": 1734489600,
      "wind_speed_avg": 0.8,
      "wind_speed_max": 2.1,
      "wind_speed_samples": 12,
      "temperature": 22.9,
      "humidity": 56.0,
      "pressure": 1012.1,
      "pm1": 5.2,
      "pm25": 12.8,
      "pm10": 18.4
    }
  ]
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/buckets_compact</code></div>
    <div class="muted">Compact array format (no keys). Full precision floats. Chunked with batching.<br>
    Array: [epoch, avgWind, maxWind, samples, tempC, humRH, pressHpa, pm1, pm25, pm10]</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "now_epoch": 1734492345,
  "bucket_seconds": 60,
  "buckets": [
    [1734489600, 0.8, 2.1, 12, 22.9, 56.0, 1012.1, 5.2, 12.8, 18.4]
  ]
}
Array indices:
[0]=epoch, [1]=avgWind(m/s), [2]=maxWind(m/s), [3]=samples,
[4]=temp(°C), [5]=humidity(%), [6]=pressure(hPa),
[7]=PM1.0(μg/m³), [8]=PM2.5(μg/m³), [9]=PM10(μg/m³)</code></pre>
  </div>

  <div class="card">
    <div><code>/api/days</code></div>
    <div class="muted">Daily summaries.<br>
    Units: Wind (m/s), Temp (°C), Hum (%), Press (hPa), PM1/PM25/PM10 (μg/m³)</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "days": [
    {
      "dayStartEpoch": 1734480000,
      "dayStartLocal": "2025-12-18 00:00",
      "avgWind": 1.2,
      "maxWind": 5.8,
      "avgTemp": 24.1,
      "minTemp": 19.5,
      "maxTemp": 28.0,
      "avgHum": 58.2,
      "minHum": 44.0,
      "maxHum": 71.5,
      "avgPress": 1012.5,
      "minPress": 1008.2,
      "maxPress": 1016.8,
      "avgPM1": 5.3,
      "maxPM1": 8.2,
      "avgPM25": 12.4,
      "maxPM25": 18.9,
      "avgPM10": 18.1,
      "maxPM10": 25.7
    }
  ]
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/clear_data</code> (POST, pw)</div>
    <div class="muted">Delete all CSV files in <code>/data</code>. Requires <code>pw</code>.</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "ok": true
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/delete</code> (POST, pw)</div>
    <div class="muted">Delete a single CSV (requires <code>path</code> and <code>pw</code>).</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "ok": true
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/reboot</code> (POST, pw)</div>
    <div class="muted">Immediate reboot of the device. Requires <code>pw</code>.</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "ok": true
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/files</code></div>
    <div class="muted">List available CSV files.</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "ok": true,
  "dir": "data",
  "files": [
    { "path": "/data/20251218.csv", "size": 12345 },
    { "path": "/data/20251217.csv", "size": 12001 }
  ]
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/ui_files</code></div>
    <div class="muted">Get web UI file details (size and last modified timestamp).</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "ok": true,
  "files": [
    { "path": "/web/index.html", "size": 8192, "lastModified": 1734492345 },
    { "path": "/web/app.js", "size": 45678, "lastModified": 1734492350 }
  ]
}</code></pre>
  </div>

  <div class="card">
    <div><code>/download?filename=YYYYMMDD.csv</code></div>
    <div class="muted">Download a specific CSV file.</div>
  </div>

  <div class="card">
    <div><code>/download_zip?days=N</code></div>
    <div class="muted">Stream a ZIP of the last N daily CSV files.</div>
  </div>

  <div class="card">
    <div><code>/upload</code> (GET)</div>
    <div class="muted">Display the file upload page for uploading web files (index.html, app.js) to the SD card.</div>
  </div>

  <div class="card">
    <div><code>/upload</code> (POST, pw)</div>
    <div class="muted">Upload files to the SD card. Requires multipart form data with <code>file</code>, <code>path</code>, and <code>pw</code> fields.</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "ok": true,
  "bytes": 12345
}</code></pre>
  </div>
</body>
</html>
)HTML";
