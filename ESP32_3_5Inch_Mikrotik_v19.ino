/**
 * Mikrotik Router Status Display for ESP32 and 3.5" TFT (480x320)
 * Enhanced with Auto-Hotspot, Splash Screen, Web Interface, Backlight Control & OTA
 * Version: 2.3 - Fixed Dark Mode, Independent Form Sections, Backlight Persistence
 */
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ElegantOTA.h>
#include <map>
#include <stdlib.h>
#include "web_interface.h"

#define SMALL_FONT "fonts/NSBold15"
#define LARGE_FONT "fonts/NSBold36"

// --- Graph Constants ---
uint64_t FIXED_MAX_MBPS = 480;
uint64_t FIXED_MIN_MBPS = 0;
uint64_t FIXED_MAX_BPS = FIXED_MAX_MBPS * 1024ULL * 1024;
uint64_t FIXED_MIN_BPS = FIXED_MIN_MBPS * 1024ULL * 1024;
const int LINE_THICKNESS = 3;

// Graph Drawing Parameters
const int GRAPH_X = 60;
const int GRAPH_Y = 108;
const int GRAPH_W = 395;
const int GRAPH_H = 150;
const int GRAPH_BOTTOM = GRAPH_Y + GRAPH_H;
const int GRAPH_INNER_H = GRAPH_H - LINE_THICKNESS + 1;

#define GRAPH_COLOR_TX TFT_BLUE
#define GRAPH_COLOR_RX TFT_YELLOW

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite graphSprite = TFT_eSprite(&tft);
TFT_eSprite gaugeSprite = TFT_eSprite(&tft);
static bool sprite_created = false;
static bool gauge_sprite_created = false;

static const int SCREEN_WIDTH = 480;
static const int SCREEN_HEIGHT = 320;
#define HISTORY_SIZE 40

// WiFi Configuration
Preferences preferences;
AsyncWebServer server(80);
String wifi_ssid = "";
String wifi_password = "";
String router_address = "";
String router_login = "";
String router_password = "";
int graph_interface_id = 2;
int backlight_brightness = 100; // 0-100%

// Hotspot Configuration
String hotspot_ssid = "";
const char* hotspot_password = "admin";
bool hotspot_mode = false;
unsigned long last_wifi_retry = 0;
const unsigned long WIFI_RETRY_INTERVAL = 300000; // 5 minutes

char routerTimeStr[16] = "00:00:00";
char routerDateStr[20] = "01-Jan-1970";
int routerCurrentMinute = -1;

static float last_ram_percent = -1.0f;
static float last_rx_percent = -1.0f;
static bool graph_static_elements_drawn = false;
static int last_minute = -1;

typedef struct {
  uint64_t rx_hour;
  uint64_t rx_day;
  uint64_t rx_week;
  uint64_t rx_month;
  long last_update_ts;
} rx_totals_t;

static rx_totals_t rx_totals = {0, 0, 0, 0, 0};
static unsigned long hour_start_time = 0;
static unsigned long day_start_time = 0;
static unsigned long week_start_time = 0;
static unsigned long month_start_time = 0;
static uint64_t hour_start_bytes = 0;
static uint64_t day_start_bytes = 0;
static uint64_t week_start_bytes = 0;
static uint64_t month_start_bytes = 0;

typedef struct {
  uint64_t rx;
  uint64_t tx;
  uint64_t time;
  int pos;
  uint64_t hist_rx[HISTORY_SIZE];
  uint64_t hist_tx[HISTORY_SIZE];
} mt_data_t;

typedef struct {
  uint32_t uptime;
  uint32_t memoryFree;
  uint32_t memoryTotal;
  float cpuLoad;
} router_info_t;

std::map<int, mt_data_t*> ifaces;
router_info_t routerInfo = {0, 0, 0, 0.0f};

// ==================== FORWARD DECLARATIONS ====================

void loadPreferences();
void savePreferences(String ssid, String pass, String rtr_addr, String rtr_user, String rtr_pass, int iface_id);
String generateHotspotSSID();
void startHotspot();
void tryWiFiConnection();
void setupWebServer();
void showSplashScreen();
void setBacklight(int brightness);
int id2int(const char* s);
uint32_t parseUptimeToSeconds(const String& s);
uint32_t parseMemoryToBytes(const String& s);
void formatUptime(uint32_t sec, char* buf, size_t len);
String convertDateFormat(const char* mt_date);
void fetchRouterInfo();
void fetchTimeFromRouter();
void loadRxTotals();
void resetRxTotals();
void saveRxTotals();
void updateRxTotals(uint64_t current_rx_bytes);
void draw_thick_line_sprite(TFT_eSprite &spr, int x0, int y0, int x1, int y1, uint16_t color, int thickness);
void drawGauge(int x, int y, int radius, int thickness, float valuePercent, const char* label);
void initGraphSprite();
void initGaugeSprite();
String scanWiFiNetworks();

// ==================== PREFERENCES FUNCTIONS ====================

void loadPreferences() {
  preferences.begin("wifi-config", true);
  wifi_ssid = preferences.getString("ssid", "");
  wifi_password = preferences.getString("password", "");
  router_address = preferences.getString("router_addr", "http://192.168.1.1");
  router_login = preferences.getString("router_user", "APIUser");
  router_password = preferences.getString("router_pass", "myleetkey");
  graph_interface_id = preferences.getInt("interface_id", 2);
  backlight_brightness = preferences.getInt("backlight", 100);
  FIXED_MAX_MBPS = preferences.getUInt("max_mbps", 480);
  FIXED_MIN_MBPS = preferences.getUInt("min_mbps", 0);
  preferences.end();
  
  // Recalculate BPS values
  FIXED_MAX_BPS = FIXED_MAX_MBPS * 1024ULL * 1024;
  FIXED_MIN_BPS = FIXED_MIN_MBPS * 1024ULL * 1024;
  
  Serial.println("=== Loaded Configuration ===");
  Serial.println("WiFi SSID: " + (wifi_ssid.length() > 0 ? wifi_ssid : "(none)"));
  Serial.println("Router: " + router_address);
  Serial.println("Router User: " + router_login);
  Serial.println("Interface ID: " + String(graph_interface_id));
  Serial.println("Backlight: " + String(backlight_brightness) + "%");
  Serial.println("Graph Max: " + String((uint32_t)FIXED_MAX_MBPS) + " Mbps");
  Serial.println("Graph Min: " + String((uint32_t)FIXED_MIN_MBPS) + " Mbps");
}

void savePreferences(String ssid, String pass, String rtr_addr, String rtr_user, String rtr_pass, int iface_id) {
  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", pass);
  preferences.putString("router_addr", rtr_addr);
  preferences.putString("router_user", rtr_user);
  preferences.putString("router_pass", rtr_pass);
  preferences.putInt("interface_id", iface_id);
  preferences.putInt("backlight", backlight_brightness);
  preferences.putUInt("max_mbps", (uint32_t)FIXED_MAX_MBPS);
  preferences.putUInt("min_mbps", (uint32_t)FIXED_MIN_MBPS);
  preferences.end();
  
  Serial.println("=== Configuration Saved ===");
  Serial.println("SSID: " + ssid);
  Serial.println("Router: " + rtr_addr);
  Serial.println("Backlight: " + String(backlight_brightness) + "%");
}

