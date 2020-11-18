/*  
	Open the file at %HomePath%\Documents\Arduino\libraries\PubSubClient\src\PubSubClient.h in your favorite code editor. Change the line (line 26 in current version) from
	#define MQTT_MAX_PACKET_SIZE 128
	to:
	#define MQTT_MAX_PACKET_SIZE 2048
	
	Also need to resolve defines in keypad/src/key.h and key.cpp and keypad.cpp. Change all IDLE references to IDLE2
*/

#define ARDUINO_MKR; // nessesary for using Logo Library on MKR devices
#include "Universum_Logo.h"
#include "./global.h"

void setup() 
{
	Serial.begin(9600);
	delay(500);

	logoStart("CovidTestDrone"); // display the debug welcome screen

	global.set(); // set everything up
	sendsDoorState = 3;
}

void loop() 
{
	// update the doorState telemetry property
	if(sendsDoorState != -1)
	{
		if(sendsDoorState == DOOR_CLOSE) // if door should be closed
		{
			Serial.println("[loop] sendsDoorState is FALSE");
			container.close(); // close the container
			doorState = DOOR_CLOSE;
		}
		else if(sendsDoorState == DOOR_OPEN) // if door should be opened
		{
			Serial.println("[loop] sendsDoorState is TRUE");
			container.open(); // open the container
			doorState = DOOR_OPEN;
		}
		
		iot.sendDoorState(); // send the state of the container to the backend
		sendsDoorState = -1;
	}

	// checking if local unlock interface used (button)
	if(get.button())
	{
		if(global.checkInterface()) // authenticate and open/close container
		{
			// get the geolocation
			global.getGeo();
		}
	}

	// send data
	if(millis() > (lastTelemetryMillis + telemetrySendInterval)) // check interval of time for telemetry data send
	{
		// get the data
		global.getData();
		
		// send the data
		iot.sendTelemetry();
		lastTelemetryMillis = millis();
	}

	iot.connect(); // keep the connection to the backend alive
	mqtt_client->loop(); // background tasks
	delay(1); // run the loop every 100ms

	// print these values if debugging
	if(debugging)
	{
		Serial.println("");
		Serial.println("data");
		Serial.println("      telemetrySendInterval     : " + String(telemetrySendInterval));
		Serial.println("      unlockPIN                 : " + String(unlockPIN));
		Serial.println("      doorState                 : " + String(doorState));
		Serial.println("      lockMode                  : " + String(lockMode));
		Serial.println("      showneo                   : " + String(showneo));
		Serial.println("");
	}
}