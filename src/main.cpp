#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>

// TFT Display
TFT_eSPI tft = TFT_eSPI();

// Backlight pin for CYD
#define TFT_BL 21

// Timing constants
#define DISPLAY_REFRESH_INTERVAL 30000
#define WIFI_RECONNECT_INTERVAL 5000
#define MQTT_RECONNECT_INTERVAL 5000

// WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Timing variables
unsigned long lastWiFiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastDisplayRefresh = 0;

// Pool data from Home Assistant
struct PoolData {
    float waterTempF = 0.0;
    float phLevel = 0.0;
    float orpLevel = 0.0;
    bool dataValid = false;
    unsigned long lastUpdate = 0;
    unsigned int msgCount = 0;
} poolData;

// Function declarations
void setupWiFi();
void setupMQTT();
void setupDisplay();
void reconnectWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void subscribeToTopics();
void updateDisplay();
void drawDashboard();
void drawStatusBar();

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== ESP32 Pool Monitor Display ===");

    setupDisplay();

    // Show boot screen
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Pool Monitor", 160, 100, 2);
    tft.setTextSize(1);
    tft.drawString("Starting up...", 160, 140, 2);

    setupWiFi();
    setupMQTT();

    Serial.println("Setup complete");
    delay(500);
}

void loop() {
    // Maintain WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        if (millis() - lastWiFiAttempt > WIFI_RECONNECT_INTERVAL) {
            reconnectWiFi();
            lastWiFiAttempt = millis();
        }
    }

    // Maintain MQTT connection
    if (!mqttClient.connected()) {
        if (millis() - lastMqttAttempt > MQTT_RECONNECT_INTERVAL) {
            reconnectMQTT();
            lastMqttAttempt = millis();
        }
    } else {
        mqttClient.loop();
    }

    // Update display periodically
    if (millis() - lastDisplayRefresh > DISPLAY_REFRESH_INTERVAL) {
        updateDisplay();
        lastDisplayRefresh = millis();
    }
}

void setupDisplay() {
    // Initialize backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Initialize TFT
    tft.init();
    tft.setRotation(1);  // Landscape mode
    tft.fillScreen(TFT_BLACK);

    Serial.println("Display initialized: 320x240 TFT");
}

void setupWiFi() {
    Serial.print("Connecting to WiFi");

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Connecting WiFi...", 160, 120, 2);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("WiFi Connected", 160, 100, 2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(WiFi.localIP().toString(), 160, 140, 2);
        delay(1000);
    } else {
        Serial.println(" Failed!");
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("WiFi Failed!", 160, 120, 2);
        delay(1000);
    }
}

void reconnectWiFi() {
    Serial.println("Reconnecting to WiFi...");
    WiFi.reconnect();
}

void setupMQTT() {
    mqttClient.setServer(MQTT_BROKER, 1883);
    mqttClient.setCallback(mqttCallback);
}

void reconnectMQTT() {
    Serial.print("Connecting to MQTT...");

    String clientId = "esp32-pool-display-" + String(ESP.getEfuseMac());
    Serial.printf(" ClientID: %s\n", clientId.c_str());
    Serial.printf(" Broker: %s:1883\n", MQTT_BROKER);

    bool connected;
    #ifdef MQTT_USER
        Serial.printf(" Using auth: %s / ****\n", MQTT_USER);
        connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
    #else
        connected = mqttClient.connect(clientId.c_str());
    #endif

    if (connected) {
        Serial.println(" Connected!");
        subscribeToTopics();
        Serial.println("Topics subscribed. Waiting for data...");

        // Show MQTT connected briefly
        tft.fillRect(0, 0, 320, 20, TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.drawString("MQTT Connected", 5, 40, 2);
    } else {
        int state = mqttClient.state();
        Serial.print(" Failed, rc=");
        Serial.println(state);
        // MQTT state: -4=timeout, -3=conn_lost, -2=conn_failed, -1=disconn, 1-5=protocol errors
        tft.fillRect(0, 0, 320, 20, TFT_BLACK);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.drawString("MQTT Fail: " + String(state), 5, 40, 2);
    }
}

void subscribeToTopics() {
    // Subscribe to Home Assistant pool sensor topics
    bool t1 = mqttClient.subscribe("homeassistant/sensor/iopool_my_pool_temperature/state");
    bool t2 = mqttClient.subscribe("homeassistant/sensor/iopool_my_pool_ph/state");
    bool t3 = mqttClient.subscribe("homeassistant/sensor/iopool_my_pool_orp/state");

    // Alternative common topic patterns
    bool t4 = mqttClient.subscribe("pool/temperature/f");
    bool t5 = mqttClient.subscribe("pool/temperature/c");
    bool t6 = mqttClient.subscribe("pool/ph");
    bool t7 = mqttClient.subscribe("pool/orp");

    Serial.printf("Subscribe results: t1=%d t2=%d t3=%d t4=%d t5=%d t6=%d t7=%d\n",
                  t1, t2, t3, t4, t5, t6, t7);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    poolData.msgCount++;
    Serial.printf("MQTT [#%u] [%s]: %s (len=%d)\n", poolData.msgCount, topic, message.c_str(), length);

    String topicStr = String(topic);

    // Parse temperature (Fahrenheit preferred, fallback to Celsius)
    if (topicStr.indexOf("temperature") >= 0) {
        float val = message.toFloat();
        if (topicStr.indexOf("/f") >= 0 || val > 50) {
            poolData.waterTempF = val;
        } else {
            // Convert Celsius to Fahrenheit
            poolData.waterTempF = val * 9.0 / 5.0 + 32.0;
        }
        poolData.lastUpdate = millis();
        poolData.dataValid = true;
        Serial.printf("  Parsed TEMP: %.1fF\n", poolData.waterTempF);
    }

    // Parse pH
    if (topicStr.indexOf("ph") >= 0 && topicStr.indexOf("orp") < 0) {
        poolData.phLevel = message.toFloat();
        poolData.lastUpdate = millis();
        poolData.dataValid = true;
        Serial.printf("  Parsed PH: %.2f\n", poolData.phLevel);
    }

    // Parse ORP
    if (topicStr.indexOf("orp") >= 0) {
        poolData.orpLevel = message.toFloat();
        poolData.lastUpdate = millis();
        poolData.dataValid = true;
        Serial.printf("  Parsed ORP: %.0f mV\n", poolData.orpLevel);
    }
}

void updateDisplay() {
    drawDashboard();
}

void drawStatusBar() {
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);

    // WiFi status - below title
    if (WiFi.status() == WL_CONNECTED) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("W", 5, 30, 2);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("X", 5, 30, 2);
    }

    // MQTT status
    if (mqttClient.connected()) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("M", 20, 30, 2);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("X", 20, 30, 2);
    }

    // Data age indicator
    if (poolData.dataValid) {
        unsigned long age = (millis() - poolData.lastUpdate) / 1000;
        if (age < 60) {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.drawString("OK", 35, 30, 2);
        } else if (age < 300) {
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawString("STALE", 35, 30, 2);
        } else {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.drawString("OLD", 35, 30, 2);
        }
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("NO DATA", 35, 30, 2);
    }
}

