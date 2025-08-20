#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

#define Device_ID "TX_master_hub_dual"
#define System_Version "2.0_Bidirectional_Dual"

// WiFi Configuration
struct WiFiConfig {
  const char* ssid;
  const char* password;
};

WiFiConfig wifiOptions[] = {
  {"ESP-Counter", "12345678"},
  {"PeopleCounter", "12345678"},
  {"MASTER-COUNTER", "12345678"},
  {"ESP-Master", "12345678"},
};
const int numWifiOptions = 4;

// Pin Definitions - Dual IR Transmitters
const int IR_LED_DOOR1_ENTRY = D1;    // IR transmitter for Door 1 Entry
const int IR_LED_DOOR1_EXIT = D2;     // IR transmitter for Door 1 Exit

// Current WiFi config
String currentSSID = "";
String currentPassword = "";
int currentWifiIndex = 0;

// Bidirectional People Counter Data
int totalPeopleInside = 0;
int door1_entries = 0;
int door1_exits = 0;
int door2_entries = 0;
int door2_exits = 0;

// Sensor status tracking with timing
struct SensorStatus {
  bool connected;
  String lastUpdate;
  int irValue;
  String beamStatus;
  String lastActivity;
  unsigned long lastSeen;
};

SensorStatus door1_entry = {false, "Never", 0, "unknown", "Never", 0};
SensorStatus door1_exit = {false, "Never", 0, "unknown", "Never", 0};
SensorStatus door2_entry = {false, "Never", 0, "unknown", "Never", 0};
SensorStatus door2_exit = {false, "Never", 0, "unknown", "Never", 0};

// Timing tracking removed - using Flutter app time instead

// Timing
unsigned long lastHeartbeat = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastSave = 0;
const unsigned long SENSOR_TIMEOUT = 60000; // 1 minute timeout

// Web Server
AsyncWebServer server(80);
bool serverStarted = false;

// EEPROM addresses for persistent data
const int EEPROM_TOTAL_ADDR = 0;
const int EEPROM_DOOR1_ENTRY_ADDR = 4;
const int EEPROM_DOOR1_EXIT_ADDR = 8;
const int EEPROM_DOOR2_ENTRY_ADDR = 12;
const int EEPROM_DOOR2_EXIT_ADDR = 16;

// Timing EEPROM addresses removed - no longer storing timestamps

void addCORSHeaders(AsyncWebServerResponse *response) {
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST");
  response->addHeader("Cache-Control", "no-cache");
}

// Dual IR LED Management
void setupDualIRTransmitters() {
  pinMode(IR_LED_DOOR1_ENTRY, OUTPUT);
  pinMode(IR_LED_DOOR1_EXIT, OUTPUT);
  
  digitalWrite(IR_LED_DOOR1_ENTRY, HIGH);  // Turn on Door 1 Entry IR LED
  digitalWrite(IR_LED_DOOR1_EXIT, HIGH);   // Turn on Door 1 Exit IR LED
  
  Serial.println("🔴 Door 1 Entry IR Transmitter ON at D1");
  Serial.println("🔴 Door 1 Exit IR Transmitter ON at D2");
  Serial.println("Dual IR transmission active for Door 1");
}

void loadSavedData() {
  // Load counters
  EEPROM.get(EEPROM_TOTAL_ADDR, totalPeopleInside);
  EEPROM.get(EEPROM_DOOR1_ENTRY_ADDR, door1_entries);
  EEPROM.get(EEPROM_DOOR1_EXIT_ADDR, door1_exits);
  EEPROM.get(EEPROM_DOOR2_ENTRY_ADDR, door2_entries);
  EEPROM.get(EEPROM_DOOR2_EXIT_ADDR, door2_exits);
  
  // Validate counter data
  if (totalPeopleInside < 0 || totalPeopleInside > 9999) totalPeopleInside = 0;
  if (door1_entries < 0 || door1_entries > 99999) door1_entries = 0;
  if (door1_exits < 0 || door1_exits > 99999) door1_exits = 0;
  if (door2_entries < 0 || door2_entries > 99999) door2_entries = 0;
  if (door2_exits < 0 || door2_exits > 99999) door2_exits = 0;
  
  Serial.printf("Loaded - Total: %d, D1E: %d, D1X: %d, D2E: %d, D2X: %d\n", 
                totalPeopleInside, door1_entries, door1_exits, door2_entries, door2_exits);
  Serial.println("Timing data removed - using Flutter app time instead");
}

