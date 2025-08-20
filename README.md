# People Counter App - Smart IR Detection System

A complete people counting solution using ESP8266 microcontrollers, IR sensors, and Flutter mobile app for real-time monitoring.

## Features

- **Dual-Door System** - Monitor entry/exit for two separate doors
- **IR Beam Detection** - Reliable people counting using infrared sensors
- **Real-Time Dashboard** - Web interface for live monitoring
- **Mobile App** - Flutter application for remote access
- **WiFi Network** - Centralized coordination via Master Hub
- **Smart Logic** - Prevents false positives with directional detection

## System Architecture

```
                    WiFi Network (ESP-Counter)
                          192.168.4.1
                              │
                    ┌─────────┴─────────┐
                    │    Master Hub      │
                    │   (TX_door1)      │
                    │  WiFi AP + Web    │
                    └─────────┬─────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
    Door 1 RX             Door 2 TX            Door 2 RX
   (RX_door1)           (TX_door2)           (RX_door2)
   IR Receivers         IR Transmitters      IR Receivers
   A0 + D3              D1 + D2             D6 + D5
 (GPIO16/GPIO0)        (GPIO5/GPIO4)       (GPIO12/GPIO14)
```

## Quick Start

### Prerequisites
- 4x NodeMCU ESP8266 boards
- 4x IR LED modules (IR333C)
- 4x IR receiver sensors (PT204-6B)
- 4x 200Ω resistors (for IR LEDs)
- 4x 10kΩ resistors (for IR receivers)
- Power supplies (3.3V)
- Arduino IDE with ESP8266 support

### Setup Steps
1. **Upload Master Hub**: `arduino/projects/TX_door1/dual_tx_master_hub.ino`
2. **Upload Door 1 RX**: `arduino/projects/RX_door1/RX_door1.ino`
3. **Upload Door 2 TX**: `arduino/projects/TX_door2/door2_tx_controller.ino`
4. **Upload Door 2 RX**: `arduino/projects/RX_door2/door2_controller.ino`
5. **Wire components** following wiring diagrams
6. **Power up devices** and monitor serial output
7. **Access web dashboard** at Master Hub IP

## Hardware Requirements

- **4x NodeMCU ESP8266** - Main microcontrollers
- **4x IR LED modules** (IR333C) - Infrared transmitters
- **4x IR receiver sensors** (PT204-6B) - Infrared detectors
- **4x 200Ω resistors** - Current limiting for IR LEDs
- **4x 10kΩ resistors** - Voltage division for IR receivers
- **4x 3.3V power supplies** - Stable power for each device
- **Breadboards and jumper wires** - For connections

## Mobile App

### Flutter Application
- **Real-time monitoring** of people counts
- **Device status** indicators
- **Cross-platform** support (iOS/Android)

### Getting Started with Flutter
```bash
flutter pub get
flutter run
```

## Arduino Projects

### Project Structure
```
arduino/
├── README.md (main overview)
├── projects/
│   ├── RX_door1/ (Door 1 receiver)
│   ├── TX_door1/ (Master Hub + Door 1 transmitter)
│   ├── TX_door2/ (Door 2 transmitter)
│   └── RX_door2/ (Door 2 receiver)
```

### Each Project Includes
- **Arduino sketch** (.ino file)
- **README documentation** 
- **Library requirements**
- **Wiring diagrams**

## Network Configuration

- **WiFi SSID**: ESP-Counter
- **Password**: 12345678
- **Master Hub IP**: 192.168.4.1
- **API Endpoint**: `/api/update`
- **Heartbeat**: Every 20-30 seconds

## Testing

### Pre-Deployment Tests
1. **Upload code** to each ESP8266 and monitor serial output
2. **Power up TX devices** and verify IR LEDs are ON
3. **Connect all devices** to Master Hub WiFi
4. **Access web dashboard** at Master Hub IP

## Troubleshooting

### Common Issues
- **WiFi Connection Failed**: Check credentials and network availability
- **Sensors Not Detecting**: Verify wiring and IR LED alignment
- **False Triggers**: Adjust sensor positioning and check for interference

### Serial Monitor
- Look for: "Connected to Master", "WiFi: OK"
- Should see: "Entry heartbeat OK", "Exit heartbeat OK"

## Documentation

### Arduino Projects
- [Main Arduino Overview](arduino/README.md)
- [Door 1 Receiver](arduino/projects/RX_door1/README.md)
- [Master Hub](arduino/projects/TX_door1/README.md)
- [Door 2 Transmitter](arduino/projects/TX_door2/README.md)
- [Door 2 Receiver](arduino/projects/RX_door2/README.md)

## Use Cases

- **Retail stores** - Customer traffic analysis
- **Offices** - Building access tracking
- **Classrooms** - Attendance monitoring
- **Libraries** - Usage statistics

## License

This project is open source. Use and modify as needed for your projects.

---

**Built with ❤️ using Flutter, Arduino, and ESP8266**