// ==================== BACKLIGHT CONTROL ====================

void setBacklight(int brightness) {
  brightness = constrain(brightness, 0, 100);
  backlight_brightness = brightness;
  
#ifdef TFT_BL
  int pwm_value = map(brightness, 0, 100, 0, 255);
  analogWrite(TFT_BL, pwm_value);
  Serial.printf("Backlight set to %d%% (PWM: %d)\n", brightness, pwm_value);
#endif
}

// ==================== WIFI SCANNING ====================

String scanWiFiNetworks() {
  Serial.println("Scanning WiFi networks...");
  int n = WiFi.scanNetworks();
  
  DynamicJsonDocument doc(4096);
  JsonArray networks = doc.createNestedArray("networks");
  
  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.printf("Found %d networks\n", n);
    for (int i = 0; i < n && i < 20; ++i) {
      JsonObject network = networks.createNestedObject();
      network["ssid"] = WiFi.SSID(i);
      network["rssi"] = WiFi.RSSI(i);
      network["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";
      
      int rssi = WiFi.RSSI(i);
      if (rssi > -50) network["strength"] = "Excellent";
      else if (rssi > -60) network["strength"] = "Good";
      else if (rssi > -70) network["strength"] = "Fair";
      else network["strength"] = "Weak";
    }
  }
  
  String output;
  serializeJson(doc, output);
  WiFi.scanDelete();
  return output;
}

// ==================== HOTSPOT FUNCTIONS ====================

String generateHotspotSSID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "Mikrotik_Display-%02X%02X", mac[4], mac[5]);
  return String(ssid);
}

void startHotspot() {
  hotspot_mode = true;
  WiFi.mode(WIFI_AP);
  hotspot_ssid = generateHotspotSSID();
  
  bool success = WiFi.softAP(hotspot_ssid.c_str(), hotspot_password);
  if (!success) {
    Serial.println("ERROR: Failed to start hotspot!");
    return;
  }
  
  IPAddress IP = WiFi.softAPIP();
  
  Serial.println("=== Hotspot Mode ===");
  Serial.println("SSID: " + hotspot_ssid);
  Serial.println("Password: " + String(hotspot_password));
  Serial.println("IP: " + IP.toString());
}

void tryWiFiConnection() {
  if (wifi_ssid.length() == 0) {
    Serial.println("No WiFi credentials saved");
    return;
  }
  
  Serial.println("Attempting WiFi connection to: " + wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    hotspot_mode = false;
    Serial.println("✓ WiFi Connected!");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("✗ WiFi connection failed");
  }
}

// ==================== WEB SERVER FUNCTIONS ====================

