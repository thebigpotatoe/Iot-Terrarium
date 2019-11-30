// ************************************************************************************************************************ //
//                                                      Libraries                                                       ***    
// ************************************************************************************************************************ //
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include "user_interface.h"
#include "FS.h"
#include <ESP8266WiFi.h>
#include "IPAddress.h"
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESPAsyncTCP.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <ESPAsyncUDP.h>
#include "DHT.h"
#include "lwip/inet.h"
#include "lwip/dns.h"

// ************************************************************************************************************************ //
//                                                      Main Variables                                                  ***    
// ************************************************************************************************************************ //
// WiFi Variables - Please enter the credentials of your wifi to access your wifi
String SSID = "";                     // This must match the exact case and style of your wifi network
String Password = "";                 // Case sensiitive

// NTP Variables - Change this offest to the timezone you are in
#define UTC_OFFSET +10                // The offest of your timezone

// Data Collection Variables - These can be changed to measure more or less data at different rates. the collection period is 
// the time between sample collections. The time peroid for smaple is given by NUM_SAMPLES * COLLECTION_PERIOD. The defaults 
// of 288 samples and 150000 milliseconds gives 12hrs of data
#define NUM_SAMPLES 288               // Default 288
#define COLLECTION_PERIOD 150000      // Default 150000

// LED Variables - Please change this to match how you have connected you LED's
#define NUM_LEDS 3                    // The number of LED's you have connected
#define DATA_PIN 5                    // The pin that the LED's data line is on

// DHT Variables - Change these to suit which DHT sensor you have purchased
#define DHT_PIN 4                     // The data pin that you have connected your DHT sesnor to
#define DHTTYPE DHT11                 // Uncomment this when using the DHT11
// #define DHTTYPE DHT22              // Uncomment this when using the DHT22
// #define DHTTYPE DHT21              // Uncomment this when using the DHT21

// ************************************************************************************************************************ //
//                                                      Additional Variables                                            ***    
// ************************************************************************************************************************ //
// File System Variables 
bool spiffsCorrectSize      = false;

// Wifi Variables and Objects 
String programmedSSID       = SSID;
String programmedPassword   = Password;
bool wifiStarting           = false;
bool softApStarted          = false;
IPAddress accessPointIP     = IPAddress(192, 168, 1, 1);
WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;

// DNS and mDNS Objects
DNSServer captivePortalDNS;
MDNSResponder::hMDNSService mdnsService;

// Webserver and OTA Objects
ESP8266WebServer restServer(80);
ESP8266HTTPUpdateServer OTAServer;

// Web Sockets Variabels and Objects
WebSocketsServer webSocket(81);
bool processingMessage = false;
bool clientNeedsUpdate = false;
bool webSocketConnecting = false;

// NTP Variables and Objects
AsyncUDP udpClient;
bool ntpTimeSet                       = false;
String ntpHostName                    = "pool.ntp.org";
IPAddress ntpIpAddress                = IPAddress(0, 0, 0, 0);
unsigned long utcOffset               = UTC_OFFSET * 3600; // in seconds
unsigned long collectionPeriod        = 3600;
unsigned long currentEpochTime        = 0;
unsigned long lastNTPCollectionTime   = 0;

// Base Variables of the Light
#define DEFAULT_NAME "IoTerrium"      // - This is the name of the device, if you change this, this will be the URL you need to type in to access it
#define STATUS_LED_BRIGHTNESS 128     // - This is the status LED brightness, change this betweeen 0 and 1024 (This is not the brightness of the neopixels)
String  Name                          = DEFAULT_NAME;
unsigned long lastCollectionTime      = 0;

// LED string object and Variables
CRGB ledString[NUM_LEDS];
bool State                            = true;
bool previousState                    = false;
int colourRed                         = 128;
int colourGreen                       = 128;
int colourBlue                        = 128;
int previousRed                       = 0;
int previousGreen                     = 0;
int previousBlue                      = 0;

// DHT Variables and Objects
DHT dht(DHT_PIN, DHTTYPE);
int tempIndex = 0;
int humidityIndex = 0;
float temperatureArray[NUM_SAMPLES] = {};
float humidityArray[NUM_SAMPLES] = {};
unsigned long lastTemperatureTime = 0;
unsigned long lastHumidityTime = 0;

// Soil Moisture Variables and Objects 
#define SOIL_PIN A0
float moistureLevel = 0;

// ************************************************************************************************************************ //
//                                                      Setup Function                                                  ***    
// ************************************************************************************************************************ //
void setup() {
  // Add a short delay on start
  delay(1000);

  // Start Serial
  Serial.begin(115200);
  Serial.println();

  // Check if the flash has been set up correctly
  spiffsCorrectSize = checkFlashConfig();
  if (spiffsCorrectSize) {
    // Init the LED's
    ledInit();
    tempHumidityInit();
    moistureInit();

    // Init the time to a default of 2 Dec
    initNTP();

    // Get saved settings
    getConfig();

    // Start Wifi
    wifiInit();

    // Setup Webserver
    webServerInit();

    // Setup websockets
    websocketsInit();
  }
  else Serial.println("[setup] -  Flash configuration was not set correctly. Please check your settings under \"tools->flash size:\"");
}

// ************************************************************************************************************************ //
//                                                      Loop Function                                                   ***    
// ************************************************************************************************************************ //
void loop() {
  // Check if the flash was correctly setup
  if (spiffsCorrectSize) {
    // Handle the captive portal
    captivePortalDNS.processNextRequest();

    // Handle mDNS 
    MDNS.update();

    // Handle the webserver
    restServer.handleClient();
    
    // Handle Websockets
    webSocket.loop();

    // Get the time when needed
    handleNTP();

    // Update WS clients when needed
    updateClients();

    // Handle the wifi connection 
    handleWifiConnection();

    // Handle the IO 
    handleIO();
  }
  else {
    delay(10000);
    Serial.println("[loop] - Flash configuration was not set correctly. Please check your settings under \"tools->flash size:\"");
  }
}
