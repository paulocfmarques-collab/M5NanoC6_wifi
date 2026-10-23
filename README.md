# M5NanoC6 Wi-Fi Firmware · Premium Technical README

![Platform](https://img.shields.io/badge/platform-ESP32--C6-00979D?style=for-the-badge&logo=espressif)
![Language](https://img.shields.io/badge/language-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus)
![Framework](https://img.shields.io/badge/framework-Arduino-00979D?style=for-the-badge&logo=arduino)
![Connectivity](https://img.shields.io/badge/connectivity-Wi--Fi%20%7C%20UDP%20%7C%20BLE-6f42c1?style=for-the-badge)
![Observability](https://img.shields.io/badge/observability-Prometheus-ff69b4?style=for-the-badge)
![Integration](https://img.shields.io/badge/integration-Home%20Assistant-41BDF5?style=for-the-badge)
![Updates](https://img.shields.io/badge/updates-OTA-orange?style=for-the-badge)

Modular firmware for **M5NanoC6 (ESP32-C6)** focused on operational reliability, observability, and smart-home/IoT integration.

- Multi-network Wi-Fi provisioning with RSSI-based priority
- Complete local web dashboard + API
- OTA updates via HTTP and ArduinoOTA
- Prometheus metrics + Home Assistant integration (MQTT Discovery)
- Serial and UDP command channels
- 20x4 I²C LCD, RGB LED, IR TX, NTP, and weather monitoring

---

## 📚 Table of Contents

1. [Overview](#-overview)
2. [Architecture](#-architecture)
3. [Boot and execution cycle](#-boot-and-execution-cycle)
4. [Complete repository inventory](#-complete-repository-inventory)
5. [Networking and connectivity](#-networking-and-connectivity)
6. [Dashboard and HTTP API](#-dashboard-and-http-api)
7. [Commands (Serial/UDP)](#-commands-serialudp)
8. [OTA](#-ota)
9. [Prometheus](#-prometheus)
10. [Home Assistant / MQTT](#-home-assistant--mqtt)
11. [Hardware, pins, and peripherals](#-hardware-pins-and-peripherals)
12. [Build and flashing](#-build-and-flashing)
13. [Partitioning](#-partitioning)
14. [Extra Mermaid flows](#-extra-mermaid-flows)
15. [Additional documentation in the repository](#-additional-documentation-in-the-repository)

---

## 🔭 Overview

- **Repository:** `paulocfmarques-collab/M5NanoC6_wifi`
- **URL:** https://github.com/paulocfmarques-collab/M5NanoC6_wifi
- **Default branch:** `main`
- **Primary language:** C++ (Arduino core for ESP32)
- **Entry point:** `M5NanoC6_wifi.ino`

### Main global services instantiated

- `HardwareController hardware;`
- `DeviceNetwork network;`
- `NTPService ntp;`
- `LCDService lcd;`
- `WeatherService clima;`
- `MetricsService metrics;`
- `HomeAssistantService homeassistant;`

---

## 🧠 Architecture

```mermaid
flowchart LR
    INO[M5NanoC6_wifi.ino]

    INO --> NET[DeviceNetwork]
    INO --> CMD[CommandHandler]
    INO --> HW[HardwareController]
    INO --> NTP[NTPService]
    INO --> LCD[LCDService]
    INO --> MET[MetricsService]
    INO --> HA[HomeAssistantService]
    INO --> WEA[WeatherService]

    NET --> OTA[OTAService]
    NET --> WEB[WebDashboard]
    NET --> MPAGE[MetricsPage]
    NET --> HAPAGE[HomeAssistantPage]
    NET --> UDP[WiFiUDP :4210]
    NET --> HTTP80[WebServer :80]
    NET --> HTTPM[Metrics Server :configurable port]

    HA --> MQTT[(MQTT Broker)]
    MET --> PROM[/metrics Prometheus format/]
```

### Functional layers

```mermaid
flowchart TD
    A[Presentation] --> B[Application Logic]
    B --> C[Platform Services]
    C --> D[Hardware/RTOS]

    A:::a --> A1[Web Dashboard]
    A:::a --> A2[HTTP API]

    B:::b --> B1[CommandHandler]
    B:::b --> B2[HomeAssistantService]
    B:::b --> B3[MetricsService]

    C:::c --> C1[WiFi / UDP / OTA / NTP]
    C:::c --> C2[Weather / Preferences NVS]

    D:::d --> D1[GPIO / NeoPixel / IR]
    D:::d --> D2[LCD I2C]
    D:::d --> D3[ESP32-C6]

    classDef a fill:#1f2937,stroke:#60a5fa,color:#fff;
    classDef b fill:#0f3d2e,stroke:#34d399,color:#fff;
    classDef c fill:#3f2a00,stroke:#f59e0b,color:#fff;
    classDef d fill:#3a1a3a,stroke:#f472b6,color:#fff;
```

---

## 🚀 Boot and execution cycle

### Initialization sequence

```mermaid
sequenceDiagram
    participant FW as Firmware
    participant HW as HardwareController
    participant MET as MetricsService
    participant HA as HomeAssistantService
    participant LCD as LCDService
    participant NET as DeviceNetwork
    participant WEA as WeatherService

    FW->>HW: begin()
    FW->>MET: begin()
    FW->>HA: begin(callback CommandHandler)
    FW->>LCD: begin()
    FW->>NET: conectar()
    NET-->>FW: Wi-Fi STA or AP portal
    FW->>WEA: begin()
```

### Main loop

```mermaid
flowchart TD
    L0[loop()] --> L1[network.processarWebServer]
    L1 --> L2[processarSerial]
    L2 --> L3[processarBotao]
    L3 --> L4[metrics.update]
    L4 --> L5[hardware.atualizarEfeitos]
    L5 --> L6[network.monitorarConexao]
    L6 --> L7[ntp.atualizar]
    L7 --> L8[lcd.atualizar]
    L8 --> L9[homeassistant.update]
    L9 --> L10{UDP message received?}
    L10 -- Yes --> L11[CommandHandler::executar]
    L10 -- No --> L12[delay(1)]
    L11 --> L12
    L12 --> L0
```

---

## 🗂️ Complete repository inventory

### Code and configuration (root)

| File | Main purpose |
|---|---|
| `M5NanoC6_wifi.ino` | Orchestrates setup/loop and service integration |
| `Config.h` | Pin constants, network/weather defaults, helpers, and config HTML |
| `DeviceNetwork.h` | Wi-Fi/AP portal, web server, routes, UDP, RSSI scan/reconnect |
| `CommandHandler.h` | Serial/UDP command parser and dispatcher |
| `HardwareController.h` | GPIO/LED/IR control and effects |
| `LCDService.h` | I²C 20x4 LCD lifecycle and rendering |
| `LCDButton.h` | Button behavior (wake/next/off) |
| `NTPService.h` | Time synchronization and NTP status |
| `WeatherService.h` | Open-Meteo weather integration |
| `OTAService.h` | HTTP OTA + ArduinoOTA |
| `OTAUploadPage.h` | OTA upload form UI |
| `WebDashboard.h` | Main dashboard HTML/JS |
| `MetricsService.h` | Metric collection and exposition |
| `MetricsPage.h` | Prometheus configuration/status page |
| `HomeAssistantService.h` | MQTT Discovery/Home Assistant integration |
| `HomeAssistantPage.h` | Home Assistant configuration UI |
| `BluetoothService.h` | BLE UART-like service with queued sessions |
| `partitions.csv` | ESP32-C6 partition table |

### Additional documentation

| File | Topic |
|---|---|
| `CHANGELOG.md` / `CHANGELOG.pdf` | Change history |
| `COMANDOS.md` / `COMANDOS.pdf` | Command reference and usage |
| `OTA.md` / `OTA.pdf` | OTA workflow and instructions |
| `PROMETHEUS.md` / `PROMETHEUS.pdf` | Metrics exposition and setup |
| `HOME_ASSISTANT.md` / `HOME_ASSISTANT.pdf` | MQTT/Home Assistant integration |

---

## 🌐 Networking and connectivity

### Wi-Fi strategy

- Stores up to **5 networks** in Preferences (`m5wifi`)
- Scans nearby APs and attempts connection by **strongest RSSI first**
- Saves index of the last successful network
- Falls back to **AP mode** (`M5NanoC6_CONFIG`) when no valid network is reachable

```mermaid
flowchart TD
    A[Start connection] --> B[Load stored SSIDs]
    B --> C{Any stored network?}
    C -- No --> AP[Start AP portal]
    C -- Yes --> D[scanNetworks]
    D --> E[Filter visible stored SSIDs]
    E --> F{Any match found?}
    F -- No --> AP
    F -- Yes --> G[Sort by RSSI desc]
    G --> H[Try auth in order]
    H --> I{Connected?}
    I -- Yes --> J[Start Web + Metrics + UDP + NTP]
    I -- No --> K{More networks?}
    K -- Yes --> H
    K -- No --> AP
```

### Connection loss recovery

- Every ~10s, checks `WiFi.status()`
- On disconnect:
  - stops UDP listener
  - updates UDP listener metric status
  - signals visually
  - restarts circular reconnection scan

---

## 🖥️ Dashboard and HTTP API

### Main routes identified

#### UI
- `GET /` (config portal in AP mode, dashboard in STA mode)
- `GET /info`
- `GET /config`
- `GET /prometheus`
- `GET /homeassistant`
- `GET /ota`

#### Status/configuration
- `GET /api/status`
- `GET /api/prometheus/config`
- `POST /api/prometheus/config`
- `GET /api/homeassistant/config`
- `POST /api/homeassistant/config`
- `GET /prometheus.yml`

#### Actions
- `POST /salvar` (save networks and reboot)
- `POST /ntp_sync`
- `POST /scan_wifi`
- `POST /set_fuso`
- `GET /reboot`
- `GET /led_on`
- `GET /led_off`
- `GET /ir_tx`
- `POST /lcd_on`
- `POST /lcd_off`

#### Metrics
- `GET /metrics` (port 80 or dedicated configured port)

### Web navigation map

```mermaid
flowchart LR
    U[Browser User] --> ROOT[/]
    ROOT --> INFO[/info]
    ROOT --> CFG[/config]
    INFO --> PROMUI[/prometheus]
    INFO --> HAUI[/homeassistant]
    INFO --> OTA[/ota]

    INFO --> APISTATUS[/api/status]
    PROMUI --> APIPROMGET[/api/prometheus/config GET]
    PROMUI --> APIPROMPOST[/api/prometheus/config POST]
    HAUI --> APIHAGET[/api/homeassistant/config GET]
    HAUI --> APIHAPOST[/api/homeassistant/config POST]
```

---

## ⌨️ Commands (Serial/UDP)

- Serial input: 115200 baud
- UDP input: port **4210**
- Commands are normalized to lowercase

### Supported commands (CommandHandler)

`help`, `info`, `status`, `version`, `build`, `reason`, `reboot`, `reset_counters`, `reset_wifi`, `alive`, `name`, `ha_user`, `ha_password`, `ha_status`, `ha_discovery`, `temp`, `cpu`, `ram`, `heap`, `flash`, `uptime`, `net_info`, `mac`, `rssi`, `ip`, `time`, `date`, `set_fuso:N`, `dst_on`, `dst_off`, `ntp_sync`, `ntp_status`, `led_on`, `led_off`, `led_blink[:ms]`, `set_rgb:R,G,B`, `set_breath:R,G,B,MS`, `lcd_on`, `lcd_off`, `ir_tx`.

### Command flow

```mermaid
flowchart TD
    A[Serial or UDP] --> B[trim + lowercase]
    B --> C{Known command?}
    C -- No --> D[Comando nao reconhecido]
    C -- Yes --> E[Execute action]
    E --> F{Serial channel?}
    F -- Yes --> G[Serial.print]
    F -- No --> H[network.responderUDP]
```

---

## 🔁 OTA

### OTA capabilities

- HTTP upload endpoint at `/ota`
- ArduinoOTA on port **3232**, hostname `M5NanoC6`
- No OTA password in current implementation
- Validates `.ino.bin` payload (rejects bootloader/partitions/merged)
- Checks OTA partition capacity before accepting upload

### OTA state machine

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> ValidatingFile: Upload start
    ValidatingFile --> Writing: bin accepted + OTA slot available
    ValidatingFile --> Failed: invalid file / no OTA partition
    Writing --> Completed: end + Update.end OK
    Writing --> Failed: write/end error
    Completed --> RebootPending
    RebootPending --> Reboot
    Failed --> Idle
    Reboot --> [*]
```

---

## 📈 Prometheus

Based on implementation + `PROMETHEUS.md`:

- `/metrics` in Prometheus 0.0.4 exposition format
- Metrics port configurable and independent from web UI (port 80)
- Configuration persisted in `telemetry` namespace
- `prometheus.yml` generated through dedicated endpoint
- Covers network, system, heap, uptime, HTTP, UDP, NTP, weather, MQTT

### Pull collection flow

```mermaid
sequenceDiagram
    participant P as Prometheus Server
    participant E as ESP32 (MetricsService)

    P->>E: GET /metrics
    E-->>P: text/plain; version=0.0.4
    P->>P: Store time series
    P->>P: Rules/alerts/dashboards
```

---

## 🏠 Home Assistant / MQTT

Based on `HOME_ASSISTANT.md`:

- Integration via **MQTT Discovery** (prefix `homeassistant`)
- Configured through `/homeassistant`
- Persistence in NVS namespace `m5ha`
- Discovery + availability + state topics
- Dedicated MQTT commands for LCD/LED/NTP/IR

### HA topology

```mermaid
flowchart LR
    ESP[M5NanoC6] -->|MQTT publish state/discovery| BROKER[(MQTT Broker)]
    HA[Home Assistant MQTT Integration] -->|subscribe| BROKER
    HA --> ENT[Auto-discovered entities]
    ENT --> UI[Home Assistant UI]
    UI -->|commands| BROKER
    BROKER -->|mqtt set topics| ESP
```

---

## 🔌 Hardware, pins, and peripherals

### Main pin mapping (`Config.h`)

- `GPIO3` → IR TX
- `GPIO7` → Blue status LED
- `GPIO9` → User button
- `GPIO19` → NeoPixel bus enable
- `GPIO20` → RGB LED data
- `GPIO2` → LCD SDA
- `GPIO1` → LCD SCL

### Other defaults

- UDP port: `4210`
- Default timezone: `-3` (Brasília)
- Default weather target: Porto Alegre (`lat -30.03283`, `lon -51.23019`)

### Peripheral map

```mermaid
flowchart TD
    MCU[ESP32-C6] --> IR[GPIO3 IR TX]
    MCU --> LEDB[GPIO7 Blue LED]
    MCU --> BTN[GPIO9 Button]
    MCU --> RGBEN[GPIO19 RGB Enable]
    MCU --> RGBDAT[GPIO20 RGB Data]
    MCU --> I2CSDA[GPIO2 SDA]
    MCU --> I2CSCL[GPIO1 SCL]
    I2CSDA --> LCD[LCD 20x4 + PCF8574]
    I2CSCL --> LCD
```

---

## 🛠️ Build and flashing

### Prerequisites

- Arduino IDE or PlatformIO with ESP32-C6 support
- Libraries used by the project:
  - `ArduinoJson`
  - `PubSubClient` (HA/MQTT integration)
  - ESP32 core dependencies (`WiFi`, `WebServer`, `Update`, `ArduinoOTA`, etc.)

### Suggested steps

1. Open `M5NanoC6_wifi.ino`
2. Select ESP32-C6 target board
3. Compile
4. Flash via USB (especially first deployment with OTA-capable partition layout)
5. Access dashboard using assigned IP

---

## 💾 Partitioning

- Repository includes `partitions.csv`
- OTA requires a valid second app slot
- If OTA partition is unavailable, firmware instructs USB flashing first

```mermaid
flowchart LR
    F[Flash] --> B[Bootloader]
    F --> T[Partition Table]
    F --> A0[App Slot A (running)]
    F --> A1[App Slot B (next OTA)]
    F --> NVS[NVS / Preferences]
    F --> O[Other partitions]
```

---

## 🔬 Extra Mermaid flows

### Metrics port change flow

```mermaid
sequenceDiagram
    participant UI as Prometheus Page
    participant API as /api/prometheus/config
    participant NET as DeviceNetwork
    participant MS as MetricsServer

    UI->>API: POST host, port, interval
    API->>NET: metrics.save(...)
    NET->>NET: metricsPortPending = true
    NET->>NET: wait ~1s in loop
    NET->>MS: stop/begin(newPort)
    NET-->>UI: GET config with active_port
```

### Connectivity state machine

```mermaid
stateDiagram-v2
    [*] --> Boot
    Boot --> WifiScan
    WifiScan --> StationConnected: valid credentials found
    WifiScan --> APConfigMode: no network/no success
    StationConnected --> Recovering: link lost
    Recovering --> WifiScan
    APConfigMode --> Rebooting: new config saved
    Rebooting --> Boot
```

---

## 📎 Additional documentation in the repository

For deeper operational details, see:

- [`COMANDOS.md`](./COMANDOS.md)
- [`OTA.md`](./OTA.md)
- [`PROMETHEUS.md`](./PROMETHEUS.md)
- [`HOME_ASSISTANT.md`](./HOME_ASSISTANT.md)
- [`CHANGELOG.md`](./CHANGELOG.md)
