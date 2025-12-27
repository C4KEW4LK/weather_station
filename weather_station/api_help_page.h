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
    <div class="muted">Current readings and status flags.</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "epoch": 1734492345,
  "local_time": "2025-12-18 22:25",
  "wind_pps": 0.7,
  "wind_ms": 1.2,
  "wind_max_since_boot": 5.4,
  "bme280_ok": true,
  "temp_c": 23.4,
  "hum_rh": 55.1,
  "press_hpa": 1012.3,
  "sd_ok": true,
  "uptime_ms": 1234567,
  "retention_days": 360
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/buckets</code></div>
    <div class="muted">Last 24h bucketed data (<code>bucket_seconds</code> + array of buckets).</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "now_epoch": 1734492345,
  "bucket_seconds": 60,
  "buckets": [
    {
      "startEpoch": 1734489600,
      "avgWind": 0.8,
      "maxWind": 2.1,
      "samples": 12,
      "avgTempC": 22.9,
      "avgHumRH": 56.0,
      "avgPressHpa": 1012.1
    }
  ]
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/days</code></div>
    <div class="muted">Daily summaries kept in RAM.</div>
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
      "maxHum": 71.5
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
    <div><code>/api/reboot</code> (POST)</div>
    <div class="muted">Immediate reboot of the device.</div>
    <pre style="background:#f9f9f9; padding:10px; border-radius:8px; overflow:auto;"><code>{
  "ok": true
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/files?dir=data</code></div>
    <div class="muted">List CSV files in <code>/data</code>.</div>
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
    <div><code>/download?path=/data/YYYYMMDD.csv</code></div>
    <div class="muted">Download a specific CSV.</div>
  </div>

  <div class="card">
    <div><code>/download_zip?days=N</code></div>
    <div class="muted">Stream a ZIP of the last N daily CSVs (<code>/data</code> directory).</div>
  </div>
</body>
</html>
)HTML";
