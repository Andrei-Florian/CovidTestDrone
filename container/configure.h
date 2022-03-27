#include "./secrets.h"

// User Settings
bool doorState = false;               // false for personal, true for enterprise
bool lockMode = true;                 // false for personal, true for enterprise
bool showneo = true;                  // false for personal, true for enterprise
double telemetrySendInterval = 10000; // the rate at which data is sent to the backend
double unlockPIN = 123456;            // the rate at which data is sent to the backend
double keepConnectionAlive = 1000000; // the time to keep the connection to backend alive for (has to be greater than sleep)
double pinCheckTimeout = 10000;       // the amount of time after which the device exits

int timeZone = 0; // difference from GMT
int sendsDoorState = -1;

// door Modes
const bool DOOR_CLOSE = false;
const bool DOOR_OPEN = true;

// Neopixel Modes
const int DEV_ERROR = 0;
const int LOOP_ERROR = 1;
const int LED_WHITE = 2;
const int LED_BLUE = 3;
const int LED_GREEN = 4;
const int WAKEUP = 5;

// Connection Pins for sensors
const int buttonPin = 1;
const int lockPin = 2;

const int ldrPin = 1;
const int breakbeamPin = 2;
const int rgb[] = {3, 4, 5};

const int RED = 0;
const int GREEN = 1;
const int BLUE = 2;

// Device properties being sent
#define DEVICE_PROP_FW_VERSION "1.0.0-20190704"
#define DEVICE_PROP_MANUFACTURER "Arduino"
#define DEVICE_PROP_PROC_MANUFACTURER "Microchip"
#define DEVICE_PROP_PROC_TYPE "SAMD21"
#define DEVICE_PROP_DEVICE_MODEL "MKR1400 GSM"

// payload templates
String telemetryPayload =
    "{"
    "{LOCATION}, "
    "\"deviceLocationAccuracy\": {deviceLocationAccuracy}, "
    "\"containerTemperature\": {containerTemperature}, "
    "\"containerHumidity\": {containerHumidity}, "
    "\"containerDoorState\": {containerDoorState}, "
    "\"testKitPresence\": {testKitPresence} "
    "}";

String doorStatePayload =
    "{"
    "{LOCATION}, "
    "\"containerState\"                      : {doorState},                  "
    "\"actionLocationAccuracy\"      : {doorStateLocationAccuracy}   "
    "}";

String basePropertyPayload =
    "{ "
    "\"manufacturer\": \"" DEVICE_PROP_MANUFACTURER "\", "
    "\"model\": \"" DEVICE_PROP_DEVICE_MODEL "\", "
    "\"processorManufacturer\": \"" DEVICE_PROP_PROC_MANUFACTURER "\", "
    "\"processorType\": \"" DEVICE_PROP_PROC_TYPE "\", "
    "\"fwVersion\": \"" DEVICE_PROP_FW_VERSION "\", "
    "\"serialNumber\": \""
    "{sn}"
    "\", "
    "\"imei\": \""
    "{imei}"
    "\"  "
    "}";

String confirmPropertyPayload =
    "{ "
    "\"containerState\":               {doorState}                           , "
    "\"lockMode\":                {lockMode}                            , "
    "\"ledFeedback\":                 {showneo}                             , "
    "\"telemetrySendInterval\":   {telemetrySendInterval}               , "
    "\"unlockPIN\":               {unlockPIN}                             "
    "}";