void saveData() {
  // Save counters only
  EEPROM.put(EEPROM_TOTAL_ADDR, totalPeopleInside);
  EEPROM.put(EEPROM_DOOR1_ENTRY_ADDR, door1_entries);
  EEPROM.put(EEPROM_DOOR1_EXIT_ADDR, door1_exits);
  EEPROM.put(EEPROM_DOOR2_ENTRY_ADDR, door2_entries);
  EEPROM.put(EEPROM_DOOR2_EXIT_ADDR, door2_exits);
  
  // Timing data removed - Flutter app handles all timing
  EEPROM.commit();
}

void updatePeopleCount(String sensor, String action) {
  if (action == "entry") {
    totalPeopleInside++;
    if (sensor == "door1_entry") {
      door1_entries++;
    } else if (sensor == "door2_entry") {
      door2_entries++;
    }
    Serial.printf("ENTRY at %s - People inside: %d\n", sensor.c_str(), totalPeopleInside);
  } 
  else if (action == "exit") {
    if (totalPeopleInside > 0) {
      totalPeopleInside--;
    }
    if (sensor == "door1_exit") {
      door1_exits++;
    } else if (sensor == "door2_exit") {
      door2_exits++;
    }
    Serial.printf("EXIT at %s - People inside: %d\n", sensor.c_str(), totalPeopleInside);
  }
  
  saveData();
}

bool createStableAP() {
  Serial.println("Creating Master WiFi AP...");
  
  for (int i = 0; i < numWifiOptions; i++) {
    currentWifiIndex = i;
    currentSSID = wifiOptions[i].ssid;
    currentPassword = wifiOptions[i].password;
    
    Serial.printf("Attempt %d: Trying '%s'...\n", i+1, currentSSID.c_str());
    
    WiFi.persistent(false);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(2000);
    WiFi.mode(WIFI_AP);
    delay(2000);
    
    bool result;
    if (currentPassword == "") {
      result = WiFi.softAP(currentSSID.c_str());
    } else {
      result = WiFi.softAP(currentSSID.c_str(), currentPassword.c_str());
    }
    
    delay(5000);
    
    if (result && WiFi.softAPIP() != IPAddress(0,0,0,0)) {
      bool stable = true;
      Serial.printf("'%s' created! Testing stability...\n", currentSSID.c_str());
      
      for (int test = 0; test < 5; test++) {
        delay(1000);
        if (WiFi.softAPIP() == IPAddress(0,0,0,0)) {
          stable = false;
          break;
        }
        Serial.printf("Stability test %d/5\n", test+1);
      }
      
      if (stable) {
        IPAddress IP = WiFi.softAPIP();
        Serial.println("DUAL TX MASTER HUB AP CREATED!");
        Serial.printf("SSID: %s\n", currentSSID.c_str());
        if (currentPassword != "") {
          Serial.printf("Password: %s\n", currentPassword.c_str());
        }
        Serial.printf("IP: %s\n", IP.toString().c_str());
        return true;
      }
    }
    
    WiFi.softAPdisconnect(true);
    delay(2000);
  }
  
  return false;
}

