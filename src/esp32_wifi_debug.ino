/*
 * ESP32-S3 WiFi Debug Tool
 * Features:
 *   1. WiFi Auto Connect + Reconnect
 *   2. Telnet Remote Serial Debug
 *   3. Web OTA Wireless Flash
 *   4. Web Status Panel
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

// ============== Config ==============
#define WIFI_SSID      "Jiuzy"
#define WIFI_PASSWORD  "wcg12345@"
#define TELNET_PORT    23
#define WEB_PORT       80
#define LED_PIN        48
// ==================================

WiFiServer telnetServer(TELNET_PORT);
WiFiClient telnetClient;
WebServer webServer(WEB_PORT);

// ============== WiFi ==============

void setupWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    
    Serial.printf("\n[WiFi] Connecting: %s ...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        blinkLED(3, 200);
    } else {
        Serial.println("\n[WiFi] Failed! Starting AP...");
        setupSoftAP();
    }
}

void setupSoftAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-Debug", "12345678");
    Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void checkWiFi() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        lastCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Disconnected, reconnecting...");
            WiFi.reconnect();
        }
    }
}

// ============== Telnet ==============

void setupTelnet() {
    telnetServer.begin();
    telnetServer.setNoDelay(true);
    Serial.printf("[Telnet] Port: %d\n", TELNET_PORT);
}

void handleTelnet() {
    if (telnetServer.hasClient()) {
        if (telnetClient && telnetClient.connected()) {
            telnetServer.available().stop();
        } else {
            telnetClient = telnetServer.available();
            Serial.println("[Telnet] Client connected");
            telnetClient.printf("=== ESP32-S3 Telnet Debug ===\r\n");
            telnetClient.printf("IP: %s\r\n> ", WiFi.localIP().toString().c_str());
        }
    }
    
    if (telnetClient && telnetClient.connected()) {
        while (Serial.available()) {
            char c = Serial.read();
            telnetClient.write(c);
        }
        while (telnetClient.available()) {
            Serial.write(telnetClient.read());
        }
    }
}

// ============== Web Server ==============

void setupWebServer() {
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/ota", HTTP_GET, handleOTAForm);
    webServer.on("/ota", HTTP_POST, handleOTAUpload);
    webServer.on("/status", HTTP_GET, handleStatus);
    webServer.on("/restart", HTTP_GET, handleRestart);
    webServer.on("/led", HTTP_POST, handleLED);
    webServer.begin();
    Serial.printf("[Web] http://%s/\n", WiFi.localIP().toString().c_str());
}

void handleRoot() {
    String ip = WiFi.localIP().toString();
    String html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>ESP32-S3 WiFi Debug</title><style>";
    html += "body{font-family:monospace;background:#1a1a2e;color:#eee;padding:20px}h1{color:#00d4ff}.card{background:#16213e;padding:15px;border-radius:8px;margin:10px 0}.btn{background:#0f3460;color:#fff;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:5px}.btn:hover{background:#00d4ff;color:#000}.btn-ota{background:#e94560}.stat{display:flex;gap:20px;flex-wrap:wrap}.stat>div{background:#0f3460;padding:10px 20px;border-radius:5px}.stat span{color:#00d4ff;font-size:1.5em}input[type=file]{background:#0f3460;padding:10px;border-radius:5px;width:100%;margin:10px 0;color:#fff}a{color:#00d4ff}</style></head><body>";
    html += "<h1>ESP32-S3 WiFi Debug</h1><div class=\"stat\"><div>IP<br><span id=\"ip\">" + ip + "</span></div><div>Signal<br><span id=\"rssi\">" + String(WiFi.RSSI()) + " dBm</span></div><div>Mem<br><span id=\"mem\">" + String(ESP.getFreeHeap()/1024) + " KB</span></div><div>Uptime<br><span id=\"uptime\">" + String(millis()/1000) + " s</span></div></div>";
    html += "<div class=\"card\"><h3>LED</h3><button class=\"btn\" onclick=\"fetch('/led?o=0',{method:'POST'})\">OFF</button><button class=\"btn\" onclick=\"fetch('/led?o=1',{method:'POST'})\">R</button><button class=\"btn\" onclick=\"fetch('/led?o=2',{method:'POST'})\">G</button><button class=\"btn\" onclick=\"fetch('/led?o=3',{method:'POST'})\">B</button><button class=\"btn\" onclick=\"fetch('/led?o=4',{method:'POST'})\">W</button></div>";
    html += "<div class=\"card\"><h3>OTA Update</h3><form method=\"POST\" action=\"/ota\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"update\" accept=\".bin\" required><button type=\"submit\" class=\"btn btn-ota\">Upload</button></form></div>";
    html += "<div class=\"card\"><button class=\"btn\" onclick=\"location.reload()\">Reload</button><button class=\"btn\" onclick=\"if(confirm('Restart?'))fetch('/restart')\">Restart</button></div>";
    html += "<p><a href=\"telnet://" + ip + ":23\">Telnet</a></p>";
    html += "<script>setInterval(()=>{fetch('/status').then(r=>r.json()).then(d=>{document.getElementById('rssi').textContent=d.rssi+' dBm';document.getElementById('uptime').textContent=d.uptime+' s'})},2000)</script></body></html>";
    webServer.send(200, "text/html; charset=utf-8", html);
}

void handleStatus() {
    String json = "{";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"uptime\":" + String(millis()/1000) + ",";
    json += "\"freeheap\":" + ESP.getFreeHeap();
    json += "}";
    webServer.send(200, "application/json", json);
}

void handleOTAForm() {
    webServer.send(200, "text/html", "<html><body style=\"font-family:monospace;background:#1a1a2e;color:#fff;padding:20px\"><h2>OTA Upload</h2><form method=\"POST\" action=\"/ota\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"update\" accept=\".bin\" required style=\"background:#16213e;padding:10px;width:100%;color:#fff\"><br><br><button type=\"submit\" style=\"background:#e94560;color:#fff;padding:15px 30px;border:none;border-radius:5px;cursor:pointer\">Upload</button></form><p><a href=\"/\" style=\"color:#00d4ff\">Home</a></p></body></html>");
}

void handleOTAUpload() {
    static bool ota_running = false;
    if (ota_running) { webServer.send(503, "text/plain", "OTA in progress"); return; }
    
    HTTPUpload& up = webServer.upload();
    if (up.status == UPLOAD_FILE_START) {
        ota_running = true;
        Serial.printf("[OTA] Start: %s\n", up.filename.c_str());
        if (telnetClient) telnetClient.stop();
        Update.begin(UPDATE_SIZE_UNKNOWN);
    } else if (up.status == UPLOAD_FILE_WRITE) {
        Update.write(up.buf, up.currentSize);
    } else if (up.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.println("[OTA] Success! Rebooting...");
            webServer.send(200, "text/plain", "OK! Rebooting");
            delay(500);
            ESP.restart();
        } else {
            webServer.send(500, "text/plain", "Failed");
        }
        ota_running = false;
    }
}

void handleRestart() {
    webServer.send(200, "text/plain", "restart");
    delay(200);
    ESP.restart();
}

void handleLED() {
    int o = webServer.arg("o").toInt();
    switch(o) {
        case 0: neopixelWrite(LED_PIN, 0, 0, 0); break;
        case 1: neopixelWrite(LED_PIN, 255, 0, 0); break;
        case 2: neopixelWrite(LED_PIN, 0, 255, 0); break;
        case 3: neopixelWrite(LED_PIN, 0, 0, 255); break;
        case 4: neopixelWrite(LED_PIN, 255, 255, 255); break;
    }
    webServer.send(200, "text/plain", "OK");
}

void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        neopixelWrite(LED_PIN, 255, 255, 255);
        delay(delayMs);
        neopixelWrite(LED_PIN, 0, 0, 0);
        delay(delayMs);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ESP32-S3 WiFi Debug v1.0 ===\n");
    
    neopixelWrite(LED_PIN, 0, 0, 0);
    setupWiFi();
    setupTelnet();
    setupWebServer();
    
    Serial.printf("Web: http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.printf("Telnet: %s:23\n", WiFi.localIP().toString().c_str());
    
    neopixelWrite(LED_PIN, 0, 0, 255);
}

void loop() {
    checkWiFi();
    handleTelnet();
    webServer.handleClient();
}
