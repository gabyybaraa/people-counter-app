# RX_door1 - Door 1 IR Receiver Controller

## Overview
Robust IR receiver controller for Door 1 with enhanced connectivity and people counting logic.

## Features
- Enhanced connectivity with retry logic
- Improved error handling and recovery
- Better startup sequence with sensor testing
- Optimized timing for reliable operation
- Comprehensive logging and debugging

## Pin Configuration
- **Entry Sensor**: A0 (analog input)
- **Exit Sensor**: D3 (digital input)

## Hardware Requirements
- NodeMCU ESP8266
- IR receiver sensors (PT204-6B phototransistors)
- 200Ω resistors for voltage division

## Library Requirements
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- WiFiClient.h

## Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **Detection Cooldown**: 2000ms
- **Heartbeat Interval**: 20000ms

## Operation
1. Connects to Master Hub WiFi network
2. Monitors IR sensors for entry/exit events
3. Sends detection events to Master Hub
4. Provides heartbeat status updates
5. Automatic reconnection on WiFi loss

## Detection Logic
- Entry: A0 sensor triggers alone
- Exit: D3 sensor triggers alone
- Both sensors together are ignored (prevents false positives)
- Debounce and cooldown periods ensure accuracy

## Serial Output
- Startup information and sensor testing
- Real-time detection events
- WiFi connection status
- Periodic status updates

## Troubleshooting
- Check WiFi credentials and network availability
- Verify sensor wiring and connections
- Monitor serial output for error messages
- Ensure Master Hub is running and accessible 