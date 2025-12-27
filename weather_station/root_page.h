#pragma once
static const char ROOT_HTML[] PROGMEM = R"HTML(

<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Weather Station</title>
  <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><circle cx='50' cy='40' r='18' fill='%23FFB000'/><circle cx='50' cy='40' r='12' fill='%23FFC933'/><path d='M50 10v8M50 62v8M78 40h8M14 40h8M71 19l6-6M23 61l6-6M71 61l6 6M23 19l6 6' stroke='%23FFB000' stroke-width='3' stroke-linecap='round'/><ellipse cx='30' cy='64' rx='16' ry='10' fill='%23E0E0E0'/><ellipse cx='50' cy='60' rx='22' ry='14' fill='%23F0F0F0'/><ellipse cx='70' cy='64' rx='16' ry='10' fill='%23E8E8E8'/></svg>"/>
  <style>
body {
  font-family: system-ui, sans-serif;
  margin: 16px;
}

.row,.files,.btnrow,.flex-between {
  display:flex;
  gap:12px;
  flex-wrap:wrap;
}
.flex-between { justify-content:space-between; align-items:center; }

.card {
  border:1px solid #ddd;
  border-radius:12px;
  padding:12px;
  min-width:260px;
}
.card-full { flex:1; min-width:320px; }