void setupWebServer() {
  // Main page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", html_page);
  });
  
  // WiFi scan endpoint
  server.on("/api/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    String result = scanWiFiNetworks();
    request->send(200, "application/json", result);
  });
  
  // Get interfaces
  server.on("/api/interfaces", HTTP_GET, [](AsyncWebServerRequest *request){
    if (hotspot_mode || router_address.length() == 0) {
      request->send(200, "application/json", "{\"interfaces\":[]}");
      return;
    }
    
    String url = router_address + "/rest/interface/ethernet/print";
    String q = "{\".proplist\": \".id,name\"}";
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(url);
    http.setAuthorization(router_login.c_str(), router_password.c_str());
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(q);
    
    if (code == 200) {
      String resp = http.getString();
      DynamicJsonDocument inDoc(2048);
      DeserializationError error = deserializeJson(inDoc, resp);
      
      if (!error && inDoc.is<JsonArray>()) {
        DynamicJsonDocument outDoc(2048);
        JsonArray interfaces = outDoc.createNestedArray("interfaces");
        
        for (JsonVariant item : inDoc.as<JsonArray>()) {
          const char* sid = item[".id"] | "";
          const char* name = item["name"] | "";
          if (sid && sid[0] != '\0') {
            int id = id2int(sid);
            if (id > 0) {
              JsonObject iface = interfaces.createNestedObject();
              iface["id"] = id;
              iface["name"] = name;
            }
          }
        }
        
        String response;
        serializeJson(outDoc, response);
        request->send(200, "application/json", response);
      } else {
        request->send(200, "application/json", "{\"interfaces\":[]}");
      }
    } else {
      request->send(200, "application/json", "{\"interfaces\":[]}");
    }
    http.end();
  });
  
  // Save WiFi configuration only
  server.on("/save-wifi", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      String ssid = doc["ssid"].as<String>();
      String pass = doc["password"].as<String>();
      
      if (ssid.length() == 0) {
        request->send(400, "application/json", "{\"error\":\"SSID required\"}");
        return;
      }
      
      wifi_ssid = ssid;
      wifi_password = pass;
      
      preferences.begin("wifi-config", false);
      preferences.putString("ssid", ssid);
      preferences.putString("password", pass);
      preferences.end();
      
      Serial.println("WiFi settings saved");
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      
      delay(1000);
      ESP.restart();
    });
  
  // Save router configuration only
  server.on("/save-router", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      String rtr_addr = doc["router_addr"].as<String>();
      String rtr_user = doc["router_user"].as<String>();
      String rtr_pass = doc["router_pass"].as<String>();
      int iface_id = doc["interface_id"] | graph_interface_id;
      
      if (rtr_addr.length() == 0) {
        request->send(400, "application/json", "{\"error\":\"Router address required\"}");
        return;
      }
      
      router_address = rtr_addr;
      router_login = rtr_user;
      router_password = rtr_pass;
      graph_interface_id = iface_id;
      
      preferences.begin("wifi-config", false);
      preferences.putString("router_addr", rtr_addr);
      preferences.putString("router_user", rtr_user);
      preferences.putString("router_pass", rtr_pass);
      preferences.putInt("interface_id", iface_id);
      preferences.end();
      
      Serial.println("Router settings saved");
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      
      delay(1000);
      ESP.restart();
    });
  
  // Save graph settings only
  server.on("/save-graph", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      if (doc.containsKey("max_mbps")) {
        FIXED_MAX_MBPS = doc["max_mbps"].as<uint32_t>();
        FIXED_MAX_BPS = FIXED_MAX_MBPS * 1024ULL * 1024;
      }
      
      if (doc.containsKey("min_mbps")) {
        FIXED_MIN_MBPS = doc["min_mbps"].as<uint32_t>();
        FIXED_MIN_BPS = FIXED_MIN_MBPS * 1024ULL * 1024;
      }
      
      preferences.begin("wifi-config", false);
      preferences.putUInt("max_mbps", (uint32_t)FIXED_MAX_MBPS);
      preferences.putUInt("min_mbps", (uint32_t)FIXED_MIN_MBPS);
      preferences.end();
      
      Serial.println("Graph settings saved");
      request->send(200, "application/json", "{\"status\":\"ok\"}");
      
      delay(1000);
      ESP.restart();
    });
  
  // Get current config
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    doc["ssid"] = wifi_ssid;
    doc["router_addr"] = router_address;
    doc["router_user"] = router_login;
    doc["interface_id"] = graph_interface_id;
    doc["backlight"] = backlight_brightness;
    doc["max_mbps"] = (uint32_t)FIXED_MAX_MBPS;
    doc["min_mbps"] = (uint32_t)FIXED_MIN_MBPS;
    
    preferences.begin("wifi-config", true);
    String theme = preferences.getString("theme", "light");
    preferences.end();
    doc["theme"] = theme;
    
    if (WiFi.status() == WL_CONNECTED) {
      doc["ip"] = WiFi.localIP().toString();
    } else if (hotspot_mode) {
      doc["ip"] = WiFi.softAPIP().toString();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // Get stats
  server.on("/api/stats", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(512);
    
    uint32_t usedMB = 0;
    uint32_t totalMB = 0;
    if (routerInfo.memoryTotal > 0) {
      usedMB = (routerInfo.memoryTotal - routerInfo.memoryFree) / 1024 / 1024;
      totalMB = routerInfo.memoryTotal / 1024 / 1024;
    }
    
    doc["cpu"] = String(routerInfo.cpuLoad, 0);
    
    char ramBuf[32];
    snprintf(ramBuf, sizeof(ramBuf), "%u/%u MB (%.0f%%)", 
             usedMB, totalMB, 
             totalMB > 0 ? (usedMB * 100.0 / totalMB) : 0.0);
    doc["ram"] = String(ramBuf);
    
    mt_data_t* iface = ifaces.count(graph_interface_id) ? ifaces[graph_interface_id] : nullptr;
    int lastIdx = iface ? ((iface->pos == 0) ? HISTORY_SIZE - 1 : iface->pos - 1) : 0;
    double rx_mbps = iface ? iface->hist_rx[lastIdx] / 1024.0 / 1024.0 : 0.0;
    double tx_mbps = iface ? iface->hist_tx[lastIdx] / 1024.0 / 1024.0 : 0.0;
    
    doc["rx"] = String(rx_mbps, 2);
    doc["tx"] = String(tx_mbps, 2);
    
    if (WiFi.status() == WL_CONNECTED) {
      doc["ip"] = WiFi.localIP().toString();
    } else if (hotspot_mode) {
      doc["ip"] = WiFi.softAPIP().toString();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // Set backlight
  server.on("/api/backlight", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<128> doc;
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      int brightness = doc["brightness"] | backlight_brightness;
      setBacklight(brightness);
      
      preferences.begin("wifi-config", false);
      preferences.putInt("backlight", backlight_brightness);
      preferences.end();
      
      Serial.println("Backlight saved: " + String(backlight_brightness) + "%");
      
      String response = "{\"status\":\"ok\",\"brightness\":" + String(backlight_brightness) + "}";
      request->send(200, "application/json", response);
    });
  
  // Save theme preference
  server.on("/api/theme", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      StaticJsonDocument<128> doc;
      DeserializationError error = deserializeJson(doc, (const char*)data);
      
      if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }
      
      String theme = doc["theme"].as<String>();
      
      preferences.begin("wifi-config", false);
      preferences.putString("theme", theme);
      preferences.end();
      
      Serial.println("Theme saved: " + theme);
      
      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
  
  ElegantOTA.begin(&server);
  
  server.begin();
  Serial.println("✓ Web server started");
  Serial.println("✓ ElegantOTA initialized at /update");
}

// ==================== SPLASH SCREEN ====================

void showSplashScreen() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN);
  tft.loadFont(LARGE_FONT, LittleFS);
  tft.drawCentreString("Mikrotik", SCREEN_WIDTH / 2, 40, 0);
  tft.setTextColor(TFT_YELLOW);
  tft.drawCentreString("Data Display", SCREEN_WIDTH / 2, 85, 0);
  tft.unloadFont();
  
  tft.fillRect(100, 130, 280, 3, TFT_BLUE);
  
  tft.loadFont(SMALL_FONT, LittleFS);
  
  if (hotspot_mode) {
    tft.setTextColor(TFT_ORANGE);
    tft.drawCentreString("CONFIGURATION MODE", SCREEN_WIDTH / 2, 160, 0);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Connect to WiFi:", 80, 190, 0);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(hotspot_ssid, 80, 210, 0);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Password:", 80, 235, 0);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(hotspot_password, 200, 235, 0);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Then browse to:", 80, 260, 0);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("http://192.168.4.1", 80, 280, 0);
    
  } else {
    tft.setTextColor(TFT_GREEN);
    tft.drawCentreString("CONNECTED", SCREEN_WIDTH / 2, 160, 0);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Network:", 80, 190, 0);
    tft.setTextColor(TFT_YELLOW);
    String displaySSID = wifi_ssid.substring(0, 25);
    tft.drawString(displaySSID, 80, 210, 0);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("IP Address:", 80, 235, 0);
    tft.setTextColor(TFT_CYAN);
    tft.drawString(WiFi.localIP().toString(), 80, 255, 0);
    
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Configure at:", 80, 280, 0);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("http://" + WiFi.localIP().toString(), 80, 300, 0);
  }
  
  tft.unloadFont();
  
  Serial.println("Splash screen displayed for 10 seconds");
  delay(10000);
}

// ==================== UTILITY FUNCTIONS ====================

int id2int(const char* s) { 
  if (!s || s[0] != '*') return 0;
  return strtol(s + 1, nullptr, 16); 
}

uint32_t parseUptimeToSeconds(const String& s) {
  String str = s;
  str.trim();
  uint32_t total = 0;
  int pos = 0;
  while (pos < str.length()) {
    int alpha = -1;
    for (int i = pos; i < str.length(); i++) {
      if (isalpha(str[i])) {
        alpha = i;
        break;
      }
    }
    if (alpha < 0) break;
    uint32_t n = str.substring(pos, alpha).toInt();
    char unit = str[alpha];
    switch (unit) {
      case 'w': total += n * 7 * 24 * 3600; break;
      case 'd': total += n * 24 * 3600; break;
      case 'h': total += n * 3600; break;
      case 'm': total += n * 60; break;
      case 's': total += n; break;
    }
    pos = alpha + 1;
  }
  return total;
}

uint32_t parseMemoryToBytes(const String& s) {
  String str = s;
  str.trim();
  if (str.endsWith("MiB")) return (uint32_t)(str.toFloat() * 1024.0 * 1024.0);
  if (str.endsWith("KiB")) return (uint32_t)(str.toFloat() * 1024.0);
  return (uint32_t)str.toInt();
}

void formatUptime(uint32_t sec, char* buf, size_t len) {
  if (sec == 0) {
    strncpy(buf, "N/A", len);
    buf[len - 1] = '\0';
    return;
  }
  uint32_t d = sec / 86400;
  sec %= 86400;
  uint32_t h = sec / 3600;
  sec %= 3600;
  uint32_t m = sec / 60;
  snprintf(buf, len, "%ud %uh %um", d, h, m);
}