void drawDashboard() {
    // Full screen clear
    tft.fillScreen(TFT_BLACK);

    // Title at top
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Pool Monitor", 160, 15, 2);

    // Status bar below title
    drawStatusBar();

    // Temperature - Large display center top
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Water Temp", 160, 50, 2);

    tft.setTextSize(4);
    if (poolData.dataValid && poolData.waterTempF > 0) {
        char tempStr[10];
        sprintf(tempStr, "%.0f F", poolData.waterTempF);

        // Color based on temperature range
        uint16_t tempColor;
        if (poolData.waterTempF < 70.0) {
            tempColor = TFT_RED;
        } else if (poolData.waterTempF <= 74.0) {
            tempColor = TFT_YELLOW;
        } else if (poolData.waterTempF <= 86.0) {
            tempColor = TFT_GREEN;
        } else if (poolData.waterTempF <= 90.0) {
            tempColor = TFT_YELLOW;
        } else {
            tempColor = TFT_RED;
        }

        tft.setTextColor(tempColor, TFT_BLACK);
        tft.drawString(tempStr, 160, 90, 2);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("-- F", 160, 90, 2);
    }

    // pH - Left side
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.drawString("pH", 80, 140, 2);

    tft.setTextSize(3);
    if (poolData.dataValid && poolData.phLevel > 0) {
        char phStr[10];
        sprintf(phStr, "%.1f", poolData.phLevel);

        // Color based on pH range
        uint16_t phColor;
        if (poolData.phLevel < 6.9) {
            phColor = TFT_RED;      // Too acidic
        } else if (poolData.phLevel <= 7.8) {
            phColor = TFT_GREEN;    // Ideal range
        } else if (poolData.phLevel <= 8.2) {
            phColor = TFT_YELLOW;   // Getting high
        } else {
            phColor = TFT_RED;      // Too alkaline
        }

        tft.setTextColor(phColor, TFT_BLACK);
        tft.drawString(phStr, 80, 175, 2);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("--", 80, 175, 2);
    }

    // ORP - Right side
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.drawString("ORP", 240, 140, 2);

    tft.setTextSize(3);
    if (poolData.dataValid && poolData.orpLevel > 0) {
        char orpStr[20];
        sprintf(orpStr, "%.0f", poolData.orpLevel);

        // Color based on ORP range
        uint16_t orpColor;
        if (poolData.orpLevel < 650) {
            orpColor = TFT_RED;       // Too low
        } else if (poolData.orpLevel <= 699) {
            orpColor = TFT_YELLOW;    // Low
        } else if (poolData.orpLevel <= 800) {
            orpColor = TFT_GREEN;     // Ideal range
        } else if (poolData.orpLevel <= 850) {
            orpColor = TFT_YELLOW;    // High
        } else {
            orpColor = TFT_RED;       // Too high
        }

        tft.setTextColor(orpColor, TFT_BLACK);
        tft.drawString(orpStr, 240, 175, 2);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("--", 240, 175, 2);
    }

    // Show message count and data validity (bottom left)
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(poolData.dataValid ? TFT_GREEN : TFT_RED, TFT_BLACK);
    char countStr[30];
    sprintf(countStr, "Msgs:%u", poolData.msgCount);
    tft.drawString(countStr, 5, 220, 2);
}
