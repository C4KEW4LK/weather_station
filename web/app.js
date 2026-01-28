// Configuration loaded from /api/config
let CONFIG = null;
let PLOT_KEYS = [];
let FILES_PER_PAGE = 30;
let MAX_PLOT_POINTS = 500;

// uPlot instances and state
const uPlotInstances = {};
const plotState = {};

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

// Generate plot containers dynamically
function generatePlots() {
  const container = document.getElementById('plots_container');
  if (!container) return;

  container.innerHTML = '';

  for (const plot of CONFIG.plots) {
    const div = document.createElement('div');

    div.innerHTML = `
      <div class="muted">${plot.title} (${plot.unit})</div>
      <div id="plot_${plot.id}" class="uplot-container"></div>
    `;

    container.appendChild(div);

    // Initialize plot state
    plotState[plot.id] = {
      data: null,
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

// Get CSS variable value
function getCSSVar(name) {
  return getComputedStyle(document.documentElement).getPropertyValue(name).trim();
}

// Check if dark mode is active
function isDarkMode() {
  return document.body.classList.contains('dark-mode');
}

// Get axis/grid colors based on theme
function getThemeColors() {
  const dark = isDarkMode();
  return {
    axes: dark ? '#999' : '#555',
    grid: dark ? '#444' : '#ddd',
    background: dark ? '#333' : '#efefef',
    midnight: dark ? '#666' : '#bbb',
    windLine: dark ? '#ccc' : '#000'
  };
}

// Resolve CSS variable for colors
function resolveColor(color) {
  if (color.startsWith('var(')) {
    const varName = color.slice(4, -1);
    return getCSSVar(varName) || color;
  }
  return color;
}

// Format timestamp for tooltip
function formatTime(epoch) {
  const d = new Date(epoch * 1000);
  const hh = d.getHours().toString().padStart(2, '0');
  const mm = d.getMinutes().toString().padStart(2, '0');
  return `${hh}:${mm}`;
}

// Format timestamp for x-axis
function formatAxisTime(self, splits) {
  return splits.map(v => {
    if (v == null) return '';
    const d = new Date(v * 1000);
    const hh = d.getHours().toString().padStart(2, '0');
    const mm = d.getMinutes().toString().padStart(2, '0');
    return `${hh}:${mm}`;
  });
}

// Downsample data for performance
function downsampleData(values, maxPoints = 500) {
  if (!values || values.length <= maxPoints) return values;
  const step = values.length / maxPoints;
  const result = [];
  for (let i = 0; i < maxPoints; i++) {
    const index = Math.floor(i * step);
    result.push(values[index]);
  }
  return result;
}

// Filter valid data points
function filterSeries(values, predicate) {
  if (!values || !values.length) return [];
  return values.filter(v => {
    if (!v) return false;
    const e = v.startEpoch ? Number(v.startEpoch) : 0;
    if (!isFinite(e) || e <= 0) return false;
    return predicate ? predicate(v) : true;
  });
}

// Convert m/s to km/h
function toKmh(v) {
  if (v === null || v === undefined) return null;
  if (!isFinite(v)) return null;
  return Number(v) * 3.6;
}

// Create midnight lines plugin for uPlot
function midnightLinesPlugin() {
  return {
    hooks: {
      drawAxes: [
        (u) => {
          const ctx = u.ctx;
          const { left, top, width, height } = u.bbox;
          const colors = getThemeColors();

          // Get x scale range
          const xMin = u.scales.x.min;
          const xMax = u.scales.x.max;

          if (xMin == null || xMax == null) return;

          // Find first midnight after xMin
          const startDate = new Date(xMin * 1000);
          const firstMidnight = new Date(startDate);
          firstMidnight.setHours(0, 0, 0, 0);
          if (firstMidnight <= startDate) {
            firstMidnight.setDate(firstMidnight.getDate() + 1);
          }

          ctx.save();
          ctx.strokeStyle = colors.midnight;
          ctx.lineWidth = 1.5;
          ctx.setLineDash([4, 2]);

          let currentMidnight = new Date(firstMidnight);
          while (currentMidnight.getTime() / 1000 <= xMax) {
            const midnightEpoch = currentMidnight.getTime() / 1000;
            const x = u.valToPos(midnightEpoch, 'x', true);

            if (x >= left && x <= left + width) {
              ctx.beginPath();
              ctx.moveTo(x, top);
              ctx.lineTo(x, top + height);
              ctx.stroke();
            }

            currentMidnight.setDate(currentMidnight.getDate() + 1);
          }

          ctx.restore();
        }
      ]
    }
  };
}

// Global tooltip element (shared across all plots)
let globalTooltip = null;

function getTooltip() {
  if (!globalTooltip) {
    globalTooltip = document.createElement('div');
    globalTooltip.className = 'tooltip';
    globalTooltip.style.display = 'none';
    document.body.appendChild(globalTooltip);
  }
  return globalTooltip;
}

// Hide tooltip
function hideGlobalTooltip() {
  if (globalTooltip) {
    globalTooltip.style.display = 'none';
  }
}

// Create tooltip plugin for uPlot
// seriesInfo is an array of {label, color} for each data series
function tooltipPlugin(unit, seriesInfo) {
  return {
    hooks: {
      setCursor: [
        (u) => {
          const tooltip = getTooltip();
          const { left, top, idx } = u.cursor;

          // Hide if no valid index or cursor is outside plot area
          if (idx == null || idx < 0 || left < 0 || top < 0) {
            tooltip.style.display = 'none';
            return;
          }

          const time = u.data[0][idx];
          if (time == null) {
            tooltip.style.display = 'none';
            return;
          }

          // Build tooltip content
          tooltip.innerHTML = '';

          // Add timestamp
          const timeSpan = document.createElement('div');
          timeSpan.textContent = `@ ${formatTime(time)}`;
          tooltip.appendChild(timeSpan);

          // Add series values
          for (let i = 0; i < seriesInfo.length; i++) {
            const dataIdx = i + 1; // data[0] is timestamps, data[1+] are series
            const val = u.data[dataIdx] ? u.data[dataIdx][idx] : null;
            const info = seriesInfo[i];
            const series = u.series[dataIdx];

            // Skip hidden series
            if (series && series.show === false) continue;

            const row = document.createElement('div');

            // Color dot
            const dot = document.createElement('span');
            dot.style.cssText = `display:inline-block;width:8px;height:8px;border-radius:50%;background:${info.color};border:1px solid #fff;margin-right:4px;vertical-align:middle;`;
            row.appendChild(dot);

            // Label and value
            const valueText = val != null && isFinite(val) ? val.toFixed(1) : '--';
            row.appendChild(document.createTextNode(`${info.label}: ${valueText} ${unit}`));

            tooltip.appendChild(row);
          }

          // Position tooltip near cursor
          const bbox = u.over.getBoundingClientRect();
          let posX = bbox.left + left + 15;
          let posY = bbox.top + top + 15;

          tooltip.style.display = 'block';
          tooltip.style.left = posX + 'px';
          tooltip.style.top = posY + 'px';

          // Adjust if going off right edge
          const tooltipRect = tooltip.getBoundingClientRect();
          if (tooltipRect.right > window.innerWidth - 5) {
            tooltip.style.left = (bbox.left + left - tooltipRect.width - 15) + 'px';
          }
          // Adjust if going off bottom
          if (tooltipRect.bottom > window.innerHeight - 5) {
            tooltip.style.top = (bbox.top + top - tooltipRect.height - 15) + 'px';
          }
        }
      ],
      setSelect: [
        () => {
          hideGlobalTooltip();
        }
      ],
      ready: [
        (u) => {
          // Hide tooltip when cursor leaves the plot area
          u.over.addEventListener('mouseleave', hideGlobalTooltip);
          u.over.addEventListener('touchend', hideGlobalTooltip);
        }
      ]
    }
  };
}

// Track which plot is currently hovered (for showing cursor points only on hovered plot)
let hoveredPlotKey = null;

// Sync zoom across all plots
let syncKey = null;

function getSyncKey() {
  if (!syncKey) {
    syncKey = {};
  }
  return syncKey;
}

// Create uPlot options for a plot
function createPlotOptions(plotConfig, width) {
  const colors = getThemeColors();
  const seriesConfig = [];
  const seriesInfo = []; // For tooltip: {label, color}

  // Add x-axis series (required by uPlot)
  seriesConfig.push({});

  // Add data series
  for (const s of plotConfig.series) {
    const color = resolveColor(s.color);
    seriesInfo.push({ label: s.label, color: color });
    seriesConfig.push({
      label: s.label,
      stroke: color,
      width: 2,
      points: { show: false },
    });
  }

  // Build array of colors for cursor points (index 0 = null for x-axis series)
  const ptColors = [null, ...seriesInfo.map(s => s.color)];
  const thisPlotKey = plotConfig.id;

  const opts = {
    width: width,
    height: 160,
    cursor: {
      x: true,
      y: false,
      points: {
        size: 8,
        width: 2,
        show: (u, seriesIdx) => hoveredPlotKey === thisPlotKey,
      },
      drag: {
        x: true,
        y: false,
        setScale: false,
      },
      sync: {
        key: getSyncKey(),
      },
    },
    select: {
      show: true,
    },
    legend: {
      show: false,
    },
    scales: {
      x: {
        time: false, // We're using epoch seconds
      },
      y: {
        auto: true,
        range: (u, min, max) => {
          // Add some padding
          const span = max - min;
          const pad = span * 0.1 || 1;
          return [min - pad, max + pad];
        }
      }
    },
    axes: [
      {
        stroke: colors.axes,
        grid: { stroke: colors.grid, width: 1 },
        ticks: { stroke: colors.axes, width: 1 },
        values: formatAxisTime,
        font: '10px system-ui',
        labelFont: '10px system-ui',
      },
      {
        stroke: colors.axes,
        grid: { stroke: colors.grid, width: 1 },
        ticks: { stroke: colors.axes, width: 1 },
        font: '10px system-ui',
        labelFont: '10px system-ui',
        size: 50,
        values: (u, splits) => splits.map(v => v != null ? v.toFixed(plotConfig.id === 'press' ? 0 : 1) : ''),
      }
    ],
    series: seriesConfig,
    plugins: [
      midnightLinesPlugin(),
      tooltipPlugin(plotConfig.unit, seriesInfo),
    ],
    hooks: {
      setSelect: [
        (u) => {
          const { left, width } = u.select;
          if (width > 10) {
            const xMin = u.posToVal(left, 'x');
            const xMax = u.posToVal(left + width, 'x');

            // Apply zoom to all plots
            applyZoomToAllPlots(xMin, xMax);

            // Mark as manual zoom
            hasManualZoom = true;
            setResetButtonVisible(true);
          }

          // Clear selection
          u.setSelect({ left: 0, top: 0, width: 0, height: 0 }, false);
        }
      ],
      ready: [
        (u) => {
          // Handle double-click to reset zoom
          u.over.addEventListener('dblclick', () => {
            resetZoom(plotConfig.id);
          });

          // Track hovered plot for showing cursor points only on hovered plot
          u.over.addEventListener('mouseenter', () => {
            hoveredPlotKey = plotConfig.id;
          });

          u.over.addEventListener('mouseleave', () => {
            hoveredPlotKey = null;
            hideGlobalTooltip();
          });
        }
      ]
    }
  };

  return opts;
}

// Apply zoom to all plots
function applyZoomToAllPlots(xMin, xMax) {
  for (const key of PLOT_KEYS) {
    plotState[key].zoomXDomain = { min: xMin, max: xMax };
    if (uPlotInstances[key]) {
      uPlotInstances[key].setScale('x', { min: xMin, max: xMax });
    }
  }
}

// Convert raw data to uPlot format
function dataToUPlot(data, fields, conversionFactor = 1) {
  if (!data || !data.length) return null;

  const downsampled = downsampleData(data, MAX_PLOT_POINTS);

  // timestamps
  const timestamps = downsampled.map(d => d.startEpoch);

  // series data
  const series = fields.map(field => {
    return downsampled.map(d => {
      const val = d[field];
      if (val === null || val === undefined || !isFinite(val)) return null;
      return Number(val) * conversionFactor;
    });
  });

  return [timestamps, ...series];
}

// Render a plot using uPlot
function renderPlot(key, rawData) {
  const plotConfig = CONFIG.plots.find(p => p.id === key);
  if (!plotConfig) return;

  const container = document.getElementById(`plot_${key}`);
  if (!container) return;

  // Get fields and conversion factor
  const fields = plotConfig.series.map(s => s.field);
  const conversionFactor = plotConfig.conversionFactor || 1;

  // Filter valid data
  const filtered = filterSeries(rawData, v => {
    return fields.some(f => {
      const val = v ? v[f] : null;
      return val !== null && val !== undefined && isFinite(val);
    });
  });

  if (!filtered.length) {
    // Clear the plot
    if (uPlotInstances[key]) {
      uPlotInstances[key].destroy();
      delete uPlotInstances[key];
    }
    container.innerHTML = '<div class="muted" style="padding:20px;text-align:center;">No data available</div>';
    plotState[key].data = null;
    plotState[key].xDomain = null;
    return;
  }

  // Convert to uPlot format
  const uData = dataToUPlot(filtered, fields, conversionFactor);
  if (!uData) return;

  // Calculate x domain
  const timestamps = uData[0];
  const xMin = Math.min(...timestamps.filter(t => t != null));
  const xMax = Math.max(...timestamps.filter(t => t != null));

  plotState[key].data = filtered;
  plotState[key].xDomain = { min: xMin, max: xMax };

  // Determine display domain (respect zoom)
  const displayDomain = plotState[key].zoomXDomain || plotState[key].xDomain;

  // Get container width
  const containerWidth = container.clientWidth || 400;

  // Create or update uPlot instance
  if (uPlotInstances[key]) {
    // Update existing instance
    uPlotInstances[key].setData(uData);
    uPlotInstances[key].setSize({ width: containerWidth, height: 160 });
    if (displayDomain) {
      uPlotInstances[key].setScale('x', { min: displayDomain.min, max: displayDomain.max });
    }
  } else {
    // Create new instance
    container.innerHTML = '';
    const opts = createPlotOptions(plotConfig, containerWidth);
    const u = new uPlot(opts, uData, container);
    uPlotInstances[key] = u;

    if (displayDomain) {
      u.setScale('x', { min: displayDomain.min, max: displayDomain.max });
    }
  }
}

// Re-render a single plot (for zoom changes)
function reRenderPlot(key) {
  const state = plotState[key];
  if (!state || !state.data) return;
  renderPlot(key, state.data);
}

// Resize observer for responsive charts
let resizeTimeout = null;
function setupResizeObserver() {
  const container = document.getElementById('plots_container');
  if (!container) return;

  const observer = new ResizeObserver(() => {
    clearTimeout(resizeTimeout);
    resizeTimeout = setTimeout(() => {
      for (const key of PLOT_KEYS) {
        const plotContainer = document.getElementById(`plot_${key}`);
        if (plotContainer && uPlotInstances[key]) {
          const width = plotContainer.clientWidth || 400;
          uPlotInstances[key].setSize({ width, height: 160 });
        }
      }
    }, 100);
  });

  observer.observe(container);
}

// Current zoom level in hours (default 24)
let currentZoomLevel = 24;
// Track if user has manually zoomed via drag (away from preset)
let hasManualZoom = false;

// Load zoom level from cookie on init
function loadZoomLevelFromCookie() {
  const saved = getCookie('zoomLevel');
  if (saved) {
    const parsed = parseInt(saved, 10);
    if ([1, 3, 6, 12, 24].includes(parsed)) {
      currentZoomLevel = parsed;
    }
  }
}

function saveZoomLevelToCookie(hours) {
  setCookie('zoomLevel', hours, 365);
}

function updateActiveZoomButton(hours) {
  document.querySelectorAll('.zoom-btn').forEach(btn => {
    const btnZoom = parseInt(btn.getAttribute('data-zoom'), 10);
    if (btnZoom === hours) {
      btn.classList.add('active');
    } else {
      btn.classList.remove('active');
    }
  });
}

function setResetButtonVisible(visible) {
  const resetBtn = document.getElementById('reset_zoom_btn');
  if (resetBtn) resetBtn.style.visibility = visible ? "visible" : "hidden";
}

function resetZoom(key) {
  // Reset to the current saved zoom level
  hasManualZoom = false;
  setResetButtonVisible(false);
  setZoomLevel(currentZoomLevel, false);
}

function setZoomLevel(hours, saveToStorage = true) {
  // Save the selected zoom level
  if (saveToStorage) {
    currentZoomLevel = hours;
    saveZoomLevelToCookie(hours);
    // Clicking a preset clears manual zoom
    hasManualZoom = false;
    setResetButtonVisible(false);
    updateActiveZoomButton(hours);
  }

  // Get current time from any plot's data, or use system time
  let nowEpoch = Math.floor(Date.now() / 1000);

  // Try to get the latest data point time from the series
  for (const plotKey of PLOT_KEYS) {
    const state = plotState[plotKey];
    if (state && state.xDomain && state.xDomain.max) {
      nowEpoch = Math.max(nowEpoch, state.xDomain.max);
    }
  }

  const secondsBack = hours * 3600;
  const minEpoch = nowEpoch - secondsBack;

  // Apply zoom to all plots
  PLOT_KEYS.forEach(plotKey => {
    const state = plotState[plotKey];
    if (state && state.xDomain) {
      // Clamp to available data range
      const clampedMin = Math.max(minEpoch, state.xDomain.min);
      const clampedMax = Math.min(nowEpoch, state.xDomain.max);

      // Only set zoom if it's actually narrower than full range
      if (clampedMax - clampedMin < state.xDomain.max - state.xDomain.min) {
        state.zoomXDomain = { min: clampedMin, max: clampedMax };
      } else {
        state.zoomXDomain = null;
      }

      // Update uPlot instance
      if (uPlotInstances[plotKey]) {
        const domain = state.zoomXDomain || state.xDomain;
        uPlotInstances[plotKey].setScale('x', { min: domain.min, max: domain.max });
      }
    }
  });
}

// Update plots for theme change
function updatePlotsTheme() {
  // Destroy and recreate all plots to pick up new colors
  for (const key of PLOT_KEYS) {
    if (plotState[key].data) {
      if (uPlotInstances[key]) {
        uPlotInstances[key].destroy();
        delete uPlotInstances[key];
      }
      renderPlot(key, plotState[key].data);
    }
  }
}

// Hook into theme toggle
const originalToggleTheme = window.toggleTheme;
window.toggleTheme = function() {
  if (typeof originalToggleTheme === 'function') {
    originalToggleTheme();
  }
  // Update plots after theme change
  setTimeout(updatePlotsTheme, 50);
};

// Table rendering
function renderDays(days) {
  const tb = document.getElementById("days");
  tb.innerHTML = "";
  for (const d of days) {
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

function numOrDash(v, digits=1) {
  if (v === null || v === undefined) return "--";
  if (!isFinite(v)) return "--";
  return Number(v).toFixed(digits);
}

function formatUptime(sec) {
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

function bytesPretty(n) {
  if (!isFinite(n)) return "";
  if (n < 1024) return `${n} B`;
  if (n < 1024*1024) return `${(n/1024).toFixed(1)} KB`;
  return `${(n/(1024*1024)).toFixed(1)} MB`;
}

function wifiQuality(rssi) {
  if (rssi === null || rssi === undefined || !isFinite(rssi)) return "--";
  if (rssi >= -50) return "Excellent";
  if (rssi >= -60) return "Good";
  if (rssi >= -70) return "Fair";
  return "Weak";
}

const filesState = {
  data: { offset: 0, total: 0, files: [] }
};

async function loadFiles(dir) {
  const target = document.getElementById('files_data');
  const navEl = document.getElementById('files_data_nav');
  const infoEl = document.getElementById('files_data_info');
  const prevBtn = document.getElementById('files_data_prev');
  const nextBtn = document.getElementById('files_data_next');

  if (!target) return;
  target.textContent = "Loading...";

  try {
    const j = await fetchJSON(`/api/files?dir=${encodeURIComponent(dir)}`);
    if (!j.ok) {
      target.textContent = `Error: ${j.error || 'unknown'}`;
      if (navEl) navEl.style.display = "none";
      return;
    }
    const files = (j.files || []).slice().sort((a,b)=> (b.path||'').localeCompare(a.path||''));

    filesState[dir].files = files;
    filesState[dir].total = files.length;

    if (!files.length) {
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

    for (const f of pageFiles) {
      const p = f.path;
      const filename = p.substring(p.lastIndexOf('/') + 1);
      const s = bytesPretty(f.size);
      const dl = `/download?filename=${encodeURIComponent(filename)}`;

      const li = document.createElement('li');

      // Create download link
      const link = document.createElement('a');
      link.href = dl;
      link.textContent = p;
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

  } catch(e) {
    target.textContent = "Error loading list";
    if (navEl) navEl.style.display = "none";
  }
}

function navigateFiles(dir, delta) {
  filesState[dir].offset += delta * FILES_PER_PAGE;
  loadFiles(dir);
}

function goToPage(dir, pageNum) {
  const page = parseInt(pageNum, 10);
  if (!isFinite(page) || page < 1) return;
  filesState[dir].offset = (page - 1) * FILES_PER_PAGE;
  loadFiles(dir);
}

function downloadZip() {
  const n = parseInt(document.getElementById("zipdays").value || "7", 10);
  const days = (isFinite(n) && n > 0) ? n : 7;
  window.location = `/download_zip?days=${encodeURIComponent(days)}`;
}

async function deleteFile(filename, dir) {
  if (!confirm("Delete this file from SD?")) return;
  const pw = prompt("Password required to delete file:");
  if (!(pw && pw.trim().length)) {
    alert("Password required.");
    return;
  }
  try {
    const body = `filename=${encodeURIComponent(filename)}&pw=${encodeURIComponent(pw.trim())}`;
    const res = await fetch("/api/delete", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body
    });
    const txt = await res.text();
    if (res.ok) {
      alert("File deleted.");
      filesState[dir].offset = 0;
      loadFiles(dir);
    } else {
      alert("Delete failed: " + txt);
    }
  } catch(e) {
    alert("Error deleting file");
  }
}

async function clearSdData() {
  if (!confirm("Clear all CSV data on the SD card? This cannot be undone.")) return;
  const pw = prompt("Password required to clear SD data:");
  if (!(pw && pw.trim().length)) {
    alert("Password required.");
    return;
  }
  try {
    const res = await fetch("/api/clear_data", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `pw=${encodeURIComponent(pw.trim())}`
    });
    const txt = await res.text();
    if (res.ok) {
      alert("SD data cleared.");
      filesState.data.offset = 0;
      loadFiles('data');
    } else {
      alert("Failed to clear SD data: " + txt);
    }
  } catch(e) {
    alert("Error clearing SD data");
  }
}

async function rebootDevice() {
  const pw = prompt("Password required to reboot device:");
  if (!(pw && pw.trim().length)) {
    alert("Password required.");
    return;
  }
  if (!confirm("Reboot the device now?")) return;
  try {
    const res = await fetch("/api/reboot", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `pw=${encodeURIComponent(pw.trim())}`
    });
    if (res.ok) {
      alert("Rebooting...");
    } else {
      const txt = await res.text();
      alert("Reboot failed: " + txt);
    }
  } catch(e) {
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

      // Render all plots
      for (const plotKey of PLOT_KEYS) {
        renderPlot(plotKey, series);
      }

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

    // Load saved zoom level from cookie
    loadZoomLevelFromCookie();
    updateActiveZoomButton(currentZoomLevel);
    debugLog('Loaded zoom level', {hours: currentZoomLevel});

    await loadConfig();
    debugLog('Starting tick');

    // Set up resize observer for responsive charts
    setupResizeObserver();

    // Load plots and summaries once on init
    await slowTick();

    // Apply saved zoom level after data is loaded
    setZoomLevel(currentZoomLevel, false);
    debugLog('Applied zoom level', {hours: currentZoomLevel});

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
