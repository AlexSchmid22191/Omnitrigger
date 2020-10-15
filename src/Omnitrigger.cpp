#include "Arduino.h"

#define debug false

// Pin definitions
const byte relay_pins[]  = {3, 4, 5, 6};
const byte lpt_pins[] = {A4, A3, A5, A2};
const byte flip_pins[] = {9, 10, 11, 12};

const byte pin_local = 7;     //High for local
const byte pin_usb = 8;    //High for USB

//State array of Valve channels
boolean channel_states[] = {false, false, false, false};

//Functions
void set_channels();
void read_pins(const byte *input);
void read_serial();

void setup()
{
    // Start serial communication
    Serial.begin(9600);

    // Set pin modes and set output pins low
    for(auto &pin : relay_pins)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    for(auto &pin : flip_pins)
    {
        pinMode(pin, INPUT_PULLUP);
    }
    for(auto &pin : lpt_pins)
    {
        pinMode(pin, INPUT);
    }

    pinMode(pin_local, INPUT_PULLUP);
    pinMode(pin_usb, INPUT_PULLUP);
}

void loop()
{
    //If local mode is enabled, read the state of the flip switch pins into the state array
    if(digitalRead(pin_local))
    {
        read_pins(flip_pins);
    }
    else
    {
        //If LPT mode is enabled, read the state of the flip switch pins into the state array
        if(!digitalRead(pin_usb))
        {
            read_pins(lpt_pins);
        }
        else
        {
            read_serial();
        }
    }
    //Write the state from the state array to the mosfet pins
    set_channels();
}

//Write the state from the state array to the mosfet pins
void set_channels()
{
    for(byte channel=0; channel < 4; channel++)
    {
        digitalWrite(relay_pins[channel], channel_states[channel]);
    }
}

//Read pins defined in input and store result in the state array
void read_pins(const byte *input)
{
    for(byte channel=0; channel < 4; channel++)
    {
        channel_states[channel] = digitalRead(input[channel]);
    }
}

void read_serial()
{
    char commandBuffer[16];

    //Check if a serial communication is requested
    while((bool)Serial.available())
    {
        //Read bytes until the start character (0x02) is encountered
        int x = Serial.read();
        if(x == 0x02)
        {
            //Clear buffer
            memset(commandBuffer, 0, 16);
            //Read into buffer
            Serial.readBytesUntil(0x0D, commandBuffer, 14);

            //Replace commas with points
            char *ptr = &commandBuffer[0];
            while(ptr != nullptr)
            {
                ptr = strchr(ptr, ',');
                if(ptr != nullptr)
                {
                    *ptr = '.';
                }
            }

            //Parse for Valve channel
            char* ctrl_seq = nullptr;
            auto channel = (uint8_t)strtol(commandBuffer, &ctrl_seq, 0) - 1;

            //Check if channel is between 1 and 4
            if(channel<0 || channel > 3)
            {
                Serial.print("ER");
                Serial.print("\n");

                //Debug
#if debug
                Serial.print("Unknown command recieved! ");
# endif
            }

            else
            {
                //Set Valve state
                if(strncmp(ctrl_seq, "SSP", 3) == 0)
                {
                    auto value = (boolean)strtol(&commandBuffer[5], nullptr, 10);
                    {
                        channel_states[channel] = value;
                        Serial.print("OK");
                        Serial.print("\n");

                        //Debug
#if debug
                        Serial.print("Channel ");
                            Serial.print(channel +1);
                            Serial.print(" set to ");
                            Serial.println(value);
#endif
                    }
                }

                    //Read valve state
                else if(strncmp(ctrl_seq, "RSP", 3) == 0)
                {
                    Serial.print(channel_states[channel]);
                    Serial.print("\n");

                    //Debug
#if debug
                    Serial.print("Channel ");
                        Serial.print(channel + 1);
                        Serial.print(" set value is ");
                        Serial.println(channel_states[channel]);
#endif
                }
            }
        }
    }
}