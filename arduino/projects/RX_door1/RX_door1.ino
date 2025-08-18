#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Robust Door Controller with Better Startup Recovery
// TEMPLATE: Modify Device_ID and pin assignments for each door

// === CONFIGURE FOR EACH DOOR ===
#define Device_ID "RX_door1"    // Change to door1_robust or door2_robust
#define System_Version "2.0"

// Door 1 pins: Entry=D6 (digital), Exit=D5 (digital)
// Door 2 pins: Entry=D3 (digital), Exit=A0 (analog)
const int IR_RECEIVER_ENTRY = D6;    // Entry sensor pin
const int IR_RECEIVER_EXIT = D5;     // Exit sensor pin
const int IR_threshold_digital = HIGH;  // Digital threshold
const bool ENTRY_IS_DIGITAL = true;     // Both sensors are digital for Door 1
// === END CONFIGURATION ===

// Master Hub connection
const char* master_server_ip = "192.168.4.1";
const char* wifi_ssid = "ESP-Counter";
const char* wifi_password = "12345678";

// Detection Variables
int entry_count = 0;
int exit_count = 0;
String entry_last_activity = "Never";
String exit_last_activity = "Never";

// Smart Detection Variables
bool entry_last_state = false;
bool exit_last_state = false;
unsigned long last_detection = 0;

// Enhanced connectivity variables
unsigned long lastHeartbeat = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastConnectionAttempt = 0;
unsigned long startupTime = 0;
bool wifiConnected = false;
bool initialHeartbeatSent = false;
int connectionAttempts = 0;

// Timing settings
const unsigned long DETECTION_COOLDOWN = 2000;
const unsigned long DEBOUNCE_TIME = 100;
const unsigned long STARTUP_DELAY = 5000;          // Wait 5s before first connection
const unsigned long RECONNECTION_INTERVAL = 10000;  // Try reconnect every 10s
const unsigned long HEARTBEAT_INTERVAL = 20000;     // Heartbeat every 20s (more frequent)
const unsigned long INITIAL_HEARTBEAT_DELAY = 2000; // Send first heartbeat quickly

