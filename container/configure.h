// User Settings
int timeZone                                = 1;        // difference from GMT
bool debugging                              = true;     // print debug data to the serial monitor

// gloabls
bool doorState                              = false;    // false for personal, true for enterprise
bool lockMode                               = true;     // false for personal, true for enterprise
bool showneo                                = true;     // false for personal, true for enterprise
double telemetrySendInterval                = 10000;    // the rate at which data is sent to the backend
double unlockPIN                            = 123456;   // the rate at which data is sent to the backend
double keepConnectionAlive                  = 1000000;  // the time to keep the connection to backend alive for (has to be greater than sleep)
double pinCheckTimeout                      = 10000;    // the amount of time after which the device exits 

int sendsDoorState                          = -1;

// Azure IoT Central device information
bool updatePropertiesNow                    = false;
static char PROGMEM iotc_enrollmentKey[]    = "";       // select administration from menu on left -> device connection -> SAS-IoT-Devices -> Primary Key
static char PROGMEM iotc_scopeId[]          = "";       // the specific device ID (click on connect -> ID Scope)
static char PROGMEM iotc_modelId[]          = "";       // custom device name (click on connect -> Device ID field)

// GSM information
static char PROGMEM PINNUMBER[]             = "";

// door Modes
const bool DOOR_CLOSE        = false;
const bool DOOR_OPEN         = true;

// Neopixel Modes
const int DEV_ERROR          = 0;
const int LOOP_ERROR         = 1;
const int LED_WHITE          = 2;
const int LED_BLUE           = 3;
const int LED_GREEN          = 4;
const int WAKEUP             = 5;

// APN data
static char PROGMEM GPRS_APN[]              = "Hologram";
static char PROGMEM GPRS_LOGIN[]            = "";
static char PROGMEM GPRS_PASSWORD[]         = "";

// Connection Pins for sensors
const int buttonPin = 1;
const int servoPin = 2;
const int rgb[] = {3, 4, 5};

const int RED   = 0;
const int GREEN = 1;
const int BLUE  = 2;

// Device properties being sent
#define DEVICE_PROP_FW_VERSION              "1.0.0-20190704"
#define DEVICE_PROP_MANUFACTURER            "Arduino"
#define DEVICE_PROP_PROC_MANUFACTURER       "Microchip"
#define DEVICE_PROP_PROC_TYPE               "SAMD21"
#define DEVICE_PROP_DEVICE_MODEL            "MKR1400 GSM"

// payload templates
String telemetryPayload = 
"{"
    "{LOCATION}, "
    "\"deviceLocationAccuracy\": {deviceLocationAccuracy}"
"}";

String doorStatePayload = 
"{"
    "{LOCATION}, "
    "\"doorState\"                      : {doorState},                  "
    "\"doorStateLocationAccuracy\"      : {doorStateLocationAccuracy}   "
"}";

String basePropertyPayload = 
"{ "
    "\"manufacturer\": \""          DEVICE_PROP_MANUFACTURER         "\", "
    "\"model\": \""                 DEVICE_PROP_DEVICE_MODEL         "\", "
    "\"processorManufacturer\": \"" DEVICE_PROP_PROC_MANUFACTURER    "\", "
    "\"processorType\": \""         DEVICE_PROP_PROC_TYPE            "\", "
    "\"fwVersion\": \""             DEVICE_PROP_FW_VERSION           "\", "
    "\"serialNumber\": \""          "{sn}"                           "\", "
    "\"imei\": \""                  "{imei}"                         "\"  "
"}";

String confirmPropertyPayload = 
"{ "
    "\"doorState\":               {doorState}                           , "
    "\"lockMode\":                {lockMode}                            , "
    "\"showneo\":                 {showneo}                             , "
    "\"telemetrySendInterval\":   {telemetrySendInterval}               , "
    "\"unlockPIN\":               {unlockPIN}                             "
"}";