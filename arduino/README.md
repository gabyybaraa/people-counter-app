# Arduino Projects - People Counter System

## Overview
This directory contains all Arduino projects for the people counting system, including IR transmitters, receivers, and the master hub controller.

## Project Structure

### Door 1 System
- **TX_entrance/dual_tx_master_hub.ino** - Master Hub (Entrance TX Door 1) with WiFi AP
- **RX_entrance/RX_entrance.ino** - Door 1 Entrance IR receiver
- **TX_exit/TX_exit.ino** - Door 1 Exit IR transmitter
- **RX_exit/RX_exit.ino** - Door 1 Exit IR receiver

### Door 2 System  
- **External TX/RX system** - Independent Door 2 entry/exit detection

## Hardware Requirements

### Common Components
- NodeMCU ESP8266 development boards
- IR LED modules (IR333C)
- IR receiver sensors (PT204-6B phototransistors)
- 200Ω current limiting resistors (for IR LEDs)
- 10kΩ voltage division resistors (for IR receivers)
- Power supply (3.3V)

### Pin Assignments
- **D1 (GPIO5)**: IR LED transmitters
- **D2 (GPIO4)**: IR LED transmitters  
- **D3 (GPIO0)**: Digital IR receivers
- **D5 (GPIO14)**: Digital IR receivers
- **D6 (GPIO12)**: Digital IR receivers
- **A0**: Analog IR receiver

## Network Configuration
- **WiFi SSID**: ESP-Counter
- **WiFi Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **Access Point**: ESP-Counter-AP (12345678)

## System Architecture

### Master Hub (TX_entrance - Door 1 Entrance)
- Creates WiFi access point
- Controls Door 1 Entrance IR transmitter (D1)
- Provides web dashboard
- Receives data from all sensors
- Tracks people counts

### Door 1 Entrance RX (RX_entrance)
- Connects to Master Hub WiFi
- Monitors entrance sensors
- Sends entry detection events to Master Hub
- Enhanced connectivity with retry logic

### Door 1 Exit TX (TX_exit)
- Connects to Master Hub WiFi
- Controls Door 1 Exit IR transmitter (D2)
- Sends status updates to Master Hub
- Monitors WiFi connection health

### Door 1 Exit RX (RX_exit)
- Connects to Master Hub WiFi
- Monitors exit sensors
- Sends exit detection events to Master Hub
- Simple, proven detection logic

### Door 2 System
- Independent TX/RX system
- Connects to Master Hub WiFi
- Sends entry/exit data to Master Hub
- Separate from Door 1 system

## Installation

### Required Libraries
```cpp
#include <ESP8266WiFi.h>        // WiFi functionality
#include <ESP8266HTTPClient.h>   // HTTP client for API calls
#include <WiFiClient.h>          // WiFi client support
#include <ESPAsyncTCP.h>         // Async TCP (Master Hub only)
#include <ESPAsyncWebServer.h>   // Async web server (Master Hub only)
#include <EEPROM.h>              // Persistent storage (Master Hub only)
```

### Setup Steps
1. Install required libraries in Arduino IDE
2. Configure WiFi credentials if needed
3. Upload appropriate sketch to each device
4. Power devices and monitor serial output
5. Access web dashboard at Master Hub IP

## Operation

### Startup Sequence
1. **Master Hub**: Creates WiFi AP and starts web server
2. **TX Devices**: Connect to WiFi and activate IR LEDs
3. **RX Devices**: Connect to WiFi and begin sensor monitoring
4. **System Ready**: All devices connected and operational

### Detection Logic
- **Entry**: Single sensor triggers (prevents false positives)
- **Exit**: Single sensor triggers (prevents false positives)
- **Cooldown**: 2-second delay between detections
- **Debounce**: 100ms delay for sensor stability

### Data Flow
1. IR sensors detect people movement
2. RX devices send events to Master Hub
3. Master Hub updates counters and web dashboard
4. Real-time monitoring available via web interface

## Troubleshooting

### Common Issues
- **WiFi Connection**: Check credentials and network availability
- **Sensor Detection**: Verify wiring and IR LED positioning
- **Power Supply**: Ensure stable 3.3V for all devices
- **Serial Output**: Monitor for error messages and status

### Testing
- **IR LEDs**: Should be continuously ON
- **Sensors**: Test with hand movement near sensors
- **WiFi**: Check connection status in serial monitor
- **Web Dashboard**: Access at Master Hub IP address

## Customization

### Pin Changes
Modify pin assignments in each sketch:
```cpp
const int IR_RECEIVER_ENTRY = D6;  // Change to preferred pin
const int IR_RECEIVER_EXIT = D5;   // Change to preferred pin
```

### Timing Adjustments
Modify detection and communication timing:
```cpp
const unsigned long DETECTION_COOLDOWN = 2000;     // Detection cooldown
const unsigned long HEARTBEAT_INTERVAL = 30000;    // Heartbeat frequency
```

### WiFi Settings
Customize network configuration:
```cpp
const char* wifi_ssid = "YourNetworkName";
const char* wifi_password = "YourPassword";
```

## Version Information
- **Current Version**: 2.0
- **Features**: Enhanced connectivity, web dashboard, dual-door support
- **Compatibility**: ESP8266 NodeMCU boards
- **Last Updated**: Current development version

## Support
For issues or questions:
1. Check serial output for error messages
2. Verify hardware connections and power supply
3. Test individual components separately
4. Monitor WiFi connection status

## License
This project is open source. Use and modify as needed for your projects. 