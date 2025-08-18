#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Door 2 TX Controller - Controls 2 IR Transmitters for Door 2
// Connects to Master Hub WiFi for monitoring and coordination

#define Device_ID "TX_dual_door2_controller"
#define System_Version "2.0_Dual_TX_WiFi"

// Master Hub connection
const char* master_server_ip = "192.168.4.1";
const char* wifi_ssid = "ESP-Counter";
const char* wifi_password = "12345678";

// Pin Definitions - Dual IR Transmitters for Door 2
const int IR_LED_DOOR2_ENTRY = D1;    // IR transmitter for Door 2 Entry
const int IR_LED_DOOR2_EXIT = D2;     // IR transmitter for Door 2 Exit

// Timing
unsigned long lastHeartbeat = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastStatusUpdate = 0;

// WiFi status
bool wifiConnected = false;

void setupDualIRTransmitters() {
  pinMode(IR_LED_DOOR2_ENTRY, OUTPUT);
  pinMode(IR_LED_DOOR2_EXIT, OUTPUT);
  
  digitalWrite(IR_LED_DOOR2_ENTRY, HIGH);  // Turn on Door 2 Entry IR LED
  digitalWrite(IR_LED_DOOR2_EXIT, HIGH);   // Turn on Door 2 Exit IR LED
  
  Serial.println("Door 2 Entry IR Transmitter ON at D1");
  Serial.println("Door 2 Exit IR Transmitter ON at D2");
  Serial.println("Dual IR transmission active for Door 2");
}

void connectToMaster() {
  Serial.println("Connecting to Master Hub WiFi...");
  WiFi.begin(wifi_ssid, wifi_password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\nConnected to Master Hub: %s\n", wifi_ssid);
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Master Hub: %s\n", master_server_ip);
  } else {
    Serial.println("\nFailed to connect to Master Hub WiFi!");
    wifiConnected = false;
  }
}

void sendStatusToMaster() {
  if (!wifiConnected) return;
  
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, String("http://") + master_server_ip + "/api/update");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  // Get current LED states
  bool entryLED = digitalRead(IR_LED_DOOR2_ENTRY);
  bool exitLED = digitalRead(IR_LED_DOOR2_EXIT);
  
  // Send Door 2 TX status to master
  String postData = "sensor=door2_tx_status" +
                   String("&action=heartbeat") +
                   "&entry_tx=" + String(entryLED ? "ON" : "OFF") +
                   "&exit_tx=" + String(exitLED ? "ON" : "OFF") +
                   "&device=" + String(Device_ID) +
                   "&uptime=" + String(millis()/1000) +
                   "&free_heap=" + String(ESP.getFreeHeap());
  
  Serial.printf("Sending TX status to Master: %s\n", postData.c_str());
  
  int httpResponseCode = http.POST(postData);
  
  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.printf("TX status sent! Response: %s\n", response.c_str());
  } else {
    Serial.printf("Failed to send TX status: %d\n", httpResponseCode);
  }
  
  http.end();
}

void monitorWiFi() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastWiFiCheck < 10000) return;
  lastWiFiCheck = currentTime;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting to Master Hub...");
    wifiConnected = false;
    connectToMaster();
  }
}

void setup() {
  Serial.begin(9600);
  delay(3000);
  
  Serial.println("\nDUAL TX CONTROLLER - DOOR 2 (WiFi)");
  Serial.println("=========================================");
  Serial.printf("Device: %s\n", Device_ID);
  Serial.printf("Version: %s\n", System_Version);
  Serial.printf("Function: Dual IR Transmitter Controller for Door 2\n");
  Serial.printf("Entry TX: D1 (GPIO5), Exit TX: D2 (GPIO4)\n");
  Serial.printf("Master Hub: %s\n", master_server_ip);
  Serial.printf("WiFi Network: %s\n", wifi_ssid);
  Serial.printf("Free RAM: %d bytes\n", ESP.getFreeHeap());
  Serial.println("=========================================");
  
  // Setup dual IR transmitters
  setupDualIRTransmitters();
  
  // Connect to Master Hub WiFi
  connectToMaster();
  
  Serial.println("Door 2 Dual TX Controller ready!");
  Serial.println("Continuous IR transmission active for both sensors");
  Serial.println("WiFi connected to Master Hub for monitoring");
  Serial.println("Pairing info:");
  Serial.println("   Door 2 Entry TX: D1 -> Door 2 Entry RX: A0");
  Serial.println("   Door 2 Exit TX: D2 -> Door 2 Exit RX: D3");
  
  if (wifiConnected) {
    sendStatusToMaster();
  }
  
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Keep both Door 2 IR LEDs on continuously
  digitalWrite(IR_LED_DOOR2_ENTRY, HIGH);
  digitalWrite(IR_LED_DOOR2_EXIT, HIGH);
  
  // WiFi monitoring
  monitorWiFi();
  
  // Send status to Master Hub every 30 seconds
  if (wifiConnected && (currentTime - lastHeartbeat > 30000)) {
    sendStatusToMaster();
    lastHeartbeat = currentTime;
  }
  
  // Local status updates every 60 seconds
  if (currentTime - lastStatusUpdate > 60000) {
    bool entryLED = digitalRead(IR_LED_DOOR2_ENTRY);
    bool exitLED = digitalRead(IR_LED_DOOR2_EXIT);
    
    Serial.printf("Door 2 TX Status - Entry: %s (D1) | Exit: %s (D2) | WiFi: %s | Uptime: %lu min\n", 
                  entryLED ? "ON" : "OFF", 
                  exitLED ? "ON" : "OFF",
                  wifiConnected ? "Connected" : "Disconnected",
                  currentTime/1000/60);
    
    Serial.printf("WiFi Signal: %d dBm | Free Heap: %d bytes\n", 
                  WiFi.RSSI(), ESP.getFreeHeap());
    
    lastStatusUpdate = currentTime;
  }
  
  delay(1000);  // Slower loop since this just transmits IR
} 