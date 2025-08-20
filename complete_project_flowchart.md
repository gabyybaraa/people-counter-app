# Complete People Counter Project - System Flowchart

## Project Overview
This is a **Smart IR Detection People Counting System** that uses:
- **4 ESP8266 devices** (2 transmitters + 2 receivers)
- **Flutter mobile app** for real-time monitoring
- **WiFi network** for centralized coordination
- **Bidirectional detection** to prevent false positives

## Complete System Architecture

```mermaid
graph TB
    subgraph "WiFi Network (ESP-Counter)"
        A[Master Hub WiFi AP<br/>192.168.4.1<br/>SSID: ESP-Counter]
    end
    
    subgraph "Hardware Layer"
        B[Master Hub<br/>TX_door1<br/>Dual IR Transmitters<br/>+ WiFi AP + Web Server]
        C[Door 1 Receiver<br/>RX_door1<br/>IR Sensors D6+D5]
        D[Door 2 Transmitter<br/>TX_door2<br/>IR Transmitters D1+D2]
        E[Door 2 Receiver<br/>RX_door2<br/>IR Sensors D6+D5]
    end
    
    subgraph "Mobile App"
        F[Flutter App<br/>Real-time Dashboard<br/>Device Monitoring]
    end
    
    A --> B
    A --> C
    A --> D
    A --> E
    A --> F
    
    B -.->|IR Beams| C
    D -.->|IR Beams| E
```

## Complete System Flow

```mermaid
flowchart TD
    subgraph "System Initialization"
        A[Power On All Devices] --> B[Master Hub Creates WiFi AP]
        B --> C[Other Devices Connect to WiFi]
        C --> D[All Devices Establish Communication]
    end
    
    subgraph "Continuous Operation"
        D --> E[Master Hub: Keep IR LEDs ON]
        D --> F[Door 1 RX: Monitor IR Sensors]
        D --> G[Door 2 TX: Keep IR LEDs ON]
        D --> H[Door 2 RX: Monitor IR Sensors]
        D --> I[Flutter App: Poll for Updates]
    end
    
    subgraph "People Detection Events"
        F --> J{Person Detected?}
        G --> K{Person Detected?}
        
        J -->|Yes| L[Send Entry/Exit to Master Hub]
        K -->|Yes| M[Send Entry/Exit to Master Hub]
        
        L --> N[Master Hub Updates Counters]
        M --> N
        
        N --> O[Save to EEPROM]
        O --> P[Update Web Dashboard]
        P --> Q[Flutter App Gets Updates]
    end
    
    subgraph "Real-time Monitoring"
        Q --> R[Update Mobile UI]
        R --> S[Display Current Count]
        S --> T[Show Device Status]
        T --> I
    end
    
    E --> I
    H --> I
```

## Device Communication Flow

```mermaid
sequenceDiagram
    participant MH as Master Hub
    participant D1R as Door 1 Receiver
    participant D2T as Door 2 Transmitter
    participant D2R as Door 2 Receiver
    participant FA as Flutter App
    
    Note over MH: Creates WiFi AP "ESP-Counter"
    MH->>D1R: Accept WiFi connection
    MH->>D2T: Accept WiFi connection
    MH->>D2R: Accept WiFi connection
    MH->>FA: Accept WiFi connection
    
    Note over MH: Starts IR transmission (D1+D2 pins)
    Note over D2T: Starts IR transmission (D1+D2 pins)
    
    loop Every 2 seconds
        FA->>MH: GET /api/test
        MH->>FA: Return current counts & status
    end
    
    loop Every 20-30 seconds
        D1R->>MH: POST /api/update (heartbeat)
        D2R->>MH: POST /api/update (heartbeat)
    end
    
    Note over D1R,D2R: IR sensors detect people
    
    alt Person enters Door 1
        D1R->>MH: POST /api/update (entry)
        MH->>MH: Increment total count
        MH->>MH: Save to EEPROM
        MH->>FA: Update available on next poll
    end
    
    alt Person exits Door 1
        D1R->>MH: POST /api/update (exit)
        MH->>MH: Decrement total count
        MH->>MH: Save to EEPROM
        MH->>FA: Update available on next poll
    end
    
    alt Person enters Door 2
        D2R->>MH: POST /api/update (entry)
        MH->>MH: Increment total count
        MH->>MH: Save to EEPROM
        MH->>FA: Update available on next poll
    end
    
    alt Person exits Door 2
        D2R->>MH: POST /api/update (exit)
        MH->>MH: Decrement total count
        MH->>MH: Save to EEPROM
        MH->>FA: Update available on next poll
    end
```

## Hardware Detection Logic

