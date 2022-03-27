#include <Keypad.h>
#include <Wire.h>
#include "SparkFunHTU21D.h"
#include "configure.h"

// temp and humidity
HTU21D gy21;

// Beam Brake
const int brakeTreshold = 500; // value below which the device assumes beam is broken

// LDR
const int LDRTreshold = 500; // value below which the device assumes the container is closed

// Keypad
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns

char keys[ROWS][COLS] =
    {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'x', '0', 'x'}};

byte rowPins[ROWS] = {7, 14, 13, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 6, 10};     //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// functions
struct RGBLed
{
    void init()
    {
        pinMode(rgb[RED], OUTPUT);
        pinMode(rgb[BLUE], OUTPUT);
        pinMode(rgb[GREEN], OUTPUT);
    }

    void white()
    {
        if (showneo)
        {
            digitalWrite(rgb[RED], HIGH);
            digitalWrite(rgb[GREEN], HIGH);
            digitalWrite(rgb[BLUE], HIGH);
        }
    }

    void red()
    {
        if (showneo)
        {
            digitalWrite(rgb[RED], HIGH);
            digitalWrite(rgb[GREEN], LOW);
            digitalWrite(rgb[BLUE], LOW);
        }
    }

    void green()
    {
        if (showneo)
        {
            digitalWrite(rgb[RED], LOW);
            digitalWrite(rgb[GREEN], HIGH);
            digitalWrite(rgb[BLUE], LOW);
        }
    }

    void blue()
    {
        if (showneo)
        {
            digitalWrite(rgb[RED], LOW);
            digitalWrite(rgb[GREEN], LOW);
            digitalWrite(rgb[BLUE], HIGH);
        }
    }

    void off()
    {
        digitalWrite(rgb[RED], LOW);
        digitalWrite(rgb[GREEN], LOW);
        digitalWrite(rgb[BLUE], LOW);
    }

    void check()
    {
        if (showneo)
        {
            if (doorState) // green led if the door is open
            {
                green();
            }
            else // red led if the door is closed
            {
                red();
            }
        }
        else // turn the LED off if LED feedback is disabled
        {
            off();
        }
    }
};

RGBLed rgbled;

struct Get
{
private:
public:
    void init()
    {
        pinMode(buttonPin, INPUT);
    }

    // get button state
    bool button()
    {
        if (digitalRead(buttonPin) == HIGH)
        {
            rgbled.blue();
            while (digitalRead(buttonPin) == HIGH)
                ;
            rgbled.off();

            return true;
        }
        return false;
    }
};

Get get;

struct LockControl
{
public:
    void open()
    {
        Serial.println("[loop] Unlocking Container");

        // unlock the container
        digitalWrite(lockPin, LOW);
        delay(2000);
    }

    void close()
    {
        Serial.println("[loop] Locking Container");

        // lock the container
        digitalWrite(lockPin, HIGH);
        delay(2000);
    }

    void init()
    {
        // initialize the lock device
        pinMode(lockPin, OUTPUT);
        Serial.println("[setup] Lock Control Initiated");
    }
};

LockControl lockControl;

struct Container
{
private:
    bool firstTime = true;

public:
    void open()
    {
        if (doorState == DOOR_CLOSE)
        {
            lockControl.open();
        }
    }

    void close()
    {
        if (doorState == DOOR_OPEN || firstTime)
        {
            lockControl.close();
            firstTime = false;
        }
    }

    void init()
    {
        lockControl.init();
        lockControl.close();
    }
};

Container container;

struct TempSensor
{
public:
    void init()
    {
        gy21.begin();
    }

    double temp()
    {
        return double(gy21.readTemperature());
    }

    double humidity()
    {
        return double(gy21.readHumidity());
    }
};

TempSensor tempSensor;

struct BreakBeamSensor
{
private:
    double getValue()
    {
        return analogRead(breakbeamPin);
    }

public:
    bool check()
    {
        return getValue() > brakeTreshold ? false : true;
    }
};

BreakBeamSensor breakBeamSensor;

struct LDRSensor
{
private:
    int getValue()
    {
        return analogRead(ldrPin);
    }

public:
    bool check()
    {
        return getValue() > brakeTreshold ? true : false;
    }
};

LDRSensor ldrSensor;