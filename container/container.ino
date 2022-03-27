/*  
	You need to go into file from library and change this line from:
	#define MQTT_MAX_PACKET_SIZE 128
	to:
	#define MQTT_MAX_PACKET_SIZE 2048

	Also need to resolve defines in keypad/src/key.h and key.cpp and keypad.cpp. Change all IDLE references to IDLE2
*/

#define ARDUINO_MKR ; // nessesary for using Logo Library on MKR devices
#include "./global.h"

void setup()
{
	Serial.begin(9600);
	delay(500);

	Serial.println("CovidTestDrone");

	global.set();
	sendsDoorState = 2;
}

void loop()
{
	// update the doorState telemetry property
	if (sendsDoorState != -1)
	{
		if (sendsDoorState == 0)
		{
			Serial.println("[loop] sendsDoorState is FALSE");
			container.close();
			doorState = DOOR_CLOSE;
		}
		else if (sendsDoorState == 1)
		{
			Serial.println("[loop] sendsDoorState is TRUE");
			container.open();
			doorState = DOOR_OPEN;
		}

		global.getData();
		iot.sendDoorState();
		sendsDoorState = -1;
	}

	rgbled.check(); // check doorState and set led accordingly

	// checking if local unlock interface used
	if (get.button())
	{
		// lock/unlock container
		if (global.checkInterface())
		{
			// get the geolocation
			global.getGeo();
		}
	}

	// send data
	if (millis() > (lastTelemetryMillis + telemetrySendInterval))
	{
		// get the data
		global.getData();

		// send the data
		iot.sendTelemetry();
		lastTelemetryMillis = millis();
	}

	iot.connect();		 // keep the connection to the backend alive
	mqtt_client->loop(); // background tasks
	delay(1);			 // run the loop every 100ms

	// print these values if debugging
	Serial.println("");
	Serial.println("data");
	Serial.println("      telemetrySendInterval     : " + String(telemetrySendInterval));
	Serial.println("      unlockPIN                 : " + String(unlockPIN));
	Serial.println("      doorState                 : " + String(doorState));
	Serial.println("      lockMode                  : " + String(lockMode));
	Serial.println("      showneo                   : " + String(showneo));
	Serial.println("      sendsDoorState            : " + String(sendsDoorState));
	Serial.println("");
}