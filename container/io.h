#include <Keypad.h>
#include <Wire.h> 
#include <Servo.h>

// Servo
Servo servo;

// Keypad
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns

char keys[ROWS][COLS] = 
{
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'x','0','x'}
};

byte rowPins[ROWS] = {7, 12, 11, 9};    //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 6, 10};        //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

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
        if(showneo)
        {
            digitalWrite(rgb[RED], HIGH);
            digitalWrite(rgb[GREEN], HIGH);
            digitalWrite(rgb[BLUE], HIGH);
        }
    }

    void red()
    {
        if(showneo)
        {
            digitalWrite(rgb[RED], HIGH);
            digitalWrite(rgb[GREEN], LOW);
            digitalWrite(rgb[BLUE], LOW);
        }
    }

    void green()
    {
        if(showneo)
        {
            digitalWrite(rgb[RED], LOW);
            digitalWrite(rgb[GREEN], HIGH);
            digitalWrite(rgb[BLUE], LOW);
        }
    }

    void blue()
    {
        if(showneo)
        {
            digitalWrite(rgb[RED], LOW);
            digitalWrite(rgb[GREEN], LOW);
            digitalWrite(rgb[BLUE], HIGH);
        }
    }

    void off()
    {
        if(showneo)
        {
            digitalWrite(rgb[RED], LOW);
            digitalWrite(rgb[GREEN], LOW);
            digitalWrite(rgb[BLUE], LOW);
        }
    }

    void check()
    {
        if(showneo)
        {
            if(doorState) // green led if the door is open
            {
                green();
            }
            else // red led if the door is closed
            {
                red();
            }
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
            if(digitalRead(buttonPin) == HIGH)
            {
                rgbled.blue();
                while(digitalRead(buttonPin) == HIGH);
                rgbled.off();

                return true;
            }
            return false;
        }
};

Get get;

struct ServoControl
{
    private:
        const int CLOSE_ANGLE = 180;
        const int OPEN_ANGLE = 90;

    public:
        void open()
        {
            servo.write(OPEN_ANGLE);
            delay(2000);
        }

        void close()
        {
            servo.write(CLOSE_ANGLE);
            delay(2000);
        }

        void init()
        {
            servo.attach(servoPin);
        }
};

ServoControl servoControl;

struct Container
{
    private:
        bool firstTime = true;

    public:
        void open()
        {
            if(doorState == DOOR_CLOSE)
            {
                Serial.println("[loop] Opening Container");
                servoControl.open();
            }
        }

        void close()
        {
            if(doorState == DOOR_OPEN || firstTime)
            {
                Serial.println("[loop] Closing Container");
                servoControl.close();
                firstTime = false;
            }
        }

        void init()
        {
            servoControl.init();
            servoControl.close();
        }
};

Container container;