#include <Arduino.h>

void setup()
{
    pinMode(4, OUTPUT);
}

void loop()
{
    digitalWrite(4, HIGH);
    delay(20);
    digitalWrite(4, LOW);
    delay(2000);

    digitalWrite(4, HIGH);
    delay(25);
    digitalWrite(4, LOW);
    delay(2000);
}