void setupWebServer() {
  if (serverStarted) return;
  
  Serial.println("Setting up Dual TX Master Hub web server...");
  
  // Root page - Bidirectional Dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    int clients = WiFi.softAPgetStationNum();
    
    String html = "<!DOCTYPE html><html><body style='font-family:Arial;margin:20px;background:#1a1a1a;color:white;'>";
    html += "<h1 style='text-align:center;color:#00bcd4;'> Dual TX Bidirectional Counter</h1>";
    html += "<div style='text-align:center;'>";
    html += "<h2 style='font-size:80px;color:#4caf50;margin:20px;'>" + String(totalPeopleInside) + "</h2>";
    html += "<p style='font-size:20px;'>People Inside Building</p>";
    html += "</div>";
    
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:20px 0;'>";
    
    // Door 1 Status
    html += "<div style='background:#2c3e50;padding:20px;border-radius:10px;border-left:5px solid #3498db;'>";
    html += "<h3 style='color:#3498db;margin-top:0;'> Door 1 (Dual TX Controlled)</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px;'>";
    html += "<div style='background:#34495e;padding:10px;border-radius:5px;'>";
    html += "<h4 style='color:#2ecc71;margin:0;'> Entry Side</h4>";
    html += "<p style='margin:5px 0;'>Count: " + String(door1_entries) + "</p>";
    html += "<p style='margin:5px 0;'>Status: " + String(door1_entry.connected ? "🟢 Online" : "🔴 Offline") + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door1_entry.lastActivity + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;color:#95a5a6;'>Timing: Flutter App</p>";
    html += "<p style='margin:5px 0;font-size:10px;color:#95a5a6;'>TX: D1 Pin</p>";
    html += "</div>";
    html += "<div style='background:#34495e;padding:10px;border-radius:5px;'>";
    html += "<h4 style='color:#e74c3c;margin:0;'> Exit Side</h4>";
    html += "<p style='margin:5px 0;'>Count: " + String(door1_exits) + "</p>";
    html += "<p style='margin:5px 0;'>Status: " + String(door1_exit.connected ? "🟢 Online" : "🔴 Offline") + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door1_exit.lastActivity + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;color:#95a5a6;'>Timing: Flutter App</p>";
    html += "<p style='margin:5px 0;font-size:10px;color:#95a5a6;'>TX: D2 Pin</p>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    // Door 2 Status
    html += "<div style='background:#2c3e50;padding:20px;border-radius:10px;border-left:5px solid #f39c12;'>";
    html += "<h3 style='color:#f39c12;margin-top:0;'> Door 2 (External TX)</h3>";
    html += "<div style='display:grid;grid-template-columns:1fr 1fr;gap:10px;'>";
    html += "<div style='background:#34495e;padding:10px;border-radius:5px;'>";
    html += "<h4 style='color:#2ecc71;margin:0;'> Entry Side</h4>";
    html += "<p style='margin:5px 0;'>Count: " + String(door2_entries) + "</p>";
    html += "<p style='margin:5px 0;'>Status: " + String(door2_entry.connected ? "🟢 Online" : "🔴 Offline") + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door2_entry.lastActivity + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;color:#95a5a6;'>Timing: Flutter App</p>";
    html += "</div>";
    html += "<div style='background:#34495e;padding:10px;border-radius:5px;'>";
    html += "<h4 style='color:#e74c3c;margin:0;'> Exit Side</h4>";
    html += "<p style='margin:5px 0;'>Count: " + String(door2_exits) + "</p>";
    html += "<p style='margin:5px 0;'>Status: " + String(door2_exit.connected ? "🟢 Online" : "🔴 Offline") + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door2_exit.lastActivity + "</p>";
    html += "<p style='margin:5px 0;font-size:12px;color:#95a5a6;'>Timing: Flutter App</p>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>";
    
    html += "<div style='text-align:center;margin:20px;'>";
    html += "<p style='color:#bdc3c7;'>Connected Clients: " + String(clients) + " | ";
    html += "System: " + String(System_Version) + " | ";
    html += "Uptime: " + String(millis()/1000/60) + " min</p>";
    html += "<p style='color:#95a5a6;font-size:12px;'>This TX controls Door 1 Entry (D1) + Door 1 Exit (D2) IR transmitters</p>";
    html += "<a href='/reset' style='background:#e74c3c;color:white;padding:15px 30px;text-decoration:none;border-radius:25px;margin:10px;'>🔄 Reset All Counters</a>";
    html += "</div>";
    
    html += "<script>setTimeout(function(){location.reload()}, 15000);</script>";
    html += "</body></html>";
    
    request->send(200, "text/html", html);
  });
  
  // Main API endpoint for Flutter app - WITH TIMING DATA
  server.on("/api/test", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"status\":\"success\",";
    json += "\"device_id\":\"" + String(Device_ID) + "\",";
    json += "\"door_type\":\"Master_Hub\",";
    json += "\"system_version\":\"" + String(System_Version) + "\",";
    json += "\"total_people_inside\":" + String(totalPeopleInside) + ",";
    json += "\"network\":\"" + currentSSID + "\",";
    json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    
    // Door 1 data (timing removed)
    json += "\"door1\":{";
    json += "\"entries\":" + String(door1_entries) + ",";
    json += "\"exits\":" + String(door1_exits) + ",";
    json += "\"entry_sensor\":{\"connected\":" + String(door1_entry.connected ? "true" : "false") + ",\"last_activity\":\"" + door1_entry.lastActivity + "\",\"ir_value\":" + String(door1_entry.irValue) + "},";
    json += "\"exit_sensor\":{\"connected\":" + String(door1_exit.connected ? "true" : "false") + ",\"last_activity\":\"" + door1_exit.lastActivity + "\",\"ir_value\":" + String(door1_exit.irValue) + "}";
    json += "},";
    
    // Door 2 data (timing removed)
    json += "\"door2\":{";
    json += "\"entries\":" + String(door2_entries) + ",";
    json += "\"exits\":" + String(door2_exits) + ",";
    json += "\"entry_sensor\":{\"connected\":" + String(door2_entry.connected ? "true" : "false") + ",\"last_activity\":\"" + door2_entry.lastActivity + "\",\"ir_value\":" + String(door2_entry.irValue) + "},";
    json += "\"exit_sensor\":{\"connected\":" + String(door2_exit.connected ? "true" : "false") + ",\"last_activity\":\"" + door2_exit.lastActivity + "\",\"ir_value\":" + String(door2_exit.irValue) + "}";
    json += "}";
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // Sensor data update endpoint - Enhanced for bidirectional sensors
  server.on("/api/update", HTTP_POST, [](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    String sensor = "";
    String action = "";
    
    // Parse sensor data
    if (request->hasParam("sensor", true)) {
      sensor = request->getParam("sensor", true)->value();
    }
    if (request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
    }
    
    // Update appropriate sensor status
    SensorStatus* currentSensor = nullptr;
    if (sensor == "door1_entry") currentSensor = &door1_entry;
    else if (sensor == "door1_exit") currentSensor = &door1_exit;
    else if (sensor == "door2_entry") currentSensor = &door2_entry;
    else if (sensor == "door2_exit") currentSensor = &door2_exit;
    
    if (currentSensor) {
      currentSensor->connected = true;
      currentSensor->lastSeen = millis();
      currentSensor->lastUpdate = String(millis()/1000) + "s";
      
      if (request->hasParam("ir_value", true)) {
        currentSensor->irValue = request->getParam("ir_value", true)->value().toInt();
      }
      if (request->hasParam("beam_status", true)) {
        currentSensor->beamStatus = request->getParam("beam_status", true)->value();
      }
      if (request->hasParam("activity_time", true)) {
        currentSensor->lastActivity = request->getParam("activity_time", true)->value();
      }
    }
    
    // Update people count for entry/exit actions
    if (action == "entry" || action == "exit") {
      updatePeopleCount(sensor, action);
    }
    
    Serial.printf("Update from %s: %s %s - Total: %d\n", 
                  clientIP.c_str(), sensor.c_str(), action.c_str(), totalPeopleInside);
    
    String json = "{\"status\":\"success\",\"total_people\":" + String(totalPeopleInside) + "}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // Reset endpoint - counters only (timing removed)
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    totalPeopleInside = 0;
    door1_entries = 0;
    door1_exits = 0;
    door2_entries = 0;
    door2_exits = 0;
    saveData();
    Serial.println("All counters reset via web interface");
    request->redirect("/");
  });

  server.on("/api/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    totalPeopleInside = 0;
    door1_entries = 0;
    door1_exits = 0;
    door2_entries = 0;
    door2_exits = 0;
    saveData();
    
    String json = "{\"success\":true,\"message\":\"All counters reset\"}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
    Serial.println("All counters reset via API");
  });

  // Additional legacy endpoints for compatibility
  server.on("/api/count", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"count\":" + String(totalPeopleInside) + ",\"status\":\"success\"}";
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"device_id\":\"" + String(Device_ID) + "\",";
    json += "\"door_type\":\"Master_Hub\",";
    json += "\"total_people_inside\":" + String(totalPeopleInside) + ",";
    json += "\"wifi_network\":\"" + currentSSID + "\",";
    json += "\"connected_clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"uptime_seconds\":" + String(millis()/1000) + ",";
    json += "\"free_memory\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"system_type\":\"Dual_TX_Master_Hub_Bidirectional\",";
    json += "\"system_stable\":true";
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  server.begin();
  serverStarted = true;
  Serial.println("DUAL TX MASTER HUB WEB SERVER STARTED!");
}

