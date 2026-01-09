# Home Assistant Integration Guide

This weather station now includes a dedicated REST API endpoint optimized for Home Assistant integration.

## API Endpoint

**URL:** `http://YOUR_DEVICE_IP/api/homeassistant`

**Method:** GET

**Response Format:** JSON

## Available Sensors

The endpoint provides the following data:
- `temperature` - Temperature in °C
- `humidity` - Relative humidity in %
- `pressure` - Atmospheric pressure in hPa
- `wind_speed` - Wind speed in m/s
- `pm1` - PM1.0 particulate matter in μg/m³
- `pm25` - PM2.5 particulate matter in μg/m³
- `pm10` - PM10 particulate matter in μg/m³
- `aqi_pm25` - Air Quality Index based on PM2.5
- `aqi_pm25_category` - AQI category (Good, Moderate, etc.)
- `aqi_pm10` - Air Quality Index based on PM10
- `aqi_pm10_category` - AQI category for PM10
- `timestamp` - Local timestamp
- `bme280_ok` - BME280 sensor status (boolean)
- `pms5003_ok` - PMS5003 sensor status (boolean)
- `wifi_rssi` - WiFi signal strength in dBm

## Home Assistant Configuration

Add the following to your `configuration.yaml`:

```yaml
sensor:
  # Temperature sensor
  - platform: rest
    name: "Weather Station Temperature"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.temperature }}"
    unit_of_measurement: "°C"
    device_class: temperature
    state_class: measurement
    scan_interval: 60

  # Humidity sensor
  - platform: rest
    name: "Weather Station Humidity"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.humidity }}"
    unit_of_measurement: "%"
    device_class: humidity
    state_class: measurement
    scan_interval: 60

  # Pressure sensor
  - platform: rest
    name: "Weather Station Pressure"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.pressure }}"
    unit_of_measurement: "hPa"
    device_class: pressure
    state_class: measurement
    scan_interval: 60

  # Wind speed sensor
  - platform: rest
    name: "Weather Station Wind Speed"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.wind_speed }}"
    unit_of_measurement: "m/s"
    icon: mdi:weather-windy
    state_class: measurement
    scan_interval: 60

  # PM2.5 sensor
  - platform: rest
    name: "Weather Station PM2.5"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.pm25 }}"
    unit_of_measurement: "μg/m³"
    device_class: pm25
    state_class: measurement
    scan_interval: 60

  # PM10 sensor
  - platform: rest
    name: "Weather Station PM10"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.pm10 }}"
    unit_of_measurement: "μg/m³"
    device_class: pm10
    state_class: measurement
    scan_interval: 60

  # AQI PM2.5 sensor
  - platform: rest
    name: "Weather Station AQI PM2.5"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.aqi_pm25 }}"
    unit_of_measurement: "AQI"
    device_class: aqi
    state_class: measurement
    scan_interval: 60

  # WiFi Signal
  - platform: rest
    name: "Weather Station WiFi Signal"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.wifi_rssi }}"
    unit_of_measurement: "dBm"
    device_class: signal_strength
    state_class: measurement
    scan_interval: 300
```

## Optimized Configuration (Single API Call)

For better efficiency, you can use a single REST sensor with JSON attributes to get all values with one API call:

```yaml
sensor:
  - platform: rest
    name: "Weather Station"
    resource: "http://YOUR_DEVICE_IP/api/homeassistant"
    value_template: "{{ value_json.temperature }}"
    unit_of_measurement: "°C"
    device_class: temperature
    state_class: measurement
    scan_interval: 60
    json_attributes:
      - humidity
      - pressure
      - wind_speed
      - pm1
      - pm25
      - pm10
      - aqi_pm25
      - aqi_pm25_category
      - aqi_pm10
      - aqi_pm10_category
      - timestamp
      - wifi_rssi

template:
  - sensor:
      - name: "Weather Station Humidity"
        state: "{{ state_attr('sensor.weather_station', 'humidity') }}"
        unit_of_measurement: "%"
        device_class: humidity
        state_class: measurement

      - name: "Weather Station Pressure"
        state: "{{ state_attr('sensor.weather_station', 'pressure') }}"
        unit_of_measurement: "hPa"
        device_class: pressure
        state_class: measurement

      - name: "Weather Station Wind Speed"
        state: "{{ state_attr('sensor.weather_station', 'wind_speed') }}"
        unit_of_measurement: "m/s"
        icon: mdi:weather-windy
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

      - name: "Weather Station AQI Category"
        state: "{{ state_attr('sensor.weather_station', 'aqi_pm25_category') }}"
        icon: mdi:air-filter
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
curl http://YOUR_DEVICE_IP/api/homeassistant
```

Or visit the API documentation page at:
```
http://YOUR_DEVICE_IP/api_help
```
