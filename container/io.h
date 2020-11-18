#include <Adafruit_NeoPixel.h>
#include <Keypad.h>
#include <Wire.h> 
#include <Servo.h>

// Servo
Servo servo;

// Neopixel Ring
const int pixels = 16;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(pixels, neopixelPin, NEO_GRB + NEO_KHZ800);

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns

char keys[ROWS][COLS] = 
{
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'#','0','*'}
};

byte rowPins[ROWS] = {7, 12, 11, 9};    //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 6, 10};        //connect to the column pinouts of the keypad

Keypad customKeypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

struct Neo
{
    public:
        // initialise the neopixel ring
        void init()
        {
            Serial.println("[setup] Initialising LED Strip");
            strip.begin();
            strip.show();
        }

        // hide all pixels
        void hide(bool animate)
        {
            if(showneo)
            {
                delay(1000);
            
                if(animate)
                {
                    for(int i = 0; i <= pixels; i++)
                    {
                        strip.setPixelColor(i, strip.Color(0, 0, 0));
                        strip.show();
                        delay(20);
                    }
                }
                else
                {
                    strip.clear();
                }
            }
        }

        // show pixels defined
        void show(int mode, int start, int end)
        {
            if(showneo)
            {
                if(mode == DEV_ERROR || mode == LOOP_ERROR)
                {
                    for(int i = 0; i < pixels; i++)
                    {
                        strip.setPixelColor(i, strip.Color(255, 0, 0));
                        
                        strip.show();
                        delay(20);
                    }

                    if(mode == LOOP_ERROR)
                    {
                        delay(1000);
                        
                        for(int i = 0; i < pixels; i++)
                        {
                            strip.setPixelColor(i, strip.Color(0, 0, 0));
                            
                            strip.show();
                            delay(20);
                        }
                    }
                }
                else if(mode == LED_WHITE)
                {
                    for(int i = start; i < end; i++)
                    {
                        strip.setPixelColor(i, strip.Color(255, 255, 255));
                        strip.show();
                        delay(20);
                    }
                }
                else if(mode == LED_BLUE)
                {
                    for(int i = start; i < end; i++)
                    {
                        strip.setPixelColor(i, strip.Color(0, 0, 255));
                        strip.show();
                        delay(20);
                    }
                }
                else if(mode == LED_GREEN)
                {
                    for(int i = start; i < end; i++)
                    {
                        strip.setPixelColor(i, strip.Color(0, 255, 0));
                        strip.show();
                        delay(20);
                    }
                }
                else // assume wakeup
                {
                    for(int i = 0; i < pixels; i++)
                    {
                        strip.setPixelColor(i, strip.Color(255, 255, 255));
                        strip.show();
                        delay(20);
                    }

                    this->hide(true);
                }
            }
        }
};

Neo neo;

struct Get
{
    public:
        // set the button pin as an input
        void init()
        {
            pinMode(buttonPin, INPUT);
        }

        // get button state 
        bool button()
        {
            if(digitalRead(buttonPin) == HIGH)
            {
                neo.show(LED_BLUE, 0, pixels);

                while(digitalRead(buttonPin) == HIGH);
                neo.hide(true);
                return true;
            }
            return false;
        }
};

Get get;

struct Battery
{
    public:
        // get the battery level
        double get()
        {
            Serial.println("[loop] Getting battery voltage");
            int sensorValue = analogRead(ADC_BATTERY);
            double voltage = sensorValue * (4.3 / 1023.0);
            return voltage;
        }
};

Battery battery;

struct ServoControl
{
    private:
        const int CLOSE_ANGLE = 0;
        const int OPEN_ANGLE = 179;

    public:
        // set the servo to the open position
        void open()
        {
            servo.write(OPEN_ANGLE);
            delay(2000);
        }

        // set the servo to the close position
        void close()
        {
            servo.write(CLOSE_ANGLE);
            delay(2000);
        }

        // attach the servo to the pin
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
        // open the container
        void open()
        {
            if(doorState == DOOR_CLOSE)
            {
                Serial.println("[loop] Opening Container");
                servoControl.open();
            }
        }

        // close the container
        void close()
        {
            if(doorState == DOOR_OPEN || firstTime)
            {
                Serial.println("[loop] Closing Container");
                servoControl.close();
                firstTime = false;
            }
        }

        // initialise by force closing the container
        void init()
        {
            servoControl.init();
            servoControl.close();
        }
};

Container container;