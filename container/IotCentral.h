// external dependencies
#include <time.h>
#include <SPI.h>
#include <MKRGSM.h>
#include <RTCZero.h>
#include <Arduino_MKRENV.h> // delete
#include <PubSubClient.h>
#include <Reset.h>
#include <ArduinoHttpClient.h>

// links to files
#include "./sha256.h"
#include "./base64.h"
#include "./parson.h"
#include "./utils.h"
#include "./InternalStorage.h" // copied from Wifi101OTA
#include "./UrlParser.h"

// pointers
int getHubHostName(char *scopeId, char *deviceId, char *key, char *hostName);

// Globals
String iothubHost;
String mqttUrl;
String userName;
String password;

GSMSSLClient mqttSslClient;
PubSubClient *mqtt_client = NULL;

bool mqttConnected = false;

long lastTelemetryMillis = 0;

double tempValue = 0.0;
double humidityValue = 0.0;
double batteryValue = 0.0;
bool panicButtonPressed = false;

typedef struct
{
    int signalQuality;
    bool isSimDetected;
    bool isRoaming;
    char imei[20];
    char iccid[20];
    long up_bytes;
    long dn_bytes;
} GsmInfo;

GsmInfo gsmInfo{0, false, false, "", "", 0, 0};

enum connection_status
{
    DISCONNECTED,
    PENDING,
    CONNECTED,
    TRANSMITTING
};

#define GPRS_STATUS 0
#define DPS_STATUS 1
#define MQTT_STATUS 2
connection_status servicesStatus[3] = {DISCONNECTED, DISCONNECTED, DISCONNECTED};

// MQTT publish topics
static const char PROGMEM IOT_EVENT_TOPIC[] = "devices/{device_id}/messages/events/";
static const char PROGMEM IOT_TWIN_REPORTED_PROPERTY[] = "$iothub/twin/PATCH/properties/reported/?$rid={request_id}";
static const char PROGMEM IOT_TWIN_REQUEST_TWIN_TOPIC[] = "$iothub/twin/GET/?$rid={request_id}";
static const char PROGMEM IOT_DIRECT_METHOD_RESPONSE_TOPIC[] = "$iothub/methods/res/{status}/?$rid={request_id}";

// MQTT subscribe topics
static const char PROGMEM IOT_TWIN_RESULT_TOPIC[] = "$iothub/twin/res/#";
static const char PROGMEM IOT_TWIN_DESIRED_PATCH_TOPIC[] = "$iothub/twin/PATCH/properties/desired/#";
static const char PROGMEM IOT_C2D_TOPIC[] = "devices/{device_id}/messages/devicebound/#";
static const char PROGMEM IOT_DIRECT_MESSAGE_TOPIC[] = "$iothub/methods/POST/#";

int requestId = 0;
int twinRequestId = -1;
double version = 0;

#include "trace.h"
#include "iotc_dps.h"
#include "pelion_certs.h"

void acknowledgeSetting(const char *propertyKey, const char *propertyValue, int version, bool success)
{
    // for IoT Central need to return acknowledgement
    const static char *PROGMEM responseTemplate = "{\"%s\":{\"value\":%s,\"statusCode\":%d,\"status\":\"%s\",\"desiredVersion\":%d}}";
    char payload[1024];

    if (!success) // check if the message being sent is a success message
    {
        sprintf(payload, responseTemplate, propertyKey, propertyValue, 401, F("unauthorized"), version);
    }
    else
    {
        sprintf(payload, responseTemplate, propertyKey, propertyValue, 200, F("completed"), version);
    }

    Serial.println("[IoT] Sending acknowledgement: " + String(payload));
    String topic = (String)IOT_TWIN_REPORTED_PROPERTY;
    char buff[20];
    topic.replace(F("{request_id}"), itoa(requestId, buff, 10));
    servicesStatus[MQTT_STATUS] = TRANSMITTING;
    ;
    mqtt_client->publish(topic.c_str(), payload);
    servicesStatus[MQTT_STATUS] = CONNECTED;
    ;
    requestId++;
}

String reutrnBoolString(bool input)
{
    if (input)
    {
        return "true";
    }

    return "false";
}

