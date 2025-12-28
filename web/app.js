// Configuration loaded from /api/config
let CONFIG = null;
let PLOT_KEYS = [];
let FILES_PER_PAGE = 30;
let MAX_PLOT_POINTS = 500;

async function fetchJSON(url, { timeoutMs = 6000 } = {}){
  const ctrl = new AbortController();
  const to = setTimeout(() => ctrl.abort(), timeoutMs);
  try{
    const r = await fetch(url, {cache:"no-store", signal: ctrl.signal});
    let txt = await r.text();
    if (!r.ok) throw new Error(`${url}: ${r.status}`);
    try{
      txt = txt.replace(/,(\s*,)+/g, ',').replace(/,(\s*)\]/g, '$1]').replace(/\[(\s*),/g, '[$1');
      return JSON.parse(txt);
    }catch(e){
      throw new Error(`parse ${url}: ${e.message}`);
    }
  }catch(e){
    if (e.name === "AbortError") throw new Error(`timeout ${url}`);
    throw e;
  } finally {
    clearTimeout(to);
  }
}

// Default fallback configuration
function getDefaultConfig() {
  return {
    plots: [
      {
        id: "wind",
        title: "Wind Speed",
        unit: "km/h",
        conversionFactor: 3.6,
        series: [
          { field: "avgWind", color: "black", label: "Wind" },
          { field: "maxWind", color: "#f0ad4e", label: "Gust" }
        ]
      },
      {
        id: "temp",
        title: "Temperature",
        unit: "°C",
        conversionFactor: 1.0,
        series: [{ field: "avgTempC", color: "#d9534f", label: "Temp" }]
      },
      {
        id: "hum",
        title: "Humidity",
        unit: "%",
        conversionFactor: 1.0,
        series: [{ field: "avgHumRH", color: "#0275d8", label: "Humidity" }]
      },
      {
        id: "press",
        title: "Pressure",
        unit: "hPa",
        conversionFactor: 1.0,
        series: [{ field: "avgPressHpa", color: "#5cb85c", label: "Pressure" }]
      }
    ],
    tableColumns: [
      { field: "day", label: "Day", unit: "", conversionFactor: 1.0, decimals: 0, bgColor: "" },
      { field: "avgWind", label: "Avg wind", unit: "km/h", conversionFactor: 3.6, decimals: 1, bgColor: "#f8f8f8" },
      { field: "maxWind", label: "Max wind", unit: "km/h", conversionFactor: 3.6, decimals: 1, bgColor: "#f8f8f8" },
      { field: "avgTemp", label: "Avg temp", unit: "°C", conversionFactor: 1.0, decimals: 1, bgColor: "" },
      { field: "minTemp", label: "Min temp", unit: "°C", conversionFactor: 1.0, decimals: 1, bgColor: "" },
      { field: "maxTemp", label: "Max temp", unit: "°C", conversionFactor: 1.0, decimals: 1, bgColor: "" },
      { field: "avgHum", label: "Avg RH", unit: "%", conversionFactor: 1.0, decimals: 1, bgColor: "#f8f8f8" },
      { field: "minHum", label: "Min RH", unit: "%", conversionFactor: 1.0, decimals: 1, bgColor: "#f8f8f8" },
      { field: "maxHum", label: "Max RH", unit: "%", conversionFactor: 1.0, decimals: 1, bgColor: "#f8f8f8" },
      { field: "avgPress", label: "Avg press", unit: "hPa", conversionFactor: 1.0, decimals: 1, bgColor: "" },
      { field: "minPress", label: "Min press", unit: "hPa", conversionFactor: 1.0, decimals: 1, bgColor: "" },
      { field: "maxPress", label: "Max press", unit: "hPa", conversionFactor: 1.0, decimals: 1, bgColor: "" }
    ],
    filesPerPage: 30,
    maxPlotPoints: 500
  };
}