String convertDateFormat(const char* mt_date) {
  if (!mt_date) return "01-Jan-1970";
  
  char month[4], day[3], year[5];
  static char formatted[20];
  
  if (sscanf(mt_date, "%3[A-Za-z]/%2[0-9]/%4[0-9]", month, day, year) == 3) {
    snprintf(formatted, sizeof(formatted), "%s-%s-%s", day, month, year);
    return String(formatted);
  }
  
  if (sscanf(mt_date, "%4[0-9]-%2[0-9]-%2[0-9]", year, month, day) == 3) {
    const char* monthNames[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int monthNum = atoi(month);
    if (monthNum >= 1 && monthNum <= 12) {
      snprintf(formatted, sizeof(formatted), "%s-%s-%s", day, monthNames[monthNum], year);
      return String(formatted);
    }
  }
  
  return String(mt_date);
}

// ==================== ROUTER DATA FUNCTIONS ====================

void fetchRouterInfo() {
  if (hotspot_mode || router_address.length() == 0) return;
  
  String url = router_address + "/rest/system/resource/print";
  HTTPClient http;
  http.setTimeout(5000);
  http.begin(url);
  http.setAuthorization(router_login.c_str(), router_password.c_str());

  StaticJsonDocument<128> q;
  JsonArray arr = q.createNestedArray(".proplist");
  arr.add("uptime");
  arr.add("cpu-load");
  arr.add("free-memory");
  arr.add("total-memory");
  
  String body;
  serializeJson(q, body);

  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  
  if (code == 200) {
    String resp = http.getString();
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, resp);
    
    if (error) {
      Serial.println("Router info parse error: " + String(error.c_str()));
    } else if (doc.is<JsonArray>() && doc.size() > 0) {
      JsonObject o = doc[0].as<JsonObject>();
      routerInfo.uptime = parseUptimeToSeconds(o["uptime"] | "0s");
      routerInfo.cpuLoad = String(o["cpu-load"] | "0%").toFloat();
      routerInfo.memoryFree = parseMemoryToBytes(o["free-memory"] | "0");
      routerInfo.memoryTotal = parseMemoryToBytes(o["total-memory"] | "0");
    }
  } else if (code > 0) {
    Serial.printf("Router info HTTP error: %d\n", code);
  }
  http.end();
}

void fetchTimeFromRouter() {
  if (hotspot_mode || router_address.length() == 0) return;
  
  String url = router_address + "/rest/system/clock/print";
  HTTPClient http;
  http.setTimeout(5000);
  http.begin(url);
  http.setAuthorization(router_login.c_str(), router_password.c_str());

  String q = "{\".proplist\": \"time,date\"}";
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(q);

  if (code == 200) {
    String resp = http.getString();
    DynamicJsonDocument doc(128);
    DeserializationError error = deserializeJson(doc, resp);
    
    if (error) {
      Serial.println("Time parse error: " + String(error.c_str()));
    } else if (doc.is<JsonArray>() && doc.size() > 0) {
      JsonObject o = doc[0].as<JsonObject>();
      const char* time_24hr = o["time"] | "00:00:00";
      const char* date_mt = o["date"] | "Jan/01/1970";

      int hour = 0, minute = 0, second = 0;
      sscanf(time_24hr, "%d:%d:%d", &hour, &minute, &second);
      routerCurrentMinute = minute;

      const char* ampm = (hour >= 12) ? "PM" : "AM";
      int display_hour = hour;
      if (display_hour == 0) display_hour = 12;
      else if (display_hour > 12) display_hour -= 12;

      snprintf(routerTimeStr, sizeof(routerTimeStr), "%02d:%02d %s", display_hour, minute, ampm);
      String formattedDate = convertDateFormat(date_mt);
      strncpy(routerDateStr, formattedDate.c_str(), sizeof(routerDateStr) - 1);
      routerDateStr[sizeof(routerDateStr) - 1] = '\0';
    }
  } else if (code > 0) {
    Serial.printf("Time fetch HTTP error: %d\n", code);
  }
  http.end();
}

// ==================== RX TOTALS FUNCTIONS ====================

void loadRxTotals() {
  if (!LittleFS.exists("/rx_totals.json")) {
    Serial.println("No saved RX totals file found - starting fresh");
    return;
  }
  
  File file = LittleFS.open("/rx_totals.json", "r");
  if (!file) {
    Serial.println("Failed to open RX totals file");
    return;
  }
  
  Serial.println("Loading RX totals from file...");
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("RX totals parse error: " + String(error.c_str()));
    return;
  }
  
  uint64_t loaded_hour = doc["rx_hour"] | 0;
  uint64_t loaded_day = doc["rx_day"] | 0;
  uint64_t loaded_week = doc["rx_week"] | 0;
  uint64_t loaded_month = doc["rx_month"] | 0;
  unsigned long loaded_hour_start = doc["hour_start"] | 0;
  unsigned long loaded_day_start = doc["day_start"] | 0;
  unsigned long loaded_week_start = doc["week_start"] | 0;
  unsigned long loaded_month_start = doc["month_start"] | 0;
  uint64_t loaded_hour_bytes = doc["hour_bytes"] | 0;
  uint64_t loaded_day_bytes = doc["day_bytes"] | 0;
  uint64_t loaded_week_bytes = doc["week_bytes"] | 0;
  uint64_t loaded_month_bytes = doc["month_bytes"] | 0;
  
  bool data_valid = true;
  
  if (loaded_hour > 0 && loaded_hour == loaded_week && loaded_hour == loaded_month) {
    Serial.println("WARNING: Identical values detected - data likely corrupt");
    data_valid = false;
  }
  
  if (loaded_hour_start == loaded_day_start &&
      loaded_day_start == loaded_week_start &&
      loaded_week_start == loaded_month_start &&
      loaded_hour_start != 0) {
    Serial.println("WARNING: All start times identical - improper initialization");
    data_valid = false;
  }
  
  unsigned long current_millis = millis();
  const unsigned long MAX_REASONABLE_TIME = 4200000000UL;
  
  if (loaded_hour_start > current_millis && loaded_hour_start < MAX_REASONABLE_TIME) {
    Serial.println("WARNING: Saved times in future - device was reset");
    data_valid = false;
  }
  
  if (data_valid) {
    rx_totals.rx_hour = loaded_hour;
    rx_totals.rx_day = loaded_day;
    rx_totals.rx_week = loaded_week;
    rx_totals.rx_month = loaded_month;
    hour_start_time = loaded_hour_start;
    day_start_time = loaded_day_start;
    week_start_time = loaded_week_start;
    month_start_time = loaded_month_start;
    hour_start_bytes = loaded_hour_bytes;
    day_start_bytes = loaded_day_bytes;
    week_start_bytes = loaded_week_bytes;
    month_start_bytes = loaded_month_bytes;
    
    Serial.println("✓ Loaded data passed validation");
  } else {
    Serial.println("✗ Validation failed - resetting totals");
    resetRxTotals();
  }
}

