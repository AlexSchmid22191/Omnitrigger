// Small Testscript to toggle pins 2 to 5 of an Arduino Uno, used to test the LPT input of the Omnitrigger box

#include <Arduino.h>

const byte pins[] = {2, 3, 4, 5};

void setup()
{

    for(auto &pin : pins)
    {
        pinMode(pin, OUTPUT);
    }

}


void loop()
{
    for(auto &pin : pins)
    {
        digitalWrite(pin, HIGH);
        delay(25);
    }
    for(auto &pin : pins)
    {
        digitalWrite(pin, LOW);
        delay(25);
    }
}