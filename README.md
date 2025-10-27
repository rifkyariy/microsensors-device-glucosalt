# DIABETIC - Health Monitoring Device

A real-time health monitoring system built on ESP32 that measures heart rate and blood oxygen saturation (SpO2) using the MAX30102 pulse oximeter sensor. The device continuously streams physiological data to a cloud API for analysis and storage.

## 🚀 Features

- **Real-time Heart Rate Monitoring**: Accurate heart rate measurement (30-220 BPM)
- **Blood Oxygen Saturation (SpO2)**: Continuous SpO2 monitoring (50-100%)
- **PPG Waveform Data**: Captures and transmits raw PPG signals (IR and Red channels)
- **WiFi Connectivity**: Wireless data transmission to cloud backend
- **Sliding Window Algorithm**: 100Hz sampling with 1-second rolling buffer
- **Data Validation**: Automatic filtering of invalid readings
- **RESTful API Integration**: Secure HTTPS communication with backend

## 📋 Hardware Requirements

- **ESP32 Development Board** (ESP32-DevKitC or similar)
- **MAX30102 Pulse Oximeter Sensor** (SparkFun or compatible)
- **Connection Wires**
- **USB Cable** for programming and power

### Wiring Diagram

| MAX30102 | ESP32 |
|----------|-------|
| VCC      | 3.3V  |
| GND      | GND   |
| SDA      | GPIO 21 (SDA) |
| SCL      | GPIO 22 (SCL) |

## 🛠️ Software Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- ESP32 Board Support Package
- Arduino Framework

### Dependencies

The following libraries are automatically managed by PlatformIO:

- `SparkFun MAX3010x Pulse and Proximity Sensor Library` - MAX30102 sensor driver
- `ArduinoJson` - JSON serialization/deserialization
- `WiFi` - ESP32 WiFi connectivity
- `HTTPClient` - HTTP/HTTPS communication
- `Wire` - I2C communication

## 📦 Installation

### 1. Clone the Repository

```bash
git clone https://github.com/rifkyariy/microsensors-device-glucosalt.git
cd microsensors-device-glucosalt
```

### 2. Open in PlatformIO

- Open the project folder in VS Code with PlatformIO extension installed
- PlatformIO will automatically download all dependencies

### 3. Configure WiFi and API Credentials

Edit the following constants in `src/main.cpp`:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* API_BASE_URL = "YOUR_API_BASE_URL";
const char* DEVICE_ID = "YOUR_DEVICE_ID";
const char* USER_ID = "YOUR_USER_ID";
const char* API_KEY = "YOUR_API_KEY";
```

### 4. Build and Upload

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

Or use the PlatformIO GUI buttons in VS Code.

## 🔧 Configuration

### Sensor Settings

The following parameters can be adjusted in `setup()`:

```cpp
#define SAMPLE_RATE_HZ 100      // Sampling frequency (100 Hz)
#define BUFFER_LENGTH 100       // Rolling buffer size (1 second at 100Hz)
#define UPDATE_INTERVAL_MS 250  // API update frequency (4 times per second)
```

### LED and Sensor Configuration

```cpp
byte ledBrightness = 0x1F;  // LED intensity (0x00 - 0xFF)
byte sampleAverage = 4;      // Sample averaging (1, 2, 4, 8, 16, 32)
byte ledMode = 2;            // Mode 2 = Red + IR
int pulseWidth = 411;        // Pulse width in µs (69, 118, 215, 411)
int adcRange = 4096;         // ADC range (2048, 4096, 8192, 16384)
```

## 📡 API Communication

### Endpoint

```
POST {API_BASE_URL}/health/metrics
```

### Request Headers

```
Content-Type: application/json
Authorization: Bearer {API_KEY}
```

### Request Payload

```json
{
  "device_id": "ESP32_001",
  "user_id": "64a6f31f-68c4-4122-b406-55dc2d75703e",
  "heart_rate": 75,
  "blood_oxygen": 98,
  "ppg_ir": [/* 100 IR samples */],
  "ppg_red": [/* 100 Red samples */]
}
```

### Response

- **2xx**: Data successfully received
- **4xx**: Client error (check credentials/payload)
- **5xx**: Server error

## 🔬 How It Works

### Sliding Window Algorithm

1. **Initialization**: Fills a 100-sample buffer (1 second at 100Hz)
2. **Continuous Sampling**: New samples arrive every 10ms
3. **Buffer Update**: Oldest sample discarded, newest added to buffer
4. **Processing**: Every 250ms, the entire 1-second window is analyzed
5. **Transmission**: Valid readings sent to API with PPG waveforms

### Data Validation

Readings are only transmitted if:
- Heart rate is valid (30-220 BPM)
- SpO2 is valid (60-100%)
- Both sensors report valid data

## 📊 Serial Monitor Output

```
Connecting to WiFi 'YOUR_SSID' ...
Connected, IP: 192.168.1.100
Priming data buffer (collecting first 1s of data)...
Buffer primed. Starting real-time updates.
HR: 75 (valid:1) | SpO2: 98 (valid:1)
POST -> 200
---
HR: 76 (valid:1) | SpO2: 97 (valid:1)
POST -> 200
---
```

## 🐛 Troubleshooting

### MAX30102 not found
- Check I2C wiring (SDA/SCL)
- Verify sensor power (3.3V)
- Try a different I2C bus speed

### WiFi Connection Timeout
- Verify SSID and password
- Check WiFi signal strength
- Ensure 2.4GHz WiFi is enabled

### Invalid Readings
- Ensure finger is properly placed on sensor
- Keep finger still during measurement
- Clean sensor surface
- Adjust LED brightness if needed

### HTTP POST Failures
- Verify API URL and credentials
- Check internet connectivity
- Review API server logs
- Ensure valid SSL certificates (for HTTPS)

## 📁 Project Structure

```
DIABETIC/
├── src/
│   └── main.cpp           # Main application code
├── include/               # Header files
├── lib/                   # Custom libraries
├── test/                  # Test files
├── platformio.ini         # PlatformIO configuration
└── README.md             # This file
```

## 🤝 Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📄 License

This project is part of the **microsensors-device-glucosalt** system developed for health monitoring applications.

## 👥 Authors

- **rifkyariy** - *Project Owner* - [GitHub](https://github.com/rifkyariy)

## 🙏 Acknowledgments

- SparkFun Electronics for the MAX30102 sensor library
- Maxim Integrated for the SpO2 algorithm
- ESP32 community for Arduino framework support
- PlatformIO for excellent development tools

## 📞 Support

For issues, questions, or suggestions:
- Open an issue on GitHub
- Contact: [Project Repository](https://github.com/rifkyariy/microsensors-device-glucosalt)

---

**⚠️ Medical Disclaimer**: This device is for educational and research purposes only. It is NOT a medical device and should NOT be used for clinical diagnosis or treatment decisions. Always consult qualified healthcare professionals for medical advice.