// Load configuration from API
async function loadConfig() {
  try {
    CONFIG = await fetchJSON('/api/config');
    console.log('Config loaded:', CONFIG);
  } catch (err) {
    console.error('Failed to load config:', err);
    CONFIG = getDefaultConfig();
  }

  // Extract plot keys from config
  PLOT_KEYS = CONFIG.plots.map(p => p.id);

  // Update global constants from config
  FILES_PER_PAGE = CONFIG.filesPerPage || 30;
  MAX_PLOT_POINTS = CONFIG.maxPlotPoints || 500;

  // Generate UI
  generatePlots();
  generateTableHeaders();
}

// Generate plot SVG elements dynamically
function generatePlots() {
  const container = document.getElementById('plots_container');
  if (!container) return;

  container.innerHTML = '';

  for (const plot of CONFIG.plots) {
    const div = document.createElement('div');

    // Build series elements
    let seriesHTML = '';
    plot.series.forEach((s, idx) => {
      // Generate correct line IDs
      let lineId;
      if (plot.series.length > 1) {
        // For multi-series (like wind), use specific names
        if (s.field.includes('max') || s.field.includes('Max')) {
          lineId = `line_${plot.id}_max`;
        } else {
          lineId = `line_${plot.id}`;
        }
      } else {
        lineId = `line_${plot.id}`;
      }

      // Generate correct dot IDs
      let dotId;
      if (plot.series.length > 1) {
        dotId = `hover_dot_${plot.id}_${s.field.includes('max') || s.field.includes('Max') ? 'max' : 'avg'}`;
      } else {
        dotId = `hover_dot_${plot.id}`;
      }

      seriesHTML += `
        <polyline id="${lineId}"
                  fill="none" stroke="${s.color}" stroke-width="2" points=""
                  clip-path="url(#plotClip_${plot.id})"></polyline>
        <circle id="${dotId}"
                cx="0" cy="0" r="4" fill="${s.color}" stroke="#fff" stroke-width="1" opacity="0"></circle>
      `;
    });

    div.innerHTML = `
      <div class="flex-between">
        <div class="muted">${plot.title} (${plot.unit})</div>
        <button class="small reset-btn" onclick="resetZoom('${plot.id}')" id="reset_${plot.id}">Reset Zoom</button>
      </div>
      <svg viewBox="0 0 600 140" preserveAspectRatio="none"
           onmousemove="plotMouseMove('${plot.id}', event)"
           onmouseleave="plotMouseLeave('${plot.id}')"
           onmousedown="plotMouseDown('${plot.id}', event)"
           onmouseup="plotMouseUp('${plot.id}', event)"
           ondblclick="resetZoom('${plot.id}')"
           style="cursor:crosshair;">
        <defs>
          <clipPath id="plotClip_${plot.id}">
            <rect x="60" y="8" width="530" height="106"/>
          </clipPath>
        </defs>
        <g class="yaxis" id="axis_${plot.id}"></g>
        <g class="xaxis" id="axis_x_${plot.id}"></g>
        <line id="hover_line_${plot.id}" x1="0" y1="0" x2="0" y2="0" stroke="#bbb" stroke-width="1" stroke-dasharray="4 3" opacity="0"></line>
        ${seriesHTML}
        <rect id="select_rect_${plot.id}" x="0" y="0" width="0" height="0" fill="rgba(100,150,250,0.2)" stroke="rgba(100,150,250,0.6)" stroke-width="1" opacity="0"></rect>
      </svg>
    `;

    container.appendChild(div);

    // Initialize plot state
    plotState[plot.id] = {
      series: null,
      xDomain: null,
      zoomXDomain: null,
      label: plot.title,
      unit: plot.unit,
      config: plot
    };
  }
}

// Generate table headers dynamically
function generateTableHeaders() {
  const thead = document.getElementById('table_header');
  if (!thead) return;

  // First row: labels
  let row1 = '<tr>';
  for (const col of CONFIG.tableColumns) {
    row1 += `<th>${col.label}</th>`;
  }
  row1 += '</tr>';

  // Second row: units
  let row2 = '<tr>';
  for (const col of CONFIG.tableColumns) {
    const unitText = col.unit ? `(${col.unit})` : '';
    row2 += `<th>${unitText}</th>`;
  }
  row2 += '</tr>';

  thead.innerHTML = row1 + row2;
}

