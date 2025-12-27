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
    body { font-family: system-ui, sans-serif; margin: 16px; }
    .row { display: flex; gap: 12px; flex-wrap: wrap; }
    .card { border: 1px solid #ddd; border-radius: 12px; padding: 12px; min-width: 260px; }
    .big { font-size: 42px; font-weight: 700; }
    .muted { color: #666; }
    svg { width: 100%; height: 140px; border-radius: 10px; background: #fafafa; }
    table { width: 100%; border-collapse: collapse; }
    td, th { border-bottom: 1px solid #eee; padding: 6px 4px; text-align: left; font-size: 14px; }
    .table-scroll { overflow-x: auto; padding-right: 20px; }
    .table-wrapper { position: relative; }
    .table-wrapper::after { content: ''; position: absolute; top: 0; right: 0; bottom: 0; width: 40px; background: linear-gradient(to right, rgba(255,255,255,0), rgba(255,255,255,1)); pointer-events: none; z-index: 4; }
    .table-scroll table { min-width: 1000px; }
    .table-scroll th:first-child, .table-scroll td:first-child { position: sticky; left: 0; background: #fff; z-index: 2; }
    .table-scroll th:first-child::after, .table-scroll td:first-child::after { content: ''; position: absolute; top: 0; right: 0; bottom: 0; width: 2px; background: #000; }
    .table-scroll thead tr th:first-child { z-index: 3; }
    .table-scroll th:nth-child(2), .table-scroll th:nth-child(3),
    .table-scroll td:nth-child(2), .table-scroll td:nth-child(3) { background: #f8f8f8; }
    .table-scroll th:nth-child(7), .table-scroll th:nth-child(8), .table-scroll th:nth-child(9),
    .table-scroll td:nth-child(7), .table-scroll td:nth-child(8), .table-scroll td:nth-child(9) { background: #f8f8f8; }
    .pill { display:inline-block; padding:2px 8px; border-radius:999px; background:#f2f2f2; margin-right:6px; margin-top:6px; }
    .small { font-size: 12px; }
    a { color: inherit; }
    .files { display:flex; gap:12px; flex-wrap:wrap; }
    .filecol { min-width: 320px; flex:1; }
    .btnrow { display:flex; gap:8px; margin: 8px 0; flex-wrap: wrap; }
    button { padding: 8px 10px; border-radius: 10px; border:1px solid #ddd; background:#fff; cursor:pointer; }
    button:disabled { opacity: 0.4; cursor: not-allowed; }
    .btnrow button.small { padding:4px 8px; font-size:12px; }
    code { background:#f6f6f6; padding:2px 6px; border-radius: 8px; }
    input { padding: 7px 10px; border-radius: 10px; border:1px solid #ddd; width: 80px; }
    .yaxis text { font-size: 10px; fill: #555; }
    .yaxis line { stroke: #ccc; }
    .xaxis text { font-size: 10px; fill: #555; }
    .xaxis line { stroke: #ccc; }
    .tooltip { position: fixed; padding:6px 8px; border-radius:8px; background:rgba(0,0,0,0.8); color:#fff; font-size:12px; pointer-events:none; z-index:1000; }
    .plots-grid { display:grid; gap:12px; width: 100%; max-width: 100%; overflow-x: auto; box-sizing: border-box; }
    .plots-grid.cols-4 { grid-template-columns: repeat(1, minmax(330px, 1fr)); }
    .plots-grid.cols-2 { grid-template-columns: repeat(1, minmax(330px, 1fr)); }
    .plots-grid.cols-1 { grid-template-columns: repeat(1, minmax(330px, 1fr)); }
    @media (min-width: 825px) {
      .plots-grid.cols-4 { grid-template-columns: repeat(2, minmax(330px, 1fr)); }
      .plots-grid.cols-2 { grid-template-columns: repeat(2, minmax(330px, 1fr)); }
    }
    @media (min-width: 1600px) {
      .plots-grid.cols-4 { grid-template-columns: repeat(4, minmax(330px, 1fr)); }
    }
  </style>
</head>
<body>
  <div style="display:flex; justify-content:space-between; align-items:flex-start; gap:12px;">
    <h2>Weather Station</h2>
    <div class="muted small">Time: <span id="tlocal">--</span></div>
  </div>

  <div class="row">
    <div class="card" style="flex:1; min-width:320px;">
      <div class="muted">Current Measurement</div>
      <div class="row" style="gap:16px; align-items:center;">
        <div style="flex:1; min-width:120px;">
          <div class="muted small" style="font-weight:700;">Wind (km/h)</div>
          <div class="big" id="now">--.-</div>
        </div>
        <div style="width:1px; align-self:stretch; background:#eee;"></div>
        <div style="flex:1; min-width:120px;">
          <div class="muted small" style="font-weight:700;">Temp (&deg;C)</div>
          <div class="big" id="temp">--.-</div>
        </div>
        <div style="width:1px; align-self:stretch; background:#eee;"></div>
        <div style="flex:1; min-width:120px;">
          <div class="muted small" style="font-weight:700;">RH (%)</div>
          <div class="big" id="rh">--.-</div>
        </div>
        <div style="width:1px; align-self:stretch; background:#eee;"></div>
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
      </div>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card" style="flex:1; min-width:320px;">
      <div id="plots_container" class="plots-grid cols-4">
        <div>
          <div style="display:flex; justify-content:space-between; align-items:center;">
            <div class="muted">Wind Speed (km/h)</div>
            <button class="small" onclick="resetZoom('wind')" id="reset_wind" style="display:none; padding:2px 6px; font-size:11px;">Reset Zoom</button>
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
          <div style="display:flex; justify-content:space-between; align-items:center;">
            <div class="muted">Temperature (&deg;C)</div>
            <button class="small" onclick="resetZoom('temp')" id="reset_temp" style="display:none; padding:2px 6px; font-size:11px;">Reset Zoom</button>
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
          <div style="display:flex; justify-content:space-between; align-items:center;">
            <div class="muted">Humidity (%)</div>
            <button class="small" onclick="resetZoom('hum')" id="reset_hum" style="display:none; padding:2px 6px; font-size:11px;">Reset Zoom</button>
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
          <div style="display:flex; justify-content:space-between; align-items:center;">
            <div class="muted">Pressure (hPa)</div>
            <button class="small" onclick="resetZoom('press')" id="reset_press" style="display:none; padding:2px 6px; font-size:11px;">Reset Zoom</button>
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
    <div class="card" style="flex:1; min-width:320px;">
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
    <div class="card" style="flex:1; min-width:320px;">
      <div class="muted">Download data</div>

      <div class="btnrow">
        <input id="zipdays" type="number" min="1" value="7"/>
        <button onclick="downloadZip()">Download last N days as .ZIP</button>
      </div>

      <div class="files">
        <div class="filecol">
          <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;">
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
    <div class="card" style="flex:1; min-width:320px;">
      <div class="btnrow" style="justify-content:space-between; align-items:center;">
        <button onclick="window.location='/api_help'">API reference</button>
        <div style="display:flex; gap:8px;">
          <button style="background:#fee; border-color:#f5a3a3; color:#a00" onclick="clearSdData()">Clear SD data</button>
          <button style="background:#eef; border-color:#a3b5f5; color:#003399" onclick="rebootDevice()">Reboot</button>
        </div>
      </div>
    </div>
  </div>

<script>
// ------------------- CONFIG -------------------
const FILES_PER_PAGE = {{FILES_PER_PAGE}};  // Number of daily files to show per page (configured in .ino)
const MAX_PLOT_POINTS = {{MAX_PLOT_POINTS}};  // Maximum number of points to display on plots (configured in .ino)

// ------------------- HELPERS -------------------
async function fetchJSON(url, { timeoutMs = 6000 } = {}){
  const ctrl = new AbortController();
  const to = setTimeout(() => ctrl.abort(), timeoutMs);
  try{
    const r = await fetch(url, {cache:"no-store", signal: ctrl.signal});
    let txt = await r.text();
    if (!r.ok) {
      throw new Error(`fetch failed ${url}: ${r.status} body=${txt.slice(0,200)}`);
    }
    try{
      // Clean up malformed JSON: remove multiple consecutive commas in arrays
      txt = txt.replace(/,(\s*,)+/g, ',').replace(/,(\s*)\]/g, '$1]').replace(/\[(\s*),/g, '[$1');
      return JSON.parse(txt);
    }catch(e){
      throw new Error(`parse failed ${url}: ${e.message} body_snip=${txt.slice(0,200)}...`);
    }
  }catch(e){
    if (e.name === "AbortError") throw new Error(`timeout after ${timeoutMs}ms for ${url}`);
    throw e;
  } finally {
    clearTimeout(to);
  }
}

const chartDims = { width: 600, height: 140, padLeft: 60, padRight: 10, padTop: 8, padBottom: 26 };
const plotState = {
  wind: { series: null, xDomain: null, originalXDomain: null, zoomXDomain: null, label: "Wind", unit: "km/h" },
  temp: { series: null, xDomain: null, originalXDomain: null, zoomXDomain: null, label: "Temp", unit: "°C" },
  hum:  { series: null, xDomain: null, originalXDomain: null, zoomXDomain: null, label: "Humidity", unit: "%" },
  press:{ series: null, xDomain: null, originalXDomain: null, zoomXDomain: null, label: "Pressure", unit: "hPa" }
};

const dragState = {
  active: false,
  plotKey: null,
  startX: 0,
  currentX: 0,
  lastClickTime: 0
};

const tooltipEl = (() => {
  const el = document.createElement("div");
  el.className = "tooltip";
  el.style.display = "none";
  document.body.appendChild(el);
  return el;
})();

function clearAxis(axisId){
  if (!axisId) return;
  const ax = document.getElementById(axisId);
  if (ax) ax.innerHTML = "";
}

function setHoverLine(key, xPx){
  const line = document.getElementById(`hover_line_${key}`);
  if (!line) return;
  if (xPx === null || !isFinite(xPx)) {
    line.style.opacity = "0";
    return;
  }
  line.setAttribute("x1", xPx.toFixed(1));
  line.setAttribute("x2", xPx.toFixed(1));
  line.setAttribute("y1", chartDims.padTop);
  line.setAttribute("y2", chartDims.height - chartDims.padBottom);
  line.style.opacity = "1";
}

function clearHoverLines(){
  ["wind","temp","hum","press"].forEach(k => setHoverLine(k, null));
}

function setHoverDot(key, xPx, yPx){
  const dot = document.getElementById(`hover_dot_${key}`);
  if (!dot) return;
  if (xPx === null || yPx === null || !isFinite(xPx) || !isFinite(yPx)) {
    dot.style.opacity = "0";
    return;
  }
  dot.setAttribute("cx", xPx.toFixed(1));
  dot.setAttribute("cy", yPx.toFixed(1));
  dot.style.opacity = "1";
}

function clearHoverDots(){
  ["temp","hum","press"].forEach(k => setHoverDot(k, null, null));
  setHoverDot("wind_avg", null, null);
  setHoverDot("wind_max", null, null);
}

function computeDomain(values, fields){
  let min = Infinity, max = -Infinity;
  for (const field of fields){
    for (const v of values){
      const raw = v ? v[field] : null;
      const val = (raw === null || raw === undefined || !isFinite(raw)) ? null : Number(raw);
      if (val === null) continue;
      if (val < min) min = val;
      if (val > max) max = val;
    }
  }
  if (!isFinite(min) || !isFinite(max)){
    min = 0;
    max = 1;
  }
  if (Math.abs(max - min) < 1e-3) max = min + 1.0;
  return {min, max};
}

function computeTimeDomain(values){
  let min = Infinity, max = -Infinity;
  for (const v of values){
    const e = v ? Number(v.startEpoch) : NaN;
    if (!isFinite(e) || e <= 0) continue;
    if (e < min) min = e;
    if (e > max) max = e;
  }
  if (!isFinite(min) || !isFinite(max)) return null;
  if (Math.abs(max - min) < 1e-3) max = min + 1.0;
  return {min, max};
}

function downsampleData(values, maxPoints = 1000){
  if (!values || values.length <= maxPoints) return values;
  const step = values.length / maxPoints;
  const result = [];
  for (let i = 0; i < maxPoints; i++){
    const index = Math.floor(i * step);
    result.push(values[index]);
  }
  return result;
}

function filterSeries(values, predicate){
  if (!values || !values.length) return [];
  return values.filter(v => {
    if (!v) return false;
    const e = v.startEpoch ? Number(v.startEpoch) : 0;
    if (!isFinite(e) || e <= 0) return false;
    return predicate ? predicate(v) : true;
  });
}

function hoverPlot(key, evt){
  const state = plotState[key];
  if (!state || !state.series || !state.series.length) return hideTooltip();
  const rect = evt.currentTarget.getBoundingClientRect();
  if (rect.width <= 0) return hideTooltip();
  // Map pointer to SVG/viewBox coordinates so scaling does not offset the hover line.
  const relX = (evt.clientX - rect.left) * (chartDims.width / rect.width);
  const dims = chartDims;
  const xRange = dims.width - dims.padLeft - dims.padRight;
  const xDomain = state.zoomXDomain || state.xDomain;
  let targetEpoch = null;
  if (xDomain && xRange > 0){
    const norm = Math.max(0, Math.min(1, (relX - dims.padLeft) / xRange));
    targetEpoch = xDomain.min + norm * (xDomain.max - xDomain.min);
  }
  let best = null;
  for (const item of state.series){
    if (!item) continue;
    const vAvg = (key === "wind") ? item.avgWind :
                 (key === "temp") ? item.avgTempC :
                 (key === "hum") ? item.avgHumRH :
                 item.avgPressHpa;
    const vMax = (key === "wind") ? item.maxWind : null;
    if (vAvg === null || vAvg === undefined || !isFinite(vAvg)) continue;
    const e = item.startEpoch ? Number(item.startEpoch) : NaN;
    if (targetEpoch !== null && isFinite(e)){
      const dx = Math.abs(e - targetEpoch);
      if (!best || dx < best.dx) best = { item, vAvg, vMax, e, dx };
    } else {
      if (!best) best = { item, vAvg, vMax, e, dx: 0 };
    }
  }
  if (!best) return hideTooltip();
  const date = new Date(best.e * 1000);
  const hh = date.getHours().toString().padStart(2, "0");
  const mm = date.getMinutes().toString().padStart(2, "0");
  let txt = `${state.label}: ${best.vAvg.toFixed(1)} ${state.unit}`;
  if (key === "wind" && best.vMax !== null && best.vMax !== undefined && isFinite(best.vMax)) {
    txt += ` (max ${best.vMax.toFixed(1)})`;
  }
  txt += ` @ ${hh}:${mm}`;
  tooltipEl.textContent = txt;
  tooltipEl.style.display = "block";

  // Switch tooltip to left side when cursor is beyond 2/3 of the way across
  const cursorFraction = (evt.clientX - rect.left) / rect.width;
  const tooltipOffsetX = cursorFraction > 0.66 ? -12 : 12;
  tooltipEl.style.left = cursorFraction > 0.66 ? `${evt.clientX + tooltipOffsetX - tooltipEl.offsetWidth}px` : `${evt.clientX + tooltipOffsetX}px`;
  tooltipEl.style.top = `${evt.clientY + 12}px`;

  // vertical hover line
  let xPx = null;
  let yPx = null;
  if (xDomain && xRange > 0 && isFinite(xDomain.min) && isFinite(xDomain.max) && xDomain.max > xDomain.min) {
    const normX = (best.e - xDomain.min) / (xDomain.max - xDomain.min);
    const clamped = Math.max(0, Math.min(1, normX));
    xPx = dims.padLeft + clamped * xRange;
    // Filter to visible data for y-axis domain
    const visibleSeries = state.zoomXDomain ? state.series.filter(v => {
      const e = v ? Number(v.startEpoch) : NaN;
      return isFinite(e) && e >= state.zoomXDomain.min && e <= state.zoomXDomain.max;
    }) : state.series;
    const domainData = visibleSeries.length > 0 ? visibleSeries : state.series;
    const domain = state.label === "Wind" ? computeDomain(domainData, ["avgWind","maxWind"]) :
                   state.label === "Temp" ? computeDomain(domainData, ["avgTempC"]) :
                   state.label === "Humidity" ? computeDomain(domainData, ["avgHumRH"]) :
                   computeDomain(domainData, ["avgPressHpa"]);
    const spanY = domain.max - domain.min;
    if (spanY > 0 && isFinite(spanY)) {
      const yRange = dims.height - dims.padTop - dims.padBottom;
      const normY = (best.vAvg - domain.min) / spanY;
      yPx = dims.height - dims.padBottom - normY * yRange;

      // For wind, also show max dot
      if (key === "wind" && best.vMax !== null && best.vMax !== undefined && isFinite(best.vMax)) {
        const normYMax = (best.vMax - domain.min) / spanY;
        const yPxMax = dims.height - dims.padBottom - normYMax * yRange;
        setHoverDot("wind_max", xPx, yPxMax);
      }
    }
  }
  setHoverLine(key, xPx);
  if (key === "wind") {
    setHoverDot("wind_avg", xPx, yPx);
  } else {
    setHoverDot(key, xPx, yPx);
  }
}

function hideTooltip(){
  tooltipEl.style.display = "none";
  clearHoverLines();
  clearHoverDots();
}

function plotMouseDown(key, evt){
  if (evt.button !== 0) return; // Only left button

  // Detect double-click (clicks within 300ms)
  const now = Date.now();
  const timeSinceLastClick = now - dragState.lastClickTime;
  dragState.lastClickTime = now;

  // If this is a double-click, skip drag initialization
  if (timeSinceLastClick < 300) {
    return;
  }

  const rect = evt.currentTarget.getBoundingClientRect();
  const relX = (evt.clientX - rect.left) * (chartDims.width / rect.width);
  dragState.active = true;
  dragState.plotKey = key;
  dragState.startX = relX;
  dragState.currentX = relX;
}

function plotMouseMove(key, evt){
  const rect = evt.currentTarget.getBoundingClientRect();
  const relX = (evt.clientX - rect.left) * (chartDims.width / rect.width);

  if (dragState.active && dragState.plotKey === key) {
    // Update drag selection
    dragState.currentX = relX;
    updateSelectionRect(key);
    hideTooltip();
    evt.preventDefault(); // Prevent text selection while dragging
  } else {
    // Show hover tooltip
    hoverPlot(key, evt);
  }
}

function plotMouseLeave(key){
  if (dragState.active && dragState.plotKey === key) {
    dragState.active = false;
    hideSelectionRect(key);
  }
  hideTooltip();
}

function plotMouseUp(key, evt){
  if (!dragState.active || dragState.plotKey !== key) return;
  dragState.active = false;

  const dims = chartDims;
  const xRange = dims.width - dims.padLeft - dims.padRight;
  const startX = Math.max(dims.padLeft, Math.min(dims.width - dims.padRight, dragState.startX));
  const endX = Math.max(dims.padLeft, Math.min(dims.width - dims.padRight, dragState.currentX));

  const minX = Math.min(startX, endX);
  const maxX = Math.max(startX, endX);
  const dragWidth = maxX - minX;

  // Only zoom if selection is wide enough (at least 20px)
  if (dragWidth < 20) {
    hideSelectionRect(key);
    return;
  }

  const state = plotState[key];
  if (!state || !state.xDomain) {
    hideSelectionRect(key);
    return;
  }

  // Store original domain if not already zoomed
  if (!state.originalXDomain) {
    state.originalXDomain = { ...state.xDomain };
  }

  // Calculate new zoom domain
  const normStart = (minX - dims.padLeft) / xRange;
  const normEnd = (maxX - dims.padLeft) / xRange;
  const currentDomain = state.zoomXDomain || state.xDomain;
  const span = currentDomain.max - currentDomain.min;

  const newZoomDomain = {
    min: currentDomain.min + normStart * span,
    max: currentDomain.min + normEnd * span
  };

  // Apply zoom to ALL plots
  ["wind", "temp", "hum", "press"].forEach(plotKey => {
    const plotSt = plotState[plotKey];
    if (plotSt) {
      if (!plotSt.originalXDomain && plotSt.xDomain) {
        plotSt.originalXDomain = { ...plotSt.xDomain };
      }
      plotSt.zoomXDomain = { ...newZoomDomain };
    }
  });

  // Re-render all plots
  hideSelectionRect(key);
  ["wind", "temp", "hum", "press"].forEach(plotKey => {
    reRenderPlot(plotKey);
  });

  // Show all reset buttons
  ["wind", "temp", "hum", "press"].forEach(plotKey => {
    const resetBtn = document.getElementById(`reset_${plotKey}`);
    if (resetBtn) resetBtn.style.display = "block";
  });
}

function resetZoom(key){
  // Reset all plots together
  ["wind", "temp", "hum", "press"].forEach(plotKey => {
    const state = plotState[plotKey];
    if (state) {
      state.zoomXDomain = null;
      state.originalXDomain = null;
    }
  });

  // Re-render all plots
  ["wind", "temp", "hum", "press"].forEach(plotKey => {
    reRenderPlot(plotKey);
  });

  // Hide all reset buttons
  ["wind", "temp", "hum", "press"].forEach(plotKey => {
    const resetBtn = document.getElementById(`reset_${plotKey}`);
    if (resetBtn) resetBtn.style.display = "none";
  });
}

function updateSelectionRect(key){
  const rect = document.getElementById(`select_rect_${key}`);
  if (!rect) return;

  const dims = chartDims;
  const startX = Math.max(dims.padLeft, Math.min(dims.width - dims.padRight, dragState.startX));
  const endX = Math.max(dims.padLeft, Math.min(dims.width - dims.padRight, dragState.currentX));
  const minX = Math.min(startX, endX);
  const maxX = Math.max(startX, endX);

  rect.setAttribute("x", minX);
  rect.setAttribute("y", dims.padTop);
  rect.setAttribute("width", maxX - minX);
  rect.setAttribute("height", dims.height - dims.padTop - dims.padBottom);
  rect.setAttribute("opacity", "1");
}

function hideSelectionRect(key){
  const rect = document.getElementById(`select_rect_${key}`);
  if (rect) rect.setAttribute("opacity", "0");
}

function reRenderPlot(key){
  const state = plotState[key];
  if (!state || !state.series) return;

  if (key === "wind") {
    renderWind(state.series);
  } else if (key === "temp") {
    renderSingleSeries(state.series, "avgTempC", "line_temp", "#d9534f", "axis_temp", "axis_x_temp");
  } else if (key === "hum") {
    renderSingleSeries(state.series, "avgHumRH", "line_hum", "#0275d8", "axis_hum", "axis_x_hum");
  } else if (key === "press") {
    renderSingleSeries(state.series, "avgPressHpa", "line_press", "#5cb85c", "axis_press", "axis_x_press");
  }
}

function renderAxis(axisId, domain, dims = chartDims){
  const axis = document.getElementById(axisId);
  if (!axis) return;

  // Calculate scale factor to prevent text stretching
  const svg = axis.closest('svg');
  const scaleX = svg ? (svg.getBoundingClientRect().width / dims.width) : 1;
  const textScale = scaleX > 0 ? (1 / scaleX) : 1;

  const span = domain.max - domain.min;
  const ticks = 4;
  const axisX = dims.padLeft - 12;
  const usableH = dims.height - dims.padTop - dims.padBottom;
  let html = `<line x1="${axisX}" y1="${dims.padTop}" x2="${axisX}" y2="${dims.height - dims.padBottom}" stroke-width="1"/>`;
  for (let i=0; i<=ticks; i++){
    const frac = i / ticks;
    const value = domain.max - frac * span;
    const y = dims.padTop + frac * usableH;
    html += `<line x1="${axisX}" y1="${y.toFixed(1)}" x2="${(axisX+6)}" y2="${y.toFixed(1)}" stroke-width="1"/>`;
    html += `<text x="${axisX - 4}" y="${(y + 3).toFixed(1)}" text-anchor="end" transform="scale(${textScale.toFixed(3)}, 1)" transform-origin="${axisX - 4} ${(y + 3).toFixed(1)}">${value.toFixed(1)}</text>`;
  }
  axis.innerHTML = html;
}

function renderXAxis(axisId, xDomain, dims = chartDims){
  const axis = document.getElementById(axisId);
  if (!axis) return;
  if (!xDomain){
    axis.innerHTML = "";
    return;
  }

  // Calculate scale factor to prevent text stretching
  const svg = axis.closest('svg');
  const scaleX = svg ? (svg.getBoundingClientRect().width / dims.width) : 1;
  const textScale = scaleX > 0 ? (1 / scaleX) : 1;

  const xRange = dims.width - dims.padLeft - dims.padRight;
  const baseY = dims.height - dims.padBottom + 4;
  const span = xDomain.max - xDomain.min;
  const targetTicks = 10;
  const tickSeconds = Math.max(1, Math.round(span / targetTicks));
  const firstTick = Math.ceil(xDomain.min / tickSeconds) * tickSeconds;
  let html = `<line x1="${dims.padLeft}" y1="${baseY}" x2="${dims.width - dims.padRight}" y2="${baseY}" stroke-width="1"/>`;
  for (let t = firstTick; t <= xDomain.max + 1; t += tickSeconds){
    const norm = (t - xDomain.min) / span;
    const x = dims.padLeft + norm * xRange;
    const d = new Date(t * 1000);
    const hh = d.getHours().toString().padStart(2, "0");
    const mm = d.getMinutes().toString().padStart(2, "0");
    html += `<line x1="${x.toFixed(1)}" y1="${baseY}" x2="${x.toFixed(1)}" y2="${(baseY+4)}" stroke-width="1"/>`;
    html += `<text x="${x.toFixed(1)}" y="${(baseY+14)}" text-anchor="middle" transform="scale(${textScale.toFixed(3)}, 1)" transform-origin="${x.toFixed(1)} ${(baseY+14)}">${hh}:${mm}</text>`;
  }
  axis.innerHTML = html;
}

function renderPolyline(values, field, elemId, domain, dims = chartDims, color, xDomain){
  const el = document.getElementById(elemId);
  if (!el) return;

  // Downsample to max points for performance
  const downsampled = downsampleData(values, MAX_PLOT_POINTS);

  const xRange = dims.width - dims.padLeft - dims.padRight;
  const yRange = dims.height - dims.padTop - dims.padBottom;
  const span = domain.max - domain.min;
  const xSpan = xDomain ? (xDomain.max - xDomain.min) : 0;
  const n = downsampled.length;

  let pts = [];
  for (let i=0;i<n;i++){
    const raw = downsampled[i] ? downsampled[i][field] : null;
    const val = (raw === null || raw === undefined || !isFinite(raw)) ? null : Number(raw);
    if (val === null) continue; // skip empty points entirely
    let x;
    const epoch = downsampled[i] ? Number(downsampled[i].startEpoch) : NaN;
    if (xDomain && isFinite(epoch) && epoch > 0 && xSpan > 0){
      const xNorm = (epoch - xDomain.min) / xSpan;
      x = dims.padLeft + xNorm * xRange;
    } else {
      x = n > 1 ? dims.padLeft + (i/(n-1))*xRange : dims.padLeft;
    }
    const yNorm = (val - domain.min)/span;
    const y = dims.height - dims.padBottom - yNorm*yRange;
    pts.push(`${x.toFixed(1)},${y.toFixed(1)}`);
  }

  if (color) el.setAttribute("stroke", color);
  el.setAttribute("points", pts.join(" "));
}

function renderSingleSeries(values, field, elemId, color, axisId, xAxisId){
  const filtered = filterSeries(values, v => {
    const val = v ? v[field] : null;
    return val !== null && val !== undefined && isFinite(val);
  });
  if (!filtered.length){
    const el = document.getElementById(elemId);
    if (el) el.setAttribute("points", "");
    clearAxis(axisId);
    if (xAxisId) clearAxis(xAxisId);
    const key = elemId === "line_temp" ? "temp" : elemId === "line_hum" ? "hum" : "press";
    if (plotState[key]) {
      plotState[key].series = null;
      plotState[key].xDomain = null;
    }
    return;
  }
  const xDomain = computeTimeDomain(filtered);
  const key = elemId === "line_temp" ? "temp" : elemId === "line_hum" ? "hum" : "press";

  // Use zoom domain if available
  const displayDomain = (plotState[key] && plotState[key].zoomXDomain) ? plotState[key].zoomXDomain : xDomain;

  // Filter data to visible range for y-axis scaling
  const visibleData = displayDomain ? filtered.filter(v => {
    const e = v ? Number(v.startEpoch) : NaN;
    return isFinite(e) && e >= displayDomain.min && e <= displayDomain.max;
  }) : filtered;

  const domain = computeDomain(visibleData.length > 0 ? visibleData : filtered, [field]);

  renderAxis(axisId, domain);
  if (xAxisId) renderXAxis(xAxisId, displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : filtered, field, elemId, domain, chartDims, color, displayDomain);
  if (plotState[key]) {
    plotState[key].series = filtered;
    plotState[key].xDomain = xDomain;
  }
}

function renderWind(values){
  const filtered = filterSeries(values, v => {
    const a = v ? v.avgWind : null;
    const m = v ? v.maxWind : null;
    return (a !== null && a !== undefined && isFinite(a)) || (m !== null && m !== undefined && isFinite(m));
  });
  if (!filtered.length){
    ["line_wind","line_wind_max"].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.setAttribute("points", "");
    });
    clearAxis("axis_wind");
    clearAxis("axis_x_wind");
    plotState.wind.series = null;
    plotState.wind.xDomain = null;
    return;
  }
  const valuesKm = filtered.map(v => {
    if (!v) return v;
    return Object.assign({}, v, {
      avgWind: toKmh(v.avgWind),
      maxWind: toKmh(v.maxWind)
    });
  });
  const xDomain = computeTimeDomain(valuesKm);

  // Use zoom domain if available
  const displayDomain = plotState.wind.zoomXDomain ? plotState.wind.zoomXDomain : xDomain;

  // Filter data to visible range for y-axis scaling
  const visibleData = displayDomain ? valuesKm.filter(v => {
    const e = v ? Number(v.startEpoch) : NaN;
    return isFinite(e) && e >= displayDomain.min && e <= displayDomain.max;
  }) : valuesKm;

  const domain = computeDomain(visibleData.length > 0 ? visibleData : valuesKm, ["avgWind", "maxWind"]);
  const hasValid = Number.isFinite(domain.min) && Number.isFinite(domain.max);
  if (!hasValid){
    ["line_wind","line_wind_max"].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.setAttribute("points", "");
    });
    clearAxis("axis_wind");
    clearAxis("axis_x_wind");
    plotState.wind.series = null;
    plotState.wind.xDomain = null;
    return;
  }

  renderAxis("axis_wind", domain);
  renderXAxis("axis_x_wind", displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : valuesKm, "avgWind", "line_wind", domain, chartDims, "black", displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : valuesKm, "maxWind", "line_wind_max", domain, chartDims, "#f0ad4e", displayDomain);
  plotState.wind.series = valuesKm;
  plotState.wind.xDomain = xDomain;
}

function renderDays(days){
  const tb = document.getElementById("days");
  tb.innerHTML = "";
  for (const d of days){
    const dayLabel = (d.dayStartLocal && typeof d.dayStartLocal === "string") ? d.dayStartLocal.split(" ")[0] : "--";
    const tr = document.createElement("tr");
    tr.innerHTML =
      `<td>${dayLabel}</td>` +
      `<td>${numOrDash(toKmh(d.avgWind),1)}</td>` +
      `<td>${numOrDash(toKmh(d.maxWind),1)}</td>` +
      `<td>${numOrDash(d.avgTemp,1)}</td>` +
      `<td>${numOrDash(d.minTemp,1)}</td>` +
      `<td>${numOrDash(d.maxTemp,1)}</td>` +
      `<td>${numOrDash(d.avgHum,1)}</td>` +
      `<td>${numOrDash(d.minHum,1)}</td>` +
      `<td>${numOrDash(d.maxHum,1)}</td>` +
      `<td>${numOrDash(d.avgPress,1)}</td>` +
      `<td>${numOrDash(d.minPress,1)}</td>` +
      `<td>${numOrDash(d.maxPress,1)}</td>`;
    tb.appendChild(tr);
  }
}

function numOrDash(v, digits=1){
  if (v === null || v === undefined) return "--";
  if (!isFinite(v)) return "--";
  return Number(v).toFixed(digits);
}

function formatUptime(ms){
  if (!isFinite(ms) || ms < 0) return "--";
  const totalSec = Math.floor(ms / 1000);
  if (totalSec < 60) return `${totalSec}s`;
  const totalMin = Math.floor(totalSec / 60);
  if (totalMin < 60) return `${totalMin}m`;
  const totalHours = Math.floor(totalMin / 60);
  if (totalHours < 24) {
    const remMin = totalMin % 60;
    return remMin > 0 ? `${totalHours}h ${remMin}m` : `${totalHours}h`;
  }
  const days = Math.floor(totalHours / 24);
  return `${days}d`;
}
function toKmh(v){
  if (v === null || v === undefined) return null;
  if (!isFinite(v)) return null;
  return Number(v) * 3.6;
}

function bytesPretty(n){
  if (!isFinite(n)) return "";
  if (n < 1024) return `${n} B`;
  if (n < 1024*1024) return `${(n/1024).toFixed(1)} KB`;
  return `${(n/(1024*1024)).toFixed(1)} MB`;
}

function wifiQuality(rssi){
  if (rssi === null || rssi === undefined || !isFinite(rssi)) return "--";
  if (rssi >= -50) return "Excellent";
  if (rssi >= -60) return "Good";
  if (rssi >= -70) return "Fair";
  return "Weak";
}

const filesState = {
  data: { offset: 0, total: 0, files: [] }
};

async function loadFiles(dir){
  const target = document.getElementById('files_data');
  const navEl = document.getElementById('files_data_nav');
  const infoEl = document.getElementById('files_data_info');
  const prevBtn = document.getElementById('files_data_prev');
  const nextBtn = document.getElementById('files_data_next');

  if (!target) return;
  target.textContent = "Loading...";

  try{
    const j = await fetchJSON(`/api/files?dir=${encodeURIComponent(dir)}`);
    if (!j.ok){
      target.textContent = `Error: ${j.error || 'unknown'}`;
      if (navEl) navEl.style.display = "none";
      return;
    }
    const files = (j.files || []).slice().sort((a,b)=> (b.path||'').localeCompare(a.path||''));

    filesState[dir].files = files;
    filesState[dir].total = files.length;

    if (!files.length){
      target.textContent = "(none)";
      if (navEl) navEl.style.display = "none";
      return;
    }

    // Show navigation if more than FILES_PER_PAGE
    if (navEl) {
      navEl.style.display = files.length > FILES_PER_PAGE ? "flex" : "none";
    }

    // Clamp offset
    const maxOffset = Math.max(0, files.length - FILES_PER_PAGE);
    if (filesState[dir].offset > maxOffset) filesState[dir].offset = maxOffset;
    if (filesState[dir].offset < 0) filesState[dir].offset = 0;

    const offset = filesState[dir].offset;
    const pageFiles = files.slice(offset, offset + FILES_PER_PAGE);

    let html = "<ul style='margin:8px 0; padding-left:18px'>";
    for (const f of pageFiles){
      const p = f.path;
      const s = bytesPretty(f.size);
      const dl = `/download?path=${encodeURIComponent(p)}`;
      html += `<li><a href="${dl}">${p}</a> <span class="muted">(${s})</span> <button class="small" style="padding:4px 6px; border-radius:6px;" onclick="deleteFile('${p}','${dir}')">Delete</button></li>`;
    }
    html += "</ul>";
    target.innerHTML = html;

    // Update pagination info and button states
    const totalPages = Math.ceil(files.length / FILES_PER_PAGE);
    const currentPage = Math.floor(offset / FILES_PER_PAGE) + 1;

    if (infoEl) {
      const start = offset + 1;
      const end = Math.min(offset + FILES_PER_PAGE, files.length);
      infoEl.textContent = `${start}-${end} of ${files.length}`;
    }
    if (prevBtn) prevBtn.disabled = offset === 0;
    if (nextBtn) nextBtn.disabled = offset + FILES_PER_PAGE >= files.length;

    // Populate page selector
    const pageSelector = document.getElementById(`files_${dir}_page`);
    if (pageSelector) {
      pageSelector.innerHTML = '';
      for (let i = 1; i <= totalPages; i++) {
        const option = document.createElement('option');
        option.value = i;
        option.textContent = `Page ${i}`;
        if (i === currentPage) option.selected = true;
        pageSelector.appendChild(option);
      }
    }

  }catch(e){
    target.textContent = "Error loading list";
    if (navEl) navEl.style.display = "none";
  }
}

function navigateFiles(dir, delta){
  filesState[dir].offset += delta * FILES_PER_PAGE;
  loadFiles(dir);
}

function goToPage(dir, pageNum){
  const page = parseInt(pageNum, 10);
  if (!isFinite(page) || page < 1) return;
  filesState[dir].offset = (page - 1) * FILES_PER_PAGE;
  loadFiles(dir);
}

function downloadZip(){
  const n = parseInt(document.getElementById("zipdays").value || "7", 10);
  const days = (isFinite(n) && n > 0) ? n : 7;
  window.location = `/download_zip?days=${encodeURIComponent(days)}`;
}

async function deleteFile(pathRaw, dir){
  if (!confirm("Delete this file from SD?")) return;
  const pw = prompt("Password required to delete file:");
  if (!(pw && pw.trim().length)) {
    alert("Password required.");
    return;
  }
  try{
    const body = `path=${encodeURIComponent(pathRaw)}&pw=${encodeURIComponent(pw.trim())}`;
    const res = await fetch("/api/delete", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body
    });
    const txt = await res.text();
    if (res.ok){
      alert("File deleted.");
      filesState[dir].offset = 0;  // Reset to first page
      loadFiles(dir);
    } else {
      alert("Delete failed: " + txt);
    }
  }catch(e){
    alert("Error deleting file");
  }
}

async function clearSdData(){
  if (!confirm("Clear all CSV data on the SD card? This cannot be undone.")) return;
  const pw = prompt("Password required to clear SD data:");
  if (!(pw && pw.trim().length)) {
    alert("Password required.");
    return;
  }
  try{
    const res = await fetch("/api/clear_data", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `pw=${encodeURIComponent(pw.trim())}`
    });
    const txt = await res.text();
    if (res.ok){
      alert("SD data cleared.");
      filesState.data.offset = 0;  // Reset to first page
      loadFiles('data');
    } else {
      alert("Failed to clear SD data: " + txt);
    }
  }catch(e){
    alert("Error clearing SD data");
  }
}

async function rebootDevice(){
  if (!confirm("Reboot the device now?")) return;
  try{
    const res = await fetch("/api/reboot", { method: "POST" });
    if (res.ok){
      alert("Rebooting...");
    } else {
      const txt = await res.text();
      alert("Reboot failed: " + txt);
    }
  }catch(e){
    alert("Error sending reboot request");
  }
}

async function tick(){
  try{
    const now = await fetchJSON("/api/now", { timeoutMs: 2500 });
    const windKmh = toKmh(now.wind_ms);
    document.getElementById("now").textContent = numOrDash(windKmh, 1);
    document.getElementById("pps").textContent = numOrDash(now.wind_pps, 1);
    document.getElementById("uptime").textContent = formatUptime(now.uptime_ms);
    document.getElementById("tlocal").textContent = now.local_time || "--";

    document.getElementById("temp").textContent = numOrDash(now.temp_c, 1);
    document.getElementById("rh").textContent = numOrDash(now.hum_rh, 1);
    document.getElementById("pres").textContent = numOrDash(now.press_hpa, 1);

    document.getElementById("bmeok").textContent = now.bme280_ok ? "OK" : "ERR";
    document.getElementById("sdok").textContent  = now.sd_ok ? "OK" : "ERR";
    document.getElementById("wifi").textContent = now.wifi_rssi !== null && now.wifi_rssi !== undefined ? String(now.wifi_rssi) : "--";
    document.getElementById("wifi_quality").textContent = wifiQuality(now.wifi_rssi);

    const [bucketsRes, daysRes] = await Promise.allSettled([
      fetchJSON("/api/buckets", { timeoutMs: 2500 }),
      fetchJSON("/api/days", { timeoutMs: 2500 })
    ]);

    if (bucketsRes.status === "fulfilled") {
      let series = Array.isArray(bucketsRes.value.buckets) ? bucketsRes.value.buckets : [];
      // Filter out null/undefined entries that may result from malformed JSON
      series = series.filter(b => b && typeof b === 'object' && b.startEpoch);
      const bucketSeconds = bucketsRes.value.bucket_seconds || "--";
      document.getElementById("bucket_sec").textContent = bucketSeconds;
      renderWind(series);
      renderSingleSeries(series, "avgTempC", "line_temp", "#d9534f", "axis_temp", "axis_x_temp");
      renderSingleSeries(series, "avgHumRH", "line_hum", "#0275d8", "axis_hum", "axis_x_hum");
      renderSingleSeries(series, "avgPressHpa", "line_press", "#5cb85c", "axis_press", "axis_x_press");
    } else {
      console.warn("buckets fetch failed", bucketsRes.reason);
    }

    if (daysRes.status === "fulfilled") {
      if (!Array.isArray(daysRes.value.days)) throw new Error("days payload missing .days array");
      renderDays(daysRes.value.days);
    } else {
      console.warn("days fetch failed", daysRes.reason);
    }
  }catch(e){
    console.warn("tick failed", e);
  }
}
tick();
setInterval(tick, 2000);
loadFiles('data');
</script>

</body>
</html>
)HTML";
