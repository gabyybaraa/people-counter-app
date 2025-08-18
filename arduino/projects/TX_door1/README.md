# TX_door1 - Dual TX Master Hub

## Overview
Central controller and IR transmitter for Door 1 that coordinates the entire people counting network.

## Features
- Dual IR LED control for Door 1 entry/exit
- Web dashboard for real-time monitoring
- WiFi access point creation
- API endpoint for sensor data
- People counting and tracking
- Automatic sensor status monitoring

## Pin Configuration
- **Entry IR LED**: D1 (GPIO5)
- **Exit IR LED**: D2 (GPIO4)

## Hardware Requirements
- NodeMCU ESP8266
- 2x IR LED modules (IR333C)
- 200Ω current limiting resistors
- Power supply (3.3V)

## Library Requirements
- ESP8266WiFi.h
- ESPAsyncTCP.h
- ESPAsyncWebServer.h
- EEPROM.h

## Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Access Point SSID**: ESP-Counter-AP
- **Access Point Password**: 12345678
- **Web Server Port**: 80

## Operation
1. Creates WiFi access point for sensor devices
2. Continuously powers Door 1 IR LEDs
3. Provides web dashboard for monitoring
4. Receives data from all door sensors
5. Tracks people counts and sensor status

## IR Transmission
- **Entry LED**: Continuously ON at D1
- **Exit LED**: Continuously ON at D2
- **Purpose**: Create IR beams for Door 1 RX sensors
- **Pairing**: Works with Door 1 RX sensors (A0 and D3)

## Web Dashboard
- **Real-time counts**: Entry/exit for both doors
- **Sensor status**: Online/offline monitoring
- **People tracking**: Current count inside
- **System info**: WiFi status, uptime, memory
- **Auto-refresh**: Updates every 5 seconds

## API Endpoints
- **POST /api/update**: Receive sensor data
- **GET /**: Main dashboard
- **GET /reset**: Reset all counters

## Network Architecture
- **Master Hub**: This device (WiFi AP + IR TX)
- **Door 1 RX**: Connects to WiFi, sends entry/exit data
- **Door 2 TX**: Connects to WiFi, sends status
- **Door 2 RX**: Connects to WiFi, sends entry/exit data

## Serial Output
- Startup information and configuration
- WiFi access point creation
- Web server status
- Periodic system updates
- Sensor connection monitoring

## Troubleshooting
- Verify IR LED connections and resistors
- Check WiFi credentials and network
- Monitor serial output for errors
- Ensure proper power supply voltage
- Access web dashboard at device IP 