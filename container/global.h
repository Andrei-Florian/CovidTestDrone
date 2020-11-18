#include "IotCentral.h"

// initialise the IoT Central connection
void iotset()
{
    //begin rtc
    rtc.begin();

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
    sha256->initHmac((const uint8_t*)decodedKey, (size_t)decodedKeyLength);
    sha256->print(String(iotc_modelId).c_str());
    char* sign = (char*) sha256->resultHmac();
    memset(iotc_enrollmentKey, 0, sizeof(iotc_enrollmentKey)); 
    base64_encode(iotc_enrollmentKey, sign, HASH_LENGTH);
    delete(sha256);
    neo.show(LED_WHITE, 12, 13);

    getHubHostName((char*)iotc_scopeId, (char*)String(iotc_modelId).c_str(), (char*)iotc_enrollmentKey, hostName);
    servicesStatus[DPS_STATUS] = CONNECTED;
    
    iothubHost = hostName;
    Serial.println("");
    Serial.println("[IoT] Hostname: " + String(hostName));

    // create SAS token and user name for connecting to MQTT broker
    mqttUrl = iothubHost + urlEncode(String((char*)F("/devices/") + String(iotc_modelId)).c_str());
    String password = "";
    long expire = rtctime.get() + 864000;
    password = createIotHubSASToken(iotc_enrollmentKey, mqttUrl, expire);
    userName = iothubHost + "/" + String(iotc_modelId) + (char*)F("/?api-version=2018-06-30");
    neo.show(LED_WHITE, 13, 14);

    // connect to the IoT Hub MQTT broker
    mqtt_client = new PubSubClient(mqttSslClient);
    mqtt_client->setServer(iothubHost.c_str(), 8883);
    mqtt_client->setCallback(callback);
    
    connectMQTT(String(iotc_modelId), userName, password);
    neo.show(LED_WHITE, 14, 15);
    
    // request full digital twin update
    String topic = (String)IOT_TWIN_REQUEST_TWIN_TOPIC;
    char buff[20];
    topic.replace(F("{request_id}"), itoa(requestId, buff, 10));
    twinRequestId = requestId;
    requestId++;
    servicesStatus[MQTT_STATUS] = TRANSMITTING;  ;
    mqtt_client->publish(topic.c_str(), "");
    servicesStatus[MQTT_STATUS] = CONNECTED;  ;
    
    // initialize timers
    lastTelemetryMillis = millis();
}

struct Global
{
	private:
		// check if the inputted PIN is correct
        bool confirmPIN(double localPIN)
        {
            if(localPIN == unlockPIN)
            {
                return true;
            }

            return false;
        }

        // loop through neopixel LEDs when inputting PIN
        void loopNeoFeedback(int input)
        {
            switch(input)
            {
                case 1:
                    neo.show(LED_BLUE, 0, 2);
                    break;
                case 2:
                    neo.show(LED_BLUE, 2, 4);
                    break;
                case 3:
                    neo.show(LED_BLUE, 4, 6);
                    break;
                case 4:
                    neo.show(LED_BLUE, 6, 8);
                    break;
                case 5:
                    neo.show(LED_BLUE, 8, 10);
                    break;
                case 6:
                    neo.show(LED_BLUE, 10, 12);
                    break;
            }
        }

        // receive PIN input
        bool PINCheck()
        {
            String rawResult = "";
            double pin = 0;
            int errorNumber = 0;

            double startMillis = millis();

            while(errorNumber < 3) // 3 attempts at password
            {
                char c = customKeypad.getKey();

                if(c) // if there is a key
                {
                    startMillis = millis();
                    
                    if(c != 'x') // check that input is a number
                    {
                        rawResult += c;
                        Serial.println(rawResult);
                        loopNeoFeedback(rawResult.length());

                        if(rawResult.length() > 5) // if 6 digits inputted
                        {
                            delay(1000);
                            Serial.println("[loop] Checking PIN");
                            pin = rawResult.toDouble();
                            neo.show(LED_BLUE, 12, 16);
                            delay(500);
                            
                            if(this->confirmPIN(pin)) // if PIN is correct
                            {
                                Serial.println("[loop] PIN Correct");
                                neo.show(LED_GREEN, 0, 16);
                                neo.hide(true);
                                return true;
                            }
                            else
                            {
                                Serial.println("[loop] PIN Incorrect");
                                neo.show(LOOP_ERROR, 0, 0);

                                errorNumber++;
                                rawResult = "";
                                pin = 0;
                            }
                        }
                    }
                }

                // break and return error after 10 seconds of inactivity
                if((millis() - startMillis) > pinCheckTimeout)
                {
                    neo.show(LOOP_ERROR, 0, 0);
                    break;
                }
            }

            return false;
        }

	public:
		// global variables
        struct Location
        {
            double latitude;
            double longitude;
            double altitude;
            double accuracy;
        };

        struct Status
        {
            double batteryRemaining;
            bool batteryConnected;
            bool testPresence;
        };

        Location location;
        Status status;
        
		// get and save the geolocation
        void getGeo()
		{
			loc.get(); // lock onto the satellite and get the geolocation values

			// set the global variables
			location.latitude = loc.latitude;
			location.longitude = loc.longitude;
			location.altitude = loc.altitude;
            location.accuracy = loc.gpsacc;
		}
        
