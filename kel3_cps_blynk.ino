#define BLYNK_TEMPLATE_ID "TMPL6x0H0XEjQ"
#define BLYNK_TEMPLATE_NAME "Flame Sensor"
#define BLYNK_DEVICE_NAME "Flame Detector"
#define BLYNK_AUTH_TOKEN "gmJTzOIGwxOGUruPpq3J8EGq1kCyBzwz"

#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BlynkSimpleEsp32.h>

// WiFi Credentials
const char* ssid = "PesbalGirl";
const char* password = "sisfotelco";

// Web Server
WebServer server(80);

// Define I2C pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Sensor Pins
#define MQ2_ANALOG_PIN 34
#define FLAME_SENSOR_PIN 19

// Buzzer and LED Pins
#define BUZZER_PIN 13
#define LED_PIN 12

// Threshold for Gas Detection
#define GAS_THRESHOLD 1200

// LCD
LiquidCrystal_PCF8574 lcd(0x27);

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Gas & Flame Detector");

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
    Serial.println(WiFi.localIP());

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);

    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.begin(16, 2);
    lcd.setBacklight(255);
    lcd.setCursor(0, 0);
    lcd.print("Gas & Flame Sensor");

    pinMode(FLAME_SENSOR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    server.on("/", handleRoot);
    server.on("/status", handleSensorStatus);
    server.begin();

    delay(2000);
}

void loop() {
    Blynk.run();
    server.handleClient();
    checkBlynkConnection();

    int gasValue = analogRead(MQ2_ANALOG_PIN);
    int flameDetected = digitalRead(FLAME_SENSOR_PIN);

    Serial.printf("Gas Value: %d\n", gasValue);
    Serial.printf("Flame Status: %s\n", flameDetected ? "FIRE DETECTED" : "NO FIRE");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hello Group 3:");

    if (gasValue > GAS_THRESHOLD || flameDetected) {
        Serial.println("WARNING: Gas/Flame Detected!");
        digitalWrite(BUZZER_PIN, HIGH);
        digitalWrite(LED_PIN, HIGH);
        lcd.setCursor(0, 1);
        lcd.print(gasValue > GAS_THRESHOLD ? "Gas ON, " : "Gas OFF, ");
        lcd.print(flameDetected ? "Fire ON" : "Fire OFF");
    } else {
        Serial.println("SAFE: No Gas or Fire");
        digitalWrite(BUZZER_PIN, LOW);
        digitalWrite(LED_PIN, LOW);
        lcd.setCursor(0, 1);
        lcd.print("Gas & Fire OFF");
    }

    Blynk.virtualWrite(V0, gasValue);
    Blynk.virtualWrite(V1, flameDetected);
    Blynk.virtualWrite(V2, Blynk.connected() ? "Connected ✅" : "Disconnected ❌");

    delay(1000);
}

void checkBlynkConnection() {
    if (!Blynk.connected()) {
        Serial.println("Reconnecting to Blynk...");
        Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
    }
}

void handleRoot() {
    server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Gas & Flame Monitor</title>
        <style>
            body { font-family: Arial, sans-serif; text-align: center; padding: 20px; }
            .status { font-size: 20px; font-weight: bold; padding: 10px; }
            .safe { background: #4CAF50; color: white; }
            .danger { background: #FF5733; color: white; }
        </style>
    </head>
    <body>
        <h2>🔥 Gas & Flame Monitor</h2>
        <p>Gas Value: <span id="gasValue" class="status safe">SAFE</span></p>
        <p>Flame Status: <span id="flameStatus" class="status safe">NO FIRE</span></p>
        <script>
            function updateStatus() {
                fetch("/status")
                .then(response => response.json())
                .then(data => {
                    document.getElementById("gasValue").innerText = data.gas > 1000 ? "DANGER" : "SAFE";
                    document.getElementById("gasValue").className = "status " + (data.gas > 1000 ? "danger" : "safe");
                    document.getElementById("flameStatus").innerText = data.flame ? "FIRE DETECTED" : "NO FIRE";
                    document.getElementById("flameStatus").className = "status " + (data.flame ? "danger" : "safe");
                });
            }
            setInterval(updateStatus, 1000);
        </script>
    </body>
    </html>
    )rawliteral");
}

void handleSensorStatus() {
    int gasValue = analogRead(MQ2_ANALOG_PIN);
    int flameDetected = digitalRead(FLAME_SENSOR_PIN);
    String json = "{\"gas\": " + String(gasValue) + ", \"flame\": " + String(flameDetected) + "}";
    server.send(200, "application/json", json);
}
