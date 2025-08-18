# TX_door2 - Door 2 IR Transmitter Controller

## Overview
Dual IR transmitter controller for Door 2 that provides continuous IR beams for people detection.

## Features
- Dual IR LED control for entry and exit sensors
- WiFi connectivity to Master Hub for monitoring
- Real-time status reporting
- Automatic reconnection on WiFi loss

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
- ESP8266HTTPClient.h
- WiFiClient.h

## Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **Status Update Interval**: 30 seconds
- **WiFi Check Interval**: 10 seconds

## Operation
1. Connects to Master Hub WiFi network
2. Continuously powers both IR LEDs
3. Sends status updates to Master Hub
4. Monitors WiFi connection health
5. Provides real-time LED status

## IR Transmission
- **Entry LED**: Continuously ON at D1
- **Exit LED**: Continuously ON at D2
- **Purpose**: Create IR beams for RX sensors to detect
- **Pairing**: Works with Door 2 RX sensors (A0 and D3)

## WiFi Communication
- **Status Updates**: Every 30 seconds
- **WiFi Monitoring**: Every 10 seconds
- **Reconnection**: Automatic on connection loss
- **Data Sent**: LED states, uptime, free memory

## Serial Output
- Startup information and configuration
- WiFi connection status
- Periodic status updates
- LED state monitoring

## Troubleshooting
- Verify IR LED connections and resistors
- Check WiFi credentials and network
- Monitor serial output for errors
- Ensure proper power supply voltage 