#include <Wire.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>

// Define GPIOs
#define YELLOW_LED_PIN 12
#define RGB_LED_PIN 16
#define BUZZER_PIN 23
#define LEFT_BUTTON_PIN 26
#define RIGHT_BUTTON_PIN 27
#define lightreader 36

// I2C Configuration
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

// LCD Configuration
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Wi-Fi Configuration
const char* ssid = "Pentest Warriors";
const char* password = "password123";

WebServer server(80);

// State variables
String lcdContent = "";
bool yellowLedState = false;

// Function to initialize I2C
void i2c_master_init() {
    Wire.begin(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    Wire.setClock(I2C_MASTER_FREQ_HZ);
    Serial.println("I2C initialized successfully");
}

// Function to initialize LCD
void lcd_init() {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcdContent = "Hello, World!";
    lcd.print(lcdContent);
    delay(1000);
    lcd.clear();
}

// Function to handle button press events
void handle_button_press() {
    bool leftButtonPressed = digitalRead(LEFT_BUTTON_PIN) == LOW;
    bool rightButtonPressed = digitalRead(RIGHT_BUTTON_PIN) == LOW;

    if (leftButtonPressed || rightButtonPressed) {
        yellowLedState = true;
        digitalWrite(YELLOW_LED_PIN, HIGH); // Turn on the yellow LED
        lcd.clear();
        if (leftButtonPressed) {
            lcdContent = "Left Button";
            lcd.setCursor(0, 0);
            lcd.print(lcdContent);
            Serial.println("Left button pressed");
        } else if (rightButtonPressed) {
            lcdContent = "Right Button";
            lcd.setCursor(0, 0);
            lcd.print(lcdContent);
            Serial.println("Right button pressed");
        }
    } else {
        yellowLedState = false;
        digitalWrite(YELLOW_LED_PIN, LOW); // Turn off the yellow LED
        //Serial.println("No button pressed");
    }
}

void i2c_scanner();

void i2c_scanner() {
    Serial.println("Scanning I2C devices...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found I2C device at address: 0x");
            Serial.println(addr, HEX);
        }
    }
    Serial.println("I2C scan complete.");
}

// HTML handler function
void handleRoot() {
    String html = "<html><body><h1>Sensor Information</h1>";
    html += "<p><strong>LCD Display:</strong> " + lcdContent + "</p>";
    html += "<p><strong>Yellow LED State:</strong> " + String(yellowLedState ? "On" : "Off") + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Initialize GPIOs for LEDs, Buzzer, and Buttons
    pinMode(YELLOW_LED_PIN, OUTPUT);
    pinMode(RGB_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);

    // Ensure Yellow LED is off by default
    digitalWrite(YELLOW_LED_PIN, LOW);

    // Turn on RGB LED
    digitalWrite(RGB_LED_PIN, HIGH);

    // Beep the buzzer twice
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
    }

    // Initialize I2C
    i2c_master_init();

    // Initialize LCD
    lcd_init();

    // Run I2C Scanner
    i2c_scanner();

    // Initialize Wi-Fi
    Serial.print("Setting up Wi-Fi...");
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Wi-Fi IP Address: ");
    Serial.println(IP);

    // Set up web server
    server.on("/", handleRoot);
    server.begin();
    Serial.println("Web server started");
}

void read_light(){
    int light = analogRead(lightreader);
    lcd.setCursor(0, 1);
    lcd.printf("%04d", light);
    delay(50);
}

void loop() {
    handle_button_press();
    read_light();
    server.handleClient(); // Handle incoming HTTP requests
    delay(50); // Check button state every 50ms
}
