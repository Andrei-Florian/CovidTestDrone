# Abstract

![Cover Image](./Images/figure1.jpg)

It is undeniable that our world was turned upside down with the coming of the COVID-19 virus. Over the past three years, people from everywhere in the world had to adapt to countless quarantines and surging cases. In moments of crisis such as this, opportunities arise for individuals and businesses to develop solutions to help us navigate uncharted waters.

One of the innovations that helped us the most in the pandemic, was the release of the COVID test. Over time, we iterated over our protocols for administering tests, each country going about a procedure they deem suitable for testing the population. With time, the number of pharmaceutical companies offering testing solutions to patients increased; arguably it is now easier than ever for people to get tested for the virus. Many countries offer walk-in testing centres where patients can get a PCR test and receive the results in 24 hours and companies have developed cheap, over the counter, rapid antigen tests that offer patients a result in minutes.

There is still plenty innovation to be done in this space though. Firstly, most options for getting a COVID test involve physically leaving the premise of your home to get tested. This inevitably means that a potentially infected person will interact with others on the way to get a test or at the location of the testing centre or store.

It may also be challenging for individuals with symptoms to get to a testing centre or acquire a test. Some people may be living too far away from a centre to get there safely while having symptoms. Other people may live too far away from a centre and opt to not bother getting tested as a result - the inconvenience of getting to a testing centre may deter some people from getting tested.

CovidTestDrone enables self-administered COVID-19 tests and other medical equipment to be delivered to patient’s homes via drone delivery and returned to the lab to be analysed within minutes. There is no need for patients to leave their homes nor get into contact with other individuals to get tested and individuals that would not have had access to a test can now rapidly secure one.

This project offers the most convenient and safe way to get tested for the virus: a patient can simply order a test online, select a convenient time slot and provide a location for the drone to land. They will then receive a COVID-19 test at their doorstep, provide a specimen, and send it back for analysis via the same drone.

The drones used in the delivery and return of tests are semi-autonomous: take-offs and landings are manual whereas flying from one destination to another is autonomous. The drone can be monitored live at the base using Azure IoT Central and QGroundControl, the latter being an open-source application used to plan routes for drones to execute and monitor them during flight.

The drone will send telemetry data to the base through two data streams: using radio communication to send basic flight data such as pitch, speed, and location of the drone to QGroundControl, and GSM to send over information about the test kit, container, and status of the shipment to IoT Central.

The drone can also be controlled remotely via QGroundControl and IoT Central: the flight plan can be changed, and manual control can be regained over the drone at any time. This allows full control over the functionality of the drone remotely if an unexpected occurrence happened. The operator can also control the drone through IoT Central: the container holding the test kit can be locked and unlocked remotely and the PIN to unlock the container can be changed.

The drone will stream live footage back to the base station via the Nav-Q onboard computer. This device will stream a video feed from a camera mounted on the drone live to the QGroundControl application.

The drone has been customised to carry a COVID-19 self-administered testing kit with it. A container has been attached to the drone to securely ship the test in a climatically controlled environment. There are specific guidelines for the transport of specimens to ensure that they do not get damaged; according to the CDC, samples must be shipped to the lab for analysis within 72 hours. During this time, they must be stored between 2-8 degrees Celsius on dry ice.

The container has a locking mechanism that requires the consumer to input a PIN to open the container and extract the test. This ensures that only authorised individuals have access to the test kit and specimen. The device is also equipped with a helpful LED to indicate if the container is locked or unlocked and signalise any errors with the shipment.

The device uses GSM connectivity to transmit key data back to the base. The device gathers information such as its geolocation, as well as if the container is open or closed, locked or unlocked, etc. This data is displayed live in IoT Central.

The container is also equipped with a variety of sensors that collect crucial data while in operation and report it live to the backend, for example, the device is equipped with a temperature and humidity sensor allowing the temperature and humidity of the container to be streamed live to IoT Central ensuring that the kit is held in appropriate conditions.

The container is also equipped with an IR breakbeam sensor which checks if the test kit is in the container and reports this live to the backend. This allows the operator to always know if the test kit is in the container. A photoresistor detects whether or not the container is open.

# Repository Structure

This repository centralises all public documents, code and tests related to CovidTestDrone. The table below gives a high-level overview of the repo's structure.

| Item          | Description                                                                       |
| ------------- | --------------------------------------------------------------------------------- |
| Project Paper | The complete CovidTestDrone paper                                                 |
| container     | The source code of the application running on the drone's container               |
| Enclosure     | STL files for all the enclosures for the project                                  |
| iotcentral    | An exported snapshot of the device template that can be imported into IoT Central |
| NavQ          | Scripts for the NavQ module                                                       |
| Schematics    | Fritzing schematics for the container's circuit                                   |

# Getting Started

The steps for constructing the project are all documented in this [Hackster blog post](https://www.hackster.io/andreiflorian/covidtestdrone-f38df5).

**Please note that the IoT Central secrets have been moved to a separate file - secrets.h - which needs to be created and populated following the steps below:**

1. Create a file named _secrets.h_ in the ./container directory
2. Populate the file with the entries below and adjust them to your credentials (use the guide above for reference)

```c++
// Azure IoT Central device information
bool updatePropertiesNow = false;
static char PROGMEM iotc_enrollmentKey[] = "";
static char PROGMEM iotc_scopeId[] = "";
static char PROGMEM iotc_modelId[] = "";

// GSM information
static char PROGMEM PINNUMBER[] = "";

// APN data
static char PROGMEM GPRS_APN[] = "";
static char PROGMEM GPRS_LOGIN[] = "";
static char PROGMEM GPRS_PASSWORD[] = "";
```

3. Your code is now ready to be flashed.
