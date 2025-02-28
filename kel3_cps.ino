#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi Credentials
const char* ssid = "Magista";
const char* password = "liatkekanan";

// Web Server
WebServer server(80);

// Define I2C pins for ESP32
#define SDA_PIN 21
#define SCL_PIN 22

// Sensor Pins
#define MQ2_ANALOG_PIN 34  // A0 (Analog Output) from MQ-2
#define FLAME_SENSOR_PIN 19 // D0 (Digital Output) from Flame Sensor

// Buzzer and LED Pins
#define BUZZER_PIN 13
#define LED_PIN 12

// Threshold for Gas Detection (Adjust based on calibration)
#define GAS_THRESHOLD 1010

// LCD I2C Address (0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Gas & Flame Detector");

  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.println(WiFi.localIP()); // Print ESP32 IP Address

  // Initialize I2C for LCD
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Gas & Flame Sensor");

  // Set pin modes
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Ensure LED & Buzzer are OFF at startup
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // Web Server Handlers
  server.on("/", handleRoot);
  server.on("/status", handleSensorStatus);
  server.begin();

  delay(2000);
}

void loop() {
  server.handleClient();

  int gasValue = analogRead(MQ2_ANALOG_PIN);
  int flameDetected = digitalRead(FLAME_SENSOR_PIN);

  Serial.print("Gas Value: ");
  Serial.println(gasValue);
  Serial.print("Flame Status: ");
  Serial.println(flameDetected != LOW ? "FIRE DETECTED" : "NO FIRE");

  // Update LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hello Group 3:");

  if (gasValue > GAS_THRESHOLD || flameDetected != LOW) { // Gas or Fire detected
    Serial.println("WARNING: Gas/Flame Detected!");
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);

    lcd.setCursor(0, 1);
    if (gasValue > GAS_THRESHOLD && flameDetected != LOW) {
      lcd.print("Gas & Fire ON!");
    } else if (gasValue > GAS_THRESHOLD) {
      lcd.print("Gas ON, Fire OFF");
    } else {
      lcd.print("Fire ON, Gas OFF");
    }
  } else { // Safe Condition
    Serial.println("SAFE: No Gas or Fire");
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);

    lcd.setCursor(0, 1);
    lcd.print("Gas & Fire OFF");
  }

  delay(1000); // Update every second
}

// Serve HTML page
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
        .container { max-width: 400px; margin: auto; padding: 20px; border: 1px solid #ccc; border-radius: 10px; }
        .status { font-size: 20px; font-weight: bold; padding: 10px; border-radius: 5px; }
        .safe { background: #4CAF50; color: white; }
        .danger { background: #FF5733; color: white; }
      </style>
    </head>
    <body>
      <h2>🔥 Gas & Flame Monitor</h2>
      <div class="container">
        <p>Gas Value: <span id="gasValue" class="status safe">SAFE</span></p>
        <p>Flame Status: <span id="flameStatus" class="status safe">NO FIRE</span></p>
      </div>

      <script>
        function updateStatus() {
          fetch("/status")
            .then(response => response.json())
            .then(data => {
              document.getElementById("gasValue").innerText = data.gas > 300 ? "DANGER" : "SAFE";
              document.getElementById("gasValue").className = "status " + (data.gas > 300 ? "danger" : "safe");

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

// Send JSON sensor data
void handleSensorStatus() {
  int gasValue = analogRead(MQ2_ANALOG_PIN);
  int flameDetected = digitalRead(FLAME_SENSOR_PIN);

  String json = "{\"gas\": " + String(gasValue) + ", \"flame\": " + String(flameDetected) + "}";
  server.send(200, "application/json", json);
}