        // loop to set everything up
		void set()
		{
            Serial.println("[setup] Procedure Initiated");
			neo.init(); // set the onboard LED as an output and enable control
            
            neo.show(WAKEUP, 0, 0);
	        container.init();
            get.init();

            delay(500);
            neo.show(LED_WHITE, 0, 2);
            
            get.init(); // initialise the sensors
            neo.show(LED_WHITE, 2, 4);

			gsm.init(); // initialise the GSM service
            neo.show(LED_WHITE, 4, 6);

			rtctime.set(); // get the time from the time server and synch the onboard RTC to it
            neo.show(LED_WHITE, 6, 9);

			loc.set(); // lock onto the location from the GNSS service
            neo.show(LED_WHITE, 9, 12);

            iotset(); // connect to IoT Central
            mqtt_client->setKeepAlive(keepConnectionAlive);
            neo.show(LED_WHITE, 15, 16);

            Serial.println("[setup] complete");
			Serial.println("");
			Serial.println("");

            neo.hide(true);
            delay(500);
            neo.show(LED_GREEN, 0, 16);
		}

        // get the battery and geolocation
        void getData()
        {
            // get the battery voltage
            status.batteryRemaining = battery.get();

            // get the geolocation
            this->getGeo();
        }

        // print the data collected if debugging
		void printData()
		{
            if(debugging)
            {
                Serial.println("[loop] Data Collected");

                Serial.println("[location]  Latitude          " + String(location.latitude, 6));
                Serial.println("[location]  Longitude         " + String(location.longitude, 6));
                Serial.println("[location]  Altitude          " + String(location.altitude, 6));
                Serial.println("[location]  Accuracy          " + String(location.accuracy));

                Serial.println("");
                Serial.println("[status]    Battery           " + String(status.batteryRemaining));
                Serial.println("\n");
            }
		}

        // check if the device is PIN unlocked or button unlocked
        int checkInterface()
        {
            // check what auth mode dev is in
            if(doorState) // check if the container is open
            {
                delay(500);
                neo.show(LED_GREEN, 0, 16);
                sendsDoorState = 0;
                return true;
            }
            else
            {
                if(lockMode) // if PIN unlock implemented
                {
                    if(this->PINCheck())
                    {
                        sendsDoorState = 1;
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                    
                }
                else
                {
                    delay(500);
                    neo.show(LED_GREEN, 0, 16);
                    sendsDoorState = 1;
                    return true;
                }
            }
        }
};

Global global;

struct IotCentral
{
    public:
        // connect to the MQTT client
        void connect()
        {
            if(!mqtt_client->connected())
            {
                Serial.println("[IoT] MQTT connection lost");
                // try to reconnect
                gsm.init();
                servicesStatus[DPS_STATUS] = CONNECTED; // we won't reprovision so provide feedback that last DPS call succeeded.
                
                // since disconnection might be because SAS Token has expired or has been revoked (e.g device had been blocked), regenerate a new one
                long expire = rtctime.get() + 864000;
                password = createIotHubSASToken(iotc_enrollmentKey, mqttUrl, expire);
                connectMQTT(String(iotc_modelId), userName, password);
            }

            // give the MQTT handler time to do it's thing
            mqtt_client->loop();
        }

        // send device telemetry to the backend
        void sendTelemetry()
        {
            Serial.println("");
            Serial.println("[loop] Sending telemetry to IoT Central");

            String topic = (String)IOT_EVENT_TOPIC;
            topic.replace(F("{device_id}"), iotc_modelId);

            String payload = telemetryPayload;
            
            payload.replace(F("{LOCATION}"), "\"deviceLocation\": { \"lat\": {lat}, \"lon\": {lon}, \"alt\": {alt}}");
            payload.replace(F("{lat}"), String(global.location.latitude, 7));
            payload.replace(F("{lon}"), String(global.location.longitude, 7));
            payload.replace(F("{alt}"), String(global.location.altitude, 7));

            payload.replace(F("{batteryRemaining}"), String(global.status.batteryRemaining, 2));
            payload.replace(F("{deviceLocationAccuracy}"), String(global.location.accuracy, 2));
            
            Serial.println("[IoT] " + String(payload.c_str()));
            servicesStatus[MQTT_STATUS] = TRANSMITTING;  ;
            mqtt_client->publish(topic.c_str(), payload.c_str());
            servicesStatus[MQTT_STATUS] = CONNECTED;  ;

            Serial.println("[IoT] Telemetry is sent");
            Serial.println("");
            neo.hide(true);
        }

        // send the doorState together with the geolocation of the event to the backend
        void sendDoorState()
        {
            Serial.println("");
            Serial.println("[loop] Sending doorState values as telemetry");

            String topic = (String)IOT_EVENT_TOPIC;
            topic.replace(F("{device_id}"), iotc_modelId);

            String payload = doorStatePayload;

            payload.replace(F("{LOCATION}"), "\"doorStateLocation\": { \"lat\": {lat}, \"lon\": {lon}, \"alt\": {alt}}");
            payload.replace(F("{lat}"), String(global.location.latitude, 7));
            payload.replace(F("{lon}"), String(global.location.longitude, 7));
            payload.replace(F("{alt}"), String(global.location.altitude, 7));

            payload.replace(F("{doorStateLocationAccuracy}"), String(global.location.accuracy, 2));
            payload.replace(F("{doorState}"), String(doorState));
            
            Serial.println("[IoT] " + String(payload.c_str()));
            servicesStatus[MQTT_STATUS] = TRANSMITTING;  ;
            mqtt_client->publish(topic.c_str(), payload.c_str());
            servicesStatus[MQTT_STATUS] = CONNECTED;  ;

            Serial.println("[IoT] Telemetry is sent");
            Serial.println("");
            neo.hide(true);
        }
};

IotCentral iot;