```mermaid
flowchart TD
    subgraph "Door 1 Detection (RX_door1)"
        A1[IR Sensor D6 - Entry] --> B1{Detect IR Beam Break?}
        A2[IR Sensor D5 - Exit] --> B2{Detect IR Beam Break?}
        
        B1 -->|Yes| C1{Only D6 Triggered?}
        B2 -->|Yes| C2{Only D5 Triggered?}
        
        C1 -->|Yes| D1[Send ENTRY to Master Hub]
        C1 -->|No| E1[Ignore - False Positive]
        C2 -->|Yes| D2[Send EXIT to Master Hub]
        C2 -->|No| E2[Ignore - False Positive]
        
        D1 --> F1[Wait 2s Cooldown]
        D2 --> F2[Wait 2s Cooldown]
        E1 --> G1[Continue Monitoring]
        E2 --> G2[Continue Monitoring]
        
        F1 --> G1
        F2 --> G2
    end
    
    subgraph "Door 2 Detection (RX_door2)"
        A3[IR Sensor D6 - Entry] --> B3{Detect IR Beam Break?}
        A4[IR Sensor D5 - Exit] --> B4{Detect IR Beam Break?}
        
        B3 -->|Yes| C3{Only D6 Triggered?}
        B4 -->|Yes| C4{Only D5 Triggered?}
        
        C3 -->|Yes| D3[Send ENTRY to Master Hub]
        C3 -->|No| E3[Ignore - False Positive]
        C4 -->|Yes| D4[Send EXIT to Master Hub]
        C4 -->|No| E4[Ignore - False Positive]
        
        D3 --> F3[Wait 2s Cooldown]
        D4 --> F4[Wait 2s Cooldown]
        E3 --> G3[Continue Monitoring]
        E4 --> G4[Continue Monitoring]
        
        F3 --> G3
        F4 --> G4
    end
```

## Flutter App Data Flow

```mermaid
flowchart TD
    A[App Starts] --> B[Connect to Master Hub WiFi]
    B --> C{Connection Successful?}
    C -->|Yes| D[Start Polling Timer (2s)]
    C -->|No| E[Show Connection Error]
    
    D --> F[GET /api/test from Master Hub]
    F --> G{Response Received?}
    G -->|Yes| H[Parse JSON Data]
    G -->|No| I[Show Connection Error]
    
    H --> J[Update UI Components]
    J --> K[Display People Count]
    J --> L[Show Device Status]
    J --> M[Update Timestamps]
    
    K --> N[Wait 2 Seconds]
    L --> N
    M --> N
    N --> F
    
    E --> O[Retry Connection]
    I --> O
    O --> B
```

## Master Hub Processing Flow

```mermaid
flowchart TD
    A[Receive POST /api/update] --> B[Parse Sensor & Action]
    B --> C{Valid Sensor ID?}
    C -->|No| D[Return Error Response]
    C -->|Yes| E[Update Sensor Status]
    
    E --> F{Action = Entry/Exit?}
    F -->|No| G[Return Success - Status Only]
    F -->|Yes| H[Process People Count]
    
    H --> I{Action = Entry?}
    I -->|Yes| J[Increment Total People]
    I -->|No| K{Total > 0?}
    
    J --> L[Increment Door Entry Count]
    K -->|Yes| M[Decrement Total People]
    K -->|No| N[Keep Total at 0]
    
    L --> O[Save All Counters to EEPROM]
    M --> O
    N --> O
    
    O --> P[Update Web Dashboard]
    P --> Q[Return Success Response]
    Q --> R[Flutter App Gets Update on Next Poll]
    
    G --> R
```

## Error Handling & Recovery

```mermaid
flowchart TD
    A[System Error Detected] --> B{Error Type?}
    
    B -->|WiFi Connection Lost| C[Auto-reconnect to Master Hub]
    B -->|Sensor Timeout| D[Mark Sensor as Offline]
    B -->|EEPROM Error| E[Reset to Default Values]
    B -->|IR Detection Failure| F[Continue Monitoring Other Sensors]
    
    C --> G{Reconnection Successful?}
    G -->|Yes| H[Resume Normal Operation]
    G -->|No| I[Retry with Exponential Backoff]
    
    D --> J[Continue Operation with Reduced Sensors]
    E --> K[Start Fresh Counters]
    F --> L[Maintain System Stability]
    
    I --> M{Max Retries Reached?}
    M -->|Yes| N[Enter Error State]
    M -->|No| C
    
    H --> O[System Fully Operational]
    J --> O
    K --> O
    L --> O
    N --> P[Show Error on Dashboard]
    
    O --> Q[Monitor for New Errors]
    P --> Q
```

## Data Flow Summary

| Component | Role | Communication | Frequency |
|-----------|------|---------------|-----------|
| **Master Hub** | WiFi AP + Web Server + IR TX | Creates network, processes data | Continuous |
| **Door 1 RX** | IR Detection + WiFi Client | Sends entry/exit events | On detection + 30s heartbeat |
| **Door 2 TX** | IR Transmission | Keeps IR LEDs ON | Continuous |
| **Door 2 RX** | IR Detection + WiFi Client | Sends entry/exit events | On detection + 30s heartbeat |
| **Flutter App** | Mobile Dashboard | Polls for updates | Every 2 seconds |

## Key System Features

### 1. **Bidirectional Detection**
- Prevents false positives from simultaneous sensor triggers
- 2-second cooldown between detections
- 100ms debounce protection

### 2. **Real-time Communication**
- WiFi network for instant data transmission
- HTTP API for structured communication
- Automatic reconnection on network loss

### 3. **Data Persistence**
- EEPROM storage for counter values
- Automatic data validation
- Recovery from corruption

### 4. **Multi-platform Monitoring**
- Web dashboard on Master Hub
- Flutter mobile app
- Real-time updates across all interfaces

### 5. **Fault Tolerance**
- Automatic WiFi fallback
- Sensor timeout detection
- Graceful degradation on failures

This system provides a **robust, real-time people counting solution** that combines hardware reliability with modern software monitoring, creating a comprehensive solution for traffic analysis and building access tracking.
