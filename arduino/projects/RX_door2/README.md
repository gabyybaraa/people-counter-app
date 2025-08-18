# RX_door2 - Door 2 IR Receiver Controller

## Overview
Smart IR receiver controller for Door 2 with proven connectivity and directional people counting logic.

## Features
- Proven WiFi connectivity with retry logic
- Basic directional detection logic
- False positive prevention
- Simple cooldown system
- Automatic reconnection capabilities

## Pin Configuration
- **Entry Sensor**: D6 (digital input)
- **Exit Sensor**: D5 (digital input)

## Hardware Requirements
- NodeMCU ESP8266
- IR receiver sensors (PT204-6B phototransistors)
- 200Ω resistors for voltage division
- Power supply (3.3V)

## Library Requirements
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- WiFiClient.h

## Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **Detection Cooldown**: 2000ms
- **Heartbeat Interval**: 30000ms

## Operation
1. Connects to Master Hub WiFi network
2. Monitors IR sensors for entry/exit events
3. Sends detection events to Master Hub
4. Provides heartbeat status updates
5. Automatic reconnection on WiFi loss

## Detection Logic
- Entry: D6 sensor triggers alone
- Exit: D5 sensor triggers alone
- Both sensors together are ignored (prevents false positives)
- Debounce and cooldown periods ensure accuracy

## WiFi Communication
- **Heartbeat**: Every 30 seconds
- **WiFi Monitoring**: Every 10 seconds
- **Reconnection**: Automatic on connection loss
- **Data Sent**: Sensor readings, beam status, activity time

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