void confirmProperty()
{
    Serial.println("");
    Serial.println("[loop] Confirm digital twin properties");

    String topic = (String)IOT_TWIN_REPORTED_PROPERTY;
    char buff[60];
    topic.replace(F("{request_id}"), itoa(requestId, buff, 10));

    String payload = confirmPropertyPayload;
    payload.replace(F("{doorState}"), reutrnBoolString(doorState));
    payload.replace(F("{lockMode}"), reutrnBoolString(lockMode));
    payload.replace(F("{showneo}"), reutrnBoolString(showneo));
    payload.replace(F("{telemetrySendInterval}"), String(telemetrySendInterval));
    payload.replace(F("{unlockPIN}"), String(unlockPIN));

    servicesStatus[MQTT_STATUS] = TRANSMITTING;
    ;
    mqtt_client->publish(topic.c_str(), payload.c_str());
    servicesStatus[MQTT_STATUS] = CONNECTED;
    ;
    Serial.println("[IoT] " + String(payload.c_str()));
    Serial.println("[IoT] Digital Twin properties are sent");
    Serial.println("");
    requestId++; // not sure
}

// commands
void handleDirectMethod(String topicStr, String payloadStr)
{
    Serial.println("[IoT] Received command: " + String(topicStr.c_str()) + ", Payload: " + String(payloadStr.c_str()));
    Serial.println("[IoT] [WARNING] The device does not accept any commands (container state is changed through cloud to device messages)");
}

// cloud to device messages
void handleCloud2DeviceMessage(String topicStr, String payloadStr)
{
    Serial.println("[IoT] Processing cloud to device call: " + String(topicStr.c_str()) + ", Payload: " + String(payloadStr.c_str()));

    // check if the message is intended to change the container state
    const String method = "METHOD-NAME=COVIDTESTDRONE_61T%3ACHANGEDOORSTATE";

    if (topicStr.indexOf(method) != -1)
    {
        Serial.println("[IoT] Received message to change container state");
        Serial.println("      Command Name:   CHANGEDOORSSTATE");
        Serial.println("      Command Value: " + payloadStr);

        if (payloadStr == "true") // open door
        {
            sendsDoorState = 1;
        }
        else if (payloadStr == "false")
        {
            sendsDoorState = 0;
        }
        else
        {
            Serial.println("[IoT] Invalid payload, ignoring");
        }
    }
}

void handleTwinPropertyChange(String topicStr, String payloadStr)
{
    Serial.println("");

    Serial.println("[loop] Processing Received Digital twin change");
    Serial.println("[IoT] handleTwinPropertyChange - " + String(payloadStr.c_str()));

    // read the property values sent using JSON parser
    JSON_Value *root_value = json_parse_string(payloadStr.c_str());
    JSON_Object *root_obj = json_value_get_object(root_value);

    // variables to store values
    const char *propertyKeyRAW = json_object_get_name(root_obj, 0);
    double propertyValueNum = 0;
    version = 0;

    // store the values in variables created
    JSON_Object *valObj = json_object_get_object(root_obj, propertyKeyRAW);
    propertyValueNum = json_object_get_number(root_obj, propertyKeyRAW);
    version = json_object_get_number(root_obj, "$version");

    String propertyKey = String(propertyKeyRAW);

    // print these values
    Serial.println("    version          : " + String(version));

    // array with each datapoint
    bool error = false;
    char propertyValueStr[20];
    char *arr[] = {"lockMode", "ledFeedback", "telemetrySendInterval", "unlockPIN"};

    for (int i = 0; i < 4; i++)
    {
        if (json_object_has_value(root_obj, arr[i]))
        {
            bool boolTemp = false;
            double doubleTemp = 0;

            switch (i)
            {
            case 0:
                boolTemp = json_object_get_boolean(root_obj, arr[i]);
                lockMode = boolTemp;
                Serial.println("    lockMode set to                     " + String(boolTemp));
                break;
            case 1:
                boolTemp = json_object_get_boolean(root_obj, arr[i]);
                showneo = boolTemp;
                Serial.println("    showneo set to                      " + String(boolTemp));
                rgbled.off();
                break;
            case 2:
                doubleTemp = json_object_get_number(root_obj, arr[i]);

                if (doubleTemp > 2000000000 || doubleTemp < 1) // check if the number is too big or too small
                {
                    Serial.println("[IoT] Error - The number provided through the portal is too big or small to store");
                    error = true;
                }
                else
                {
                    telemetrySendInterval = doubleTemp;
                    Serial.println("    telemetrySendInterval set to    " + String(doubleTemp));
                }
                break;
            case 3:
                doubleTemp = json_object_get_number(root_obj, arr[i]);

                if (doubleTemp < 100000 || doubleTemp > 999999) // ensure the value has 6 digits
                {
                    Serial.println("[IoT] Error - The PIN provided is not 6 digits long");
                    error = true;
                }
                else
                {
                    unlockPIN = doubleTemp;
                    Serial.println("    unlockPIN set to   " + String(doubleTemp));
                }
                break;
            }
        }
    }

    itoa(propertyValueNum, propertyValueStr, 10);
    Serial.println("\n[IoT] Sending back value: " + String(propertyValueStr));
    if (error)
    {
        acknowledgeSetting(propertyKeyRAW, propertyValueStr, version, false);
    }
    else
    {
        acknowledgeSetting(propertyKeyRAW, propertyValueStr, version, true);
    }

    json_value_free(root_value);
    updatePropertiesNow = true;

    Serial.println("");
    delay(1000);

    // send property changed to backend as property payload
    confirmProperty();
}

