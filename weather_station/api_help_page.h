#pragma once
static const char API_HELP_HTML[] PROGMEM = R"HTML(

<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Weather Station API</title>
  <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><circle cx='50' cy='40' r='18' fill='%23FFB000'/><circle cx='50' cy='40' r='12' fill='%23FFC933'/><path d='M50 10v8M50 62v8M78 40h8M14 40h8M71 19l6-6M23 61l6-6M71 61l6 6M23 19l6 6' stroke='%23FFB000' stroke-width='3' stroke-linecap='round'/><ellipse cx='30' cy='64' rx='16' ry='10' fill='%23E0E0E0'/><ellipse cx='50' cy='60' rx='22' ry='14' fill='%23F0F0F0'/><ellipse cx='70' cy='64' rx='16' ry='10' fill='%23E8E8E8'/></svg>"/>
  <style>
    :root{--bg-color:#fff;--text-color:#333;--card-border:#e0e0e0;--card-bg:#fff;--muted-color:#666;--code-bg:#f5f5f5;--code-border:#e8e8e8;--pre-bg:#f9f9f9;--table-stripe:#f8f8f8;--link-color:#0066cc;--button-bg:#fff;--button-border:#ddd}
    body.dark-mode{--bg-color:#1a1a1a;--text-color:#e0e0e0;--card-border:#404040;--card-bg:#2a2a2a;--muted-color:#999;--code-bg:#333;--code-border:#444;--pre-bg:#2a2a2a;--table-stripe:#333;--link-color:#66b3ff;--button-bg:#2a2a2a;--button-border:#404040}
    *{box-sizing:border-box}
    body{font-family:system-ui,-apple-system,sans-serif;margin:0;padding:20px;line-height:1.6;color:var(--text-color);background-color:var(--bg-color);max-width:1200px;margin:0 auto;transition:background-color .3s,color .3s}
    h2{margin:0 0 8px 0;font-size:1.8em;font-weight:600}
    .card{border:1px solid var(--card-border);border-radius:8px;padding:16px;margin:16px 0;background:var(--card-bg);box-shadow:0 1px 3px rgba(0,0,0,.08)}
    code{background:var(--code-bg);padding:2px 6px;border-radius:4px;font-family:'Consolas','Monaco',monospace;font-size:.9em}
    .muted{color:var(--muted-color);font-size:.95em}
    a{color:var(--link-color);text-decoration:none}
    a:hover{text-decoration:underline}
    pre{margin:10px 0;overflow-x:auto}
    pre code{display:block;background:var(--pre-bg);padding:12px;border-radius:6px;border:1px solid var(--code-border);overflow-x:auto;line-height:1.4}
    table{border-collapse:collapse;font-size:.9em;margin:10px 0}
    tr:nth-child(even){background:var(--table-stripe)}
    td{padding:4px 8px;vertical-align:top}
    td:first-child{padding-right:12px}
    td:nth-child(2){padding-right:12px;white-space:nowrap}
    b{font-weight:600;font-family:'Consolas','Monaco',monospace;font-size:.95em}
    .mt-1{margin-top:8px}
    .mt-2{margin-top:10px}
    .header-flex{display:flex;justify-content:space-between;align-items:flex-start;gap:12px;margin-bottom:8px}
    #theme-toggle{padding:8px 12px;font-size:16px;cursor:pointer;border-radius:10px;border:1px solid var(--button-border);background:var(--button-bg);color:var(--text-color);transition:transform .2s}
    #theme-toggle:hover{transform:scale(1.05)}
  </style>
</head>
<body>
  <div class="header-flex">
    <div>
      <h2>API Reference</h2>
      <div class="muted">Endpoints served by this device.</div>
    </div>
    <button id="theme-toggle" onclick="toggleTheme()" title="Toggle dark mode">☾</button>
  </div>
  <div class="mt-2"><a href="/"><- Back to dashboard</a></div>

  <div class="card">
    <div><code>/api/now</code></div>
    <div class="muted">Current readings and status flags.</div>
    <pre><code>{
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
    <table>
      <tr><td><b>wind_ms</b></td><td>m/s</td><td>wind speed in meters per second</td></tr>
      <tr><td><b>temp_c</b></td><td>°C</td><td>temperature in Celsius</td></tr>
      <tr><td><b>hum_rh</b></td><td>%</td><td>relative humidity percentage</td></tr>
      <tr><td><b>press_hpa</b></td><td>hPa</td><td>atmospheric pressure in hectopascals</td></tr>
      <tr><td><b>pm1</b></td><td>μg/m³</td><td>particulate matter 1.0 micrograms per cubic meter</td></tr>
      <tr><td><b>pm25</b></td><td>μg/m³</td><td>particulate matter 2.5 micrograms per cubic meter</td></tr>
      <tr><td><b>pm10</b></td><td>μg/m³</td><td>particulate matter 10 micrograms per cubic meter</td></tr>
      <tr><td><b>cpu_temp_c</b></td><td>°C</td><td>CPU temperature in Celsius</td></tr>
      <tr><td><b>wifi_rssi</b></td><td>dBm</td><td>WiFi signal strength in decibel-milliwatts</td></tr>
      <tr><td><b>uptime_s</b></td><td>seconds</td><td>device uptime in seconds</td></tr>
    </table>
  </div>

  <div class="card">
    <div><code>/api/buckets</code></div>
    <div class="muted">Last 24h bucketed data with descriptive property names. Chunked transfer.</div>
    <pre><code>{
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
}</code>
</pre>
    <table>
      <tr><td><b>wind_speed_avg</b></td><td>m/s</td><td>average wind speed over 1min bucket</td></tr>
      <tr><td><b>wind_speed_max</b></td><td>m/s</td><td>maximum wind speed in 1min bucket</td></tr>
      <tr><td><b>temperature</b></td><td>°C</td><td>temperature in Celsius</td></tr>
      <tr><td><b>humidity</b></td><td>%</td><td>relative humidity percentage</td></tr>
      <tr><td><b>pressure</b></td><td>hPa</td><td>atmospheric pressure in hectopascals</td></tr>
      <tr><td><b>pm1</b></td><td>μg/m³</td><td>particulate matter 1.0 micrograms per cubic meter</td></tr>
      <tr><td><b>pm25</b></td><td>μg/m³</td><td>particulate matter 2.5 micrograms per cubic meter</td></tr>
      <tr><td><b>pm10</b></td><td>μg/m³</td><td>particulate matter 10 micrograms per cubic meter</td></tr>
    </table>
  </div>

  <div class="card">
    <div><code>/api/buckets_compact</code></div>
    <div class="muted">Compact array format (no keys). Full precision floats. Chunked with batching.<br>
    Array: [epoch, avgWind, maxWind, samples, tempC, humRH, pressHpa, pm1, pm25, pm10]</div>
    <pre><code>{
  "now_epoch": 1734492345,
  "bucket_seconds": 60,
  "buckets": [
    [1734489600, 0.8, 2.1, 12, 22.9, 56.0, 1012.1, 5.2, 12.8, 18.4]
  ]
}</code></pre>
    <table>
      <tr><td><b>[0]</b></td><td>epoch</td><td>Unix timestamp</td></tr>
      <tr><td><b>[1]</b></td><td>m/s</td><td>average wind speed over 1min bucket</td></tr>
      <tr><td><b>[2]</b></td><td>m/s</td><td>maximum wind speed in 1min bucket</td></tr>
      <tr><td><b>[3]</b></td><td>samples</td><td>number of wind samples in bucket</td></tr>
      <tr><td><b>[4]</b></td><td>°C</td><td>temperature in Celsius</td></tr>
      <tr><td><b>[5]</b></td><td>%</td><td>relative humidity percentage</td></tr>
      <tr><td><b>[6]</b></td><td>hPa</td><td>atmospheric pressure in hectopascals</td></tr>
      <tr><td><b>[7]</b></td><td>μg/m³</td><td>particulate matter 1.0</td></tr>
      <tr><td><b>[8]</b></td><td>μg/m³</td><td>particulate matter 2.5</td></tr>
      <tr><td><b>[9]</b></td><td>μg/m³</td><td>particulate matter 10</td></tr>
    </table>
  </div>

  <div class="card">
    <div><code>/api/days</code></div>
    <div class="muted">Daily summaries.</div>
    <pre><code>{
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
    <table>
      <tr><td><b>avgWind</b></td><td>m/s</td><td>average wind speed for the day</td></tr>
      <tr><td><b>maxWind</b></td><td>m/s</td><td>maximum wind speed for the day</td></tr>
      <tr><td><b>avgTemp</b></td><td>°C</td><td>average temperature for the day</td></tr>
      <tr><td><b>minTemp</b></td><td>°C</td><td>minimum temperature for the day</td></tr>
      <tr><td><b>maxTemp</b></td><td>°C</td><td>maximum temperature for the day</td></tr>
      <tr><td><b>avgHum</b></td><td>%</td><td>average relative humidity for the day</td></tr>
      <tr><td><b>minHum</b></td><td>%</td><td>minimum relative humidity for the day</td></tr>
      <tr><td><b>maxHum</b></td><td>%</td><td>maximum relative humidity for the day</td></tr>
      <tr><td><b>avgPress</b></td><td>hPa</td><td>average atmospheric pressure for the day</td></tr>
      <tr><td><b>minPress</b></td><td>hPa</td><td>minimum atmospheric pressure for the day</td></tr>
      <tr><td><b>maxPress</b></td><td>hPa</td><td>maximum atmospheric pressure for the day</td></tr>
      <tr><td><b>avgPM1</b></td><td>μg/m³</td><td>average particulate matter 1.0 for the day</td></tr>
      <tr><td><b>maxPM1</b></td><td>μg/m³</td><td>maximum particulate matter 1.0 for the day</td></tr>
      <tr><td><b>avgPM25</b></td><td>μg/m³</td><td>average particulate matter 2.5 for the day</td></tr>
      <tr><td><b>maxPM25</b></td><td>μg/m³</td><td>maximum particulate matter 2.5 for the day</td></tr>
      <tr><td><b>avgPM10</b></td><td>μg/m³</td><td>average particulate matter 10 for the day</td></tr>
      <tr><td><b>maxPM10</b></td><td>μg/m³</td><td>maximum particulate matter 10 for the day</td></tr>
    </table>
  </div>

  <div class="card">
    <div><code>/api/files</code></div>
    <div class="muted">List available CSV files.</div>
    <pre><code>{
  "ok": true,
  "dir": "data",
  "files": [
    { "path": "20251218.csv", "size": 12345 },
    { "path": "20251217.csv", "size": 12001 }
  ]
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/ui_files</code></div>
    <div class="muted">Get web UI file details (size and last modified timestamp).</div>
    <pre><code>{
  "ok": true,
  "files": [
    { "file": "index.html", "size": 8192, "lastModified": 1734492345 },
    { "file": "app.js", "size": 45678, "lastModified": 1734492350 }
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

  <h2 style="margin-top:32px;">Endpoints requiring a password</h2>
  <div class="muted mt-1">These endpoints rate limit after 10 attempts/hour; correct password resets the counter. Failed requests will return 401, and 429 when rate-limited.</div>

  <div class="card">
    <div><code>/api/clear_data</code> (POST, pw)</div>
    <div class="muted">Delete all CSV files in <code>/data</code>. Requires <code>pw</code>.</div>
    <pre><code>{
  "ok": true
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/delete</code> (POST, pw)</div>
    <div class="muted">Delete a single CSV file. Requires <code>filename</code> and <code>pw</code>.</div>
    <pre><code>{
  "ok": true
}</code></pre>
  </div>

  <div class="card">
    <div><code>/api/reboot</code> (POST, pw)</div>
    <div class="muted">Immediate reboot of the device. Requires <code>pw</code>.</div>
    <pre><code>{
  "ok": true
}</code></pre>
  </div>

  <div class="card">
    <div><code>/upload</code> (POST, pw)</div>
    <div class="muted">Upload files to the SD card. Requires multipart form data with <code>file</code>, <code>path</code>, and <code>pw</code> fields.</div>
    <div class="muted mt-1">Example:</div>
    <pre><code>curl -F "file=@index.html" -F "path=/web/index.html" -F "pw=yourpassword" http://device-ip/upload</code></pre>
    <div class="muted mt-1">Response:</div>
    <pre><code>{
  "ok": true,
  "bytes": 12345
}</code></pre>
  </div>

<script>
function setCookie(n,v,d){const e=new Date();e.setTime(e.getTime()+(d*864e5));document.cookie=n+"="+v+";expires="+e.toUTCString()+";path=/"}
function getCookie(n){const e=n+"=",c=document.cookie.split(';');for(let i=0;i<c.length;i++){let t=c[i];while(t.charAt(0)===' ')t=t.substring(1);if(t.indexOf(e)===0)return t.substring(e.length)}return null}
function toggleTheme(){const b=document.body,t=document.getElementById('theme-toggle');if(b.classList.contains('dark-mode')){b.classList.remove('dark-mode');t.textContent='☾';t.title='Toggle dark mode';setCookie('theme','light',365)}else{b.classList.add('dark-mode');t.textContent='☼';t.title='Toggle light mode';setCookie('theme','dark',365)}}
(function(){const t=getCookie('theme'),b=document.getElementById('theme-toggle');if(t==='dark'){document.body.classList.add('dark-mode');b.textContent='☼';b.title='Toggle light mode'}else{b.textContent='☾';b.title='Toggle dark mode'}})();
</script>
</body>
</html>
)HTML";
