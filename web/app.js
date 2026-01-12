// Configuration loaded from /api/config
let CONFIG = null;
let PLOT_KEYS = [];
let FILES_PER_PAGE = 30;
let MAX_PLOT_POINTS = 500;

function debugLog(msg, data = null) {
  console.log(msg, data || '');
}

async function fetchJSON(url, { timeoutMs = 6000 } = {}){
  // Use AbortController for proper timeout handling
  const useAbort = typeof AbortController !== 'undefined';
  const ctrl = useAbort ? new AbortController() : null;
  const signal = useAbort ? ctrl.signal : undefined;

  let timeoutId = null;

  try{
    debugLog(`Fetching ${url}...`);

    const fetchPromise = fetch(url, {cache:"no-store", signal: signal});

    let r;
    if (useAbort) {
      // Set up timeout to abort the fetch
      timeoutId = setTimeout(() => ctrl.abort(), timeoutMs);
      r = await fetchPromise;
      // Clear timeout if request succeeded
      clearTimeout(timeoutId);
    } else {
      // Fallback for old browsers without AbortController
      const timeoutPromise = new Promise((_, reject) =>
        setTimeout(() => reject(new Error(`timeout ${url}`)), timeoutMs)
      );
      r = await Promise.race([fetchPromise, timeoutPromise]);
    }

    debugLog(`Response ${url}`, {status: r.status, ok: r.ok});
    let txt = await r.text();
    debugLog(`Response length: ${txt.length} chars`);
    if (!r.ok) throw new Error(`${url}: ${r.status}`);
    try{
      txt = txt.replace(/,(\s*,)+/g, ',').replace(/,(\s*)\]/g, '$1]').replace(/\[(\s*),/g, '[$1');
      const parsed = JSON.parse(txt);
      debugLog(`Parsed JSON from ${url}`);
      return parsed;
    }catch(e){
      debugLog(`Parse error ${url}`, {error: e.message, sample: txt.substring(0, 200)});
      throw new Error(`parse ${url}: ${e.message}`);
    }
  }catch(e){
    // Clear timeout on error
    if (timeoutId) clearTimeout(timeoutId);

    debugLog(`Fetch error ${url}`, {error: e.message});
    if (e.name === "AbortError") throw new Error(`timeout ${url}`);
    throw e;
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
          { field: "avgWind", color: "var(--wind-line-color)", label: "Wind" },
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
        title: "Pressure (MSLP)",
        unit: "hPa",
        conversionFactor: 1.0,
        series: [{ field: "avgPressHpa", color: "#5cb85c", label: "Pressure" }]
      },
      {
        id: "pm",
        title: "Air Quality (PM)",
        unit: "μg/m³",
        conversionFactor: 1.0,
        series: [
          { field: "avgPM25", color: "#ff6600", label: "PM2.5" },
          { field: "avgPM10", color: "#996633", label: "PM10" },
          { field: "avgPM1", color: "#9966cc", label: "PM1.0" }
        ]
      }
    ],
    tableColumns: [
      { field: "day", label: "Day", unit: "", conversionFactor: 1.0, decimals: 0, bgColor: "", group: "" },
      { field: "avgWind", label: "avg", unit: "km/h", conversionFactor: 3.6, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Wind Speed" },
      { field: "maxWind", label: "max", unit: "km/h", conversionFactor: 3.6, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Wind Speed" },
      { field: "avgTemp", label: "avg", unit: "°C", conversionFactor: 1.0, decimals: 1, bgColor: "", group: "Temperature" },
      { field: "minTemp", label: "min", unit: "°C", conversionFactor: 1.0, decimals: 1, bgColor: "", group: "Temperature" },
      { field: "maxTemp", label: "max", unit: "°C", conversionFactor: 1.0, decimals: 1, bgColor: "", group: "Temperature" },
      { field: "avgHum", label: "avg", unit: "%", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Humidity" },
      { field: "minHum", label: "min", unit: "%", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Humidity" },
      { field: "maxHum", label: "max", unit: "%", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Humidity" },
      { field: "avgPress", label: "avg", unit: "hPa", conversionFactor: 1.0, decimals: 1, bgColor: "", group: "Pressure" },
      { field: "minPress", label: "min", unit: "hPa", conversionFactor: 1.0, decimals: 1, bgColor: "", group: "Pressure" },
      { field: "maxPress", label: "max", unit: "hPa", conversionFactor: 1.0, decimals: 1, bgColor: "", group: "Pressure" },
      { field: "avgPM1", label: "PM1 avg", unit: "μg/m³", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Particulate Matter" },
      { field: "maxPM1", label: "PM1 max", unit: "μg/m³", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Particulate Matter" },
      { field: "avgPM25", label: "PM2.5 avg", unit: "μg/m³", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Particulate Matter" },
      { field: "maxPM25", label: "PM2.5 max", unit: "μg/m³", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Particulate Matter" },
      { field: "avgPM10", label: "PM10 avg", unit: "μg/m³", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Particulate Matter" },
      { field: "maxPM10", label: "PM10 max", unit: "μg/m³", conversionFactor: 1.0, decimals: 1, bgColor: "var(--table-cell-bg)", group: "Particulate Matter" }
    ],
    filesPerPage: 30,
    maxPlotPoints: 500
  };
}

// Load configuration from API
async function loadConfig() {
  try {
    debugLog('Loading config from /api/config');
    CONFIG = await fetchJSON('/api/config');
    debugLog('Config loaded', {plots: CONFIG.plots.length, cols: CONFIG.tableColumns.length});
  } catch (err) {
    debugLog('Config load failed', {error: err.message});
    debugLog('Using fallback config');
    CONFIG = getDefaultConfig();
  }

  // Extract plot keys from config
  PLOT_KEYS = CONFIG.plots.map(p => p.id);
  debugLog('Plot keys', PLOT_KEYS);

  // Update global constants from config
  FILES_PER_PAGE = CONFIG.filesPerPage || 30;
  MAX_PLOT_POINTS = CONFIG.maxPlotPoints || 500;

  // Generate UI
  debugLog('Generating plots');
  generatePlots();
  debugLog('Generating table headers');
  generateTableHeaders();
  debugLog('UI generation complete');
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
        // For multi-series plots
        if (idx === 0) {
          lineId = `line_${plot.id}`;
        } else if (s.field.includes('max') || s.field.includes('Max')) {
          lineId = `line_${plot.id}_max`;
        } else if (s.field.includes('PM10')) {
          lineId = `line_${plot.id}_pm10`;
        } else if (s.field.includes('PM1')) {
          lineId = `line_${plot.id}_pm1`;
        } else {
          lineId = `line_${plot.id}_${idx}`;
        }
      } else {
        lineId = `line_${plot.id}`;
      }

      // Generate correct dot IDs
      let dotId;
      if (plot.series.length > 1) {
        if (s.field.includes('max') || s.field.includes('Max')) {
          dotId = `hover_dot_${plot.id}_max`;
        } else if (s.field.includes('PM10')) {
          dotId = `hover_dot_${plot.id}_pm10`;
        } else if (s.field.includes('PM1') && !s.field.includes('PM10')) {
          dotId = `hover_dot_${plot.id}_pm1`;
        } else if (idx === 0) {
          dotId = `hover_dot_${plot.id}_avg`;
        } else {
          dotId = `hover_dot_${plot.id}_${idx}`;
        }
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
           ontouchstart="plotTouchStart('${plot.id}', event)"
           ontouchmove="plotTouchMove('${plot.id}', event)"
           ontouchend="plotTouchEnd('${plot.id}', event)"
           style="cursor:crosshair;">
        <defs>
          <clipPath id="plotClip_${plot.id}">
            <rect x="60" y="8" width="530" height="106"/>
          </clipPath>
        </defs>
        <g class="yaxis" id="axis_${plot.id}"></g>
        <g class="xaxis" id="axis_x_${plot.id}"></g>
        <g class="midnight-lines" id="midnight_${plot.id}"></g>
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

  // Calculate group spans with colors
  const groups = [];
  let currentGroup = null;
  let colIdx = 0;
  for (const col of CONFIG.tableColumns) {
    if (!col.group || col.group === '') {
      groups.push({ name: '', colspan: 1, bgColor: col.bgColor, startIdx: colIdx });
      colIdx++;
    } else if (!currentGroup || currentGroup.name !== col.group) {
      currentGroup = { name: col.group, colspan: 1, bgColor: col.bgColor, startIdx: colIdx };
      groups.push(currentGroup);
      colIdx++;
    } else {
      currentGroup.colspan++;
      colIdx++;
    }
  }

  // First row: group headers
  let row1 = '<tr>';
  for (const grp of groups) {
    const bgClass = grp.bgColor ? ' class="table-alt-bg"' : '';
    if (grp.name) {
      row1 += `<th colspan="${grp.colspan}"${bgClass}>${grp.name}</th>`;
    } else {
      row1 += `<th rowspan="3">${CONFIG.tableColumns[0].label}</th>`;
    }
  }
  row1 += '</tr>';

  // Second row: sub-labels (avg, max, min)
  let row2 = '<tr>';
  for (let i = 1; i < CONFIG.tableColumns.length; i++) {
    const col = CONFIG.tableColumns[i];
    const bgClass = col.bgColor ? ' class="table-alt-bg"' : '';
    row2 += `<th${bgClass}>${col.label}</th>`;
  }
  row2 += '</tr>';

  // Third row: units
  let row3 = '<tr>';
  for (let i = 1; i < CONFIG.tableColumns.length; i++) {
    const col = CONFIG.tableColumns[i];
    const unitText = col.unit ? `(${col.unit})` : '';
    const bgClass = col.bgColor ? ' class="table-alt-bg"' : '';
    row3 += `<th${bgClass}>${unitText}</th>`;
  }
  row3 += '</tr>';

  thead.innerHTML = row1 + row2 + row3;
}

const chartDims = { width: 600, height: 140, padLeft: 60, padRight: 10, padTop: 8, padBottom: 26 };
const plotState = {
  wind: { series: null, xDomain: null, zoomXDomain: null, label: "Wind", unit: "km/h" },
  temp: { series: null, xDomain: null, zoomXDomain: null, label: "Temp", unit: "°C" },
  hum:  { series: null, xDomain: null, zoomXDomain: null, label: "Humidity", unit: "%" },
  press:{ series: null, xDomain: null, zoomXDomain: null, label: "Pressure (MSLP)", unit: "hPa" },
  pm:   { series: null, xDomain: null, zoomXDomain: null, label: "PM", unit: "μg/m³" }
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
  PLOT_KEYS.forEach(k => setHoverDot(k, null, null));
  setHoverDot("wind_avg", null, null);
  setHoverDot("wind_max", null, null);
  setHoverDot("pm_avg", null, null);
  setHoverDot("pm_pm10", null, null);
  setHoverDot("pm_pm1", null, null);
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

function colorDot(color) {
  return `<span style="display:inline-block;width:8px;height:8px;border-radius:50%;background:${color};border:1px solid #fff;margin-right:4px;vertical-align:middle;"></span>`;
}

// Safe DOM version of colorDot for tooltips
function createColorDot(color) {
  const span = document.createElement('span');
  span.style.cssText = `display:inline-block;width:8px;height:8px;border-radius:50%;background:${color};border:1px solid #fff;margin-right:4px;vertical-align:middle;`;
  return span;
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
                 (key === "press") ? item.avgPressHpa :
                 (key === "pm") ? item.avgPM25 : // Use PM2.5 as primary for PM plots
                 null;
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

  // Build tooltip safely using DOM methods
  tooltipEl.textContent = ''; // Clear previous content

  // Add timestamp
  const timeText = document.createTextNode(`@ ${hh}:${mm}`);
  tooltipEl.appendChild(timeText);

  if (key === "pm") {
    // Show all three PM values with colored dots
    const pm25 = best.item.avgPM25;
    const pm10 = best.item.avgPM10;
    const pm1 = best.item.avgPM1;

    tooltipEl.appendChild(document.createElement('br'));
    const pm25Dot = createColorDot('#ff6600');
    tooltipEl.appendChild(pm25Dot);
    tooltipEl.appendChild(document.createTextNode(`PM2.5: ${isFinite(pm25) ? pm25.toFixed(1) : '--'} ${state.unit}`));

    tooltipEl.appendChild(document.createElement('br'));
    const pm10Dot = createColorDot('#996633');
    tooltipEl.appendChild(pm10Dot);
    tooltipEl.appendChild(document.createTextNode(`PM10: ${isFinite(pm10) ? pm10.toFixed(1) : '--'} ${state.unit}`));

    tooltipEl.appendChild(document.createElement('br'));
    const pm1Dot = createColorDot('#9966cc');
    tooltipEl.appendChild(pm1Dot);
    tooltipEl.appendChild(document.createTextNode(`PM1.0: ${isFinite(pm1) ? pm1.toFixed(1) : '--'} ${state.unit}`));
  } else if (key === "wind") {
    // Show wind with colored dots - convert m/s to km/h and show gust first
    const windKmh = toKmh(best.vAvg);
    const gustKmh = toKmh(best.vMax);
    if (gustKmh !== null && gustKmh !== undefined && isFinite(gustKmh)) {
      tooltipEl.appendChild(document.createElement('br'));
      const gustDot = createColorDot('#f0ad4e');
      tooltipEl.appendChild(gustDot);
      tooltipEl.appendChild(document.createTextNode(`Gust: ${gustKmh.toFixed(1)} ${state.unit}`));
    }
    tooltipEl.appendChild(document.createElement('br'));
    const windDot = createColorDot('black');
    tooltipEl.appendChild(windDot);
    tooltipEl.appendChild(document.createTextNode(`Wind: ${windKmh.toFixed(1)} ${state.unit}`));
  } else {
    tooltipEl.appendChild(document.createElement('br'));
    tooltipEl.appendChild(document.createTextNode(`${state.label}: ${best.vAvg.toFixed(1)} ${state.unit}`));
  }
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
    const domain = state.label === "Wind Speed" ? computeDomain(domainData, ["avgWind","maxWind"]) :
                   state.label === "Temperature" ? computeDomain(domainData, ["avgTempC"]) :
                   state.label === "Humidity" ? computeDomain(domainData, ["avgHumRH"]) :
                   state.label === "Pressure (MSLP)" ? computeDomain(domainData, ["avgPressHpa"]) :
                   state.label === "Air Quality (PM)" ? computeDomain(domainData, ["avgPM1","avgPM25","avgPM10"]) :
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
      } else if (key === "pm") {
        // Position dots for all three PM series
        const pm25 = best.item.avgPM25;
        const pm10 = best.item.avgPM10;
        const pm1 = best.item.avgPM1;
        if (isFinite(pm25)) {
          const normYPM25 = (pm25 - domain.min) / spanY;
          const yPxPM25 = dims.height - dims.padBottom - normYPM25 * yRange;
          setHoverDot("pm_avg", xPx, yPxPM25);
        }
        if (isFinite(pm10)) {
          const normYPM10 = (pm10 - domain.min) / spanY;
          const yPxPM10 = dims.height - dims.padBottom - normYPM10 * yRange;
          setHoverDot("pm_pm10", xPx, yPxPM10);
        }
        if (isFinite(pm1)) {
          const normYPM1 = (pm1 - domain.min) / spanY;
          const yPxPM1 = dims.height - dims.padBottom - normYPM1 * yRange;
          setHoverDot("pm_pm1", xPx, yPxPM1);
        }
      }
    }
  }
  setHoverLine(key, xPx);
  if (key === "wind") {
    setHoverDot("wind_avg", xPx, yPx);
  } else if (key !== "pm") {
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

// Touch event handlers for mobile support
function plotTouchStart(key, evt) {
  evt.preventDefault(); // Prevent scrolling while touching plot
  if (evt.touches.length === 0) return;

  const touch = evt.touches[0];
  const now = Date.now();
  const timeSinceLastTouch = now - dragState.lastClickTime;
  dragState.lastClickTime = now;

  // Double-tap detected - reset zoom
  if (timeSinceLastTouch < 300) {
    resetZoom(key);
    return;
  }

  const rect = evt.currentTarget.getBoundingClientRect();
  const relX = (touch.clientX - rect.left) * (chartDims.width / rect.width);
  dragState.active = true;
  dragState.plotKey = key;
  dragState.startX = relX;
  dragState.currentX = relX;
  showSelectionRect(key);
}

function plotTouchMove(key, evt) {
  evt.preventDefault(); // Prevent scrolling
  if (evt.touches.length === 0) return;

  const touch = evt.touches[0];
  const rect = evt.currentTarget.getBoundingClientRect();
  const relX = (touch.clientX - rect.left) * (chartDims.width / rect.width);

  if (dragState.active && dragState.plotKey === key) {
    dragState.currentX = relX;
    updateSelectionRect(key);
    hideTooltip();
  } else {
    // Create a synthetic mouse event for hovering
    const syntheticEvent = {
      currentTarget: evt.currentTarget,
      clientX: touch.clientX,
      clientY: touch.clientY
    };
    hoverPlot(key, syntheticEvent);
  }
}

function plotTouchEnd(key, evt) {
  evt.preventDefault();
  if (!dragState.active || dragState.plotKey !== key) {
    hideTooltip();
    return;
  }

  // Use the same logic as plotMouseUp
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
    hideTooltip();
    return;
  }

  const state = plotState[key];
  if (!state || !state.xDomain) {
    hideSelectionRect(key);
    hideTooltip();
    return;
  }

  if (!state.originalXDomain) {
    state.originalXDomain = { ...state.xDomain };
  }

  let normStart = (minX - dims.padLeft) / xRange;
  let normEnd = (maxX - dims.padLeft) / xRange;

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
  hideTooltip();
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
  } else if (key === "pm") {
    renderPM(state.series);
  }
}

function renderAxis(axisId, domain, dims = chartDims){
  const axis = document.getElementById(axisId);
  if (!axis) return;

  const svg = axis.closest('svg');
  const scaleX = svg ? (svg.getBoundingClientRect().width / dims.width) : 1;
  const textScale = scaleX > 0 ? (1 / scaleX) : 1;

  // Use integer formatting for pressure axis
  const decimals = (axisId === "axis_press") ? 0 : 1;

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
    html += `<text x="${axisX - 4}" y="${(y + 3).toFixed(1)}" text-anchor="end" transform="scale(${textScale.toFixed(3)}, 1)" transform-origin="${axisX - 4} ${(y + 3).toFixed(1)}">${value.toFixed(decimals)}</text>`;
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

function renderMidnightLines(midnightId, xDomain, dims = chartDims){
  const container = document.getElementById(midnightId);
  if (!container) return;
  if (!xDomain){
    container.innerHTML = "";
    return;
  }

  const xRange = dims.width - dims.padLeft - dims.padRight;
  const span = xDomain.max - xDomain.min;
  let html = '';

  // Find first midnight within or after xDomain.min
  const startDate = new Date(xDomain.min * 1000);
  const firstMidnight = new Date(startDate);
  firstMidnight.setHours(0, 0, 0, 0);
  if (firstMidnight <= startDate) {
    firstMidnight.setDate(firstMidnight.getDate() + 1);
  }

  // Draw vertical lines at each midnight
  let currentMidnight = new Date(firstMidnight);
  while (currentMidnight / 1000 <= xDomain.max) {
    const midnightSeconds = currentMidnight / 1000;
    const norm = (midnightSeconds - xDomain.min) / span;
    const x = dims.padLeft + norm * xRange;

    html += `<line x1="${x.toFixed(1)}" y1="${dims.padTop}" x2="${x.toFixed(1)}" y2="${dims.height - dims.padBottom}" stroke="var(--midnight-line)" stroke-width="1.5" stroke-dasharray="4 2" opacity="0.8"/>`;

    currentMidnight.setDate(currentMidnight.getDate() + 1);
  }

  container.innerHTML = html;
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
  if (xAxisId) {
    renderXAxis(xAxisId, displayDomain);
    const midnightId = xAxisId.replace('axis_x_', 'midnight_');
    renderMidnightLines(midnightId, displayDomain);
  }
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
  renderMidnightLines("midnight_wind", displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : valuesKm, "avgWind", "line_wind", domain, chartDims, "var(--wind-line-color)", displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : valuesKm, "maxWind", "line_wind_max", domain, chartDims, "#f0ad4e", displayDomain);
  plotState.wind.series = filtered;  // Store original m/s data, not converted
  plotState.wind.xDomain = xDomain;
}

function renderPM(values){
  const filtered = filterSeries(values, v => {
    const pm1 = v ? v.avgPM1 : null;
    const pm25 = v ? v.avgPM25 : null;
    const pm10 = v ? v.avgPM10 : null;
    return (pm1 !== null && pm1 !== undefined && isFinite(pm1)) ||
           (pm25 !== null && pm25 !== undefined && isFinite(pm25)) ||
           (pm10 !== null && pm10 !== undefined && isFinite(pm10));
  });
  if (!filtered.length){
    ["line_pm","line_pm_pm10","line_pm_pm1"].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.setAttribute("points", "");
    });
    clearAxis("axis_pm");
    clearAxis("axis_x_pm");
    if (plotState.pm) {
      plotState.pm.series = null;
      plotState.pm.xDomain = null;
    }
    return;
  }

  const xDomain = computeTimeDomain(filtered);
  const displayDomain = (plotState.pm && plotState.pm.zoomXDomain) ? plotState.pm.zoomXDomain : xDomain;

  const visibleData = displayDomain ? filtered.filter(v => {
    const e = v ? Number(v.startEpoch) : NaN;
    return isFinite(e) && e >= displayDomain.min && e <= displayDomain.max;
  }) : filtered;

  const domain = computeDomain(visibleData.length > 0 ? visibleData : filtered, ["avgPM1", "avgPM25", "avgPM10"]);
  const hasValid = Number.isFinite(domain.min) && Number.isFinite(domain.max);
  if (!hasValid){
    ["line_pm","line_pm_pm10","line_pm_pm1"].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.setAttribute("points", "");
    });
    clearAxis("axis_pm");
    clearAxis("axis_x_pm");
    if (plotState.pm) {
      plotState.pm.series = null;
      plotState.pm.xDomain = null;
    }
    return;
  }

  renderAxis("axis_pm", domain);
  renderXAxis("axis_x_pm", displayDomain);
  renderMidnightLines("midnight_pm", displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : filtered, "avgPM25", "line_pm", domain, chartDims, "#ff6600", displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : filtered, "avgPM10", "line_pm_pm10", domain, chartDims, "#996633", displayDomain);
  renderPolyline(visibleData.length > 0 ? visibleData : filtered, "avgPM1", "line_pm_pm1", domain, chartDims, "#9966cc", displayDomain);
  if (plotState.pm) {
    plotState.pm.series = filtered;
    plotState.pm.xDomain = xDomain;
  }
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
        td.classList.add('table-alt-bg');
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

function formatUptime(sec){
  if (!isFinite(sec) || sec < 0) return "--";
  if (sec < 60) return `${sec}s`;
  const totalMin = Math.floor(sec / 60);
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

    // Create list safely using DOM APIs instead of innerHTML
    const ul = document.createElement('ul');
    ul.style.cssText = 'margin:8px 0; padding-left:18px';

    for (const f of pageFiles){
      const p = f.path;
      const filename = p.substring(p.lastIndexOf('/') + 1);
      const s = bytesPretty(f.size);
      const dl = `/download?filename=${encodeURIComponent(filename)}`;

      const li = document.createElement('li');

      // Create download link
      const link = document.createElement('a');
      link.href = dl;
      link.textContent = p; // Use textContent to prevent XSS
      li.appendChild(link);

      // Add file size
      const sizeSpan = document.createElement('span');
      sizeSpan.className = 'muted';
      sizeSpan.textContent = ` (${s})`;
      li.appendChild(sizeSpan);

      // Add delete button
      const deleteBtn = document.createElement('button');
      deleteBtn.className = 'small file-delete-btn';
      deleteBtn.textContent = 'Delete';
      deleteBtn.onclick = function() { deleteFile(filename, dir); };
      li.appendChild(document.createTextNode(' '));
      li.appendChild(deleteBtn);

      ul.appendChild(li);
    }

    // Clear target and append new list
    target.textContent = '';
    target.appendChild(ul);

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

async function deleteFile(filename, dir){
  if (!confirm("Delete this file from SD?")) return;
  const pw = prompt("Password required to delete file:");
  if (!(pw && pw.trim().length)) {
    alert("Password required.");
    return;
  }
  try{
    const body = `filename=${encodeURIComponent(filename)}&pw=${encodeURIComponent(pw.trim())}`;
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
  const pw = prompt("Password required to reboot device:");
  if (!(pw && pw.trim().length)) {
    alert("Password required.");
    return;
  }
  if (!confirm("Reboot the device now?")) return;
  try{
    const res = await fetch("/api/reboot", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `pw=${encodeURIComponent(pw.trim())}`
    });
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

function updateCurrentReadings(now) {
  const windKmh = toKmh(now.wind_ms);
  document.getElementById("now").textContent = numOrDash(windKmh, 1);
  document.getElementById("pps").textContent = numOrDash(now.wind_pps, 1);
  document.getElementById("uptime").textContent = formatUptime(now.uptime_s);
  document.getElementById("tlocal").textContent = now.local_time || "--";

  document.getElementById("temp").textContent = numOrDash(now.temp_c, 1);
  document.getElementById("rh").textContent = numOrDash(now.hum_rh, 1);
  document.getElementById("pres").textContent = numOrDash(now.press_hpa, 1);

  // Sensor status pills with colors
  document.getElementById("bmeok").textContent = now.bme280_ok ? "OK" : "ERR";
  const bmeokPill = document.getElementById("bmeok_pill");
  if (now.bme280_ok) {
    bmeokPill.style.background = "#5cb85c";
    bmeokPill.style.color = "#fff";
  } else {
    bmeokPill.style.background = "#d9534f";
    bmeokPill.style.color = "#fff";
  }

  document.getElementById("pmsok").textContent = now.pms5003_ok ? "OK" : "ERR";
  const pmsokPill = document.getElementById("pmsok_pill");
  if (now.pms5003_ok) {
    pmsokPill.style.background = "#5cb85c";
    pmsokPill.style.color = "#fff";
  } else {
    pmsokPill.style.background = "#d9534f";
    pmsokPill.style.color = "#fff";
  }

  document.getElementById("sdok").textContent  = now.sd_ok ? "OK" : "ERR";
  const sdokPill = document.getElementById("sdok_pill");
  if (now.sd_ok) {
    sdokPill.style.background = "#5cb85c";
    sdokPill.style.color = "#fff";
  } else {
    sdokPill.style.background = "#d9534f";
    sdokPill.style.color = "#fff";
  }

  // WiFi pill with colors based on quality
  document.getElementById("wifi").textContent = now.wifi_rssi !== null && now.wifi_rssi !== undefined ? String(now.wifi_rssi) : "--";
  document.getElementById("wifi_quality").textContent = wifiQuality(now.wifi_rssi);
  const wifiPill = document.getElementById("wifi_pill");
  if (now.wifi_rssi !== null && now.wifi_rssi !== undefined && isFinite(now.wifi_rssi)) {
    if (now.wifi_rssi >= -60) { // Excellent or Good
      wifiPill.style.background = "#5cb85c";
      wifiPill.style.color = "#fff";
    } else if (now.wifi_rssi >= -70) { // Fair
      wifiPill.style.background = "#f0ad4e";
      wifiPill.style.color = "#000";
    } else { // Weak
      wifiPill.style.background = "#d9534f";
      wifiPill.style.color = "#fff";
    }
  } else {
    wifiPill.style.background = "";
    wifiPill.style.color = "";
  }

  // Helper function to set AQI pill color and text
  function setAQIPill(pillId, aqiId, catId, aqiValue, category) {
    document.getElementById(aqiId).textContent = aqiValue;
    document.getElementById(catId).textContent = category;

    const pill = document.getElementById(pillId);
    if (aqiValue <= 50) {
      pill.style.background = "#5cb85c"; // Good - green
      pill.style.color = "#fff";
    } else if (aqiValue <= 100) {
      pill.style.background = "#f0ad4e"; // Moderate - yellow
      pill.style.color = "#000";
    } else {
      pill.style.background = "#d9534f"; // Unhealthy+ - red
      pill.style.color = "#fff";
    }
  }

  // Update PM2.5 AQI pill
  const aqiPM25 = now.aqi_pm25 !== null && now.aqi_pm25 !== undefined ? now.aqi_pm25 : 0;
  const aqiPM25Cat = now.aqi_pm25_category || "--";
  setAQIPill("aqi_pm25_pill", "aqi_pm25", "aqi_pm25_cat", aqiPM25, aqiPM25Cat);

  // Update PM10 AQI pill
  const aqiPM10 = now.aqi_pm10 !== null && now.aqi_pm10 !== undefined ? now.aqi_pm10 : 0;
  const aqiPM10Cat = now.aqi_pm10_category || "--";
  setAQIPill("aqi_pm10_pill", "aqi_pm10", "aqi_pm10_cat", aqiPM10, aqiPM10Cat);

  // RAM pill with colors based on usage
  const freeHeap = now.free_heap !== null && now.free_heap !== undefined ? now.free_heap : 0;
  const heapSize = now.heap_size !== null && now.heap_size !== undefined ? now.heap_size : 0;
  const usedHeap = heapSize > 0 ? heapSize - freeHeap : 0;
  const pctUsed = heapSize > 0 ? Math.round((usedHeap / heapSize) * 100) : 0;
  document.getElementById("ram").textContent = heapSize > 0 ? `${bytesPretty(freeHeap)} free (${pctUsed}% used)` : "--";
  const ramPill = document.getElementById("ram_pill");
  if (heapSize > 0) {
    if (pctUsed > 90) {
      ramPill.style.background = "#d9534f";
      ramPill.style.color = "#fff";
    } else if (pctUsed > 75) {
      ramPill.style.background = "#f0ad4e";
      ramPill.style.color = "#000";
    } else {
      ramPill.style.background = "#5cb85c";
      ramPill.style.color = "#fff";
    }
  } else {
    ramPill.style.background = "";
    ramPill.style.color = "";
  }

  // CPU temp pill with colors
  document.getElementById("cpu_temp").textContent = numOrDash(now.cpu_temp_c, 1);
  const cpuTempPill = document.getElementById("cpu_temp_pill");
  if (now.cpu_temp_c !== null && now.cpu_temp_c !== undefined && isFinite(now.cpu_temp_c)) {
    if (now.cpu_temp_c > 90) {
      cpuTempPill.style.background = "#d9534f";
      cpuTempPill.style.color = "#fff";
    } else if (now.cpu_temp_c > 70) {
      cpuTempPill.style.background = "#f0ad4e";
      cpuTempPill.style.color = "#000";
    } else {
      cpuTempPill.style.background = "#5cb85c";
      cpuTempPill.style.color = "#fff";
    }
  } else {
    cpuTempPill.style.background = "";
    cpuTempPill.style.color = "";
  }
}

// Separate tick functions for different update frequencies
let tickCount = 0;
let isFastTickRunning = false;
let isSlowTickRunning = false;

async function fastTick() {
  // Prevent overlapping requests
  if (isFastTickRunning) {
    debugLog('Skipping fastTick - already running');
    return;
  }

  isFastTickRunning = true;
  try {
    const nowRes = await fetchJSON("/api/now", { timeoutMs: 12000 });
    updateCurrentReadings(nowRes);
  } catch(e) {
    console.warn("fastTick", e);
  } finally {
    isFastTickRunning = false;
  }
}

async function slowTick() {
  // Prevent overlapping requests
  if (isSlowTickRunning) {
    debugLog('Skipping slowTick - already running');
    return;
  }

  isSlowTickRunning = true;
  try {
    const [bucketsRes, daysRes] = await Promise.allSettled([
      fetchJSON("/api/buckets_compact", { timeoutMs: 20000 }),
      fetchJSON("/api/days", { timeoutMs: 20000 })
    ]);

    if (bucketsRes.status === "fulfilled") {
      debugLog('Buckets received', {count: bucketsRes.value.buckets?.length});
      let series = Array.isArray(bucketsRes.value.buckets) ? bucketsRes.value.buckets : [];
      series = series.filter(b => Array.isArray(b) && b.length >= 7);
      debugLog('Valid buckets', {count: series.length});

      // Get current time and calculate 24h cutoff
      const nowEpoch = bucketsRes.value.now_epoch || Math.floor(Date.now() / 1000);
      const cutoff24h = nowEpoch - 86400;  // 24 hours ago

      series = series.map(b => {
        return {
          startEpoch: b[0],
          avgWind: b[1],
          maxWind: b[2],
          samples: b[3] || 0,
          avgTempC: b[4],
          avgHumRH: b[5],
          avgPressHpa: b[6],
          avgPM1: b.length > 7 ? b[7] : null,
          avgPM25: b.length > 8 ? b[8] : null,
          avgPM10: b.length > 9 ? b[9] : null
        };
      });

      // Filter to only show last 24 hours
      series = series.filter(b => b.startEpoch >= cutoff24h);
      debugLog('Filtered to last 24h', {count: series.length});

      if (series.length > 0) debugLog('Sample data', series[0]);
      const bucketSeconds = bucketsRes.value.bucket_seconds || "--";
      document.getElementById("bucket_sec").textContent = bucketSeconds;
      debugLog('Rendering plots');
      renderWind(series);
      renderSingleSeries(series, "avgTempC", "line_temp", "#d9534f", "axis_temp", "axis_x_temp");
      renderSingleSeries(series, "avgHumRH", "line_hum", "#0275d8", "axis_hum", "axis_x_hum");
      renderSingleSeries(series, "avgPressHpa", "line_press", "#5cb85c", "axis_press", "axis_x_press");
      renderPM(series);
      debugLog('Plots rendered');
    } else {
      debugLog("Buckets failed", {error: bucketsRes.reason?.message});
    }

    if (daysRes.status === "fulfilled") {
      if (!Array.isArray(daysRes.value.days)) throw new Error("days array missing");
      renderDays(daysRes.value.days);
    } else {
      console.warn("days", daysRes.reason);
    }
  } catch(e) {
    console.warn("slowTick", e);
  } finally {
    isSlowTickRunning = false;
  }
}

function combinedTick() {
  tickCount++;

  // Fast updates every tick (2s)
  fastTick();

  // Slow updates every 15 seconds (7.5 ticks * 2s = 15s, round down to 7)
  if (tickCount % 7 === 0) {
    slowTick();
  }
}

// Initialize application
(async function init() {
  try {
    debugLog('Initializing app');
    debugLog('User Agent', {ua: navigator.userAgent.substring(0, 100)});
    debugLog('URL', {url: window.location.href});

    await loadConfig();
    debugLog('Starting tick');

    // Load plots and summaries once on init
    await slowTick();

    // Then update current readings every 2s, plots every 14s (7 ticks)
    setInterval(combinedTick, 2000);
    loadFiles('data');

    debugLog('Initialization complete');
  } catch (error) {
    debugLog('INIT FAILED', {error: error.message, stack: error.stack});
    // Show error on page
    const body = document.body;
    const errorDiv = document.createElement('div');
    errorDiv.style.cssText = 'position:fixed;top:0;left:0;right:0;background:#d9534f;color:white;padding:20px;z-index:9999;';
    errorDiv.innerHTML = `
      <h3>Initialization Error</h3>
      <p>${error.message || error}</p>
      <p><small>Click Debug button to see details</small></p>
    `;
    body.insertBefore(errorDiv, body.firstChild);
  }
})();
