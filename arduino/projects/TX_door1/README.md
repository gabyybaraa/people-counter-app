# TX_door1 - Dual TX Master Hub

## Overview
Advanced master hub that controls dual IR transmitters for Door 1 and provides comprehensive WiFi access point, web dashboard, and API endpoints for the entire people counting system.

## Features
- **Dual IR Transmitters**: Door 1 Entry (D1) and Exit (D2) IR LEDs
- **WiFi Access Point**: Creates stable network with multiple fallback options
- **Professional Web Dashboard**: Real-time counter display with dark theme
- **Comprehensive API**: Multiple endpoints for Flutter app integration
- **EEPROM Persistence**: Counters and timing data survive power cycles
- **Advanced WiFi Management**: 4 fallback networks with stability testing
- **Sensor Monitoring**: Real-time status tracking for all door sensors

## Hardware Requirements
- **NodeMCU ESP8266** (or compatible ESP8266 board)
- **2x IR333C LEDs** (IR transmitters)
- **2x 200Ω Resistors** (for IR LEDs)
- **Breadboard and jumper wires**

## Pin Configuration
```
Door 1 Entry TX (D1/GPIO5): IR333C LED + 200Ω resistor
Door 1 Exit TX (D2/GPIO4):  IR333C LED + 200Ω resistor
```

## Wiring Diagram
```
3.3V → 200Ω → IR333C LED → D1 (Entry TX)
3.3V → 200Ω → IR333C LED → D2 (Exit TX)
GND → IR333C LED (both LEDs)
```

## WiFi Networks (Fallback Order)
1. **ESP-Counter** (password: 12345678)
2. **PeopleCounter** (password: 12345678)
3. **MASTER-COUNTER** (password: 12345678)
4. **ESP-Master** (password: 12345678)

## Web Interface
- **Dashboard**: `http://[IP]/` - Professional counter display
- **API Test**: `http://[IP]/api/test` - Flutter app endpoint
- **Sensor Updates**: `http://[IP]/api/update` - RX sensor data
- **Reset Counters**: `http://[IP]/reset` - Clear all data
- **Status API**: `http://[IP]/api/status` - System information

## API Endpoints
- **`/api/test`**: Comprehensive data for Flutter app
- **`/api/update`**: Receives sensor data from RX devices
- **`/api/reset`**: Resets all counters via API
- **`/api/count`**: Simple counter endpoint
- **`/api/status`**: System status and information

## Data Persistence
- **Total People Inside**: EEPROM address 0
- **Door 1 Entries**: EEPROM address 4
- **Door 1 Exits**: EEPROM address 8
- **Door 2 Entries**: EEPROM address 12
- **Door 2 Exits**: EEPROM address 16
- **Timing Data**: EEPROM addresses 20-99 (20 bytes each)

## Sensor Integration
- **RX_door1**: Connects via WiFi, sends door1_entry/door1_exit events
- **RX_door2**: Connects via WiFi, sends door2_entry/door2_exit events
- **Heartbeat Monitoring**: 1-minute timeout for sensor connections
- **Real-time Updates**: Immediate counter updates on sensor events

## System Features
- **Automatic AP Recreation**: Recreates WiFi if it becomes unstable
- **Sensor Timeout Management**: Tracks sensor connection status
- **Auto-save**: Saves data every 5 minutes
- **WiFi Monitoring**: Checks connection every 10 seconds
- **Status Updates**: Comprehensive logging every 30 seconds

## Testing
1. Power on the device
2. Verify WiFi access point creation
3. Connect to the WiFi network
4. Access web dashboard at device IP
5. Test API endpoints
6. Verify IR LEDs are continuously ON
7. Test sensor data reception

## Troubleshooting
- **WiFi Issues**: Check fallback networks, restart device
- **Web Dashboard**: Verify IP address, check browser compatibility
- **API Problems**: Check endpoint URLs, verify data format
- **IR LEDs**: Verify wiring and 3.3V power supply
- **Sensor Issues**: Check WiFi credentials, verify Master Hub IP

## Dependencies
- ESPAsyncTCP.h
- ESPAsyncWebServer.h
- EEPROM.h 