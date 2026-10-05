# 📝 Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### 🎨 Added
- **Enhanced README documentation** with real file contents and command details
- **Automatic build-based versioning system** documentation (AAMMDD.HHMM format)
- **Complete command reference** mapping all 30+ UDP commands with examples
- **ASCII flowchart** for boot sequence and state transitions
- **Mermaid sequence diagram** for UDP communication flow
- **LED RGB state indicators** documentation with color codes and meanings
- **Performance benchmarks** table with measured metrics
- **Comprehensive troubleshooting** section with solutions for WiFi, AP, UDP, NTP and LED
- **Security guidelines** section with production recommendations
- **Hardware pinout table** with GPIO validation status
- **NTP server list** and timezone configuration examples
- **Web portal mockup** details for configuration and info screens
- **Version verification examples** via UDP commands (version, build, info)
- **Implementation details** of `obterVersaoAutomatica()` function from CommandHandler.h

### 🔄 Changed
- Reorganized README structure for better navigation with jump links
- Enhanced module documentation with actual function names
- Improved command table accuracy against `CommandHandler.h` implementation (30+ commands)
- Updated performance metrics with realistic boot times and latencies
- Reworked troubleshooting for quick diagnostics and solutions
- Restructured security section with practical recommendations
- Converted UDP communication diagram from ASCII to Mermaid sequence chart
- Added detailed versioning system explanation with examples

### ✅ Fixed
- README alignment with actual repository structure and file organization
- Command table accuracy for all UDP commands (diagnostics, config, hardware)
- Documentation consistency between architecture and implementation
- Portal web documentation with actual HTML template details
- Hardware pinout validation status indicators
- Versioning documentation now matches actual implementation

---

## [1.0.1] - 2026-10-05

### 🎨 Added
- **Professional README** with shields.io badges and brand consistency
- **Quick start guide** with 3-step setup procedure
- **Architecture diagram** showing module organization
- **Mermaid flowchart** for boot sequence and state transitions
- **Command reference** split into diagnostics, configuration and hardware control
- **Automatic versioning system** based on compilation date/time
- **Performance benchmarks** table
- **Security advisory** section
- **Resources links** for ESP32, M5Stack and tools

### 🔧 Changed
- Visual presentation optimized for GitHub community standards
- README now includes interactive navigation links
- Command examples with real usage patterns
- Hardware pinning table with validated GPIO assignments
- Version system uses `__DATE__` and `__TIME__` macros for automatic versioning

### 🐛 Fixed
- README structure to match actual firmware capabilities
- Corrected command examples to match implementation
- Removed placeholder and outdated sections
- Version format standardized to AAMMDD.HHMM

---

## [1.0.0] - 2026-10-05

### 🚀 Initial Release

#### ✨ Core Features
- **Wi‑Fi Provisioning:** Automatic STA connection with fallback to captive AP portal
- **UDP Command Server:** Port 4210 for remote device control with 30+ commands
- **NTP Synchronization:** Local time with timezone (GMT-3 default) and DST support
- **Hardware Control:** RGB LED (NeoPixel), status LED, user button, IR emitter
- **Device Diagnostics:** CPU, RAM, flash, PSRAM, temperature, uptime, network metrics
- **OTA Updates:** Over-The-Air firmware updates via ArduinoOTA
- **Automatic Versioning:** Build-based version system (AAMMDD.HHMM format)
- **Modular Architecture:** Separate header files for each component

#### 📦 Components Included
- `M5NanoC6_wifi.ino` - Main firmware entry point (60 lines)
- `Config.h` - Hardware GPIO configuration and portal HTML templates
- `HardwareController.h` - LED RGB effects and GPIO management
- `DeviceNetwork.h` - Wi‑Fi STA/AP, UDP server, OTA, web server
- `NTPService.h` - NTP time synchronization with timezone/DST
- `CommandHandler.h` - UDP command parsing and execution with automatic versioning (300+ lines)

#### 🎯 Capabilities
- **AP Mode:** Captive portal on `http://192.168.4.1` with WiFi configuration
- **STA Mode:** Persistent Wi‑Fi credential storage in `Preferences`
- **Reset Function:** Factory reset via button press (>3 seconds)
- **Visual Feedback:** RGB LED state indication with colors (green, blue, red, yellow, magenta, white)
- **Timezone Support:** GMT-3 default (Brasília) with dynamic configuration
- **Diagnostic Commands:** 30+ queries for comprehensive system status
- **Breath Effect:** Customizable LED pulsing animation with configurable duration
- **Automatic Build Versioning:** Version format `AAMMDD.HHMM` extracted from compilation timestamp