// http://atmel.force.com/support/articles/en_US/FAQ/Reading-unique-serial-number-on-SAM-D20-SAM-D21-SAM-R21-devices
String chipId()
{
    volatile uint32_t val1, val2, val3, val4;
    volatile uint32_t *ptr1 = (volatile uint32_t *)0x0080A00C;
    val1 = *ptr1;
    volatile uint32_t *ptr = (volatile uint32_t *)0x0080A040;
    val2 = *ptr;
    ptr++;
    val3 = *ptr;
    ptr++;
    val4 = *ptr;

    char buf[33];
    sprintf(buf, "%8x%8x%8x%8x", val1, val2, val3, val4);
    return String(buf);
}

// callback for MQTT subscriptions
void callback(char *topic, byte *payload, unsigned int length)
{
    String topicStr = (String)topic;
    topicStr.toUpperCase();
    String payloadStr = (String)((char *)payload);
    payloadStr.remove(length);
    if (topicStr.startsWith(F("$IOTHUB/METHODS/POST/")))
    { // direct method callback
        handleDirectMethod(topicStr, payloadStr);
    }
    else if (topicStr.indexOf(F("/MESSAGES/DEVICEBOUND/")) > -1)
    { // cloud to device message
        handleCloud2DeviceMessage(topicStr, payloadStr);
    }
    else if (topicStr.startsWith(F("$IOTHUB/TWIN/PATCH/PROPERTIES/DESIRED")))
    { // digital twin desired property change
        handleTwinPropertyChange(topicStr, payloadStr);
    }
    else if (topicStr.startsWith(F("$IOTHUB/TWIN/RES")))
    { // digital twin response

        int result = atoi(topicStr.substring(topicStr.indexOf(F("/RES/")) + 5, topicStr.indexOf(F("/?$"))).c_str());
        int msgId = atoi(topicStr.substring(topicStr.indexOf(F("$RID=")) + 5, topicStr.indexOf(F("$VERSION=")) - 1).c_str());
        if (msgId == twinRequestId)
        {
            Serial.println("");
            Serial.println("[loop] Device Twin Update Requested");
            updatePropertiesNow = true;

            // twin request processing
            twinRequestId = -1;

            gsm.readGSMInfo(); // get the imei and iccid

            // output limited to 128 bytes so this output may be truncated
            Serial.println("[IoT] Current state of device twin: " + String(payloadStr.c_str()));

            // read the property values sent using JSON parser
            JSON_Value *root_value = json_parse_string(payloadStr.c_str());
            JSON_Object *root_obj = json_value_get_object(root_value);
            JSON_Object *desireObj = json_object_get_object(root_obj, "desired");

            // append to local variables
            char *keys[] = {"telemetrySendInterval", "unlockPIN", "lockMode", "ledFeedback"};
            double constVals[] = {10000, 123456, true, true}; // default vals for variables
            double arr[4];

            for (int i = 0; i < 2; i++)
            {
                double localVal = json_object_get_number(desireObj, keys[i]);
                if (localVal > 2000000000) // check if the number is too big
                {
                    Serial.println("[IoT] Error - The number provided through the portal is too big to store");
                    arr[i] = constVals[i];
                }
                else
                {
                    if (i == 1) // if processing PIN
                    {
                        if (arr[i] < 100000 || arr[i] > 999999)
                        {
                            arr[i] = constVals[i];
                        }
                    }
                    arr[i] = localVal;
                }

                Serial.println(localVal);
            }

            for (int i = 2; i < 4; i++)
            {
                arr[i] = json_object_get_boolean(desireObj, keys[i]);
            }

            // append verified values to globals
            telemetrySendInterval = arr[0];
            unlockPIN = arr[1];
            lockMode = arr[2];
            showneo = arr[3];

            // print these values
            Serial.println("      telemetrySendInterval     : " + String(telemetrySendInterval));
            Serial.println("      unlockPIN                 : " + String(unlockPIN));
            Serial.println("      lockMode                  : " + String(lockMode));
            Serial.println("      showneo                   : " + String(showneo));

            String topic = (String)IOT_TWIN_REPORTED_PROPERTY;
            char buff[20];
            topic.replace(F("{request_id}"), itoa(requestId, buff, 10));
            String payload = basePropertyPayload;

            payload.replace("{sn}", chipId());
            payload.replace("{imei}", gsm.imei);

            servicesStatus[MQTT_STATUS] = TRANSMITTING;
            ;
            Serial.println("[IoT] Publishing full twin: " + String(payload.c_str()));

            mqtt_client->publish(topic.c_str(), payload.c_str());
            servicesStatus[MQTT_STATUS] = CONNECTED;
            ;
            requestId++;

            // send confirmation
            confirmProperty();
        }
        else
        {
            if (result >= 200 && result < 300)
            {
                Serial.println("[IoT] Twin property " + String(msgId) + " updated");
            }
            else
            {
                Serial.println("[IoT] Twin property " + String(msgId) + "update failed - err: " + String(result));
            }
            Serial.println("");
        }
    }
    else
    { // unknown message
        Serial.println("[IoT] Unknown message arrived [" + String(topic) + "]Payload contains: " + String(payloadStr.c_str()));
    }
}

