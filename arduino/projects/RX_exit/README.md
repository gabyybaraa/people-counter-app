# RX_door2 - Door 2 IR Receiver Controller

## Overview
Smart IR receiver controller for Door 2 that detects people entering and exiting using bidirectional IR sensors. Connects to Master Hub via WiFi for real-time people counting.

## Features
- **Bidirectional Detection**: Entry (D6) and Exit (D5) IR sensors
- **Smart Logic**: Prevents false positives from simultaneous sensor triggers
- **WiFi Connectivity**: Connects to Master Hub at 192.168.4.1
- **Real-time Counting**: Sends entry/exit events to Master Hub
- **Heartbeat Monitoring**: Continuous status updates to Master Hub
- **Debounce Protection**: 100ms debounce with 2-second cooldown

## Hardware Requirements
- **NodeMCU ESP8266** (or compatible ESP8266 board)
- **2x PT204-6B Phototransistors** (IR receivers)
- **2x 10kΩ Resistors** (for IR receivers)
- **Breadboard and jumper wires**

## Pin Configuration
```
Entry Sensor (D6/GPIO12): PT204-6B + 10kΩ resistor
Exit Sensor (D5/GPIO14):  PT204-6B + 10kΩ resistor
```

## Wiring Diagram
```
3.3V → 10kΩ → PT204-6B → D6 (Entry)
3.3V → 10kΩ → PT204-6B → D5 (Exit)
GND → PT204-6B (both sensors)
```

## Network Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **API Endpoint**: /api/update

## Detection Logic
1. **Entry Detection**: Only D6 sensor triggers (D5 must be clear)
2. **Exit Detection**: Only D5 sensor triggers (D6 must be clear)
3. **False Positive Prevention**: Both sensors triggering simultaneously = ignored
4. **Cooldown Period**: 2 seconds between detections
5. **Debounce**: 100ms verification after initial trigger

## Serial Output
- Device initialization and WiFi connection status
- Real-time detection events with timestamps
- Heartbeat confirmations to Master Hub
- Sensor status updates every 30 seconds

## API Communication
Sends POST requests to Master Hub with:
- `sensor`: door2_entry or door2_exit
- `action`: entry or exit
- `ir_value`: sensor reading
- `beam_status`: clear
- `activity_time`: formatted timestamp
- `device`: RX_door2

## Testing
1. Power on the device
2. Verify WiFi connection to Master Hub
3. Test entry sensor (D6) - should count as entry
4. Test exit sensor (D5) - should count as exit
5. Test both sensors simultaneously - should be ignored
6. Verify counts appear on Master Hub dashboard

## Troubleshooting
- **WiFi Issues**: Check SSID/password, ensure Master Hub is running
- **No Detection**: Verify IR sensor connections and 3.3V power
- **False Counts**: Check for IR interference or sensor misalignment
- **Connection Loss**: Device automatically reconnects to Master Hub

## Dependencies
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- WiFiClient.h 