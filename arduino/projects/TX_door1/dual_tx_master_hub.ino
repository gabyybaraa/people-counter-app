#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

// Dual TX Master Hub - Bidirectional People Counter
// Controls IR transmitters and provides web interface

#define Device_ID "TX_dual_master_hub"
#define System_Version "2.0_Bidirectional_Dual"

// WiFi Configuration
const char* wifi_ssid = "ESP-Counter";
const char* wifi_password = "12345678";
const char* ap_ssid = "ESP-Counter-AP";
const char* ap_password = "12345678";

// Pin Definitions
const int IR_LED_DOOR1_ENTRY = D1;    // Door 1 Entry IR LED
const int IR_LED_DOOR1_EXIT = D2;     // Door 1 Exit IR LED

// People counting variables
int door1_entries = 0;
int door1_exits = 0;
int door2_entries = 0;
int door2_exits = 0;
int totalPeopleInside = 0;

// Timing variables
unsigned long lastUpdate = 0;
unsigned long lastWebUpdate = 0;
unsigned long lastSerialUpdate = 0;

// WiFi and server
AsyncWebServer server(80);
bool wifiConnected = false;
int clients = 0;

// Door sensor connection tracking
struct DoorSensor {
  bool connected;
  String lastActivity;
  unsigned long lastSeen;
};

DoorSensor door1_entry = {false, "Never", 0};
DoorSensor door1_exit = {false, "Never", 0};
DoorSensor door2_entry = {false, "Never", 0};
DoorSensor door2_exit = {false, "Never", 0};

// Time tracking
String door1_last_entry_time = "Never";
String door1_last_exit_time = "Never";
String door2_last_entry_time = "Never";
String door2_last_exit_time = "Never";

void setupIRTransmitters() {
  pinMode(IR_LED_DOOR1_ENTRY, OUTPUT);
  pinMode(IR_LED_DOOR1_EXIT, OUTPUT);
  
  digitalWrite(IR_LED_DOOR1_ENTRY, HIGH);  // Turn on Door 1 Entry IR LED
  digitalWrite(IR_LED_DOOR1_EXIT, HIGH);   // Turn on Door 1 Exit IR LED
  
  Serial.println("Door 1 Entry IR Transmitter ON at D1");
  Serial.println("Door 1 Exit IR Transmitter ON at D2");
  Serial.println("Dual IR transmission active for Door 1");
}

void setupWiFi() {
  // Try to connect to existing network first
  WiFi.begin(wifi_ssid, wifi_password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\nConnected to WiFi: %s\n", wifi_ssid);
    Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    // Create access point if connection fails
    Serial.println("\nWiFi connection failed. Creating access point...");
    WiFi.softAP(ap_ssid, ap_password);
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("Access Point created: %s\n", ap_ssid);
    Serial.printf("AP IP: %s\n", ip.toString().c_str());
    wifiConnected = false;
  }
}

void setupWebServer() {
  // API endpoint for sensor updates
  server.on("/api/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    String response = "{\"status\":\"received\"}";
    
    if (request->hasParam("sensor", true) && request->hasParam("action", true)) {
      String sensor = request->getParam("sensor", true)->value();
      String action = request->getParam("action", true)->value();
      String device = request->getParam("device", true)->value();
      
      // Update sensor connection status
      if (sensor == "door1_entry") {
        door1_entry.connected = true;
        door1_entry.lastSeen = millis();
        if (action == "entry") {
          door1_entries++;
          totalPeopleInside++;
          door1_last_entry_time = getCurrentTime();
        }
      } else if (sensor == "door1_exit") {
        door1_exit.connected = true;
        door1_exit.lastSeen = millis();
        if (action == "exit") {
          door1_exits++;
          totalPeopleInside = max(0, totalPeopleInside - 1);
          door1_last_exit_time = getCurrentTime();
        }
      } else if (sensor == "door2_entry") {
        door2_entry.connected = true;
        door2_entry.lastSeen = millis();
        if (action == "entry") {
          door2_entries++;
          totalPeopleInside++;
          door2_last_entry_time = getCurrentTime();
        }
      } else if (sensor == "door2_exit") {
        door2_exit.connected = true;
        door2_exit.lastSeen = millis();
        if (action == "exit") {
          door2_exits++;
          totalPeopleInside = max(0, totalPeopleInside - 1);
          door2_last_exit_time = getCurrentTime();
        }
      }
      
      // Update last activity
      if (request->hasParam("activity_time", true)) {
        String activityTime = request->getParam("activity_time", true)->value();
        if (sensor == "door1_entry") door1_entry.lastActivity = activityTime;
        else if (sensor == "door1_exit") door1_exit.lastActivity = activityTime;
        else if (sensor == "door2_entry") door2_entry.lastActivity = activityTime;
        else if (sensor == "door2_exit") door2_exit.lastActivity = activityTime;
      }
      
      Serial.printf("Received: %s %s from %s\n", sensor.c_str(), action.c_str(), device.c_str());
    }
    
    request->send(200, "application/json", response);
  });
  
  // Main dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = generateDashboardHTML();
    request->send(200, "text/html", html);
  });
  
  // Reset counters
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    door1_entries = 0;
    door1_exits = 0;
    door2_entries = 0;
    door2_exits = 0;
    totalPeopleInside = 0;
    request->redirect("/");
  });
  
  server.begin();
  Serial.println("Web server started");
}

String generateDashboardHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>People Counter Dashboard</title>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;margin:20px;background:#f5f5f5;}";
  html += ".container{max-width:1200px;margin:0 auto;}";
  html += ".header{text-align:center;margin-bottom:30px;}";
  html += ".stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;margin-bottom:30px;}";
  html += ".stat-card{background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
  html += ".stat-card h3{margin-top:0;color:#333;}";
  html += ".total-people{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:white;text-align:center;}";
  html += ".total-people h2{font-size:3em;margin:10px 0;}";
  html += ".door-section{background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);margin-bottom:20px;}";
  html += ".door-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;}";
  html += ".entry{color:#2ecc71;}";
  html += ".exit{color:#e74c3c;}";
  html += ".controls{text-align:center;margin:20px 0;}";
  html += ".btn{background:#3498db;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px;}";
  html += ".btn:hover{background:#2980b9;}";
  html += ".status{font-size:12px;color:#666;}";
  html += "</style>";
  html += "<script>";
  html += "setInterval(function(){location.reload();},5000);";
  html += "</script></head><body>";
  
  html += "<div class='container'>";
  html += "<div class='header'>";
  html += "<h1>People Counter Dashboard</h1>";
  html += "<p>Real-time monitoring of entry/exit counts</p>";
  html += "</div>";
  
  // Total people inside
  html += "<div class='stat-card total-people'>";
  html += "<h2>" + String(totalPeopleInside) + "</h2>";
  html += "<p>People Currently Inside</p>";
  html += "</div>";
  
  // Statistics grid
  html += "<div class='stats-grid'>";
  html += "<div class='stat-card'>";
  html += "<h3>Door 1</h3>";
  html += "<p><strong>Entries:</strong> " + String(door1_entries) + "</p>";
  html += "<p><strong>Exits:</strong> " + String(door1_exits) + "</p>";
  html += "<p><strong>Net:</strong> " + String(door1_entries - door1_exits) + "</p>";
  html += "</div>";
  
  html += "<div class='stat-card'>";
  html += "<h3>Door 2</h3>";
  html += "<p><strong>Entries:</strong> " + String(door2_entries) + "</p>";
  html += "<p><strong>Exits:</strong> " + String(door2_exits) + "</p>";
  html += "<p><strong>Net:</strong> " + String(door2_entries - door2_exits) + "</p>";
  html += "</div>";
  
  html += "<div class='stat-card'>";
  html += "<h3>System Info</h3>";
  html += "<p><strong>WiFi:</strong> " + String(wifiConnected ? "Connected" : "Access Point") + "</p>";
  html += "<p><strong>IP:</strong> " + WiFi.localIP().toString() + "</p>";
  html += "<p><strong>Uptime:</strong> " + String(millis()/1000/60) + " min</p>";
  html += "</div>";
  html += "</div>";
  
  // Door details
  html += "<div class='door-section'>";
  html += "<h3>Door 1 Details</h3>";
  html += "<div class='door-grid'>";
  
  html += "<div class='entry'>";
  html += "<h4 style='color:#2ecc71;margin:0;'> Entry Side</h4>";
  html += "<p style='margin:5px 0;'>Count: " + String(door1_entries) + "</p>";
  html += "<p style='margin:5px 0;'>Status: " + String(door1_entry.connected ? "Online" : "Offline") + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door1_entry.lastActivity + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;color:#2ecc71;'>Time: " + door1_last_entry_time + "</p>";
  html += "</div>";
  
  html += "<div class='exit'>";
  html += "<h4 style='color:#e74c3c;margin:0;'> Exit Side</h4>";
  html += "<p style='margin:5px 0;'>Count: " + String(door1_exits) + "</p>";
  html += "<p style='margin:5px 0;'>Status: " + String(door1_exit.connected ? "Online" : "Offline") + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door1_exit.lastActivity + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;color:#e74c3c;'>Time: " + door1_last_exit_time + "</p>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='door-section'>";
  html += "<h3>Door 2 Details</h3>";
  html += "<div class='door-grid'>";
  
  html += "<div class='entry'>";
  html += "<h4 style='color:#2ecc71;margin:0;'> Entry Side</h4>";
  html += "<p style='margin:5px 0;'>Count: " + String(door2_entries) + "</p>";
  html += "<p style='margin:5px 0;'>Status: " + String(door2_entry.connected ? "Online" : "Offline") + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door2_entry.lastActivity + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;color:#2ecc71;'>Time: " + door2_last_entry_time + "</p>";
  html += "</div>";
  
  html += "<div class='exit'>";
  html += "<h4 style='color:#e74c3c;margin:0;'> Exit Side</h4>";
  html += "<p style='margin:5px 0;'>Count: " + String(door2_exits) + "</p>";
  html += "<p style='margin:5px 0;'>Status: " + String(door2_exit.connected ? "Online" : "Offline") + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;'>Last: " + door2_exit.lastActivity + "</p>";
  html += "<p style='margin:5px 0;font-size:12px;color:#e74c3c;'>Time: " + door2_last_exit_time + "</p>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  // Controls
  html += "<div class='controls'>";
  html += "<a href='/reset' class='btn'>Reset All Counters</a>";
  html += "</div>";
  
  html += "</div></body></html>";
  return html;
}