void resetRxTotals() {
  Serial.println("!!! RESETTING ALL RX TOTALS !!!");
  rx_totals.rx_hour = 0;
  rx_totals.rx_day = 0;
  rx_totals.rx_week = 0;
  rx_totals.rx_month = 0;
  hour_start_time = 0;
  day_start_time = 0;
  week_start_time = 0;
  month_start_time = 0;
  hour_start_bytes = 0;
  day_start_bytes = 0;
  week_start_bytes = 0;
  month_start_bytes = 0;
  
  if (LittleFS.exists("/rx_totals.json")) {
    LittleFS.remove("/rx_totals.json");
    Serial.println("Deleted saved totals file");
  }
  
  saveRxTotals();
  Serial.println("Reset complete");
}

void saveRxTotals() {
  StaticJsonDocument<256> doc;
  doc["rx_hour"] = rx_totals.rx_hour;
  doc["rx_day"] = rx_totals.rx_day;
  doc["rx_week"] = rx_totals.rx_week;
  doc["rx_month"] = rx_totals.rx_month;
  doc["hour_start"] = hour_start_time;
  doc["day_start"] = day_start_time;
  doc["week_start"] = week_start_time;
  doc["month_start"] = month_start_time;
  doc["hour_bytes"] = hour_start_bytes;
  doc["day_bytes"] = day_start_bytes;
  doc["week_bytes"] = week_start_bytes;
  doc["month_bytes"] = month_start_bytes;

  File file = LittleFS.open("/rx_totals.json", "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
  } else {
    Serial.println("ERROR: Failed to save RX totals");
  }
}

void updateRxTotals(uint64_t current_rx_bytes) {
  unsigned long now = millis();
  
  if (hour_start_time == 0) {
    Serial.println("=== INITIALIZING RX TOTALS ===");
    hour_start_time = now;
    day_start_time = now;
    week_start_time = now;
    month_start_time = now;
    hour_start_bytes = current_rx_bytes;
    day_start_bytes = current_rx_bytes;
    week_start_bytes = current_rx_bytes;
    month_start_bytes = current_rx_bytes;
    rx_totals.rx_hour = 0;
    rx_totals.rx_day = 0;
    rx_totals.rx_week = 0;
    rx_totals.rx_month = 0;
    return;
  }

  rx_totals.rx_hour = (current_rx_bytes >= hour_start_bytes) ? current_rx_bytes - hour_start_bytes : 0;
  rx_totals.rx_day = (current_rx_bytes >= day_start_bytes) ? current_rx_bytes - day_start_bytes : 0;
  rx_totals.rx_week = (current_rx_bytes >= week_start_bytes) ? current_rx_bytes - week_start_bytes : 0;
  rx_totals.rx_month = (current_rx_bytes >= month_start_bytes) ? current_rx_bytes - month_start_bytes : 0;

  const unsigned long MS_PER_HOUR = 3600000UL;
  const unsigned long MS_PER_DAY = 86400000UL;
  const unsigned long MS_PER_WEEK = 604800000UL;
  const unsigned long MS_PER_MONTH = 2592000000UL;

  if (now - hour_start_time >= MS_PER_HOUR) {
    Serial.println(">>> Hour period reset");
    hour_start_time = now;
    hour_start_bytes = current_rx_bytes;
  }

  if (now - day_start_time >= MS_PER_DAY) {
    Serial.println(">>> Day period reset");
    day_start_time = now;
    day_start_bytes = current_rx_bytes;
  }

  if (now - week_start_time >= MS_PER_WEEK) {
    Serial.println(">>> Week period reset");
    week_start_time = now;
    week_start_bytes = current_rx_bytes;
  }

  if (now - month_start_time >= MS_PER_MONTH) {
    Serial.println(">>> Month period reset");
    month_start_time = now;
    month_start_bytes = current_rx_bytes;
  }
}

// ==================== DISPLAY FUNCTIONS ====================

void draw_thick_line_sprite(TFT_eSprite &spr, int x0, int y0, int x1, int y1, uint16_t color, int thickness) {
  if (thickness <= 1) {
    spr.drawLine(x0, y0, x1, y1, color);
    return;
  }
  for (int offset = 0; offset < thickness; offset++) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    double len = sqrt(dx * dx + dy * dy);
    if (len > 0) {
      double px = -dy / len;
      double py = dx / len;
      spr.drawLine(x0 + (int)(px * offset), y0 + (int)(py * offset),
                   x1 + (int)(px * offset), y1 + (int)(py * offset), color);
    }
  }
}

void drawGauge(int x, int y, int radius, int thickness, float valuePercent, const char* label) {
  const int startAngle = 150;
  const int endAngle = 390;
  const int totalAngleRange = endAngle - startAngle;

  gaugeSprite.fillSprite(TFT_BLACK);

  for (int i = 0; i < totalAngleRange; i += 5) {
    int segmentStartAngle = startAngle + i;
    float sx1 = radius + (radius - thickness) * cos(segmentStartAngle * DEG_TO_RAD);
    float sy1 = radius + (radius - thickness) * sin(segmentStartAngle * DEG_TO_RAD);
    float sx2 = radius + radius * cos(segmentStartAngle * DEG_TO_RAD);
    float sy2 = radius + radius * sin(segmentStartAngle * DEG_TO_RAD);
    gaugeSprite.drawLine((int)sx1, (int)sy1, (int)sx2, (int)sy2, TFT_DARKGREY);
  }

  float cappedPercent = constrain(valuePercent, 0.0f, 100.0f);
  int filledAngle = (int)(totalAngleRange * (cappedPercent / 100.0f));

  for (int i = 0; i < filledAngle; i += 2) {
    int currentAngle = startAngle + i;
    uint16_t color;

    if (cappedPercent < 50) {
      uint8_t r = map((int)cappedPercent, 0, 50, 0, 255);
      color = gaugeSprite.color565(r, 255, 0);
    } else {
      uint8_t g = map((int)cappedPercent, 50, 100, 255, 0);
      color = gaugeSprite.color565(255, g, 0);
    }

    float sx1 = radius + (radius - thickness) * cos(currentAngle * DEG_TO_RAD);
    float sy1 = radius + (radius - thickness) * sin(currentAngle * DEG_TO_RAD);
    float sx2 = radius + radius * cos(currentAngle * DEG_TO_RAD);
    float sy2 = radius + radius * sin(currentAngle * DEG_TO_RAD);
    gaugeSprite.drawLine((int)sx1, (int)sy1, (int)sx2, (int)sy2, color);
  }

  gaugeSprite.setTextColor(TFT_WHITE);
  gaugeSprite.loadFont(SMALL_FONT, LittleFS);
  char percentStr[10];
  snprintf(percentStr, sizeof(percentStr), "%.1f%%", cappedPercent);
  gaugeSprite.drawCentreString(percentStr, radius, radius - 10, 0);
  gaugeSprite.drawCentreString(label, radius, radius + 10, 0);
  gaugeSprite.unloadFont();

  gaugeSprite.pushSprite(x - radius, y - radius);
}

