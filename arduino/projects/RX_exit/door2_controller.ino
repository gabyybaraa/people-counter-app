#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Door 2 Smart-Lite Controller
// Uses PROVEN simple connectivity + lightweight directional logic
#define Device_ID "RX_door2"
#define System_Version "2.0"

// Master Hub connection
const char* master_server_ip = "192.168.4.1";
const char* wifi_ssid = "ESP-Counter";
const char* wifi_password = "12345678";

// Pin Definitions - WORKING pins
const int IR_RECEIVER_ENTRY = D6;    // Entry (digital)
const int IR_RECEIVER_EXIT = D5;     // Exit (digital)

// Detection Variables
int entry_count = 0;
int exit_count = 0;
// Timing removed - Flutter app handles all timing

// Simple Smart Detection Variables
bool entry_active = false;
bool exit_active = false;
bool entry_last_state = false;
bool exit_last_state = false;

unsigned long entry_trigger_time = 0;
unsigned long exit_trigger_time = 0;
unsigned long last_detection = 0;

// Timing settings - SIMPLE and SAFE
const unsigned long DETECTION_COOLDOWN = 2000;  // 2 seconds between detections
const unsigned long DEBOUNCE_TIME = 100;        // Simple debounce
const int IR_threshold_digital = HIGH;          // Digital sensor threshold

// Connectivity variables (PROVEN working)
unsigned long lastHeartbeat = 0;
unsigned long lastWiFiCheck = 0;
bool wifiConnected = false;

void handleSimpleDirectionalDetection() {
  unsigned long currentTime = millis();
  
  // Skip if in cooldown period
  if (currentTime - last_detection < DETECTION_COOLDOWN) {
    return;
  }
  
  // Read sensors
  int entryReading = digitalRead(IR_RECEIVER_ENTRY);
  int exitReading = digitalRead(IR_RECEIVER_EXIT);
  
  bool entryTriggered = (entryReading == IR_threshold_digital);
  bool exitTriggered = (exitReading == IR_threshold_digital);
  
  if (entryTriggered && !exitTriggered && !entry_last_state) {
    // Entry triggered alone - this is an ENTRY
    delay(DEBOUNCE_TIME);
    
    // Re-check after debounce
    entryReading = digitalRead(IR_RECEIVER_ENTRY);
    exitReading = digitalRead(IR_RECEIVER_EXIT);
    entryTriggered = (entryReading == IR_threshold_digital);
    exitTriggered = (exitReading == IR_threshold_digital);
    
    if (entryTriggered && !exitTriggered) {
      entry_count++;
      
      Serial.printf("DOOR 2 ENTRY! Count: %d (D6 only)\n", 
                    entry_count);
      
      sendEventToMaster("door2_entry", "entry", entryReading);
      last_detection = currentTime;
    }
  }
  
  if (exitTriggered && !entryTriggered && !exit_last_state) {
    // Exit triggered alone - this is an EXIT
    delay(DEBOUNCE_TIME);
    
    // Re-check after debounce
    entryReading = digitalRead(IR_RECEIVER_ENTRY);
    exitReading = digitalRead(IR_RECEIVER_EXIT);
    entryTriggered = (entryReading == IR_threshold_digital);
    exitTriggered = (exitReading == IR_threshold_digital);
    
    if (exitTriggered && !entryTriggered) {
      exit_count++;
      
      Serial.printf("DOOR 2 EXIT! Count: %d (D5 only)\n", 
                    exit_count);
      
      sendEventToMaster("door2_exit", "exit", exitReading);
      last_detection = currentTime;
    }
  }
  
  // Log ambiguous detections for debugging
  if (entryTriggered && exitTriggered && (!entry_last_state || !exit_last_state)) {
    Serial.println("Both sensors triggered simultaneously - ignoring to prevent false positive");
  }
  
  // Update states
  entry_last_state = entryTriggered;
  exit_last_state = exitTriggered;
}

void connectToMaster() {
  Serial.println("Connecting to Master Hub...");
  WiFi.begin(wifi_ssid, wifi_password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\nConnected to Master: %s\n", wifi_ssid);
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nFailed to connect to Master Hub!");
    wifiConnected = false;
  }
}

