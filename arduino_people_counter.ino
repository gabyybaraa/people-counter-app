#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>

// ====== AP Credentials ======
const char* ssid = "ESP8266-PeopleCounter";
const char* password = "12345678";

// ====== Pin Definitions ======
const int IR_SENSOR_ENTRY = D1;
const int IR_SENSOR_EXIT  = D2;
const int STATUS_LED      = D0;

// ====== People Counter Vars ======
int peopleCount = 0;
bool entryTriggered = false;
bool exitTriggered = false;
bool personDetected = false;
unsigned long lastTriggerTime = 0;
const unsigned long TRIGGER_DELAY = 1000;

// ====== Last Entry/Exit Time Vars ======
unsigned long lastEntryMillis = 0;
unsigned long lastExitMillis = 0;

// NTP Time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 8 * 3600, 60000); // Singapore time (UTC+8), update every 60s
String lastEntryTime = "Never";
String lastExitTime = "Never";

// EEPROM Address
const int EEPROM_ADDRESS = 0;

// ====== Async Web Server ======
AsyncWebServer server(80);

// Add CORS headers to all responses
void addCORSHeaders(AsyncWebServerResponse *response) {
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
  response->addHeader("Access-Control-Max-Age", "3600");
  response->addHeader("Cache-Control", "no-cache");
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(IR_SENSOR_ENTRY, INPUT_PULLUP);
  pinMode(IR_SENSOR_EXIT, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  EEPROM.begin(512);
  loadSavedCount();
  timeClient.begin();

  // ====== Start AP Mode ======
  Serial.println("\n=== ESP8266 People Counter Starting ===");
  
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  bool apResult = WiFi.softAP(ssid, password, 1, 0, 8);
  
  if (apResult) {
    Serial.println("✓ Access Point created successfully!");
  } else {
    Serial.println("✗ Failed to create Access Point!");
  }
  
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Password: %s\n", password);
  Serial.printf("IP Address: %s\n", IP.toString().c_str());
  Serial.printf("Max Clients: 8\n");
  Serial.println("=====================================");

  // ====== CORS Preflight Handler ======
  server.on("/*", HTTP_OPTIONS, [](AsyncWebServerRequest *request){
    Serial.println("CORS Preflight request received");
    AsyncWebServerResponse *response = request->beginResponse(200);
    addCORSHeaders(response);
    request->send(response);
  });

  // ====== Root Page ======
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("Root page requested from: %s\n", clientIP.c_str());
    
    String html = "<!DOCTYPE html>";
    html += "<html><head><title>People Counter ESP8266</title>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<meta http-equiv='refresh' content='3'/>";
    html += "<style>";
    html += "body{font-family:Arial;text-align:center;margin:20px;background:#f0f0f0;}";
    html += ".container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
    html += ".count{font-size:48px;color:#2196F3;margin:20px 0;}";
    html += ".status{color:#4CAF50;margin:10px 0;}";
    html += "a{display:inline-block;margin:10px;padding:10px 20px;background:#2196F3;color:white;text-decoration:none;border-radius:5px;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<h1> People Counter</h1>";
    html += "<div class='count'>" + String(peopleCount) + "</div>";
    html += "<div class='status'>✓ Device Online</div>";
    html += "<p><strong>IP:</strong> " + WiFi.softAPIP().toString() + "</p>";
    html += "<p><strong>Clients:</strong> " + String(WiFi.softAPgetStationNum()) + "</p>";
    html += "<p><strong>Uptime:</strong> " + String(millis()/1000) + "s</p>";
    html += "<div>";
    html += "<a href='/api/test'>Test API</a>";
    html += "<a href='/api/reset'>Reset</a>";
    html += "</div>";
    html += "</div></body></html>";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    addCORSHeaders(response);
    request->send(response);
  });

  // ====== API Endpoints ======
  
  // Test endpoint - Primary endpoint for Flutter app
  server.on("/api/test", HTTP_GET, [](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("API test called from: %s\n", clientIP.c_str());
    
    String json = "{";
    json += "\"status\":\"success\",";
    json += "\"count\":" + String(peopleCount) + ",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"wifi_connected\":true,";
    json += "\"last_entry\":\"" + lastEntryTime + "\",";
    json += "\"last_exit\":\"" + lastExitTime + "\"";
    json += "}";

    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // Count endpoint
  server.on("/api/count", HTTP_GET, [](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("Count requested from: %s (Count: %d)\n", clientIP.c_str(), peopleCount);
    
    String json = "{";
    json += "\"count\":" + String(peopleCount) + ",";
    json += "\"status\":\"success\"";
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // Status endpoint
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("Status requested from: %s\n", clientIP.c_str());
    
    String json = "{";
    json += "\"wifi_connected\":true,";
    json += "\"client_count\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"sensor_entry\":" + String(!digitalRead(IR_SENSOR_ENTRY)) + ",";
    json += "\"sensor_exit\":" + String(!digitalRead(IR_SENSOR_EXIT));
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // Reset endpoint (POST)
  server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("Reset requested via POST from: %s\n", clientIP.c_str());
    
    peopleCount = 0;
    lastEntryMillis = 0;
    lastExitMillis = 0;
    saveCount();
    
    String json = "{";
    json += "\"success\":true,";
    json += "\"count\":0,";
    json += "\"message\":\"Counter reset successfully\"";
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // Reset endpoint (GET for browser)
  server.on("/api/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    Serial.printf("Reset requested via GET from: %s\n", clientIP.c_str());
    
    peopleCount = 0;
    lastEntryMillis = 0;
    lastExitMillis = 0;
    saveCount();
    
    String json = "{";
    json += "\"success\":true,";
    json += "\"count\":0,";
    json += "\"message\":\"Counter reset successfully\"";
    json += "}";
    
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // 404 handler
  server.onNotFound([](AsyncWebServerRequest *request){
    String clientIP = request->client()->remoteIP().toString();
    String url = request->url();
    Serial.printf("404 - %s requested %s\n", clientIP.c_str(), url.c_str());
    
    String json = "{\"error\":\"Not found\",\"url\":\"" + url + "\"}";
    AsyncWebServerResponse *response = request->beginResponse(404, "application/json", json);
    addCORSHeaders(response);
    request->send(response);
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started on port 80!");
  Serial.printf("Access at: http://%s\n", IP.toString().c_str());
  Serial.println("Waiting for Flutter app connections...\n");
}

void loop() {
  static unsigned long lastClientCheck = 0;
  static unsigned long lastStatusPrint = 0;
  unsigned long currentTime = millis();
  timeClient.update();
  
  // Print connected clients every 30 seconds
  if (currentTime - lastClientCheck > 30000) {
    int clientCount = WiFi.softAPgetStationNum();
    Serial.printf("Connected clients: %d\n", clientCount);
    lastClientCheck = currentTime;
  }
  
  // Print status every 60 seconds
  if (currentTime - lastStatusPrint > 60000) {
    Serial.printf("Current count: %d | Free heap: %d | Uptime: %ds\n", 
                  peopleCount, ESP.getFreeHeap(), millis()/1000);
    lastStatusPrint = currentTime;
  }
  
  handlePeopleDetection();
  
  // LED indicates people inside
  digitalWrite(STATUS_LED, peopleCount > 0 ? HIGH : LOW);
}

void handlePeopleDetection() {
  bool entryBlocked = !digitalRead(IR_SENSOR_ENTRY);
  bool exitBlocked  = !digitalRead(IR_SENSOR_EXIT);
  unsigned long currentTime = millis();

  if (currentTime - lastTriggerTime < TRIGGER_DELAY) return;

  if (entryBlocked && !personDetected) {
    entryTriggered = true;
    personDetected = true;
    Serial.println("Entry sensor triggered");
  }

  if (exitBlocked && !personDetected) {
    exitTriggered = true;
    personDetected = true;
    Serial.println("Exit sensor triggered");
  }

  if (!entryBlocked && !exitBlocked && personDetected) {
    if (entryTriggered) {
      peopleCount++;
      lastEntryMillis = millis();
      if (timeClient.isTimeSet()) {
        unsigned long epochTime = timeClient.getEpochTime();
        setTime(epochTime);
        char dateTimeStr[20];
        sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
        lastEntryTime = String(dateTimeStr);
      }
      Serial.printf("Person entered! New count: %d\n", peopleCount);
      saveCount();
    }

    if (exitTriggered) {
      if (peopleCount > 0) {
        peopleCount--;
        lastExitMillis = millis();
        if (timeClient.isTimeSet()) {
          unsigned long epochTime = timeClient.getEpochTime();
          setTime(epochTime);
          char dateTimeStr[20];
          sprintf(dateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
          lastExitTime = String(dateTimeStr);
        }
        Serial.printf("\u2B05 Person exited! New count: %d\n", peopleCount);
        saveCount();
      } else {
        Serial.println("Exit detected but count already 0");
      }
    }

    entryTriggered = false;
    exitTriggered = false;
    personDetected = false;
    lastTriggerTime = currentTime;
  }
}

void loadSavedCount() {
  EEPROM.get(EEPROM_ADDRESS, peopleCount);
  if (peopleCount < 0 || peopleCount > 9999) {
    peopleCount = 0;
    saveCount();
  }
  Serial.printf("Loaded count from EEPROM: %d\n", peopleCount);
}

void saveCount() {
  EEPROM.put(EEPROM_ADDRESS, peopleCount);
  EEPROM.commit();
} 