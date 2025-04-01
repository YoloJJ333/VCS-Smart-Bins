#include <Wire.h>
#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <LittleFS.h>

// Define GPIOs
#define lightreader 36

// I2C Configuration
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

// Wi-Fi Configuration
const char *ssid = "VCS Smart Bin Portal";
const char *password = "password 123";

// Controller Configuration
const int id = 0;

WebServer server(80);

// Function to initialize I2C
void i2c_master_init()
{
    Wire.begin(I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    Wire.setClock(I2C_MASTER_FREQ_HZ);
    Serial.println("I2C initialized successfully");
}

void i2c_scanner();

void i2c_scanner()
{
    Serial.println("Scanning I2C devices...");
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Found I2C device at address: 0x");
            Serial.println(addr, HEX);
        }
    }
    Serial.println("I2C scan complete.");
}

// HTML handler function
void handleRoot()
{
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Failed to load homepage");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

int read_light()
{
    int light = analogRead(lightreader);
    return light;
}

// API to send JSON data
void handleData()
{
    // copy paste each line to add data to be sent through json (remember to double check ',')
    String json = "{\
    \"testContent\":\"" + String(read_light()) + "\",\
    \"id\":\"" + String(id) + "\",\
    \"weight\":\"" + String(read_light()) + "\",\
    \"time\":\"" + String(read_light()) + "\"\
    }";
    server.send(200, "application/json", json);
}

void setup()
{
    // Initialize Serial for debugging
    Serial.begin(115200);

    // Initialize I2C
    i2c_master_init();

    // Run I2C Scanner
    i2c_scanner();

    // Initialize Wi-Fi
    Serial.print("Setting up Wi-Fi...");
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Wi-Fi IP Address: ");
    Serial.println(IP);

    // Set up web server
    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS Mount Failed!");
        return;
    }
    Serial.println("LittleFS Initialized!");

    server.on("/", HTTP_GET, handleRoot);
    server.on("/data", HTTP_GET, handleData);
    server.begin();
    Serial.println("Web server started");
}

void loop()
{
    server.handleClient(); // Handle incoming HTTP requests
}
