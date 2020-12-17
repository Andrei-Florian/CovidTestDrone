#include "IotCentral.h"

struct Global
{
	private:
		// private functions

        bool confirmPIN(double localPIN)
        {
            if(localPIN == unlockPIN)
            {
                return true;
            }

            return false;
        }

        bool PINCheck()
        {
            String rawResult = "";
            double pin = 0;
            int errorNumber = 0;

            double startMillis = millis();
            rgbled.off();

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
                        
                        rgbled.blue();
                        delay(200);
                        rgbled.off();

                        if(rawResult.length() > 5) // if 6 digits inputted
                        {
                            delay(1000);
                            Serial.println("[loop] Checking PIN");
                            pin = rawResult.toDouble();
                            delay(500);
                            
                            if(this->confirmPIN(pin)) // if PIN is correct
                            {
                                Serial.println("[loop] PIN Correct");
                                rgbled.green();
                                delay(500);
                                rgbled.off();
                                return true;
                            }
                            else
                            {
                                Serial.println("[loop] PIN Incorrect");
                                
                                rgbled.red();
                                delay(500);
                                rgbled.off();

                                errorNumber++;
                                rawResult = "";
                                pin = 0;
                            }
                        }
                    }
                }

                if((millis() - startMillis) > pinCheckTimeout)
                {
                    rgbled.red();
                    delay(500);
                    rgbled.off();

                    delay(500);
                    rgbled.blue();
                    delay(500);
                    rgbled.off();

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

        Location location;
        
		// global functions
        void getGeo()
		{
			loc.get(); // lock onto the satellite and get the geolocation values

			// set the global variables
			location.latitude = loc.latitude;
			location.longitude = loc.longitude;
			location.altitude = loc.altitude;
            location.accuracy = loc.gpsacc;
		}
        
		void set()
		{
            Serial.println("[setup] Procedure Initiated");
			rgbled.init(); // set the RGB LED as an output and enable control
            get.init(); // set the button as an input

            rgbled.white();
	        container.init();

            delay(500);
            
            get.init(); // initialise the sensors

			gsm.init(); // initialise the GSM service

			rtctime.set(); // get the time from the time server and synch the onboard RTC to it

			loc.set(); // lock onto the location from the GNSS service

            iotset(); // connect to IoT Central
            mqtt_client->setKeepAlive(keepConnectionAlive);

            Serial.println("[setup] complete");
			Serial.println("");
			Serial.println("");

            rgbled.off();
            delay(500);
		}

        void getData()
        {
            // get the geolocation
            this->getGeo();
        }

		void printData()
		{
			Serial.println("[loop] Data Collected");

            Serial.println("[location]  Latitude          " + String(location.latitude, 6));
            Serial.println("[location]  Longitude         " + String(location.longitude, 6));
            Serial.println("[location]  Altitude          " + String(location.altitude, 6));
            Serial.println("[location]  Accuracy          " + String(location.accuracy));
            Serial.println("\n");
		}

        int checkInterface()
        {
            // check what auth mode dev is in
            if(doorState) // check if the container is open
            {
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

            payload.replace(F("{deviceLocationAccuracy}"), String(global.location.accuracy, 2));
            
            Serial.println("[IoT] " + String(payload.c_str()));
            servicesStatus[MQTT_STATUS] = TRANSMITTING;  ;
            mqtt_client->publish(topic.c_str(), payload.c_str());
            servicesStatus[MQTT_STATUS] = CONNECTED;  ;

            Serial.println("[IoT] Telemetry is sent");
            Serial.println("");
        }

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
        }
};

IotCentral iot;