// connect to Azure IoT Hub via MQTT
void connectMQTT(String deviceId, String username, String password)
{
    mqttSslClient.stop(); // force close any existing connection
    servicesStatus[MQTT_STATUS] = DISCONNECTED;

    Serial.println("[IoT] Starting IoT Hub connection");
    int retry = 0;
    while (!mqtt_client->connected())
    {
        servicesStatus[MQTT_STATUS] = PENDING;
        Serial.println("[IoT] MQTT connection attempt #" + String(retry + 1));

        if (mqtt_client->connect(deviceId.c_str(), username.c_str(), password.c_str()))
        {
            Serial.println("[IoT] IoT Hub connection successful");
            mqttConnected = true;
        }
        else
        {
            servicesStatus[MQTT_STATUS] = DISCONNECTED;
            Serial.println("[IoT] IoT Hub connection error. MQTT rc=" + String(mqtt_client->state()));

            rgbled.red();
            delay(500);
            rgbled.white();

            delay(2000);
            retry++;
        }

        if (retry > 5)
        {
            rgbled.red();
            while (true)
                ;
        }
    }

    if (mqtt_client->connected())
    {
        servicesStatus[MQTT_STATUS] = CONNECTED;

        // add subscriptions
        mqtt_client->subscribe(IOT_TWIN_RESULT_TOPIC);        // twin results
        mqtt_client->subscribe(IOT_TWIN_DESIRED_PATCH_TOPIC); // twin desired properties
        String c2dMessageTopic = IOT_C2D_TOPIC;
        c2dMessageTopic.replace(F("{device_id}"), deviceId);
        mqtt_client->subscribe(c2dMessageTopic.c_str());  // cloud to device messages
        mqtt_client->subscribe(IOT_DIRECT_MESSAGE_TOPIC); // direct messages
    }
    else
    {
        servicesStatus[MQTT_STATUS] = DISCONNECTED;
    }
}

