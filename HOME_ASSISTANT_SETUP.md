# Home Assistant Integration Guide

This weather station now includes a dedicated REST API endpoint optimized for Home Assistant integration.

## API Endpoint

**URL:** `http://YOUR_DEVICE_IP/api/now`

**Method:** GET

**Response Format:** JSON

## Available Sensors

The endpoint provides the following data:

**Environmental:**
- `temp_c` - Temperature in °C
- `hum_rh` - Relative humidity in %
- `press_hpa` - Atmospheric pressure in hPa

**Wind:**
- `wind_ms` - Wind speed in m/s
- `wind_pps` - Wind sensor pulses per second

**Air Quality:**
- `pm1` - PM1.0 particulate matter in μg/m³
- `pm25` - PM2.5 particulate matter in μg/m³
- `pm10` - PM10 particulate matter in μg/m³
- `aqi_pm25` - Air Quality Index based on PM2.5
- `aqi_pm25_category` - AQI category (Good, Moderate, etc.)
- `aqi_pm10` - Air Quality Index based on PM10
- `aqi_pm10_category` - AQI category for PM10

**System:**
- `epoch` - Unix timestamp
- `local_time` - Local timestamp string
- `cpu_temp_c` - ESP32 CPU temperature in °C
- `uptime_s` - Device uptime in seconds
- `wifi_rssi` - WiFi signal strength in dBm
- `free_heap` - Free heap memory in bytes
- `heap_size` - Total heap size in bytes
- `retention_days` - Data retention days (0 = forever)

**Status:**
- `bme280_ok` - BME280 sensor status (boolean)
- `pms5003_ok` - PMS5003 sensor status (boolean)
- `sd_ok` - SD card status (boolean)

## Home Assistant Configuration

Add the following to your `configuration.yaml`:

## Configuration (Single API Call)

For better efficiency, you can use a single REST sensor with JSON attributes to get all values with one API call:

```yaml
sensor:
  - platform: rest
    name: "Weather Station"
    resource: "http://YOUR_DEVICE_IP/api/now"
    value_template: "{{ value_json.temp_c }}"
    unit_of_measurement: "°C"
    device_class: temperature
    state_class: measurement
    scan_interval: 10
    json_attributes:
      - hum_rh
      - press_hpa
      - wind_ms
      - wind_pps
      - pm1
      - pm25
      - pm10
      - aqi_pm25
      - aqi_pm25_category
      - aqi_pm10
      - aqi_pm10_category
      - local_time
      - cpu_temp_c
      - wifi_rssi
      - uptime_s
      - bme280_ok
      - pms5003_ok
      - sd_ok

template:
  - sensor:
      # Environmental Sensors
      - name: "Weather Station Humidity"
        state: "{{ state_attr('sensor.weather_station', 'hum_rh') }}"
        unit_of_measurement: "%"
        device_class: humidity
        state_class: measurement

      - name: "Weather Station Pressure"
        state: "{{ state_attr('sensor.weather_station', 'press_hpa') }}"
        unit_of_measurement: "hPa"
        device_class: pressure
        state_class: measurement

      # Wind Sensors
      - name: "Weather Station Wind Speed"
        state: "{{ state_attr('sensor.weather_station', 'wind_ms') }}"
        unit_of_measurement: "m/s"
        icon: mdi:weather-windy
        state_class: measurement

      - name: "Weather Station Wind Speed (km/h)"
        state: "{{ (state_attr('sensor.weather_station', 'wind_ms') | float * 3.6) | round(1) }}"
        unit_of_measurement: "km/h"
        icon: mdi:weather-windy
        state_class: measurement

      # Air Quality Sensors
      - name: "Weather Station PM1.0"
        state: "{{ state_attr('sensor.weather_station', 'pm1') }}"
        unit_of_measurement: "μg/m³"
        device_class: pm1
        state_class: measurement

      - name: "Weather Station PM2.5"
        state: "{{ state_attr('sensor.weather_station', 'pm25') }}"
        unit_of_measurement: "μg/m³"
        device_class: pm25
        state_class: measurement

      - name: "Weather Station PM10"
        state: "{{ state_attr('sensor.weather_station', 'pm10') }}"
        unit_of_measurement: "μg/m³"
        device_class: pm10
        state_class: measurement

      - name: "Weather Station AQI PM2.5"
        state: "{{ state_attr('sensor.weather_station', 'aqi_pm25') }}"
        unit_of_measurement: "AQI"
        device_class: aqi
        state_class: measurement

      - name: "Weather Station AQI PM10"
        state: "{{ state_attr('sensor.weather_station', 'aqi_pm10') }}"
        unit_of_measurement: "AQI"
        device_class: aqi
        state_class: measurement

      - name: "Weather Station AQI Category"
        state: "{{ state_attr('sensor.weather_station', 'aqi_pm25_category') }}"
        icon: mdi:air-filter

      # System Sensors
      - name: "Weather Station CPU Temperature"
        state: "{{ state_attr('sensor.weather_station', 'cpu_temp_c') }}"
        unit_of_measurement: "°C"
        device_class: temperature
        state_class: measurement

      - name: "Weather Station WiFi Signal"
        state: "{{ state_attr('sensor.weather_station', 'wifi_rssi') }}"
        unit_of_measurement: "dBm"
        device_class: signal_strength
        state_class: measurement

      - name: "Weather Station Uptime"
        state: "{{ (state_attr('sensor.weather_station', 'uptime_s') | int / 3600) | round(1) }}"
        unit_of_measurement: "hours"
        icon: mdi:clock-outline

  - binary_sensor:
      # Status Sensors
      - name: "Weather Station BME280 Status"
        state: "{{ state_attr('sensor.weather_station', 'bme280_ok') }}"
        device_class: connectivity

      - name: "Weather Station PMS5003 Status"
        state: "{{ state_attr('sensor.weather_station', 'pms5003_ok') }}"
        device_class: connectivity

      - name: "Weather Station SD Card Status"
        state: "{{ state_attr('sensor.weather_station', 'sd_ok') }}"
        device_class: connectivity
```

## Notes

1. Replace `YOUR_DEVICE_IP` with your weather station's actual IP address
2. The `scan_interval` is set to 60 seconds by default - adjust as needed
3. The optimized configuration makes only one API call per scan interval, reducing load on your ESP32
4. All sensor values return `null` if the reading is invalid or unavailable
5. The endpoint automatically handles sensor failures gracefully

## Testing

You can test the endpoint with curl:
```bash
curl http://YOUR_DEVICE_IP/api/now
```

Or visit the API documentation page at:
```
http://YOUR_DEVICE_IP/api_help
```
