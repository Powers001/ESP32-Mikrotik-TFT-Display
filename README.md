# ESP32-Mikrotik-TFT-Display
An external 3.5 inch TFT display for Mikrotik Routers. It features auto adjusting real-time bandwidth, CPU, RAM, Date &amp; Time. Web-interface allows for brightness adjustment, firmware updates, WiFi hotspot management, etc.

# 🌐 Mikrotik Router Status Display

A feature-rich ESP32-based network monitoring dashboard that displays real-time statistics from your Mikrotik router on a 3.5" TFT display (480x320). Features a beautiful web interface, OTA updates, and comprehensive traffic monitoring.

![Version](https://img.shields.io/badge/version-2.3-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-green)
![License](https://img.shields.io/badge/license-MIT-orange)

## ✨ Features

### 📊 Display Features
- **Real-time Traffic Graphs**: Visualize RX/TX network traffic with smooth, scrolling line graphs
- **Animated Gauges**: CPU and RAM usage displayed as color-coded circular gauges
- **Traffic Totals**: Track data usage over hourly, daily, weekly, and monthly periods
- **Router Statistics**: Display uptime, CPU load, memory usage, and current speeds
- **Customizable Graph Range**: Set minimum and maximum values for the Y-axis (0-10,000 Mbps)
- **Hardware Sprite Acceleration**: Smooth graphics using TFT sprite buffers
- **Backlight Control**: Adjustable brightness (0-100%) with persistence across reboots

### 🌐 Web Interface
- **Modern Responsive Design**: Beautiful gradient interface that works on desktop and mobile
- **Dark Mode Support**: Toggle between light and dark themes (preference saved)
- **WiFi Network Scanner**: Built-in scanner to discover and select available networks
- **Live Statistics Dashboard**: View CPU, RAM, RX, and TX stats in real-time
- **Interface Selection**: Choose which Mikrotik interface to monitor
- **Independent Configuration Sections**: Save WiFi, router, and graph settings separately
- **Visual Feedback**: Animated cards, smooth transitions, and loading indicators

### 🔧 Configuration & Management
- **Auto-Hotspot Mode**: Automatically creates a WiFi hotspot if connection fails
- **Persistent Settings**: All configurations saved to flash memory
- **Router API Integration**: Connects via Mikrotik REST API
- **OTA Updates**: Wireless firmware updates via ElegantOTA
- **Splash Screen**: 10-second startup screen showing connection status
- **Automatic Retry**: Attempts WiFi reconnection every 5 minutes in hotspot mode
- **Data Validation**: Robust error handling and data corruption detection

### 📈 Traffic Monitoring
- **Multi-Interface Support**: Monitor any Mikrotik ethernet interface
- **Historical Data**: 40-point history buffer for graph visualization
- **Bandwidth Smoothing**: Weighted averaging to reduce graph jitter
- **Overflow Protection**: Handles counter rollovers and unrealistic values
- **Persistent Totals**: Traffic totals survive device reboots
- **Time-based Resets**: Automatic hourly/daily/weekly/monthly counter resets

## 🛠️ Hardware Requirements

### Required Components
- **ESP32 Development Board** (any variant with sufficient GPIO pins)
- **3.5" TFT LCD Display** (480x320 pixels)
  - ILI9488 or compatible controller
  - SPI interface
  - With or without touch (touch not used in this project)
- **MicroSD Card** (for LittleFS filesystem and fonts)
- **USB Cable** for initial programming and power
- **Mikrotik Router** with REST API enabled

### Recommended TFT Displays
- 3.5" ILI9488 SPI TFT
- ESP32-3248S035C (integrated ESP32 + display)
- Any 480x320 TFT with TFT_eSPI support

## 📌 Pin Configuration

The pin configuration is defined in the `TFT_eSPI` library's `User_Setup.h` file. Here's a typical configuration for ESP32:

```cpp
// TFT Display Pins (SPI)
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select
#define TFT_DC   2   // Data/Command
#define TFT_RST  4   // Reset

// Backlight Control (optional but recommended)
#define TFT_BL   27  // Backlight PWM control
```

**Note**: Adjust these pins according to your specific ESP32 board and TFT module. Some displays have fixed pin assignments.

## 📚 Required Libraries

Install these libraries via Arduino Library Manager or PlatformIO:

### Core Libraries
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** by Bodmer - Fast TFT library for ESP32
- **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)** by Benoit Blanchon - JSON parsing and serialization
- **[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)** by me-no-dev - Async HTTP server
- **[AsyncTCP](https://github.com/me-no-dev/AsyncTCP)** by me-no-dev - Async TCP library (required by ESPAsyncWebServer)
- **[ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)** by Ayush Sharma - OTA update interface

### Built-in ESP32 Libraries
- **WiFi** - WiFi connectivity
- **HTTPClient** - HTTP requests to Mikrotik API
- **Preferences** - NVS flash storage
- **LittleFS** - File system for fonts and data storage
- **SPI** - SPI communication

## 🚀 Installation

### 1. Install Arduino IDE or PlatformIO
- **Arduino IDE**: Download from [arduino.cc](https://www.arduino.cc/en/software)
- **PlatformIO**: Install as VS Code extension

### 2. Install ESP32 Board Support
Arduino IDE:
```
File → Preferences → Additional Board Manager URLs
Add: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
Tools → Board → Boards Manager → Search "ESP32" → Install
```

### 3. Install Required Libraries
```
Tools → Manage Libraries
Search and install:
- TFT_eSPI
- ArduinoJson
- ESPAsyncWebServer
- AsyncTCP
- ElegantOTA
```

### 4. Configure TFT_eSPI
Edit `TFT_eSPI/User_Setup.h` or create a custom setup file:
```cpp
#define ILI9488_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480
// Add pin definitions as shown above
```

### 5. Prepare Fonts
Create a `data/fonts/` directory in your project and add:
- `NSBold15` - Small font for labels and text
- `NSBold36` - Large font for titles

Use TFT_eSPI's font converter tool or obtain compatible `.vlw` font files.

### 6. Upload Filesystem
Upload the `data` folder to LittleFS:
- Arduino IDE: Use [ESP32 LittleFS plugin](https://github.com/lorol/arduino-esp32littlefs-plugin)
- PlatformIO: `pio run --target uploadfs`

### 7. Upload Sketch
- Select your ESP32 board
- Choose correct COM port
- Click Upload

## ⚙️ Initial Configuration

### 1. First Boot (Hotspot Mode)
On first boot, the device will create a WiFi hotspot:
- **SSID**: `Mikrotik_Display-XXXX` (XX = last 2 MAC address bytes)
- **Password**: `admin`
- **IP Address**: `192.168.4.1`

### 2. Connect and Configure
1. Connect your phone/computer to the hotspot
2. Open browser to `http://192.168.4.1`
3. Configure settings:

#### WiFi Settings
- Click "Scan WiFi Networks"
- Select your network
- Enter password
- Click "Save WiFi & Restart"

#### Router Settings
- **Router Address**: `http://192.168.1.1` (your Mikrotik's IP)
- **API Username**: Create a user in Mikrotik with API access
- **API Password**: Password for that user
- **Interface to Monitor**: Select from dropdown (loads from router)

#### Graph Settings
- **Minimum Mbps**: Lower bound of graph (typically 0)
- **Maximum Mbps**: Upper bound of graph (match your connection speed)

#### Display Settings
- **Backlight Brightness**: Adjust screen brightness (0-100%)

### 3. Mikrotik Router Setup

Create an API user on your Mikrotik router:
```
/user add name=APIUser password=yourpassword group=read
```

Ensure REST API is enabled (enabled by default on RouterOS 7.x+):
```
/ip/service/print
# Ensure "api" and "www-ssl" or "www" are enabled
```

## 📖 Usage

### Normal Operation
After configuration, the display shows:
- **Top Section**: Date, time, CPU, RAM, uptime, current speeds
- **Middle Section**: Animated gauges for RAM and RX usage percentage
- **Center**: Real-time traffic graph (RX in yellow, TX in blue)
- **Bottom**: Hourly, daily, weekly, and monthly traffic totals

### Web Interface Access
When connected to your WiFi network:
- Find the device's IP address (shown on splash screen)
- Browse to `http://[device-ip]`
- Access configuration and live statistics

### OTA Updates
1. Access web interface
2. Click "Open OTA Update Portal"
3. Upload new `.bin` firmware file
4. Device will update and restart automatically

### Reconfiguration
If you need to reconfigure:
- Access the web interface at the device's IP
- Modify settings as needed
- Each section saves independently
- Device restarts after saving

## 🔍 Troubleshooting

### Display Shows "Config Mode" Continuously
- WiFi credentials may be incorrect
- Router may be out of range
- Check SSID and password in web interface

### No Data on Graph
- Verify Mikrotik IP address is correct
- Check API username/password
- Ensure router has REST API enabled
- Verify selected interface ID is correct

### Graph Shows Spikes/Errors
- Check that Max Mbps setting matches your connection
- Ensure router counters aren't resetting
- Verify stable WiFi connection

### OTA Update Fails
- Ensure `.bin` file is compiled for same ESP32 board
- Check available flash space
- Try smaller firmware (disable features if needed)

### Display Backlight Not Working
- Verify `TFT_BL` pin is defined in `User_Setup.h`
- Check that pin supports PWM output
- Some displays have hardware-controlled backlights

## 🎨 Customization

### Graph Colors
Edit in main `.ino` file:
```cpp
#define GRAPH_COLOR_TX TFT_BLUE    // Change transmit line color
#define GRAPH_COLOR_RX TFT_YELLOW  // Change receive line color
```

### Graph History
Change history buffer size:
```cpp
#define HISTORY_SIZE 40  // Increase for longer history
```

### Polling Interval
Adjust update frequency in `loop()`:
```cpp
delay(500);  // Change to 1000 for slower updates
```

### Web Interface Theme
Modify colors in `web_interface.h`:
```css
--primary: #667eea;      /* Primary gradient color */
--secondary: #764ba2;    /* Secondary gradient color */
```

## 📊 API Endpoints

The device exposes these REST endpoints:

- `GET /` - Web interface
- `GET /api/scan` - Scan WiFi networks
- `GET /api/interfaces` - List Mikrotik interfaces
- `GET /api/config` - Get current configuration
- `GET /api/stats` - Get live statistics
- `POST /save-wifi` - Save WiFi settings
- `POST /save-router` - Save router settings
- `POST /save-graph` - Save graph settings
- `POST /api/backlight` - Set backlight brightness
- `POST /api/theme` - Save theme preference
- `GET /update` - ElegantOTA update portal

## 🙏 Credits & Acknowledgments

### Library Authors
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** - © Bodmer - Optimized TFT library
- **[ArduinoJson](https://github.com/bblanchon/ArduinoJson)** - © Benoit Blanchon - JSON library
- **[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)** - © me-no-dev - Async web server
- **[AsyncTCP](https://github.com/me-no-dev/AsyncTCP)** - © me-no-dev - Async TCP library
- **[ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)** - © Ayush Sharma - OTA updates

### Special Thanks
- Mikrotik for their REST API documentation
- ESP32 Arduino Core team
- Arduino and PlatformIO communities

## 📄 License

This project is released under the MIT License. Feel free to modify and distribute.

```
MIT License

Copyright (c) 2024

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

### Development Setup
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on hardware
5. Submit a pull request

## 📞 Support

- **Issues**: Open an issue on GitHub
- **Discussions**: Use GitHub Discussions for questions
- **Documentation**: Check this README and code comments

## 🗺️ Roadmap

Potential future enhancements:
- [ ] Touch screen support for configuration
- [ ] Multiple interface monitoring on one screen
- [ ] Alert notifications for bandwidth limits
- [ ] Historical data logging to SD card
- [ ] Custom widget layouts
- [ ] MQTT support for home automation
- [ ] Multi-language support
- [ ] Mobile app companion

---

**Made with ❤️ for network enthusiasts and Mikrotik users**

*Last Updated: November 2024 | Version 2.3*