// create an IoT Hub SAS token for authentication
String createIotHubSASToken(char *key, String url, long expire)
{
    Serial.println("");
    Serial.println("[IoT] Creating IoT Hub SAS Token");

    url.toLowerCase();
    String stringToSign = url + "\n" + String(expire);
    int keyLength = strlen(key);

    int decodedKeyLength = base64_dec_len(key, keyLength);
    char decodedKey[decodedKeyLength];

    base64_decode(decodedKey, key, keyLength);

    Sha256 *sha256 = new Sha256();
    sha256->initHmac((const uint8_t *)decodedKey, (size_t)decodedKeyLength);
    sha256->print(stringToSign);
    char *sign = (char *)sha256->resultHmac();
    int encodedSignLen = base64_enc_len(HASH_LENGTH);
    char encodedSign[encodedSignLen];
    base64_encode(encodedSign, sign, HASH_LENGTH);
    delete (sha256);

    return (char *)F("SharedAccessSignature sr=") + url + (char *)F("&sig=") + urlEncode((const char *)encodedSign) + (char *)F("&se=") + String(expire);
}

void iotset()
{
    delay(100);

    // connect to GSM network
    Serial.println("[IoT] Getting IoT Hub host from Azure IoT DPS");
    servicesStatus[DPS_STATUS] = PENDING;

    char hostName[64] = {0};

    // compute derived key
    int keyLength = strlen(iotc_enrollmentKey);
    int decodedKeyLength = base64_dec_len(iotc_enrollmentKey, keyLength);
    char decodedKey[decodedKeyLength];
    base64_decode(decodedKey, iotc_enrollmentKey, keyLength);
    Sha256 *sha256 = new Sha256();
    sha256->initHmac((const uint8_t *)decodedKey, (size_t)decodedKeyLength);
    sha256->print(String(iotc_modelId).c_str());
    char *sign = (char *)sha256->resultHmac();
    memset(iotc_enrollmentKey, 0, sizeof(iotc_enrollmentKey));
    base64_encode(iotc_enrollmentKey, sign, HASH_LENGTH);
    delete (sha256);

    Serial.println("DEBUG - " + String(getHubHostName((char *)iotc_scopeId, (char *)String(iotc_modelId).c_str(), (char *)iotc_enrollmentKey, hostName)));
    servicesStatus[DPS_STATUS] = CONNECTED;

    iothubHost = hostName;
    Serial.println("DEBUG - iothubHost - " + String(iothubHost));
    Serial.println("DEBUG - hostName - " + String(hostName));

    Serial.println("");
    Serial.println("[IoT] Hostname: " + String(hostName));

    // create SAS token and user name for connecting to MQTT broker
    mqttUrl = iothubHost + urlEncode(String((char *)F("/devices/") + String(iotc_modelId)).c_str());
    Serial.println("DEBUG - mqttUrl - " + String(mqttUrl));

    String password = "";
    long check = rtctime.get();
    long expire = check + 864000;
    Serial.println("DEBUG - check - " + String(check));
    Serial.println("DEBUG - expire - " + String(expire));

    password = createIotHubSASToken(iotc_enrollmentKey, mqttUrl, expire);
    userName = iothubHost + "/" + String(iotc_modelId) + (char *)F("/?api-version=2018-06-30");

    Serial.println("DEBUG - username - " + String(userName));
    Serial.println("DEBUG - password - " + String(password));

    // connect to the IoT Hub MQTT broker
    mqtt_client = new PubSubClient(mqttSslClient);
    mqtt_client->setServer(iothubHost.c_str(), 8883);
    mqtt_client->setCallback(callback);

    connectMQTT(String(iotc_modelId), userName, password);

    // request full digital twin update
    String topic = (String)IOT_TWIN_REQUEST_TWIN_TOPIC;
    char buff[20];
    topic.replace(F("{request_id}"), itoa(requestId, buff, 10));
    twinRequestId = requestId;
    requestId++;
    servicesStatus[MQTT_STATUS] = TRANSMITTING;
    ;
    mqtt_client->publish(topic.c_str(), "");
    servicesStatus[MQTT_STATUS] = CONNECTED;
    ;

    // initialize timers
    lastTelemetryMillis = millis();
}