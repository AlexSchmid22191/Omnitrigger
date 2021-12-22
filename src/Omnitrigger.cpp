#include "Arduino.h"

#define debug false
#define PULSE_MODE true

// Pin definitions
const byte relay_pins[]  = {3, 4, 5, 6};
const byte lpt_pins[] = {A3, A2, A1, A0};
const byte flip_pins[] = {9, 10, 11, 12};

const byte pin_local = 7;     //High for local
const byte pin_usb = 8;    //High for USB

//State array of Valve channels
boolean channel_states[] = {false, false, false, false};

volatile boolean pulse_flag = false;
volatile unsigned long pulse_length = 0;


//Functions
void set_channels();
void read_pins(const byte *input);
void read_serial();
void pulse();

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

    #if PULSE_MODE
    pinMode(2, INPUT);
    attachInterrupt(digitalPinToInterrupt(2), pulse, CHANGE);
    #endif

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
            #if PULSE_MODE
            if(pulse_flag)
            {
                unsigned long pulse_millis = pulse_length / 1000;
                // The tens digit indicated relay number
                unsigned long relay = (pulse_millis + 2) / 10;
                // The ones digit indicates state: 8-(1)2 --> false, 3-7 --> true
                boolean state = ((pulse_millis + 2) % 10) > 4;
                channel_states[relay - 1] = state;
                pulse_flag = false;
            }
            #endif
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
        if(x == 0x02 || x == '#')
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
            auto channel = (uint8_t)strtol(commandBuffer, &ctrl_seq, 10) - 1;
            // Map the channels 5 to 8 onto channels 1 to 4 (this is required as only 5 to 8 can be set via Javalab))
            channel %= 4;

            //Check if channel is between 1 and 4 (index 0 to 3)
            if(channel<0 || channel>3)
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
        // Minimal mode for Kirsten
        if(x == ':')
        {
            memset(commandBuffer, 0, 16);
            //Read into buffer
            Serial.readBytesUntil(0x0D, commandBuffer, 14);
            auto code = strtol(commandBuffer, nullptr, 10);
            byte channel = code/10;
            bool state = (bool)(code % 10);
            channel_states[channel-1] = state;
        }
    }
}

void pulse()
{
    bool state = digitalRead(2);
    pulse_length = state ? micros() : micros() - pulse_length;
    pulse_flag = !state;
}