void sendEventToMaster(String sensor, String action, int irValue) {
  if (!wifiConnected) return;
  
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, String("http://") + master_server_ip + "/api/update");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String beamStatusStr = "clear";  // Always clear after detection
  
  String postData = "sensor=" + sensor +
                   "&action=" + action +
                   "&ir_value=" + String(irValue) +
                   "&beam_status=" + beamStatusStr +
                   "&device=" + String(Device_ID);
  
  Serial.printf("Sending %s to Master: %s\n", action.c_str(), postData.c_str());
  
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
  
  WiFiClient client;
  HTTPClient http;
  
  // Send entry heartbeat (PROVEN working format)
  http.begin(client, String("http://") + master_server_ip + "/api/update");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  int entryIR = digitalRead(IR_RECEIVER_ENTRY);
  String entryStatus = (entryIR == HIGH) ? "broken" : "clear";
  
  String postData1 = "sensor=door2_entry&action=heartbeat&ir_value=" + String(entryIR) + 
                    "&beam_status=" + entryStatus + "&device=" + String(Device_ID);
  
  int httpResponseCode1 = http.POST(postData1);
  
  if (httpResponseCode1 == 200) {
    Serial.printf("Entry heartbeat OK - D6: %d, Status: %s\n", entryIR, entryStatus.c_str());
  } else {
    Serial.printf("Entry heartbeat failed: %d\n", httpResponseCode1);
  }
  
  http.end();
  
  // Send exit heartbeat (PROVEN working format)
  delay(100);
  http.begin(client, String("http://") + master_server_ip + "/api/update");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  int exitIR = digitalRead(IR_RECEIVER_EXIT);
  String exitStatus = (exitIR == IR_threshold_digital) ? "broken" : "clear";
  
  String postData2 = "sensor=door2_exit&action=heartbeat&ir_value=" + String(exitIR) + 
                    "&beam_status=" + exitStatus + "&device=" + String(Device_ID);
  
  int httpResponseCode2 = http.POST(postData2);
  
  if (httpResponseCode2 == 200) {
    Serial.printf("Exit heartbeat OK - D5: %d, Status: %s\n", exitIR, exitStatus.c_str());
  } else {
    Serial.printf("Exit heartbeat failed: %d\n", httpResponseCode2);
  }
  
  http.end();
}

void monitorWiFi() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastWiFiCheck < 10000) return;
  lastWiFiCheck = currentTime;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    wifiConnected = false;
    connectToMaster();
  }
}

void setup() {
  Serial.begin(9600);
  delay(3000);
  
  Serial.println("\nDOOR 2 CONTROLLER");
  Serial.println("===================================");
  Serial.printf("Device: %s\n", Device_ID);
  Serial.printf("Features:\n");
  Serial.printf("PROVEN connectivity (from simple test)\n");
  Serial.printf("Basic directional logic\n");
  Serial.printf("False positive prevention\n");
  Serial.printf("Simple cooldown system\n");
  Serial.printf("Entry: D6 (digital), Exit: D5 (digital)\n");
  Serial.printf("Cooldown: %lu ms\n", DETECTION_COOLDOWN);
  Serial.println("===================================");
  
  // Setup pins
  pinMode(IR_RECEIVER_ENTRY, INPUT);
  pinMode(IR_RECEIVER_EXIT, INPUT);
  
  // Test sensors
  Serial.println("Testing sensors...");
  for (int i = 0; i < 5; i++) {
    int entryIR = digitalRead(IR_RECEIVER_ENTRY);
    int exitIR = digitalRead(IR_RECEIVER_EXIT);
    Serial.printf("Test %d - Entry D6: %d | Exit D5: %d\n", i+1, entryIR, exitIR);
    delay(500);
  }
  
  // Initialize states
  entry_last_state = (digitalRead(IR_RECEIVER_ENTRY) == IR_threshold_digital);
  exit_last_state = (digitalRead(IR_RECEIVER_EXIT) == IR_threshold_digital);
  
  connectToMaster();
  
  if (wifiConnected) {
    Serial.println("Door 2 Controller ready!");
    Serial.println("Logic: Only count when ONE sensor triggers (prevents cross-trigger)");
    sendHeartbeatToMaster();
  } else {
    Serial.println("WiFi connection failed!");
  }
  
  Serial.println("Test guidelines:");
  Serial.println("Single sensor trigger = Valid detection");
  Serial.println("Both sensors together = Ignored (prevents false positives)");
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Handle directional detection (lightweight)
  handleSimpleDirectionalDetection();
  
  // WiFi monitoring (PROVEN working)
  monitorWiFi();
  
  // Heartbeat (PROVEN working)
  if (wifiConnected && (currentTime - lastHeartbeat > 30000)) {
    sendHeartbeatToMaster();
    lastHeartbeat = currentTime;
  }
  
  // Status updates
  static unsigned long lastStatus = 0;
  if (currentTime - lastStatus > 30000) {
    int entryIR = digitalRead(IR_RECEIVER_ENTRY);
    int exitIR = digitalRead(IR_RECEIVER_EXIT);
    
    Serial.printf("Door 2 - Entry:%d Exit:%d | WiFi:%s | D6:%d D5:%d\n", 
                  entry_count, exit_count, wifiConnected ? "OK" : "FAIL", entryIR, exitIR);
    lastStatus = currentTime;
  }
  
  delay(100);  // Standard loop delay
} 