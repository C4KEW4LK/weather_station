#pragma once
static const char ROOT_HTML[] PROGMEM = R"HTML(

<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1"/>
  <title>Weather Station</title>
  <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><circle cx='50' cy='40' r='18' fill='%23FFB000'/><circle cx='50' cy='40' r='12' fill='%23FFC933'/><path d='M50 10v8M50 62v8M78 40h8M14 40h8M71 19l6-6M23 61l6-6M71 61l6 6M23 19l6 6' stroke='%23FFB000' stroke-width='3' stroke-linecap='round'/><ellipse cx='30' cy='70' rx='18' ry='12' fill='%23E0E0E0'/><ellipse cx='50' cy='75' rx='22' ry='14' fill='%23F0F0F0'/><ellipse cx='70' cy='70' rx='16' ry='11' fill='%23E8E8E8'/></svg>"/>
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
    .btnrow button.small { padding:4px 8px; font-size:12px; }
    code { background:#f6f6f6; padding:2px 6px; border-radius: 8px; }
    input { padding: 7px 10px; border-radius: 10px; border:1px solid #ddd; width: 80px; }
    .yaxis text { font-size: 10px; fill: #555; }
    .yaxis line { stroke: #ccc; }
    .xaxis text { font-size: 10px; fill: #555; }
    .xaxis line { stroke: #ccc; }
    .tooltip { position: fixed; padding:6px 8px; border-radius:8px; background:rgba(0,0,0,0.8); color:#fff; font-size:12px; pointer-events:none; z-index:1000; }
    .plots-grid { display:grid; gap:12px; }
    .plots-grid.cols-4 { grid-template-columns: repeat(4, minmax(220px, 1fr)); }
    .plots-grid.cols-2 { grid-template-columns: repeat(2, minmax(260px, 1fr)); }
    .plots-grid.cols-1 { grid-template-columns: 1fr; }
    @media (max-width: 1200px) { .plots-grid.cols-4 { grid-template-columns: repeat(2, minmax(260px, 1fr)); } }
    @media (max-width: 720px) { .plots-grid.cols-4, .plots-grid.cols-2 { grid-template-columns: 1fr; } }
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
        <span class="pill">BME280: <span id="bmeok">--</span></span>
        <span class="pill">SD: <span id="sdok">--</span></span>
      </div>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card" style="flex:1; min-width:320px;">
      <div id="plots_container" class="plots-grid cols-4">
        <div class="card" style="min-width:260px;">
          <div class="muted">Wind Speed (km/h)</div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="hoverPlot('wind', event)" onmouseleave="hideTooltip()">
            <g class="yaxis" id="axis_wind"></g>
            <g class="xaxis" id="axis_x_wind"></g>
            <line id="hover_line_wind" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <circle id="hover_dot_wind" cx="0" cy="0" r="4" fill="#f0ad4e" stroke="#000" stroke-width="1" opacity="0"></circle>
            <polyline id="line_wind" fill="none" stroke="black" stroke-width="2" points=""></polyline>
            <polyline id="line_wind_max" fill="none" stroke="#f0ad4e" stroke-width="2" points=""></polyline>
          </svg>
        </div>
        <div class="card" style="min-width:260px;">
          <div class="muted">Temperature (&deg;C)</div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="hoverPlot('temp', event)" onmouseleave="hideTooltip()">
            <g class="yaxis" id="axis_temp"></g>
            <g class="xaxis" id="axis_x_temp"></g>
            <line id="hover_line_temp" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <circle id="hover_dot_temp" cx="0" cy="0" r="4" fill="#d9534f" stroke="#000" stroke-width="1" opacity="0"></circle>
            <polyline id="line_temp" fill="none" stroke="#d9534f" stroke-width="2" points=""></polyline>
          </svg>
        </div>
        <div class="card" style="min-width:260px;">
          <div class="muted">Humidity (%)</div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="hoverPlot('hum', event)" onmouseleave="hideTooltip()">
            <g class="yaxis" id="axis_hum"></g>
            <g class="xaxis" id="axis_x_hum"></g>
            <line id="hover_line_hum" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <circle id="hover_dot_hum" cx="0" cy="0" r="4" fill="#0275d8" stroke="#000" stroke-width="1" opacity="0"></circle>
            <polyline id="line_hum" fill="none" stroke="#0275d8" stroke-width="2" points=""></polyline>
          </svg>
        </div>
        <div class="card" style="min-width:260px;">
          <div class="muted">Pressure (hPa)</div>
          <svg viewBox="0 0 600 140" preserveAspectRatio="none" onmousemove="hoverPlot('press', event)" onmouseleave="hideTooltip()">
            <g class="yaxis" id="axis_press"></g>
            <g class="xaxis" id="axis_x_press"></g>
            <line id="hover_line_press" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
            <circle id="hover_dot_press" cx="0" cy="0" r="4" fill="#5cb85c" stroke="#000" stroke-width="1" opacity="0"></circle>
            <polyline id="line_press" fill="none" stroke="#5cb85c" stroke-width="2" points=""></polyline>
          </svg>
        </div>
      </div>
    </div>
  </div>

  <div class="row" style="margin-top: 14px;">
    <div class="card" style="flex:1; min-width:320px;">
      <div class="muted">Daily summaries</div>
      <table>
        <thead>
          <tr>
            <th>Day</th><th>Avg wind (km/h)</th><th>Max wind (km/h)</th>
            <th>Avg temp</th><th>Min temp</th><th>Max temp</th>
            <th>Avg RH</th><th>Min RH</th><th>Max RH</th>
          </tr>
        </thead>
        <tbody id="days"></tbody>
      </table>
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
          <div class="muted">Daily files (/data)</div>
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
async function fetchJSON(url, { timeoutMs = 6000 } = {}){
  const ctrl = new AbortController();
  const to = setTimeout(() => ctrl.abort(), timeoutMs);
  try{
    const r = await fetch(url, {cache:"no-store", signal: ctrl.signal});
    const txt = await r.text();
    if (!r.ok) {
      throw new Error(`fetch failed ${url}: ${r.status} body=${txt.slice(0,200)}`);
    }
    try{
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
  wind: { series: null, xDomain: null, label: "Wind", unit: "km/h" },
  temp: { series: null, xDomain: null, label: "Temp", unit: "°C" },
  hum:  { series: null, xDomain: null, label: "Humidity", unit: "%" },
  press:{ series: null, xDomain: null, label: "Pressure", unit: "hPa" }
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
  ["wind","temp","hum","press"].forEach(k => setHoverDot(k, null, null));
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
  const xDomain = state.xDomain;
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
  tooltipEl.style.left = `${evt.clientX + 12}px`;
  tooltipEl.style.top = `${evt.clientY + 12}px`;

  // vertical hover line
  let xPx = null;
  let yPx = null;
  if (xDomain && xRange > 0 && isFinite(xDomain.min) && isFinite(xDomain.max) && xDomain.max > xDomain.min) {
    const normX = (best.e - xDomain.min) / (xDomain.max - xDomain.min);
    const clamped = Math.max(0, Math.min(1, normX));
    xPx = dims.padLeft + clamped * xRange;
    const domain = state.label === "Wind" ? computeDomain(state.series, ["avgWind","maxWind"]) :
                   state.label === "Temp" ? computeDomain(state.series, ["avgTempC"]) :
                   state.label === "Humidity" ? computeDomain(state.series, ["avgHumRH"]) :
                   computeDomain(state.series, ["avgPressHpa"]);
    const spanY = domain.max - domain.min;
    if (spanY > 0 && isFinite(spanY)) {
      const yRange = dims.height - dims.padTop - dims.padBottom;
      const normY = (best.vAvg - domain.min) / spanY;
      yPx = dims.height - dims.padBottom - normY * yRange;
    }
  }
  setHoverLine(key, xPx);
  setHoverDot(key, xPx, yPx);
}

function hideTooltip(){
  tooltipEl.style.display = "none";
  clearHoverLines();
  clearHoverDots();
}

function renderAxis(axisId, domain, dims = chartDims){
  const axis = document.getElementById(axisId);
  if (!axis) return;
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
    html += `<text x="${axisX - 4}" y="${(y + 3).toFixed(1)}" text-anchor="end">${value.toFixed(1)}</text>`;
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
  const xRange = dims.width - dims.padLeft - dims.padRight;
  const baseY = dims.height - dims.padBottom + 4;
  const span = xDomain.max - xDomain.min;
  const targetTicks = 12;
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
    html += `<text x="${x.toFixed(1)}" y="${(baseY+14)}" text-anchor="middle">${hh}:${mm}</text>`;
  }
  axis.innerHTML = html;
}

function renderPolyline(values, field, elemId, domain, dims = chartDims, color, xDomain){
  const el = document.getElementById(elemId);
  if (!el) return;

  const xRange = dims.width - dims.padLeft - dims.padRight;
  const yRange = dims.height - dims.padTop - dims.padBottom;
  const span = domain.max - domain.min;
  const xSpan = xDomain ? (xDomain.max - xDomain.min) : 0;
  const n = values.length;

  let pts = [];
  for (let i=0;i<n;i++){
    const raw = values[i] ? values[i][field] : null;
    const val = (raw === null || raw === undefined || !isFinite(raw)) ? null : Number(raw);
    if (val === null) continue; // skip empty points entirely
    let x;
    const epoch = values[i] ? Number(values[i].startEpoch) : NaN;
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
  const domain = computeDomain(filtered, [field]);
  const xDomain = computeTimeDomain(filtered);
  renderAxis(axisId, domain);
  if (xAxisId) renderXAxis(xAxisId, xDomain);
  renderPolyline(filtered, field, elemId, domain, chartDims, color, xDomain);
  const key = elemId === "line_temp" ? "temp" : elemId === "line_hum" ? "hum" : "press";
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
  const domain = computeDomain(valuesKm, ["avgWind", "maxWind"]);
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
  const xDomain = computeTimeDomain(valuesKm);
  renderAxis("axis_wind", domain);
  renderXAxis("axis_x_wind", xDomain);
  renderPolyline(valuesKm, "avgWind", "line_wind", domain, chartDims, "black", xDomain);
  renderPolyline(valuesKm, "maxWind", "line_wind_max", domain, chartDims, "#f0ad4e", xDomain);
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
      `<td>${numOrDash(d.maxHum,1)}</td>`;
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

async function loadFiles(dir){
  const target = document.getElementById(dir === 'data' ? 'files_data' : 'files_chunks');
  if (!target) return;
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
      html += `<li><a href="${dl}">${p}</a> <span class="muted">(${s})</span> <button class="small" style="padding:4px 6px; border-radius:6px;" onclick="deleteFile('${p}','${dir}')">Delete</button></li>`;
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

    const [bucketsRes, daysRes] = await Promise.allSettled([
      fetchJSON("/api/buckets", { timeoutMs: 2500 }),
      fetchJSON("/api/days", { timeoutMs: 2500 })
    ]);

    if (bucketsRes.status === "fulfilled") {
      const series = Array.isArray(bucketsRes.value.buckets) ? bucketsRes.value.buckets : [];
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