#### 🎮 UDP Commands (30+)

**Diagnostics:** help, info, status, cpu, ram, flash, psram, temp, mac, net_info, time, date, uptime, reason, version, build, alive, ota_info

**Configuration:** reset_wifi, set_fuso, fuso_status, dst_on, dst_off, dst_status

**Hardware:** led_on, led_off, set_rgb, set_breath, ir_tx

#### 📦 Versioning System

**Format:** `AAMMDD.HHMM` (Year-Month-Day.Hour-Minute)

**Examples:**
- `261005.2326` → Compiled on October 5, 2026 at 23:26
- `261010.1430` → Compiled on October 10, 2026 at 14:30

**Implementation:**
- Function: `obterVersaoAutomatica()` in `CommandHandler.h` (lines 30-58)
- Uses: `__DATE__` and `__TIME__` compiler macros
- Automatic: No manual configuration required
- Unique: Each build generates a different version
- Zero overhead: Compiled at compile-time

**Verification Commands:**
- `version` → Returns formatted version (AAMMDD.HHMM)
- `build` → Returns raw build date/time
- `info` → Returns complete device information including version

#### 🔒 Security Considerations
- UDP communication (unencrypted) - local networks only
- Web portal without authentication by default
- Recommended for private LAN deployment
- Firewall/VPN protection recommended for external exposure
- Production recommendations: HTTP Basic auth, HTTPS, command validation, rate-limiting

#### ⚡ Performance
- **Boot time:** < 3 seconds (with saved Wi‑Fi)
- **UDP response latency:** < 50ms
- **Memory footprint:** ~60KB RAM (heap free)
- **Flash usage:** ~350KB (~1% of total)
- **Wi‑Fi reconnection:** < 8 seconds
- **Idle power consumption:** ~50mA @ 3.3V
- **NTP sync time:** < 2s (first time)
- **Version retrieval:** < 10ms (from cache)

#### 🎨 Hardware Features
- **LED RGB (NeoPixel):** GPIO 20 (data), GPIO 19 (enable)
- **Status LED (Blue):** GPIO 7
- **User Button:** GPIO 9 (with debounce)
- **IR Emitter:** GPIO 3 (38kHz modulation)
- **Serial:** 115200 baud for debugging

#### 📡 Network Stack
- **WiFi:** 2.4GHz 802.11 b/g/n
- **UDP:** Port 4210, text-based protocol
- **HTTP:** Web server for configuration and info panel
- **NTP:** Multiple server fallback (a.st1.ntp.br, pool.ntp.org, time.nist.gov)
- **OTA:** Arduino OTA protocol

#### 📱 Platform Support
- **Board:** M5Stack M5NanoC6
- **MCU:** ESP32-C6 (RISC-V architecture)
- **Build Tools:** Arduino IDE 1.8.x+, PlatformIO
- **Languages:** C++17 with Arduino framework
- **Dependencies:** WiFi, WebServer, Preferences, Adafruit NeoPixel

#### 🔌 GPIO Pin Assignment
| Signal | GPIO | Function |
| :--- | :---: | :--- |
| LED RGB Data | 20 | NeoPixel data line |
| LED RGB Enable | 19 | LED power control |
| Status LED | 7 | Blue indicator |
| User Button | 9 | Reset trigger |
| IR TX | 3 | Infra-red transmitter |

---

## 🔗 Related Resources

- [GitHub Issues](https://github.com/paulocfmarques-collab/M5NanoC6_wifi/issues)
- [Pull Requests](https://github.com/paulocfmarques-collab/M5NanoC6_wifi/pulls)
- [M5Stack Documentation](https://docs.m5stack.com/en/core/nanoC6)
- [ESP32 Technical Reference](https://www.espressif.com/en/products/socs/esp32)
- [NeoPixel Guide](https://learn.adafruit.com/adafruit-neopixel-uberguide)

---

<div align="center">

**Last Updated:** October 5, 2026

For the latest updates and discussions, visit the [GitHub repository](https://github.com/paulocfmarques-collab/M5NanoC6_wifi).

</div>