void initGraphSprite() {
  const int graph_width = GRAPH_W;
  const int graph_height = GRAPH_H; 
  
  graphSprite.setColorDepth(16);
  
  void* spritePtr = graphSprite.createSprite(graph_width, graph_height);
  
  if (spritePtr == nullptr) {
    Serial.println("ERROR: Failed to create graph sprite - not enough RAM!");
    Serial.println("Falling back to direct rendering");
    sprite_created = false;
    return;
  }
  
  sprite_created = true;
  Serial.println("✓ Graph sprite created successfully");
  Serial.printf("Sprite size: %d bytes\n", graph_width * graph_height * 2);
}

void initGaugeSprite() {
  const int gauge_size = 70;
  
  gaugeSprite.setColorDepth(16);
  
  void* spritePtr = gaugeSprite.createSprite(gauge_size, gauge_size);
  
  if (spritePtr == nullptr) {
    Serial.println("ERROR: Failed to create gauge sprite - not enough RAM!");
    gauge_sprite_created = false;
    return;
  }
  
  gauge_sprite_created = true;
  Serial.println("✓ Gauge sprite created successfully");
  Serial.printf("Gauge sprite size: %d bytes\n", gauge_size * gauge_size * 2);
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n=== Mikrotik Display Starting ===");

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed! Formatting...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("ERROR: LittleFS init failed after format!");
      while (true) delay(100);
    }
    Serial.println("LittleFS formatted and mounted");
  } else {
    Serial.println("✓ LittleFS mounted");
  }

  loadRxTotals();
  loadPreferences();

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  Serial.println("✓ TFT initialized");

  initGraphSprite();
  initGaugeSprite();

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  delay(100);
  setBacklight(backlight_brightness);
  Serial.println("✓ Backlight initialized to: " + String(backlight_brightness) + "%");
#else
  Serial.println("⚠ TFT_BL not defined - backlight control unavailable");
#endif

  tryWiFiConnection();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Starting hotspot mode...");
    startHotspot();
  }
  
  setupWebServer();
  showSplashScreen();
  tft.fillScreen(TFT_BLACK);
  
  Serial.println("=== Setup Complete ===");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("Current backlight: " + String(backlight_brightness) + "%");
}

// ==================== MAIN LOOP ====================