void handleSimpleDirectionalDetection() {
  unsigned long currentTime = millis();
  
  // Skip if in cooldown period or during startup
  if (currentTime - last_detection < DETECTION_COOLDOWN || 
      currentTime - startupTime < STARTUP_DELAY) {
    return;
  }
  
  // Read sensors - both are digital for Door 1
  int entryReading = digitalRead(IR_RECEIVER_ENTRY);
  int exitReading = digitalRead(IR_RECEIVER_EXIT);
  bool entryTriggered = (entryReading == IR_threshold_digital);
  bool exitTriggered = (exitReading == IR_threshold_digital);
  
  // Simple directional logic: only count single sensor triggers
  if (entryTriggered && !exitTriggered && !entry_last_state) {
    delay(DEBOUNCE_TIME);
    
    // Re-check after debounce
    entryReading = digitalRead(IR_RECEIVER_ENTRY);
    exitReading = digitalRead(IR_RECEIVER_EXIT);
    entryTriggered = (entryReading == IR_threshold_digital);
    exitTriggered = (exitReading == IR_threshold_digital);
    
    if (entryTriggered && !exitTriggered) {
      entry_count++;
      
      // Format time
      unsigned long seconds = currentTime / 1000;
      int hours = (seconds / 3600) % 24;
      int minutes = (seconds % 3600) / 60;
      int secs = seconds % 60;
      entry_last_activity = String(hours) + ":" + 
                           (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                           (secs < 10 ? "0" : "") + String(secs);
      
      Serial.printf("ENTRY! Count: %d (Reading: %d) Time: %s\n", 
                    entry_count, entryReading, entry_last_activity.c_str());
      
      sendEventToMaster("door1_entry", "entry", entryReading);
      last_detection = currentTime;
    }
  }
  
  if (exitTriggered && !entryTriggered && !exit_last_state) {
    delay(DEBOUNCE_TIME);
    
    // Re-check after debounce
    entryReading = digitalRead(IR_RECEIVER_ENTRY);
    exitReading = digitalRead(IR_RECEIVER_EXIT);
    entryTriggered = (entryReading == IR_threshold_digital);
    exitTriggered = (exitReading == IR_threshold_digital);
    
    if (exitTriggered && !entryTriggered) {
      exit_count++;
      
      // Format time
      unsigned long seconds = currentTime / 1000;
      int hours = (seconds / 3600) % 24;
      int minutes = (seconds % 3600) / 60;
      int secs = seconds % 60;
      exit_last_activity = String(hours) + ":" + 
                          (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                          (secs < 10 ? "0" : "") + String(secs);
      
      Serial.printf("EXIT! Count: %d (Reading: %d) Time: %s\n", 
                    exit_count, exitReading, exit_last_activity.c_str());
      
      sendEventToMaster("door1_exit", "exit", exitReading);
      last_detection = currentTime;
    }
  }
  
  // Log ambiguous detections
  if (entryTriggered && exitTriggered && (!entry_last_state || !exit_last_state)) {
    Serial.printf("Both sensors triggered - ignoring (Entry: %d, Exit: %d)\n", 
                  entryReading, exitReading);
  }
  
  // Update states
  entry_last_state = entryTriggered;
  exit_last_state = exitTriggered;
}

void connectToMaster() {
  unsigned long currentTime = millis();
  
  // Don't try to connect too frequently
  if (currentTime - lastConnectionAttempt < RECONNECTION_INTERVAL) {
    return;
  }
  
  lastConnectionAttempt = currentTime;
  connectionAttempts++;
  
  Serial.printf("Connection attempt #%d to Master Hub...\n", connectionAttempts);
  
  // Disconnect first to ensure clean connection
  WiFi.disconnect();
  delay(100);
  
  WiFi.begin(wifi_ssid, wifi_password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {  // Reduced attempts for faster retry
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    connectionAttempts = 0;  // Reset counter on success
    Serial.printf("\nConnected to Master: %s\n", wifi_ssid);
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
    
    // Send initial heartbeat quickly
    if (!initialHeartbeatSent) {
      delay(INITIAL_HEARTBEAT_DELAY);
      sendHeartbeatToMaster();
      initialHeartbeatSent = true;
      Serial.println("Initial heartbeat sent - device should appear online!");
    }
  } else {
    Serial.printf("\nConnection attempt #%d failed\n", connectionAttempts);
    wifiConnected = false;
  }
}

void sendEventToMaster(String sensor, String action, int irValue) {
  if (!wifiConnected) {
    Serial.println("No WiFi - cannot send event");
    return;
  }
  
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, String("http://") + master_server_ip + "/api/update");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(5000);  // 5 second timeout
  
  String beamStatusStr = "clear";
  String activityTime = "";
  
  if (sensor.indexOf("entry") >= 0) {
    activityTime = entry_last_activity;
  } else if (sensor.indexOf("exit") >= 0) {
    activityTime = exit_last_activity;
  }
  
  String postData = "sensor=" + sensor +
                   "&action=" + action +
                   "&ir_value=" + String(irValue) +
                   "&beam_status=" + beamStatusStr +
                   "&activity_time=" + activityTime +
                   "&device=" + String(Device_ID);
  
  Serial.printf("Sending %s: %s\n", action.c_str(), postData.c_str());
  
  int httpResponseCode = http.POST(postData);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.printf("%s sent! Response: %s\n", action.c_str(), response.c_str());
  } else {
    Serial.printf("Failed to send %s: %d\n", action.c_str(), httpResponseCode);
  }
  
  http.end();
}

void sendHeartbeatToMaster() {
  if (!wifiConnected) {
    Serial.println("No WiFi - skipping heartbeat");
    return;
  }
  
  Serial.println("Sending heartbeat...");
  
  WiFiClient client;
  HTTPClient http;
  
  // Send entry heartbeat
  http.begin(client, String("http://") + master_server_ip + "/api/update");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(5000);
  
  int entryIR = digitalRead(IR_RECEIVER_ENTRY);
  int exitIR = digitalRead(IR_RECEIVER_EXIT);
  String entryStatus = (entryIR == IR_threshold_digital) ? "broken" : "clear";
  String exitStatus = (exitIR == IR_threshold_digital) ? "broken" : "clear";
  
  String postData1 = "sensor=door1_entry&action=heartbeat&ir_value=" + String(entryIR) + 
                    "&beam_status=" + entryStatus + "&activity_time=" + entry_last_activity + 
                    "&device=" + String(Device_ID);
  
  int httpResponseCode1 = http.POST(postData1);
  
  if (httpResponseCode1 == 200) {
    Serial.printf("Entry heartbeat OK - D6: %d, Status: %s\n", 
                  entryIR, entryStatus.c_str());
  } else {
    Serial.printf("Entry heartbeat failed: %d\n", httpResponseCode1);
  }
  
  http.end();
  
  // Send exit heartbeat
  delay(200);
  http.begin(client, String("http://") + master_server_ip + "/api/update");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(5000);
  
  String postData2 = "sensor=door1_exit&action=heartbeat&ir_value=" + String(exitIR) + 
                    "&beam_status=" + exitStatus + "&activity_time=" + exit_last_activity + 
                    "&device=" + String(Device_ID);
  
  int httpResponseCode2 = http.POST(postData2);
  
  if (httpResponseCode2 == 200) {
    Serial.printf("Exit heartbeat OK - D5: %d, Status: %s\n", 
                  exitIR, exitStatus.c_str());
  } else {
    Serial.printf("Exit heartbeat failed: %d\n", httpResponseCode2);
  }
  
  http.end();
  
  lastHeartbeat = millis();
}

void monitorWiFi() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastWiFiCheck < 5000) return;  // Check every 5 seconds
  lastWiFiCheck = currentTime;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting reconnection...");
    wifiConnected = false;
    initialHeartbeatSent = false;  // Reset so we send initial heartbeat again
    connectToMaster();
  }
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  
  startupTime = millis();
  
  Serial.println("\nROBUST DOOR CONTROLLER");
  Serial.println("==============================");
  Serial.printf("Device: %s\n", Device_ID);
  Serial.printf("System: %s\n", System_Version);
  Serial.printf("Pin Config: Entry=D6(digital), Exit=D5(digital)\n");
  Serial.printf("Startup delay: %lu ms\n", STARTUP_DELAY);
  Serial.printf("Heartbeat interval: %lu ms\n", HEARTBEAT_INTERVAL);
  Serial.println("==============================");
  
  // Setup pins - both are digital for Door 1
  pinMode(IR_RECEIVER_ENTRY, INPUT);      // D6 - Entry sensor
  pinMode(IR_RECEIVER_EXIT, INPUT);       // D5 - Exit sensor
  
  Serial.printf("Startup delay: %lu seconds (waiting for Master Hub)...\n", STARTUP_DELAY/1000);
  
  // Test sensors during startup delay
  for (int i = 0; i < 5; i++) {
    int entryIR = digitalRead(IR_RECEIVER_ENTRY);
    int exitIR = digitalRead(IR_RECEIVER_EXIT);
    Serial.printf("Sensor test %d - Entry D6: %d | Exit D5: %d\n", i+1, entryIR, exitIR);
    delay(1000);
  }
  
  // Initialize states
  entry_last_state = (digitalRead(IR_RECEIVER_ENTRY) == IR_threshold_digital);
  exit_last_state = (digitalRead(IR_RECEIVER_EXIT) == IR_threshold_digital);
  
  Serial.println("Initial sensor states:");
  Serial.printf("  Entry D6: %s | Exit D5: %s\n", 
                entry_last_state ? "DETECTED" : "CLEAR",
                exit_last_state ? "DETECTED" : "CLEAR");
  
  Serial.println("Starting connection process...");
  connectToMaster();
  
  Serial.println("Setup complete - monitoring mode active");
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Handle detection (only after startup delay)
  if (currentTime - startupTime >= STARTUP_DELAY) {
    handleSimpleDirectionalDetection();
  }
  
  // WiFi monitoring
  monitorWiFi();
  
  // Regular heartbeat
  if (wifiConnected && (currentTime - lastHeartbeat > HEARTBEAT_INTERVAL)) {
    sendHeartbeatToMaster();
  }
  
  // Status updates
  static unsigned long lastStatus = 0;
  if (currentTime - lastStatus > 30000) {
    int entryIR = digitalRead(IR_RECEIVER_ENTRY);
    int exitIR = digitalRead(IR_RECEIVER_EXIT);
    
    Serial.printf("Status - Entry:%d Exit:%d | WiFi:%s | D6:%d D5:%d | Uptime: %lu min\n", 
                  entry_count, exit_count, 
                  wifiConnected ? "OK" : "FAIL", 
                  entryIR, exitIR,
                  currentTime/1000/60);
    lastStatus = currentTime;
  }
  
  delay(100);
} 