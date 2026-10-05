# 📝 Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### 🎨 Added
- **README visual upgrade** with professional badges, emojis and GitHub-optimized layout
- **Interactive command table** with examples and descriptions
- **Mermaid flowchart** for boot and operational flow
- **LED state indicators** documentation with color codes
- **Performance metrics** table with response times and memory usage
- **Troubleshooting section** with verified solutions
- **Security notes** for network exposure warnings
- **Contribution guidelines** with step-by-step git workflow

### 🔄 Changed
- Reorganized README structure for better navigation with jump links
- Enhanced command reference with categorized tables
- Improved hardware pinout documentation with GPIO descriptions
- Updated portal web section with UI mockup details
- Reworked NTP section with server list and timezone info
- Restructured troubleshooting for quick diagnostics

### ✅ Fixed
- README alignment with actual repository structure
- Command table accuracy against `CommandHandler.h` implementation
- Documentation consistency between sections
- Broken links and outdated references removed

---

## [1.0.1] - 2026-10-05

### 🎨 Added
- **Professional README** with shields.io badges and brand consistency
- **Quick start guide** with 3-step setup procedure
- **Architecture diagram** showing module organization
- **Mermaid flowchart** for boot sequence and state transitions
- **Command reference** split into diagnostics, configuration and hardware control
- **Performance benchmarks** table
- **Security advisory** section
- **Resources links** for ESP32, M5Stack and tools

### 🔧 Changed
- Visual presentation optimized for GitHub community standards
- README now includes interactive navigation links
- Command examples with real usage patterns
- Hardware pinning table with validated GPIO assignments

### 🐛 Fixed
- README structure to match actual firmware capabilities
- Corrected command examples to match implementation
- Removed placeholder and outdated sections

---

## [1.0.0] - 2026-10-05

### 🚀 Initial Release

#### ✨ Core Features
- **Wi‑Fi Provisioning:** Automatic connection with fallback to captive portal
- **UDP Command Server:** Port 4210 for remote device control
- **NTP Synchronization:** Local time with timezone and DST support
- **Hardware Control:** RGB LED, status LED, button, IR emitter
- **Device Diagnostics:** CPU, RAM, flash, uptime, network metrics
- **OTA Updates:** Over-The-Air firmware updates via ArduinoOTA
- **Modular Architecture:** Separate header files for each component

#### 📦 Components Included
- `M5NanoC6_wifi.ino` - Main firmware entry point
- `Config.h` - Hardware configuration and portal HTML
- `HardwareController.h` - LED and GPIO management
- `DeviceNetwork.h` - Wi‑Fi, AP, UDP and OTA handling
- `NTPService.h` - Time synchronization service
- `CommandHandler.h` - UDP command parsing and execution

#### 🎯 Capabilities
- **AP Mode:** Configurable portal on `http://192.168.4.1`
- **STA Mode:** Persistent Wi‑Fi storage in `Preferences`
- **Reset Function:** Factory reset via button press
- **Visual Feedback:** RGB LED state indication
- **Timezone Support:** GMT-3 default with CLI configuration
- **Diagnostic Commands:** 18+ queries for system status

#### 🔒 Security Considerations
- UDP communication (unencrypted) - local networks only
- Web portal without authentication by default
- Recommended for private LAN deployment
- Firewall/VPN protection for external exposure

#### ⚡ Performance
- Boot time: < 3 seconds (with Wi‑Fi)
- UDP response latency: < 50ms
- Memory footprint: ~60KB RAM
- Flash usage: ~350KB
- Wi‑Fi reconnection: < 8 seconds

#### 📱 Platform Support
- **Board:** M5Stack M5NanoC6
- **MCU:** ESP32-C6
- **Build Tools:** Arduino IDE 1.8.x+, PlatformIO
- **Libraries:** WiFi, WebServer, Preferences, Adafruit NeoPixel

---

## 🔗 Related Resources

- [GitHub Issues](https://github.com/paulocfmarques-collab/M5NanoC6_wifi/issues)
- [Pull Requests](https://github.com/paulocfmarques-collab/M5NanoC6_wifi/pulls)
- [M5Stack Documentation](https://docs.m5stack.com/en/core/nanoC6)
- [ESP32 Technical Reference](https://www.espressif.com/en/products/socs/esp32)

---

<div align="center">

**Last Updated:** October 5, 2026

For the latest updates and discussions, visit the [GitHub repository](https://github.com/paulocfmarques-collab/M5NanoC6_wifi).

</div>