void loop() {
  ElegantOTA.loop();
  
  if (hotspot_mode && millis() - last_wifi_retry > WIFI_RETRY_INTERVAL) {
    last_wifi_retry = millis();
    Serial.println("Retrying WiFi connection...");
    tryWiFiConnection();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected! Restarting...");
      delay(1000);
      ESP.restart();
    }
  }
  
  if (hotspot_mode) {
    static unsigned long last_update = 0;
    if (millis() - last_update > 5000) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_ORANGE);
      tft.loadFont(LARGE_FONT, LittleFS);
      tft.drawCentreString("Config Mode", SCREEN_WIDTH / 2, 100, 0);
      tft.unloadFont();
      
      tft.loadFont(SMALL_FONT, LittleFS);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("Connect to: " + hotspot_ssid, SCREEN_WIDTH / 2, 160, 0);
      tft.drawCentreString("Password: " + String(hotspot_password), SCREEN_WIDTH / 2, 185, 0);
      tft.drawCentreString("Browse to: http://192.168.4.1", SCREEN_WIDTH / 2, 220, 0);
      tft.unloadFont();
      
      last_update = millis();
    }
    delay(1000);
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting reconnect...");
    WiFi.reconnect();
    delay(2000);
    return;
  }
  
  unsigned long current_time = millis();
  bool api_data_fresh = false;

  {
    String url = router_address + "/rest/interface/ethernet/print";
    String q = "{\".proplist\": \".id,name,rx-bytes,tx-bytes,running\"}";
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(url);
    http.setAuthorization(router_login.c_str(), router_password.c_str());
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(q);
    
    if (code == 200) {
      String resp = http.getString();
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, resp);
      
      if (error) {
        Serial.println("Interface data parse error: " + String(error.c_str()));
      } else if (doc.is<JsonArray>()) {
        api_data_fresh = true;
        uint64_t nowMs = millis();
        
        for (JsonVariant item : doc.as<JsonArray>()) {
          const char* sid = item[".id"] | "";
          if (!sid || sid[0] == '\0') continue;
          
          int id = id2int(sid);
          if (id == 0) continue;

          uint64_t rx = (uint64_t)atoll(item["rx-bytes"] | "0");
          uint64_t tx = (uint64_t)atoll(item["tx-bytes"] | "0");
          
          mt_data_t* iface;
          if (!ifaces.count(id)) {
            iface = (mt_data_t*)malloc(sizeof(mt_data_t));
            if (!iface) {
              Serial.println("ERROR: Failed to allocate interface memory");
              continue;
            }
            memset(iface, 0, sizeof(mt_data_t));
            iface->rx = rx;
            iface->tx = tx;
            iface->time = nowMs;
            ifaces[id] = iface;
            Serial.printf("Initialized interface %d\n", id);
          } else {
            iface = ifaces[id];
            
            uint64_t elapsed_ms;
            if (nowMs >= iface->time) {
              elapsed_ms = nowMs - iface->time;
            } else {
              elapsed_ms = (0xFFFFFFFFUL - iface->time) + nowMs + 1;
            }
            
            if (elapsed_ms < 100) {
              continue;
            }
            
            double dt = elapsed_ms / 1000.0;
            
            uint64_t drx = (rx >= iface->rx) ? (rx - iface->rx) : rx;
            uint64_t dtx = (tx >= iface->tx) ? (tx - iface->tx) : tx;
            
            uint64_t rx_bps = (uint64_t)((drx / dt) * 8.0);
            uint64_t tx_bps = (uint64_t)((dtx / dt) * 8.0);
            
            const uint64_t MAX_REASONABLE_BPS = 10ULL * 1024 * 1024 * 1024;
            
            if (rx_bps > MAX_REASONABLE_BPS) {
              Serial.printf("Interface %d: Rejected unrealistic RX: %llu bps\n", id, rx_bps);
              rx_bps = 0;
            }
            
            if (tx_bps > MAX_REASONABLE_BPS) {
              Serial.printf("Interface %d: Rejected unrealistic TX: %llu bps\n", id, tx_bps);
              tx_bps = 0;
            }
            
            int prevIdx = (iface->pos == 0) ? HISTORY_SIZE - 1 : iface->pos - 1;
            uint64_t prev_rx = iface->hist_rx[prevIdx];
            uint64_t prev_tx = iface->hist_tx[prevIdx];
            
            rx_bps = (rx_bps * 7 + prev_rx * 3) / 10;
            tx_bps = (tx_bps * 7 + prev_tx * 3) / 10;
            
            iface->hist_rx[iface->pos] = rx_bps;
            iface->hist_tx[iface->pos] = tx_bps;
            
            if (++iface->pos >= HISTORY_SIZE) {
              iface->pos = 0;
            }
            
            iface->rx = rx;
            iface->tx = tx;
            iface->time = nowMs;
          }
        }
      }
    } else if (code > 0) {
      Serial.printf("Interface fetch HTTP error: %d\n", code);
    }
    http.end();
  }

  fetchRouterInfo();
  
  {
    const int TOP_TEXT_CLEAR_WIDTH = 338;

    if (routerCurrentMinute == -1 || routerCurrentMinute != last_minute) {
      fetchTimeFromRouter();
      last_minute = routerCurrentMinute;

      char timeDisplayBuf[48];
      snprintf(timeDisplayBuf, sizeof(timeDisplayBuf), "%s %s", routerDateStr, routerTimeStr);

      tft.loadFont(SMALL_FONT, LittleFS);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK); 
      tft.setTextPadding(TOP_TEXT_CLEAR_WIDTH);
      tft.drawString(timeDisplayBuf, 10, 8, 0);
      tft.unloadFont();
    }

    {
      uint32_t usedMB = 0;
      uint32_t totalMB = 0;
      if (routerInfo.memoryTotal > 0) {
        usedMB = (routerInfo.memoryTotal - routerInfo.memoryFree) / 1024 / 1024;
        totalMB = routerInfo.memoryTotal / 1024 / 1024;
      }
      
      char upBuf[32];
      formatUptime(routerInfo.uptime, upBuf, sizeof(upBuf));
      char sysBuf[128];
      snprintf(sysBuf, sizeof(sysBuf), "CPU: %.0f%% | RAM: %u/%u MB | Up: %s",
                routerInfo.cpuLoad, usedMB, totalMB, upBuf);

      tft.loadFont(SMALL_FONT, LittleFS);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextPadding(TOP_TEXT_CLEAR_WIDTH);
      tft.drawString(sysBuf, 10, 30, 0);
      tft.unloadFont();
    }

    {
      mt_data_t* iface = ifaces.count(graph_interface_id) ? ifaces[graph_interface_id] : nullptr;
      int lastIdx = iface ? ((iface->pos == 0) ? HISTORY_SIZE - 1 : iface->pos - 1) : 0; 
      double rx_mbps = iface ? iface->hist_rx[lastIdx] / 1024.0 / 1024.0 : 0.0;
      double tx_mbps = iface ? iface->hist_tx[lastIdx] / 1024.0 / 1024.0 : 0.0;
      char speedBuf[64];
      snprintf(speedBuf, sizeof(speedBuf), "TX: %.2f Mbps | RX: %.2f Mbps", tx_mbps, rx_mbps);

      tft.loadFont(SMALL_FONT, LittleFS);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setTextPadding(TOP_TEXT_CLEAR_WIDTH);
      tft.drawString(speedBuf, 10, 52, 0);
      
      tft.setTextPadding(0); 
      tft.unloadFont();
    }
  }

  {
    const int GAUGE_RADIUS = 30;
    const int GAUGE_THICKNESS = 8;
    const int GAUGE_Y = 35;
    const int RAM_GAUGE_X = 380;
    const int RX_GAUGE_X = 446;

    float ramUsagePercent = 0.0;
    if (routerInfo.memoryTotal > 0) {
      ramUsagePercent = (float)(routerInfo.memoryTotal - routerInfo.memoryFree) / routerInfo.memoryTotal * 100.0;
    }

    float rxUsagePercent = 0.0;
    if (FIXED_MAX_MBPS > FIXED_MIN_MBPS) {
      mt_data_t* iface = ifaces.count(graph_interface_id) ? ifaces[graph_interface_id] : nullptr;
      int lastIdx = iface ? ((iface->pos == 0) ? HISTORY_SIZE - 1 : iface->pos - 1) : 0;
      double rx_mbps = iface ? iface->hist_rx[lastIdx] / 1024.0 / 1024.0 : 0.0;
      
      rxUsagePercent = (float)((rx_mbps - FIXED_MIN_MBPS) / (FIXED_MAX_MBPS - FIXED_MIN_MBPS)) * 100.0;
      rxUsagePercent = constrain(rxUsagePercent, 0.0, 100.0);
    }

    if (abs(ramUsagePercent - last_ram_percent) > 0.1f) {
      drawGauge(RAM_GAUGE_X, GAUGE_Y, GAUGE_RADIUS, GAUGE_THICKNESS, ramUsagePercent, "RAM");
      last_ram_percent = ramUsagePercent;
    }

    if (abs(rxUsagePercent - last_rx_percent) > 0.1f) {
      drawGauge(RX_GAUGE_X, GAUGE_Y, GAUGE_RADIUS, GAUGE_THICKNESS, rxUsagePercent, "RX%");
      last_rx_percent = rxUsagePercent;
    }
  }

  {
    mt_data_t* iface = ifaces.count(graph_interface_id) ? ifaces[graph_interface_id] : nullptr;
    if (iface && api_data_fresh) {
      updateRxTotals(iface->rx);
    }

    static uint64_t last_displayed_hour = UINT64_MAX;
    static uint64_t last_displayed_day = UINT64_MAX;
    static uint64_t last_displayed_week = UINT64_MAX;
    static uint64_t last_displayed_month = UINT64_MAX;

    bool totals_changed = (last_displayed_hour != rx_totals.rx_hour) ||
                          (last_displayed_day != rx_totals.rx_day) ||
                          (last_displayed_week != rx_totals.rx_week) ||
                          (last_displayed_month != rx_totals.rx_month);

    if (totals_changed) {
      tft.loadFont(SMALL_FONT, LittleFS);

      char totalsStr[100];
      snprintf(totalsStr, sizeof(totalsStr), "1H: %.2f | Day: %.2f | Wk: %.2f | Mo: %.2f - GB",
                rx_totals.rx_hour / 1024.0 / 1024.0 / 1024.0,
                rx_totals.rx_day / 1024.0 / 1024.0 / 1024.0,
                rx_totals.rx_week / 1024.0 / 1024.0 / 1024.0,
                rx_totals.rx_month / 1024.0 / 1024.0 / 1024.0);
      
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setTextPadding(465);
      tft.drawString(totalsStr, 10, 285, 0);
      
      tft.setTextPadding(0);
      tft.unloadFont();

      last_displayed_hour = rx_totals.rx_hour;
      last_displayed_day = rx_totals.rx_day;
      last_displayed_week = rx_totals.rx_week;
      last_displayed_month = rx_totals.rx_month;

      static unsigned long last_save_time = 0;
      if (current_time - last_save_time >= 60000) { 
        saveRxTotals();
        last_save_time = current_time;
      }
    }
  }

  if (ifaces.count(graph_interface_id)) {
    const int graph_left = GRAPH_X;
    const int graph_top = GRAPH_Y;
    const int graph_width = GRAPH_W;
    const int graph_height = GRAPH_H;
    const int graph_bottom = GRAPH_BOTTOM;
    const int graph_inner_height = GRAPH_INNER_H;

    mt_data_t* iface = ifaces[graph_interface_id];

    uint64_t range_bps = FIXED_MAX_BPS - FIXED_MIN_BPS;
    if (range_bps == 0) range_bps = 1;

    if (!sprite_created && !graph_static_elements_drawn) {
      initGraphSprite();
    }
    
    if (!gauge_sprite_created) {
      initGaugeSprite();
    }

    if (!graph_static_elements_drawn) {
      tft.drawRect(graph_left, graph_top, graph_width, graph_height, TFT_WHITE);
      
      tft.setTextColor(TFT_WHITE);
      tft.loadFont(SMALL_FONT, LittleFS);

      double step = (FIXED_MAX_MBPS - FIXED_MIN_MBPS) / 4.0;
      char ybuf[12];
      int font_h = tft.fontHeight(0);

      for (int i = 0; i < 5; i++) {
        int y = graph_top + i * (graph_height / 4);
        int y_text = y - (font_h / 2);
        
        double mark_value = FIXED_MAX_MBPS - (i * step);
        snprintf(ybuf, sizeof(ybuf), "%5.0f", mark_value);
        tft.drawString(ybuf, graph_left - 45, y_text, 0);
        
        if (i > 0 && i < 4) { 
            tft.drawFastHLine(graph_left + 1, y, graph_width - 2, TFT_DARKGREY);
        }
        if (i == 4) {
            tft.drawFastHLine(graph_left + 1, graph_bottom - 1, graph_width - 2, TFT_DARKGREY);
        }
      }
      tft.drawString("Mbps", graph_left - 55, graph_top - 25, 0);

      char xbuf[8];
      for (int i = 0; i <= HISTORY_SIZE; i += 10) {
        int x = graph_left + i * graph_width / HISTORY_SIZE;
        snprintf(xbuf, sizeof(xbuf), "%ds", HISTORY_SIZE - i);
        tft.drawCentreString(xbuf, x, graph_bottom + 5, 0);
      }

      tft.unloadFont();
      graph_static_elements_drawn = true;
    }

    if (sprite_created) {
      graphSprite.fillSprite(TFT_BLACK);

      for (int i = 1; i < 4; i++) {
        int y = i * (graph_height / 4);
        graphSprite.drawFastHLine(0, y, graph_width, TFT_DARKGREY);
      }
      graphSprite.drawFastHLine(0, graph_height - 1, graph_width, TFT_DARKGREY);

      int p = iface->pos;
      int prev_x = -1, prev_rx_y = -1, prev_tx_y = -1;
      bool first_point = true;

      for (int i = 0; i < HISTORY_SIZE; i++) {
        if (p >= HISTORY_SIZE) p = 0; 
        uint64_t bits_rx = iface->hist_rx[p];
        uint64_t bits_tx = iface->hist_tx[p];

        int rx_h, tx_h;
        
        if (bits_rx <= FIXED_MIN_BPS) {
          rx_h = 0;
        } else if (bits_rx >= FIXED_MAX_BPS) {
          rx_h = graph_inner_height;
        } else {
          rx_h = (int)(((bits_rx - FIXED_MIN_BPS) * (uint64_t)graph_inner_height) / range_bps);
        }
        
        if (bits_tx <= FIXED_MIN_BPS) {
          tx_h = 0;
        } else if (bits_tx >= FIXED_MAX_BPS) {
          tx_h = graph_inner_height;
        } else {
          tx_h = (int)(((bits_tx - FIXED_MIN_BPS) * (uint64_t)graph_inner_height) / range_bps);
        }

        rx_h = constrain(rx_h, 0, graph_inner_height);
        tx_h = constrain(tx_h, 0, graph_inner_height);

        int current_x = (i * graph_width) / (HISTORY_SIZE - 1);
        int current_rx_y = (graph_height - 1) - rx_h; 
        int current_tx_y = (graph_height - 1) - tx_h;

        if (prev_x != -1 && !first_point) {
          draw_thick_line_sprite(graphSprite, prev_x, prev_tx_y, current_x, current_tx_y, GRAPH_COLOR_TX, LINE_THICKNESS);
          draw_thick_line_sprite(graphSprite, prev_x, prev_rx_y, current_x, current_rx_y, GRAPH_COLOR_RX, LINE_THICKNESS);
        }

        prev_x = current_x;
        prev_rx_y = current_rx_y;
        prev_tx_y = current_tx_y;
        first_point = false;
        p++;
      }

      graphSprite.pushSprite(graph_left + 1, graph_top + 1);
      
    } else {
      tft.fillRect(graph_left + 1, graph_top + 1, graph_width - 2, graph_height - 2, TFT_BLACK);

      for (int i = 1; i < 4; i++) {
        int y = graph_top + i * (graph_height / 4);
        tft.drawFastHLine(graph_left + 1, y, graph_width - 2, TFT_DARKGREY);
      }
      tft.drawFastHLine(graph_left + 1, graph_bottom - 1, graph_width - 2, TFT_DARKGREY);
      
      int p = iface->pos;
      int prev_x = -1, prev_rx_y = -1, prev_tx_y = -1;
      bool first_point = true;

      for (int i = 0; i < HISTORY_SIZE; i++) {
        if (p >= HISTORY_SIZE) p = 0;
        uint64_t bits_rx = iface->hist_rx[p];
        uint64_t bits_tx = iface->hist_tx[p];

        int rx_h, tx_h;
        
        if (bits_rx <= FIXED_MIN_BPS) {
          rx_h = 0;
        } else if (bits_rx >= FIXED_MAX_BPS) {
          rx_h = graph_inner_height;
        } else {
          rx_h = (int)(((bits_rx - FIXED_MIN_BPS) * (uint64_t)graph_inner_height) / range_bps);
        }
        
        if (bits_tx <= FIXED_MIN_BPS) {
          tx_h = 0;
        } else if (bits_tx >= FIXED_MAX_BPS) {
          tx_h = graph_inner_height;
        } else {
          tx_h = (int)(((bits_tx - FIXED_MIN_BPS) * (uint64_t)graph_inner_height) / range_bps);
        }

        rx_h = constrain(rx_h, 0, graph_inner_height);
        tx_h = constrain(tx_h, 0, graph_inner_height);

        int current_x = graph_left + (i * graph_width) / (HISTORY_SIZE - 1);
        int current_rx_y = (graph_bottom - 1) - rx_h;
        int current_tx_y = (graph_bottom - 1) - tx_h;

        if (prev_x != -1 && !first_point) {
          tft.drawLine(prev_x, prev_tx_y, current_x, current_tx_y, GRAPH_COLOR_TX);
          tft.drawLine(prev_x, prev_rx_y, current_x, current_rx_y, GRAPH_COLOR_RX);
        }

        prev_x = current_x;
        prev_rx_y = current_rx_y;
        prev_tx_y = current_tx_y;
        first_point = false;
        p++;
      }
    }
  }
  
  delay(500);
}