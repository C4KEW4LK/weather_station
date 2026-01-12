#pragma once

// ==================== HARDWARE CONFIGURATION ====================

// Security
// UI and API password
static const char* API_PASSWORD = "ChangeMe";
//Over the air update password
static const char* OTA_PASSWORD = "PleaseChangeMe";

// Wind Sensor (Pulse-based Anemometer)
namespace WindConfig {
  static constexpr int   PULSE_PIN = 3;                 // GPIO pin
  static constexpr float PPS_TO_MS = 1.75f / 20.0f;     // Calibration: 20pps = 1.75m/s
  static constexpr uint32_t PPS_WINDOW_MS = 1000;       // Calculate every 1 second
  static constexpr uint32_t DEBOUNCE_US = 2000;         // 2ms debounce
}

// BME280 Environmental Sensor (I2C)
namespace BME280Config {
  static constexpr bool     ENABLE = true;
  static constexpr uint8_t  I2C_ADDR_PRIMARY = 0x76;
  static constexpr uint8_t  I2C_ADDR_FALLBACK = 0x77;
  static constexpr uint32_t POLL_INTERVAL_MS = 2000;
  static constexpr float    ALTITUDE_METERS = 580.0f;  // Station altitude for MSLP calculation (0 = sea level)
}

// PMS5003 Air Quality Sensor (UART)
namespace PMS5003Config {
  static constexpr bool     ENABLE = true;
  static constexpr int      RX_PIN = 4;
  static constexpr int      TX_PIN = 5;  // Not used, but required for Serial2.begin()
  static constexpr uint32_t POLL_INTERVAL_MS = 2000;  // Poll every 2 seconds
}

// SD Card (SPI)
namespace SDConfig {
  static constexpr bool ENABLE = true;
  static constexpr int  CS_PIN = 20;
}

// Data Logging
namespace LogConfig {
  static constexpr int BUCKET_SECONDS = 60;             // Log interval (1 minute)
  static constexpr int BUCKETS_24H = 24 * 60 * 60 / BUCKET_SECONDS;
  static constexpr int RETENTION_DAYS = 0;           // 0 = never delete
  static constexpr int DAYS_HISTORY = 30;               // RAM history
  static_assert((86400 % BUCKET_SECONDS) == 0, "BUCKET_SECONDS must divide evenly into 24h");
}

// Network & Time
namespace NetworkConfig {
  static const char* NTP_SERVER_1 = "pool.ntp.org";
  static const char* NTP_SERVER_2 = "time.google.com";
  static const char* NTP_SERVER_3 = "time.windows.com";
  static const char* TIMEZONE = "AEST-10AEDT-11,M10.1.0/02:00:00,M4.1.0/03:00:00"; // Sydney
  static const char* AP_NAME = "Anemometer-Setup";
  static constexpr uint32_t AP_TIMEOUT_S = 180;
}

// Web UI
namespace UIConfig {
  static constexpr int FILES_PER_PAGE = 30;
  static constexpr int MAX_PLOT_POINTS = 500;
}

// ==================== PLOT CONFIGURATION ====================

struct PlotSeries {
  const char* field;       // Data field name (e.g., "avgWind", "maxWind")
  const char* color;       // SVG color
  const char* label;       // Display label for tooltip
  bool visible;            // Show by default
};

struct PlotConfig {
  const char* id;          // Unique ID (e.g., "wind", "temp")
  const char* title;       // Display title
  const char* unit;        // Unit string (e.g., "km/h", "°C")
  float conversionFactor;  // Multiply data by this (e.g., 3.6 for m/s to km/h)
  PlotSeries series[3];    // Max 3 series per plot
  int seriesCount;
};

// Plot definitions
static const PlotConfig PLOTS[] = {
  {
    "wind",
    "Wind Speed",
    "km/h",
    3.6f,  // m/s to km/h
    {
      {"avgWind", "black", "Wind", true},
      {"maxWind", "#f0ad4e", "Gust", true}
    },
    2
  },
  {
    "temp",
    "Temperature",
    "°C",
    1.0f,
    {
      {"avgTempC", "#d9534f", "Temp", true},
      {nullptr, nullptr, nullptr, false}
    },
    1
  },
  {
    "hum",
    "Humidity",
    "%",
    1.0f,
    {
      {"avgHumRH", "#0275d8", "Humidity", true},
      {nullptr, nullptr, nullptr, false}
    },
    1
  },
  {
    "press",
    "Pressure (MSLP)",
    "hPa",
    1.0f,
    {
      {"avgPressHpa", "#5cb85c", "Pressure", true},
      {nullptr, nullptr, nullptr, false}
    },
    1
  },
  {
    "pm",
    "Air Quality (PM)",
    "μg/m³",
    1.0f,
    {
      {"avgPM25", "#ff6600", "PM2.5", true},
      {"avgPM10", "#996633", "PM10", true},
      {"avgPM1", "#9966cc", "PM1.0", true}
    },
    3
  }
};

static constexpr int PLOTS_COUNT = sizeof(PLOTS) / sizeof(PLOTS[0]);

// ==================== TABLE CONFIGURATION ====================

struct TableColumn {
  const char* field;       // Data field name
  const char* label;       // Column sub-header (avg, max, min)
  const char* unit;        // Unit to display
  float conversionFactor;  // Multiply by this
  int decimals;            // Decimal places
  const char* bgColor;     // Background color (hex or empty for default)
  const char* group;       // Group header (Wind Speed, Temperature, etc.)
};

static const TableColumn TABLE_COLUMNS[] = {
  {"day", "Day", "", 1.0f, 0, "", ""},
  {"avgWind", "avg", "km/h", 3.6f, 1, "#f8f8f8", "Wind Speed"},
  {"maxWind", "max", "km/h", 3.6f, 1, "#f8f8f8", "Wind Speed"},
  {"avgTemp", "avg", "°C", 1.0f, 1, "", "Temperature"},
  {"minTemp", "min", "°C", 1.0f, 1, "", "Temperature"},
  {"maxTemp", "max", "°C", 1.0f, 1, "", "Temperature"},
  {"avgHum", "avg", "%", 1.0f, 1, "#f8f8f8", "Humidity"},
  {"minHum", "min", "%", 1.0f, 1, "#f8f8f8", "Humidity"},
  {"maxHum", "max", "%", 1.0f, 1, "#f8f8f8", "Humidity"},
  {"avgPress", "avg", "hPa", 1.0f, 1, "", "Pressure"},
  {"minPress", "min", "hPa", 1.0f, 1, "", "Pressure"},
  {"maxPress", "max", "hPa", 1.0f, 1, "", "Pressure"},
  {"avgPM1", "PM1 avg", "μg/m³", 1.0f, 1, "#f8f8f8", "Particulate Matter"},
  {"maxPM1", "PM1 max", "μg/m³", 1.0f, 1, "#f8f8f8", "Particulate Matter"},
  {"avgPM25", "PM2.5 avg", "μg/m³", 1.0f, 1, "#f8f8f8", "Particulate Matter"},
  {"maxPM25", "PM2.5 max", "μg/m³", 1.0f, 1, "#f8f8f8", "Particulate Matter"},
  {"avgPM10", "PM10 avg", "μg/m³", 1.0f, 1, "#f8f8f8", "Particulate Matter"},
  {"maxPM10", "PM10 max", "μg/m³", 1.0f, 1, "#f8f8f8", "Particulate Matter"}
};

static constexpr int TABLE_COLUMNS_COUNT = sizeof(TABLE_COLUMNS) / sizeof(TABLE_COLUMNS[0]);
