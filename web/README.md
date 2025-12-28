# Web Files for Weather Station

These files should be uploaded to the SD card at `/web/` directory.

## Files

### `index.html` (Ready to use ✅)
- Dynamic HTML template with placeholders for plots and table
- No hardcoded sensor data
- Responsive layout
- Upload button included

### `app.js` (Ready to use ✅)
- Complete JavaScript application with dynamic configuration support
- Automatically loads configuration from `/api/config`
- Dynamically generates plots based on config
- Dynamically generates table headers and data
- Falls back to default config if API fails

## Upload Instructions

1. **Flash the firmware** with the modular refactoring changes
2. **Power on** your ESP32 and connect to WiFi
3. **Navigate** to `http://<device-ip>/upload`
4. **Upload index.html:**
   - Select `web/index.html`
   - Enter password (default: "ChangeMe")
   - Click "Upload HTML"
5. **Upload app.js:**
   - Select `web/app.js`
   - Enter password
   - Click "Upload JavaScript"
6. **Done!** Navigate to `http://<device-ip>/` to see your weather station

## Features

### Dynamic Configuration
- Plots and table columns are defined in `config.h`
- Add/remove sensors without touching HTML/JS
- Change units, colors, labels in config
- UI updates automatically

### Default Configuration
If `/api/config` fails to load, the app uses this default:

**Plots:**
- Wind Speed (km/h) - black line for average, orange for gust
- Temperature (°C) - red line
- Humidity (%) - blue line
- Pressure (hPa) - green line

**Table Columns:**
- Day
- Wind (avg, max)
- Temperature (avg, min, max)
- Humidity (avg, min, max)
- Pressure (avg, min, max)

### Browser Console
Open browser console (F12) to see:
- `Config loaded: {plots: [...], tableColumns: [...]}` when successful
- Error messages if config fails to load
- Which config is being used (API or fallback)

## File Sizes

- `index.html`: ~8 KB
- `app.js`: ~36 KB
- **Total**: ~44 KB on SD card

## Adding a New Sensor

Example: Adding a CO2 sensor

1. **Edit `weather_station/config.h`:**
   ```cpp
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

   // Add to TABLE_COLUMNS array:
   {"avgCO2", "Avg CO2", "ppm", 1.0f, 0, ""}
   ```

2. **Add field to BucketSample struct:**
   ```cpp
   float avgCO2;  // ppm
   ```

3. **Implement sensor reading** in your code

4. **Reflash firmware** - that's it! The UI automatically updates.

No need to re-upload HTML/JS files unless you want to change styling or add custom features.

## Troubleshooting

**Plots don't appear:**
- Check browser console for errors
- Verify `/api/config` returns valid JSON
- Check that CONFIG object is loaded

**Table is empty:**
- Verify `/api/days` returns data
- Check CONFIG.tableColumns in console

**Everything is blank:**
- Make sure you uploaded both files
- Check SD card is mounted
- Try the fallback page at device IP to access upload

**Wrong colors/units:**
- Config is cached - do a hard refresh (Ctrl+F5)
- Check `/api/config` endpoint directly in browser
- Verify you reflashed firmware after editing config.h

## Modifying the UI

To customize styles, layout, or add features:

1. Edit `index.html` and/or `app.js` locally
2. Re-upload via `/upload` endpoint
3. Hard refresh your browser (Ctrl+F5)

The configuration structure remains the same, so you can:
- Change CSS styles
- Modify chart dimensions
- Add new UI elements
- Customize animations

All while keeping the dynamic configuration system intact.