void checkSensorTimeouts() {
  unsigned long currentTime = millis();
  
  if (currentTime - door1_entry.lastSeen > SENSOR_TIMEOUT && door1_entry.connected) {
    door1_entry.connected = false;
    Serial.println("Door 1 Entry sensor timeout");
  }
  if (currentTime - door1_exit.lastSeen > SENSOR_TIMEOUT && door1_exit.connected) {
    door1_exit.connected = false;
    Serial.println("Door 1 Exit sensor timeout");
  }
  if (currentTime - door2_entry.lastSeen > SENSOR_TIMEOUT && door2_entry.connected) {
    door2_entry.connected = false;
    Serial.println("Door 2 Entry sensor timeout");
  }
  if (currentTime - door2_exit.lastSeen > SENSOR_TIMEOUT && door2_exit.connected) {
    door2_exit.connected = false;
    Serial.println("Door 2 Exit sensor timeout");
  }
}

void setup() {
  Serial.begin(9600);
  delay(3000);
  
  Serial.println("\n DUAL TX MASTER HUB (BIDIRECTIONAL) 🏢");
  Serial.println("==========================================");
  Serial.printf("Device: %s\n", Device_ID);
  Serial.printf("Version: %s\n", System_Version);
  Serial.printf("Role: Master Hub + Dual IR TX Controller + WiFi AP + Web Server\n");
  Serial.printf("IR Transmitters: Door 1 Entry (D1) + Door 1 Exit (D2)\n");
  Serial.printf("Free RAM: %d bytes\n", ESP.getFreeHeap());
  Serial.println("==========================================");
  
  // Initialize EEPROM for counters only (timing removed)
  EEPROM.begin(32);
  loadSavedData();
  
  // Setup dual IR transmitters for Door 1
  setupDualIRTransmitters();
  
  // Create WiFi AP
  if (createStableAP()) {
    setupWebServer();
    Serial.printf("Master Network: %s\n", currentSSID.c_str());
    Serial.printf("Password: %s\n", currentPassword.c_str());
    Serial.printf("Web Interface: http://%s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("Flutter API: http://%s/api/test\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("Sensor Update URL: http://%s/api/update\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("Could not create Master WiFi AP!");
  }
  
  Serial.println("Dual TX Master Hub ready! Waiting for bidirectional sensor data...\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Keep both Door 1 IR LEDs on continuously
  digitalWrite(IR_LED_DOOR1_ENTRY, HIGH);
  digitalWrite(IR_LED_DOOR1_EXIT, HIGH);
  
  // Check sensor timeouts
  checkSensorTimeouts();
  
  // Status updates every 30 seconds
  if (currentTime - lastHeartbeat > 30000) {
    int clients = WiFi.softAPgetStationNum();
    IPAddress ip = WiFi.softAPIP();
    
    Serial.printf("Dual TX Master - People:%d | D1(E:%d,X:%d) | D2(E:%d,X:%d) | Clients:%d\n", 
                  totalPeopleInside, door1_entries, door1_exits, door2_entries, door2_exits, clients);
    Serial.printf("Sensors: D1E:%s D1X:%s D2E:%s D2X:%s | IR_TX: D1+D2 ON\n",
                  door1_entry.connected ? "ON" : "OFF",
                  door1_exit.connected ? "ON" : "OFF", 
                  door2_entry.connected ? "ON" : "OFF",
                  door2_exit.connected ? "ON" : "OFF");
    Serial.println("Timing: Handled by Flutter app");
    
    lastHeartbeat = currentTime;
  }
  
  // Auto-save every 5 minutes
  if (currentTime - lastSave > 300000) {
    saveData();
    lastSave = currentTime;
  }
  
  // WiFi monitoring
  if (currentTime - lastWiFiCheck > 10000) {
    IPAddress ip = WiFi.softAPIP();
    if (ip == IPAddress(0,0,0,0)) {
      Serial.println("MASTER AP DIED! Recreating...");
      createStableAP();
    }
    lastWiFiCheck = currentTime;
  }
  
  delay(100);
} 