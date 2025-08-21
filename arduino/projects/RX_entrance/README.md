# RX_entrance - Door 1 Entrance IR Receiver Controller

## Overview
Smart IR receiver controller for Door 1 entrance that detects people entering using IR sensors. Connects to Master Hub via WiFi for real-time people counting.

## Features
- **Entrance Detection**: Entry IR sensor for Door 1
- **Smart Logic**: Prevents false positives with debounce protection
- **WiFi Connectivity**: Connects to Master Hub at 192.168.4.1
- **Real-time Counting**: Sends entry events to Master Hub
- **Heartbeat Monitoring**: Continuous status updates to Master Hub
- **Debounce Protection**: 100ms debounce with 2-second cooldown

## Hardware Requirements
- **NodeMCU ESP8266** (or compatible ESP8266 board)
- **2x Phototransistors** (IR receivers)
- **2x 10kΩ Resistors** (for IR receivers)
- **Breadboard and jumper wires**

## Pin Configuration
```
Entry Sensor (A0): PT204-6B + 10kΩ resistor
```

## Wiring Diagram
```
3.3V → 10kΩ → PT204-6B → A0 (Entry)
GND → PT204-6B
```

## Network Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **API Endpoint**: /api/update

## Detection Logic
1. **Entry Detection**: D6 sensor triggers when IR beam is broken
2. **False Positive Prevention**: Debounce protection prevents multiple triggers
3. **Cooldown Period**: 2 seconds between detections
4. **Debounce**: 100ms verification after initial trigger

## Serial Output
- Device initialization and WiFi connection status
- Real-time detection events with timestamps
- Heartbeat confirmations to Master Hub
- Sensor status updates every 30 seconds

## API Communication
Sends POST requests to Master Hub with:
- `sensor`: door1_entry
- `action`: entry
- `ir_value`: sensor reading
- `beam_status`: clear
- `activity_time`: formatted timestamp
- `device`: RX_entrance

## Testing
1. Power on the device
2. Verify WiFi connection to Master Hub
3. Test entry sensor (A0) - should count as entry
4. Verify entry counts appear on Master Hub dashboard

## Troubleshooting
- **WiFi Issues**: Check SSID/password, ensure Master Hub is running
- **No Detection**: Verify IR sensor connections and 3.3V power
- **False Counts**: Check for IR interference or sensor misalignment
- **Connection Loss**: Device automatically reconnects to Master Hub

## Dependencies
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- WiFiClient.h 