const chartDims = { width: 600, height: 140, padLeft: 60, padRight: 10, padTop: 8, padBottom: 26 };
const plotState = {
  wind: { series: null, xDomain: null, zoomXDomain: null, label: "Wind", unit: "km/h" },
  temp: { series: null, xDomain: null, zoomXDomain: null, label: "Temp", unit: "°C" },
  hum:  { series: null, xDomain: null, zoomXDomain: null, label: "Humidity", unit: "%" },
  press:{ series: null, xDomain: null, zoomXDomain: null, label: "Pressure", unit: "hPa" }
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
  PLOT_KEYS.forEach(k => setHoverLine(k, null));
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
  PLOT_KEYS.slice(1).forEach(k => setHoverDot(k, null, null));
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
  const relX = (evt.clientX - rect.left) * (chartDims.width / rect.width);
  const dims = chartDims, xRange = dims.width - dims.padLeft - dims.padRight;
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

  const cursorFraction = (evt.clientX - rect.left) / rect.width;
  const tooltipOffsetX = cursorFraction > 0.66 ? -12 : 12;
  tooltipEl.style.left = cursorFraction > 0.66 ? `${evt.clientX + tooltipOffsetX - tooltipEl.offsetWidth}px` : `${evt.clientX + tooltipOffsetX}px`;
  tooltipEl.style.top = `${evt.clientY + 12}px`;
  let xPx = null, yPx = null;
  if (xDomain && xRange > 0 && isFinite(xDomain.min) && isFinite(xDomain.max) && xDomain.max > xDomain.min) {
    const normX = (best.e - xDomain.min) / (xDomain.max - xDomain.min);
    const clamped = Math.max(0, Math.min(1, normX));
    xPx = dims.padLeft + clamped * xRange;
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
  if (evt.button !== 0) return;
  const now = Date.now();
  const timeSinceLastClick = now - dragState.lastClickTime;
  dragState.lastClickTime = now;

  // Double-click detected - reset zoom
  if (timeSinceLastClick < 300) {
    resetZoom(key);
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
    dragState.currentX = relX;
    updateSelectionRect(key);
    hideTooltip();
    evt.preventDefault();
  } else {
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

  if (dragWidth < 20) {
    hideSelectionRect(key);
    return;
  }

  const state = plotState[key];
  if (!state || !state.xDomain) {
    hideSelectionRect(key);
    return;
  }

  if (!state.originalXDomain) {
    state.originalXDomain = { ...state.xDomain };
  }

  let normStart = (minX - dims.padLeft) / xRange;
  let normEnd = (maxX - dims.padLeft) / xRange;

  // Clamp to 0-1 range to handle edge cases
  normStart = Math.max(0, Math.min(1, normStart));
  normEnd = Math.max(0, Math.min(1, normEnd));

  const currentDomain = state.zoomXDomain || state.xDomain;
  const span = currentDomain.max - currentDomain.min;

  const newZoomDomain = {
    min: currentDomain.min + normStart * span,
    max: currentDomain.min + normEnd * span
  };

  PLOT_KEYS.forEach(plotKey => {
    const plotSt = plotState[plotKey];
    if (plotSt) {
      if (!plotSt.originalXDomain && plotSt.xDomain) {
        plotSt.originalXDomain = { ...plotSt.xDomain };
      }
      plotSt.zoomXDomain = { ...newZoomDomain };
    }
  });

  hideSelectionRect(key);
  PLOT_KEYS.forEach(reRenderPlot);
  setResetButtons(true);
}

function setResetButtons(visible){
  PLOT_KEYS.forEach(plotKey => {
    const resetBtn = document.getElementById(`reset_${plotKey}`);
    if (resetBtn) resetBtn.style.display = visible ? "block" : "none";
  });
}

function resetZoom(key){
  PLOT_KEYS.forEach(plotKey => {
    const state = plotState[plotKey];
    if (state) {
      state.zoomXDomain = null;
      state.originalXDomain = null;
    }
  });
  PLOT_KEYS.forEach(reRenderPlot);
  setResetButtons(false);
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
    if (val === null) continue;
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

  const displayDomain = (plotState[key] && plotState[key].zoomXDomain) ? plotState[key].zoomXDomain : xDomain;

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

  const displayDomain = plotState.wind.zoomXDomain ? plotState.wind.zoomXDomain : xDomain;

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
    const tr = document.createElement("tr");

    for (const col of CONFIG.tableColumns) {
      const td = document.createElement("td");

      if (col.field === "day") {
        const dayLabel = (d.dayStartLocal && typeof d.dayStartLocal === "string")
          ? d.dayStartLocal.split(" ")[0] : "--";
        td.textContent = dayLabel;
      } else {
        const value = d[col.field];
        if (value != null && isFinite(value)) {
          const converted = value * col.conversionFactor;
          td.textContent = converted.toFixed(col.decimals);
        } else {
          td.textContent = "--";
        }
      }

      if (col.bgColor) {
        td.style.backgroundColor = col.bgColor;
      }

      tr.appendChild(td);
    }

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

    if (navEl) {
      navEl.style.display = files.length > FILES_PER_PAGE ? "flex" : "none";
    }

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

    const totalPages = Math.ceil(files.length / FILES_PER_PAGE);
    const currentPage = Math.floor(offset / FILES_PER_PAGE) + 1;

    if (infoEl) {
      const start = offset + 1;
      const end = Math.min(offset + FILES_PER_PAGE, files.length);
      infoEl.textContent = `${start}-${end} of ${files.length}`;
    }
    if (prevBtn) prevBtn.disabled = offset === 0;
    if (nextBtn) nextBtn.disabled = offset + FILES_PER_PAGE >= files.length;

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
      filesState[dir].offset = 0;
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
      filesState.data.offset = 0;
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

    const freeHeap = now.free_heap !== null && now.free_heap !== undefined ? now.free_heap : 0;
    const heapSize = now.heap_size !== null && now.heap_size !== undefined ? now.heap_size : 0;
    const usedHeap = heapSize > 0 ? heapSize - freeHeap : 0;
    const pctUsed = heapSize > 0 ? Math.round((usedHeap / heapSize) * 100) : 0;
    document.getElementById("ram").textContent = heapSize > 0 ? `${bytesPretty(freeHeap)} free (${pctUsed}% used)` : "--";

    const [bucketsRes, daysRes] = await Promise.allSettled([
      fetchJSON("/api/buckets_ui", { timeoutMs: 10000 }),
      fetchJSON("/api/days", { timeoutMs: 5000 })
    ]);

    if (bucketsRes.status === "fulfilled") {
      let series = Array.isArray(bucketsRes.value.buckets) ? bucketsRes.value.buckets : [];
      series = series.filter(b => Array.isArray(b) && b.length >= 7);
      series = series.map(b => {
        // Convert array format to object format
        // Array format: [epoch, avgWind, maxWind, samples, tempC, humRH, pressHpa]
        // All values are already in full precision (no conversion needed)
        return {
          startEpoch: b[0],
          avgWind: b[1],
          maxWind: b[2],
          samples: b[3] || 0,
          avgTempC: b[4],
          avgHumRH: b[5],
          avgPressHpa: b[6]
        };
      });
      const bucketSeconds = bucketsRes.value.bucket_seconds || "--";
      document.getElementById("bucket_sec").textContent = bucketSeconds;
      renderWind(series);
      renderSingleSeries(series, "avgTempC", "line_temp", "#d9534f", "axis_temp", "axis_x_temp");
      renderSingleSeries(series, "avgHumRH", "line_hum", "#0275d8", "axis_hum", "axis_x_hum");
      renderSingleSeries(series, "avgPressHpa", "line_press", "#5cb85c", "axis_press", "axis_x_press");
    } else {
      console.warn("buckets", bucketsRes.reason);
    }

    if (daysRes.status === "fulfilled") {
      if (!Array.isArray(daysRes.value.days)) throw new Error("days array missing");
      renderDays(daysRes.value.days);
    } else {
      console.warn("days", daysRes.reason);
    }
  }catch(e){
    console.warn("tick", e);
  }
}

// Initialize application
(async function init() {
  await loadConfig();
  tick();
  setInterval(tick, 2000);
  loadFiles('data');
})();