.big { font-size:42px; font-weight:700; }
.small { font-size:12px; }
.muted { color:#666; }

svg {
  width:100%;
  height:140px;
  border-radius:10px;
  background:#fafafa;
}

table { width:100%; border-collapse:collapse; }
td,th {
  border-bottom:1px solid #eee;
  padding:6px 4px;
  font-size:14px;
  text-align:left;
}

.table-wrapper { position:relative; }
.table-scroll {
  overflow-x:auto;
  padding-right:20px;
}
.table-scroll table { min-width:1000px; }

.table-wrapper::after {
  content:'';
  position:absolute;
  inset:0 0 0 auto;
  width:40px;
  background:linear-gradient(to right,transparent,#fff);
  pointer-events:none;
  z-index:4;
}

.table-scroll :is(th,td):first-child {
  position:sticky;
  left:0;
  background:#fff;
  z-index:2;
}
.table-scroll thead th:first-child { z-index:3; }

.table-scroll :is(th,td):first-child::after {
  content:'';
  position:absolute;
  inset:0 0 0 auto;
  width:2px;
  background:#000;
}

.table-scroll :is(th,td):nth-child(2),
.table-scroll :is(th,td):nth-child(3),
.table-scroll :is(th,td):nth-child(7),
.table-scroll :is(th,td):nth-child(8),
.table-scroll :is(th,td):nth-child(9) {
  background:#f8f8f8;
}

.pill {
  display:inline-block;
  padding:2px 8px;
  margin:6px 6px 0 0;
  border-radius:999px;
  background:#f2f2f2;
}

a { color:inherit; }

.filecol { flex:1; min-width:320px; }

button,input {
  border:1px solid #ddd;
  border-radius:10px;
  background:#fff;
}
button {
  padding:8px 10px;
  cursor:pointer;
}
button:disabled {
  opacity:.4;
  cursor:not-allowed;
}
.btnrow button.small {
  padding:4px 8px;
  font-size:12px;
}

input { padding:7px 10px; width:80px; }

code {
  background:#f6f6f6;
  padding:2px 6px;
  border-radius:8px;
}

:is(.xaxis,.yaxis) text {
  font-size:10px;
  fill:#555;
}
:is(.xaxis,.yaxis) line {
  stroke:#ccc;
}

.tooltip {
  position:fixed;
  padding:6px 8px;
  border-radius:8px;
  background:rgba(0,0,0,.8);
  color:#fff;
  font-size:12px;
  pointer-events:none;
  z-index:1000;
}

.plots-grid {
  display:grid;
  gap:12px;
  width:100%;
  overflow-x:auto;
  box-sizing:border-box;
  grid-template-columns:repeat(1,minmax(330px,1fr));
}
@media (min-width:825px) {
  .plots-grid { grid-template-columns:repeat(2,minmax(330px,1fr)); }
}
@media (min-width:1600px) {
  .plots-grid { grid-template-columns:repeat(4,minmax(330px,1fr)); }
}

.divider {
  width:1px;
  background:#eee;
  align-self:stretch;
}

.reset-btn {
  display:none;
  padding:2px 6px;
  font-size:11px;
}

.sensor-item { flex:1; min-width:120px; }

  </style>
</head>
<body>
  <div class="flex-between" style="align-items:flex-start; gap:12px;">
    <h2>Weather Station</h2>
    <div class="muted small">Time: <span id="tlocal">--</span></div>
  </div>

  <div class="row">
    <div class="card card-full">
      <div class="muted">Current Measurement</div>
      <div class="row" style="gap:16px; align-items:center;">
        <div class="sensor-item">
          <div class="muted small" style="font-weight:700;">Wind (km/h)</div>
          <div class="big" id="now">--.-</div>
        </div>
        <div class="divider"></div>
        <div class="sensor-item">
          <div class="muted small" style="font-weight:700;">Temp (&deg;C)</div>
          <div class="big" id="temp">--.-</div>
        </div>
        <div class="divider"></div>
        <div class="sensor-item">
          <div class="muted small" style="font-weight:700;">RH (%)</div>
          <div class="big" id="rh">--.-</div>
        </div>
        <div class="divider"></div>
        <div style="flex:1; min-width:140px;">
          <div class="muted small" style="font-weight:700;">Pressure (hPa)</div>
          <div class="big" id="pres">----.-</div>
        </div>
      </div>

      <div class="row" style="margin-top:10px; gap:10px; flex-wrap:wrap;">
        <span class="pill">Uptime: <span id="uptime">--</span></span>
        <span class="pill">PPS: <span id="pps">--</span></span>
        <span class="pill">Bucket: <span id="bucket_sec">--</span>s</span>
        <span class="pill">BME280: <span id="bmeok">--</span></span>
        <span class="pill">SD: <span id="sdok">--</span></span>
        <span class="pill">WiFi: <span id="wifi">--</span> dBm (<span id="wifi_quality">--</span>)</span>
        <span class="pill">RAM: <span id="ram">--</span></span>
      </div>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card card-full">
      <div id="plots_container" class="plots-grid">
        <div>
          <div class="flex-between">
            <div class="muted">Wind Speed (km/h)</div>
            <button class="small reset-btn" onclick="resetZoom('wind')" id="reset_wind">Reset Zoom</button>
          </div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="plotMouseMove('wind', event)" onmouseleave="plotMouseLeave('wind')" onmousedown="plotMouseDown('wind', event)" onmouseup="plotMouseUp('wind', event)" ondblclick="resetZoom('wind')" style="cursor:crosshair;">
            <defs>
              <clipPath id="plotClip_wind">
                <rect x="60" y="8" width="530" height="106"/>
              </clipPath>
            </defs>
            <g class="yaxis" id="axis_wind"></g>
            <g class="xaxis" id="axis_x_wind"></g>
            <line id="hover_line_wind" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <polyline id="line_wind" fill="none" stroke="black" stroke-width="2" points="" clip-path="url(#plotClip_wind)"></polyline>
            <polyline id="line_wind_max" fill="none" stroke="#f0ad4e" stroke-width="2" points="" clip-path="url(#plotClip_wind)"></polyline>
            <circle id="hover_dot_wind_avg" cx="0" cy="0" r="4" fill="black" stroke="#fff" stroke-width="1" opacity="0"></circle>
            <circle id="hover_dot_wind_max" cx="0" cy="0" r="4" fill="#f0ad4e" stroke="#fff" stroke-width="1" opacity="0"></circle>
            <rect id="select_rect_wind" x="0" y="0" width="0" height="0" fill="rgba(100,150,250,0.2)" stroke="rgba(100,150,250,0.6)" stroke-width="1" opacity="0"></rect>
          </svg>
        </div>
        <div>
          <div class="flex-between">
            <div class="muted">Temperature (&deg;C)</div>
            <button class="small reset-btn" onclick="resetZoom('temp')" id="reset_temp">Reset Zoom</button>
          </div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="plotMouseMove('temp', event)" onmouseleave="plotMouseLeave('temp')" onmousedown="plotMouseDown('temp', event)" onmouseup="plotMouseUp('temp', event)" ondblclick="resetZoom('temp')" style="cursor:crosshair;">
            <defs>
              <clipPath id="plotClip_temp">
                <rect x="60" y="8" width="530" height="106"/>
              </clipPath>
            </defs>
            <g class="yaxis" id="axis_temp"></g>
            <g class="xaxis" id="axis_x_temp"></g>
            <line id="hover_line_temp" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <circle id="hover_dot_temp" cx="0" cy="0" r="4" fill="#d9534f" stroke="#000" stroke-width="1" opacity="0"></circle>
            <polyline id="line_temp" fill="none" stroke="#d9534f" stroke-width="2" points="" clip-path="url(#plotClip_temp)"></polyline>
            <rect id="select_rect_temp" x="0" y="0" width="0" height="0" fill="rgba(100,150,250,0.2)" stroke="rgba(100,150,250,0.6)" stroke-width="1" opacity="0"></rect>
          </svg>
        </div>
        <div>
          <div class="flex-between">
            <div class="muted">Humidity (%)</div>
            <button class="small reset-btn" onclick="resetZoom('hum')" id="reset_hum">Reset Zoom</button>
          </div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="plotMouseMove('hum', event)" onmouseleave="plotMouseLeave('hum')" onmousedown="plotMouseDown('hum', event)" onmouseup="plotMouseUp('hum', event)" ondblclick="resetZoom('hum')" style="cursor:crosshair;">
            <defs>
              <clipPath id="plotClip_hum">
                <rect x="60" y="8" width="530" height="106"/>
              </clipPath>
            </defs>
            <g class="yaxis" id="axis_hum"></g>
            <g class="xaxis" id="axis_x_hum"></g>
            <line id="hover_line_hum" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <circle id="hover_dot_hum" cx="0" cy="0" r="4" fill="#0275d8" stroke="#000" stroke-width="1" opacity="0"></circle>
            <polyline id="line_hum" fill="none" stroke="#0275d8" stroke-width="2" points="" clip-path="url(#plotClip_hum)"></polyline>
            <rect id="select_rect_hum" x="0" y="0" width="0" height="0" fill="rgba(100,150,250,0.2)" stroke="rgba(100,150,250,0.6)" stroke-width="1" opacity="0"></rect>
          </svg>
        </div>
        <div>
          <div class="flex-between">
            <div class="muted">Pressure (hPa)</div>
            <button class="small reset-btn" onclick="resetZoom('press')" id="reset_press">Reset Zoom</button>
          </div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="plotMouseMove('press', event)" onmouseleave="plotMouseLeave('press')" onmousedown="plotMouseDown('press', event)" onmouseup="plotMouseUp('press', event)" ondblclick="resetZoom('press')" style="cursor:crosshair;">
            <defs>
              <clipPath id="plotClip_press">
                <rect x="60" y="8" width="530" height="106"/>
              </clipPath>
            </defs>
            <g class="yaxis" id="axis_press"></g>
            <g class="xaxis" id="axis_x_press"></g>
            <line id="hover_line_press" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <circle id="hover_dot_press" cx="0" cy="0" r="4" fill="#5cb85c" stroke="#000" stroke-width="1" opacity="0"></circle>
            <polyline id="line_press" fill="none" stroke="#5cb85c" stroke-width="2" points="" clip-path="url(#plotClip_press)"></polyline>
            <rect id="select_rect_press" x="0" y="0" width="0" height="0" fill="rgba(100,150,250,0.2)" stroke="rgba(100,150,250,0.6)" stroke-width="1" opacity="0"></rect>
          </svg>
        </div>
      </div>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card card-full">
      <div class="muted">Daily summaries</div>
      <div class="table-wrapper">
        <div class="table-scroll">
          <table>
          <thead>
            <tr>
              <th>Day</th>
              <th>Avg wind</th><th>Max wind</th>
              <th>Avg temp</th><th>Min temp</th><th>Max temp</th>
              <th>Avg RH</th><th>Min RH</th><th>Max RH</th>
              <th>Avg press</th><th>Min press</th><th>Max press</th>
            </tr>
            <tr>
              <th></th>
              <th>(km/h)</th><th>(km/h)</th>
              <th>(&deg;C)</th><th>(&deg;C)</th><th>(&deg;C)</th>
              <th>(%)</th><th>(%)</th><th>(%)</th>
              <th>(hPa)</th><th>(hPa)</th><th>(hPa)</th>
            </tr>
          </thead>
          <tbody id="days"></tbody>
          </table>
        </div>
      </div>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card card-full">
      <div class="muted">Download data</div>

      <div class="btnrow">
        <input id="zipdays" type="number" min="1" value="7"/>
        <button onclick="downloadZip()">Download last N days as .ZIP</button>
      </div>

      <div class="files">
        <div class="filecol">
          <div class="flex-between" style="margin-bottom:4px;">
            <div class="muted">Daily files (/data)</div>
            <div id="files_data_nav" style="display:none; align-items:center; gap:8px;">
              <button class="small" onclick="navigateFiles('data', -1)" id="files_data_prev" style="padding:4px 8px;">←</button>
              <span class="muted small" id="files_data_info"></span>
              <button class="small" onclick="navigateFiles('data', 1)" id="files_data_next" style="padding:4px 8px;">→</button>
              <select id="files_data_page" onchange="goToPage('data', this.value)" style="padding:2px 4px; border-radius:6px; border:1px solid #ddd; font-size:12px;"></select>
            </div>
          </div>
          <div id="files_data" class="small"></div>
        </div>
      </div>

    </div>
  </div>

  <div class="row" style="margin-top: 10px;">
    <div class="card card-full">
      <div class="btnrow flex-between">
        <button onclick="window.location='/api_help'">API reference</button>
        <div style="display:flex; gap:8px;">
          <button style="background:#fee; border-color:#f5a3a3; color:#a00" onclick="clearSdData()">Clear SD data</button>
          <button style="background:#eef; border-color:#a3b5f5; color:#003399" onclick="rebootDevice()">Reboot</button>
        </div>
      </div>
    </div>
  </div>

<script src="/root.js"></script>

</body>
</html>
)HTML";
