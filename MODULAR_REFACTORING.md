# Weather Station Modular Refactoring - Implementation Guide

## Overview

The weather station code has been refactored to support modular configuration where:
- **Hardware settings** are in `config.h`
- **Plot and table definitions** are configurable in `config.h`
- **HTML/JS templates** are stored on SD card at `/web/index.html` and `/web/app.js`
- **Built-in upload interface** at `/upload` for managing web files

## Changes Made

### 1. Created `config.h`

Location: `weather_station/config.h`

This file contains all configuration organized into namespaces:
- `WindConfig` - Wind sensor pin, calibration, timing
- `BME280Config` - I2C address, polling interval
- `SDConfig` - SD card chip select pin
- `LogConfig` - Bucket size, retention, history
- `NetworkConfig` - NTP servers, timezone, WiFi AP settings
- `UIConfig` - Files per page, max plot points

Plus plot and table configuration structures:
- `PLOTS[]` array defining plots (wind, temp, humidity, pressure)
- `TABLE_COLUMNS[]` array defining daily summary table columns

### 2. Created `upload_page.h`

Location: `weather_station/upload_page.h`

Minimal HTML upload interface served at `/upload` endpoint for uploading:
- `/web/index.html` - Main UI page
- `/web/app.js` - JavaScript application

### 3. Modified `weather_station.ino`

**Changes:**
- Includes `config.h` and `upload_page.h`
- Removed `root_page.h` and `root_page_js_mini.h` includes
- Updated all config references to use namespaces (e.g., `PULSE_PIN` → `WindConfig::PULSE_PIN`)
- Added `handleApiConfig()` - serves `/api/config` with plot and table definitions
- Added `handleUpload()` - handles file uploads to `/web/` directory
- Modified `handleRoot()` - serves from SD card (`/web/index.html`) with fallback
- Modified `handleRootJs()` - serves from SD card (`/web/app.js`) with fallback
- Registered new routes: `/api/config`, `/upload` (GET and POST)

### 4. Created Dynamic HTML Template

Location: `web/index.html`

Changes from original:
- Removed hardcoded plot SVGs (lines 226-303 in original)
- Added placeholder: `<div id="plots_container" class="plots-grid">` for dynamic plot generation
- Removed hardcoded table headers (lines 315-328 in original)
- Added placeholder: `<thead id="table_header">` for dynamic table generation
- Added "Upload web files" button linking to `/upload`
- Kept all CSS and overall structure intact

## Next Steps to Complete Implementation

### Step 1: Create app.js

You need to create `/web/app.js` based on the existing **non-minified** `root_page_js.h` with these modifications:

1. **Start from** `weather_station/root_page_js.h` (the **unminified** version, NOT `root_page_js_mini.h`)

   Quick extract command (Windows PowerShell):
   ```powershell
   Get-Content weather_station/root_page_js.h | Select-Object -Skip 2 | Select-Object -SkipLast 1 > web/app_base.js
   ```

   Or (Git Bash/Linux/Mac):
   ```bash
   sed '1,2d;$d' weather_station/root_page_js.h > web/app_base.js
   ```

2. **Remove** the PROGMEM wrapper (if not using the command above):
   - Delete first 2 lines: `#pragma once` and `static const char ROOT_JS[] PROGMEM = R"JS(`
   - Delete last line: `)JS";`

3. **Replace** template variables at the top of the file:
   - Replace `const FILES_PER_PAGE = {{FILES_PER_PAGE}};`
   - Replace `const MAX_PLOT_POINTS = {{MAX_PLOT_POINTS}};`

   These will be set dynamically from the config (see code below)

**Add at the beginning (before the existing `const FILES_PER_PAGE` line):**
```javascript
// Configuration loaded from /api/config
let CONFIG = null;
let PLOT_KEYS = [];
let FILES_PER_PAGE = 30;  // Will be updated from config
let MAX_PLOT_POINTS = 500;  // Will be updated from config

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

// Then REMOVE or comment out the old template variable lines:
// Delete these lines from the extracted JavaScript:
// const FILES_PER_PAGE = {{FILES_PER_PAGE}};
// const MAX_PLOT_POINTS = {{MAX_PLOT_POINTS}};
// const PLOT_KEYS = ["wind","temp","hum","press"];  // This becomes dynamic

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
      seriesHTML += `
        <polyline id="line_${plot.id}${plot.series.length > 1 ? '_' + idx : ''}"
                  fill="none" stroke="${s.color}" stroke-width="2" points=""
                  clip-path="url(#plotClip_${plot.id})"></polyline>
        <circle id="hover_dot_${plot.id}${plot.series.length > 1 ? '_' + (s.field.includes('max') ? 'max' : 'avg') : ''}"
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

  // Apply background colors to table cells (requires adding style rules or inline styles)
  // This can be done via CSS or JavaScript after table is populated
}
```

**Modify the `renderDays()` function** (around line 569):
```javascript
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
        td.style.backgroundColor = col.bgColor;
      }

      tr.appendChild(td);
    }

    tb.appendChild(tr);
  }
}
```

**Replace the initialization** (last 3 lines around line 867-869):
```javascript
// Initialize application
(async function init() {
  await loadConfig();
  tick();
  setInterval(tick, 2000);
  loadFiles('data');
})();
```

### Step 2: Upload Files to SD Card

1. **Flash the modified firmware** to your ESP32
2. **Power on** the device and connect to WiFi
3. **Navigate** to `http://<device-ip>/upload` in your browser
4. **Upload** `web/index.html`:
   - Select the file
   - Enter the password (default: "ChangeMe" - set in `config.h` as `API_PASSWORD`)
   - Click "Upload HTML"
