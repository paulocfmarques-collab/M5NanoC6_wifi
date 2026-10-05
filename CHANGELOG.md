# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- README aligned with the actual repository structure and current firmware modules.
- Command reference updated to match the real implementation in `CommandHandler.h`.
- Documentation for Wi‑Fi provisioning, UDP control, NTP setup, OTA and hardware pin mapping.
- Troubleshooting section aligned with the current runtime behavior of the project.

### Changed
- Reworked the project overview to reflect the real modular architecture of the M5NanoC6 firmware.
- Updated the command table to include only commands that are actually implemented in code.
- Corrected the repository documentation to remove stale references and old examples.

### Fixed
- References to non-existent files or outdated module structure.
- Documentation mismatches between the README and the actual firmware implementation.
- Inaccurate descriptions of the supported commands and runtime behavior.

## [1.0.0] - 2026-10-05

### Added
- Initial M5NanoC6 Wi‑Fi provisioning flow.
- Automatic connection to saved SSID/password using `Preferences`.
- Captive configuration portal in AP mode.
- UDP command server on port `4210`.
- GPIO and hardware control for RGB LED, status LED, button and IR pulse emitter.
- NTP-based local time and timezone management with DST support.
- Device diagnostics for CPU, RAM, flash, uptime, network and reset reason.
- OTA support using ArduinoOTA.
- Modular project layout split into dedicated header files.

### Features
- Access point mode for first-time network setup.
- Persistent Wi‑Fi storage and restart-based recovery.
- Remote command execution via UDP.
- Local diagnostics and telemetry.
- User reset support for clearing saved network configuration.
- Lightweight modular embedded architecture for M5NanoC6 devices.

---

For more details about the current project status and future improvements, see the repository issues and pull requests on GitHub.