String getCurrentTime() {
  unsigned long seconds = millis() / 1000;
  int hours = (seconds / 3600) % 24;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  return String(hours) + ":" + 
         (minutes < 10 ? "0" : "") + String(minutes) + ":" +
         (secs < 10 ? "0" : "") + String(secs);
}

void checkSensorConnections() {
  unsigned long currentTime = millis();
  
  // Check if sensors are still connected (5 minute timeout)
  if (currentTime - door1_entry.lastSeen > 300000) door1_entry.connected = false;
  if (currentTime - door1_exit.lastSeen > 300000) door1_exit.connected = false;
  if (currentTime - door2_entry.lastSeen > 300000) door2_entry.connected = false;
  if (currentTime - door2_exit.lastSeen > 300000) door2_exit.connected = false;
}

void setup() {
  Serial.begin(9600);
  delay(3000);
  
  Serial.println("\nDUAL TX MASTER HUB - BIDIRECTIONAL PEOPLE COUNTER");
  Serial.println("==================================================");
  Serial.printf("Device: %s\n", Device_ID);
  Serial.printf("Version: %s\n", System_Version);
  Serial.printf("Function: Master Hub + Door 1 IR Transmitter\n");
  Serial.printf("Door 1 Entry TX: D1 (GPIO5)\n");
  Serial.printf("Door 1 Exit TX: D2 (GPIO4)\n");
  Serial.printf("WiFi Network: %s\n", wifi_ssid);
  Serial.printf("Access Point: %s\n", ap_ssid);
  Serial.println("==================================================");
  
  // Setup IR transmitters
  setupIRTransmitters();
  
  // Setup WiFi
  setupWiFi();
  
  // Setup web server
  setupWebServer();
  
  Serial.println("Dual TX Master Hub ready!");
  Serial.println("IR transmission active for Door 1");
  Serial.println("Web dashboard available at device IP");
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Keep Door 1 IR LEDs on continuously
  digitalWrite(IR_LED_DOOR1_ENTRY, HIGH);
  digitalWrite(IR_LED_DOOR1_EXIT, HIGH);
  
  // Check sensor connections
  checkSensorConnections();
  
  // Update client count
  clients = WiFi.softAPgetStationNum();
  
  // Status updates every 60 seconds
  if (currentTime - lastSerialUpdate > 60000) {
    IPAddress ip = WiFi.softAPIP();
    
    Serial.printf("Dual TX Master - People:%d | D1(E:%d,X:%d) | D2(E:%d,X:%d) | Clients:%d\n", 
                  totalPeopleInside, door1_entries, door1_exits, door2_entries, door2_exits, clients);
    Serial.printf("Sensors: D1E:%s D1X:%s D2E:%s D2X:%s | IR_TX: D1+D2 ON\n",
                  door1_entry.connected ? "ON" : "OFF",
                  door1_exit.connected ? "ON" : "OFF",
                  door2_entry.connected ? "ON" : "OFF",
                  door2_exit.connected ? "ON" : "OFF");
    Serial.printf("Times - D1E:%s D1X:%s D2E:%s D2X:%s\n",
                  door1_last_entry_time.c_str(), door1_last_exit_time.c_str(),
                  door2_last_entry_time.c_str(), door2_last_exit_time.c_str());
    
    lastSerialUpdate = currentTime;
  }
  
  delay(1000);  // 1 second loop delay
} 