5. **Upload** `web/app.js`:
   - Select the file
   - Enter the password
   - Click "Upload JavaScript"
6. **Navigate** to `http://<device-ip>/` - you should see the full weather station interface

**Note:** The upload endpoint is password-protected using the same password as delete/clear operations (`API_PASSWORD` in `config.h`). After 10 failed attempts, the endpoint will be rate-limited.

### Step 3: Verify Configuration

1. Visit `http://<device-ip>/api/config` to verify the configuration JSON is served correctly
2. Check browser console for any errors
3. Verify plots and table are generated dynamically

## Adding New Sensors

To add a new sensor (e.g., CO2):

1. **Edit `config.h`:**
   ```cpp
   namespace CO2Config {
     static constexpr bool ENABLE = true;
     static constexpr uint8_t I2C_ADDR = 0x61;
   }

   // Add to PLOTS array:
   {
     "co2",
     "CO2 Level",
     "ppm",
     1.0f,
     {
       {"avgCO2", "#9C27B0", "CO2", true},
       {nullptr, nullptr, nullptr, false}
     },
     1
   }
   ```

2. **Add field to `BucketSample` struct:**
   ```cpp
   float avgCO2;  // ppm
   ```

3. **Implement sensor reading** and accumulation in main loop

4. **Recompile and flash** - UI automatically updates to show new plot!

## File Structure

```
weather_station/
  ├── weather_station/
  │   ├── weather_station.ino    (modified)
  │   ├── config.h               (new)
  │   ├── upload_page.h          (new)
  │   ├── api_help_page.h        (unchanged)
  │   └── (root_page*.h deleted after upload)
  ├── web/
  │   ├── index.html             (to upload to SD)
  │   └── app.js                 (to upload to SD - you need to create this)
  └── MODULAR_REFACTORING.md     (this file)
```

## Flash Savings

- **Before:** ~91KB (firmware + embedded HTML/JS)
- **After:** ~66KB (firmware only)
- **Savings:** ~25KB flash memory

## API Endpoints

- `/` - Main UI (from SD or fallback)
- `/root.js` - JavaScript (from SD or fallback)
- `/upload` - Upload interface (GET)
- `/upload` - File upload handler (POST)
- `/api/config` - Configuration JSON
- `/api/now` - Current sensor data
- `/api/buckets` - Time-series data
- `/api/days` - Daily summaries
- `/api/files` - File listing
- `/download?filename=20251228.csv` - Download file
- `/download_zip?days=7` - Download ZIP archive
- `/api/delete` (POST) - Delete file
- `/api/clear_data` (POST) - Clear all data
- `/api/reboot` (POST) - Reboot device

## Troubleshooting

**Problem:** Main page shows "Setup Required"
**Solution:** Upload `index.html` to SD card at `/web/index.html`

**Problem:** JavaScript errors in console
**Solution:** Upload `app.js` to SD card at `/web/app.js`

**Problem:** Plots don't appear
**Solution:** Check `/api/config` endpoint, verify CONFIG is loaded in browser console

**Problem:** Upload fails with "unauthorized" error
**Solution:** Verify you're using the correct password (set in `config.h` as `API_PASSWORD`, default is "ChangeMe")

**Problem:** Upload fails with "rate_limited" error
**Solution:** Too many failed password attempts. Wait or restart the device to reset the rate limit counter

**Problem:** Upload fails with "sd_not_available"
**Solution:** Check SD card is properly inserted and mounted

**Problem:** Upload fails with "forbidden_path"
**Solution:** Only `/web/` directory uploads are allowed for security. File paths must start with `/web/`

## Security Notes

⚠️ **IMPORTANT:** Change the default password!

The default password is `"ChangeMe"` which is **NOT secure**. To change it:

1. Edit `config.h` and change the line:
   ```cpp
   static const char* API_PASSWORD = "ChangeMe";
   ```
   to something secure, like:
   ```cpp
   static const char* API_PASSWORD = "YourSecurePassword123!";
   ```

2. Reflash the firmware

This password protects:
- File uploads (`/upload`)
- File deletion (`/api/delete`)
- Clear all data (`/api/clear_data`)

**Rate limiting:** After 10 failed password attempts, these endpoints will be blocked until the device is rebooted or the rate limit window expires.

## Notes

- After uploading to SD card, the embedded `root_page.h` and `root_page_js_mini.h` files are no longer used by the firmware
- **Keep `root_page_js.h`** (unminified version) in the repo as the source for regenerating `web/app.js` if needed
- The `root_page_js_mini.h` (minified version) is no longer needed since we're not space-constrained on SD card
- The firmware falls back to a minimal HTML page if SD files are missing
- Configuration changes require reflashing firmware, but UI changes only need re-uploading files to SD
- The unminified JavaScript (~27KB) is much easier to read and modify than the minified version (~15KB)

## Benefits

- Modular hardware configuration
- Easy to add/remove sensors
- Update UI without reflashing
- Reduced flash usage
- Single source of truth for configuration
