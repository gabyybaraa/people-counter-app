# TX_door2 - Door 2 Dual IR Transmitter Controller

## Overview
WiFi-enabled dual IR transmitter controller for Door 2 that provides continuous IR beams and connects to Master Hub for monitoring and coordination.

## Features
- **Dual IR Transmitters**: Door 2 Entry (D1) and Exit (D2) IR LEDs
- **WiFi Connectivity**: Connects to Master Hub WiFi network
- **Status Monitoring**: Sends transmitter status to Master Hub
- **Continuous Operation**: IR LEDs stay ON for reliable detection
- **Heartbeat Updates**: Regular status reports every 30 seconds
- **Automatic Reconnection**: Reconnects to WiFi if connection lost

## Hardware Requirements
- **NodeMCU ESP8266** (or compatible ESP8266 board)
- **2x IR333C LEDs** (IR transmitters)
- **2x 200Ω Resistors** (for IR LEDs)
- **Breadboard and jumper wires**

## Pin Configuration
```
Door 2 Entry TX (D1/GPIO5): IR333C LED + 200Ω resistor
Door 2 Exit TX (D2/GPIO4):  IR333C LED + 200Ω resistor
```

## Wiring Diagram
```
3.3V → 200Ω → IR333C LED → D1 (Entry TX)
3.3V → 200Ω → IR333C LED → D2 (Exit TX)
GND → IR333C LED (both LEDs)
```

## Network Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **API Endpoint**: /api/update

## IR Transmission
- **Entry LED (D1)**: Continuously ON for Door 2 Entry RX sensor
- **Exit LED (D2)**: Continuously ON for Door 2 Exit RX sensor
- **Purpose**: Create IR beams that RX sensors can detect
- **Pairing**: Works with Door 2 RX sensors (D6 and D5)

## WiFi Communication
- **Connection**: Connects to Master Hub WiFi network
- **Status Updates**: Sends transmitter status every 30 seconds
- **Data Format**: Includes LED states, uptime, and memory info
- **Reconnection**: Automatically reconnects if WiFi lost

## API Communication
Sends POST requests to Master Hub with:
- `sensor`: door2_tx_status
- `action`: heartbeat
- `entry_tx`: ON/OFF status of Entry LED
- `exit_tx`: ON/OFF status of Exit LED
- `device`: TX_dual_door2_controller
- `uptime`: Device uptime in seconds
- `free_heap`: Available memory

## Serial Output
- Device initialization and WiFi connection status
- IR transmitter setup confirmation
- WiFi connection attempts and results
- Status updates every 60 seconds
- WiFi signal strength and memory usage

## Testing
1. Power on the device
2. Verify WiFi connection to Master Hub
3. Check IR LEDs are continuously ON
4. Verify status updates in Master Hub
5. Test WiFi reconnection by disconnecting power
6. Monitor serial output for status information

## Troubleshooting
- **WiFi Issues**: Check SSID/password, ensure Master Hub is running
- **IR LEDs**: Verify wiring and 3.3V power supply
- **No Status Updates**: Check Master Hub IP and WiFi connection
- **Connection Loss**: Device automatically reconnects to Master Hub

## Dependencies
- ESP8266WiFi.h
- ESP8266HTTPClient.h
- WiFiClient.h 