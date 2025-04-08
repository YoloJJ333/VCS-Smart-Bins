#include <Wire.h>
#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <HX711_ADC.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Define GPIOs
const int lightreader = 36;
const int HX711_dout = 17; // mcu > HX711 dout pin
const int HX711_sck = 16;  // mcu > HX711 sck pin

// I2C Configuration
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000

// Wi-Fi Configuration
// const char *ssid = "VCS Smart Bin Portal";
const char *ssid = "PenTest Warriors";
const char *password = "Penwarriorz23";

// Controller Configuration
const int id = 0;

// HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

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
  if (!file)
  {
    server.send(500, "text/plain", "Failed to load homepage");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

const int calVal_eepromAdress = 0;
unsigned long t = 0;

void calibrate()
{
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  while (_resume == false)
  {
    LoadCell.update();
    if (Serial.available() > 0)
    {
      if (Serial.available() > 0)
      {
        char inByte = Serial.read();
        if (inByte == 't')
          LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true)
    {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false)
  {
    LoadCell.update();
    if (Serial.available() > 0)
    {
      known_mass = Serial.parseFloat();
      if (known_mass != 0)
      {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet();                                          // refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); // get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;
  while (_resume == false)
  {
    if (Serial.available() > 0)
    {
      char inByte = Serial.read();
      if (inByte == 'y')
      {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n')
      {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("***");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("***");
}

void changeSavedCalFactor()
{
  float oldCalibrationValue = LoadCell.getCalFactor();
  boolean _resume = false;
  Serial.println("***");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false)
  {
    if (Serial.available() > 0)
    {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0)
      {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false)
  {
    if (Serial.available() > 0)
    {
      char inByte = Serial.read();
      if (inByte == 'y')
      {
#if defined(ESP8266) || defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n')
      {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("***");
}

int read_light()
{
  int light = analogRead(lightreader);
  return light;
}

boolean newDataReady = false;

float read_weight()
{
  if (newDataReady) {
    newDataReady = false;
    return LoadCell.getData();
  }
  return -1;
}

// API to send JSON data
void handleData()
{
  // copy paste each line to add data to be sent through json (remember to double check ',')
  String json = "{\
    \"id\":\"" + String(id) + "\",\
    \"weight\":\"" + String(read_weight()) + "\",\
    \"time\":\"" + String(millis()) + "\",\
    \"light\":\"" + String(read_light()) + "\"\
    }";
  server.send(200, "application/json", json);
}

void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  // Initialize I2C
  i2c_master_init();

  // Run I2C Scanner
  i2c_scanner();

  // Initialize Wi-Fi
  Serial.print("Setting up Wi-Fi...");
  // WiFi.softAP(ssid, password);
  Wifi.begin(ssid, password);
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

  LoadCell.begin();
  // LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 4000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true;                 // set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag())
  {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1)
      ;
  }
  else
  {
    LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update())
    ;
  calibrate(); // start calibration procedure
}

void loop()
{
  server.handleClient(); // Handle incoming HTTP requests

  // receive command from serial terminal
  if (Serial.available() > 0)
  {
    char inByte = Serial.read();
    if (inByte == 't')
      LoadCell.tareNoDelay(); // tare
    else if (inByte == 'r')
      calibrate(); // calibrate
    else if (inByte == 'c')
      changeSavedCalFactor(); // edit calibration value manually
  }

  // check if last tare operation is complete
  if (LoadCell.getTareStatus() == true)
  {
    Serial.println("Tare complete");
  }

  if (LoadCell.update()) {
    newDataReady = true;
    Serial.println(LoadCell.getData());
  }
}

// #include <ThreeWire.h>
// #include <RtcDS1302.h>
// ThreeWire myWire(26,27,35); // IO/DAT, SCLK/CLK, CE/RST
// RtcDS1302<ThreeWire> Rtc(myWire);
// #define countof(a) (sizeof(a) / sizeof(a[0]))
// void printDateTime(const RtcDateTime& dt)
// {
//  char datestring[20];

//  snprintf_P(datestring,
//  countof(datestring),
//  PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
//  dt.Month(),
//  dt.Day(),
//  dt.Year(),
//  dt.Hour(),
//  dt.Minute(),
//  dt.Second() );
// }
// void setup ()
// {
//  Serial.begin(115200);
//  Serial.print("compiled: ");
//  Serial.print(__DATE__);
//  Serial.println(__TIME__);
//  Rtc.Begin();
//  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
//  printDateTime(compiled);
//  Serial.println();
// //  Rtc.SetDateTime(compiled);
//  if (!Rtc.IsDateTimeValid())
//  {
//  // Common Causes:
//  // 1) first time you ran and the device wasn't running yet
//  // 2) the battery on the device is low or even missing
//  Serial.println("RTC lost confidence in the DateTime!");
//  Rtc.SetDateTime(compiled);
//  }
//  if (Rtc.GetIsWriteProtected())
//  {
//  Serial.println("RTC was write protected, enabling writing now");

//  Rtc.SetIsWriteProtected(false);
//  }
//  if (!Rtc.GetIsRunning())
//  {
//  Serial.println("RTC was not actively running, starting now");
//  Rtc.SetIsRunning(true);
//  }
//  RtcDateTime now = Rtc.GetDateTime();
//  if (now < compiled)
//  {
//  Serial.println("RTC is older than compile time! (Updating DateTime)");
//  Rtc.SetDateTime(compiled);
//  }
//  else if (now > compiled)
//  {
//  Serial.println("RTC is newer than compile time. (this is expected)");
//  }
//  else if (now == compiled)
//  {
//  Serial.println("RTC is the same as compile time! (not expected but all is fine)");
//  }
// }
// void loop ()
// {
//  RtcDateTime now = Rtc.GetDateTime();
//  printDateTime(now);
//  Serial.println();
//  if (!now.IsValid())
//  {
//  // Common Causes:
//  // 1) the battery on the device is low or even missing and the power line was disconnected
//  Serial.println("RTC lost confidence in the DateTime!");
//  }
//  delay(10000); // ten seconds
// }