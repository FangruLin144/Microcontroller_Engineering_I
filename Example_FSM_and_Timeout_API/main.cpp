#include "mbed.h"

#define NUMBER_OF_STATES 4
#define GLOBAL_REFRESH_PERIOD 0.01

typedef enum {e_init, e_red, e_green, e_blue} Program_State;
Program_State e_program_state;

class LED {                                           //Begin LED class definition

    protected:                                          //Protected (Private) data member declaration
        DigitalOut outputSignal;                        //Declaration of DigitalOut object
        bool status;                                    //Variable to recall the state of the LED

    public:                                             //Public declarations
        LED(PinName pin) : outputSignal(pin)
        {
            off();   //Constructor - user provides the pin name, which is assigned to the DigitalOut
        }

        void on(void)                                   //Public member function for turning the LED on
        {
            outputSignal = 0;                           //Set output to 0 (LED is active low)
            status = true;                              //Set the status variable to show the LED is on
        }

        void off(void)                                  //Public member function for turning the LED off
        {
            outputSignal = 1;                           //Set output to 1 (LED is active low)
            status = false;                             //Set the status variable to show the LED is off
        }

        void toggle(void)                               //Public member function for toggling the LED
        {
            if (status)                                 //Check if the LED is currently on
                off();                                  //Turn off if so
            else                                        //Otherwise...
                on();                                   //Turn the LED on
        }

        bool getStatus(void)                            //Public member function for returning the status of the LED
        {
            return status;                              //Returns whether the LED is currently on or off
        }
};

class Potentiometer  {                              //Begin Potentiometer class definition
    private:                                            //Private data member declaration
        AnalogIn inputSignal;                           //Declaration of AnalogIn object
        float VDD, currentSampleNorm, currentSampleVolts; //Float variables to speficy the value of VDD and most recent samples

    public:                                             // Public declarations
        Potentiometer(PinName pin, float v) : inputSignal(pin), VDD(v) {}   //Constructor - user provided pin name assigned to AnalogIn...
                                                                            //VDD is also provided to determine maximum measurable voltage
        float amplitudeVolts(void)                      //Public member function to measure the amplitude in volts
        {
            return (inputSignal.read()*VDD);            //Scales the 0.0-1.0 value by VDD to read the input in volts
        }
        
        float amplitudeNorm(void)                       //Public member function to measure the normalised amplitude
        {
            return inputSignal.read();                  //Returns the ADC value normalised to range 0.0 - 1.0
        }
        
        void sample(void)                               //Public member function to sample an analogue voltage
        {
            currentSampleNorm = inputSignal.read();       //Stores the current ADC value to the class's data member for normalised values (0.0 - 1.0)
            currentSampleVolts = currentSampleNorm * VDD; //Converts the normalised value to the equivalent voltage (0.0 - 3.3 V) and stores this information
        }
        
        float getCurrentSampleVolts(void)               //Public member function to return the most recent sample from the potentiometer (in volts)
        {
            return currentSampleVolts;                  //Return the contents of the data member currentSampleVolts
        }
        
        float getCurrentSampleNorm(void)                //Public member function to return the most recent sample from the potentiometer (normalised)
        {
            return currentSampleNorm;                   //Return the contents of the data member currentSampleNorm  
        }
};

Program_State get_next_state(Program_State state, bool next_state) {
    uint8_t state_index = static_cast<uint8_t>(state);
    if (next_state == true) {
        state_index = (state_index + 1) % NUMBER_OF_STATES;
    } else {
        state_index = (state_index + NUMBER_OF_STATES - 1) % NUMBER_OF_STATES;
    }
    return static_cast<Program_State>(state_index);
}

class Led_Sequencer {
    private:
        LED red, green, blue;

    public:
        Led_Sequencer(PinName r, PinName g, PinName b)
            : red(r), green(g), blue(b) {}
        void sequence() {
            e_program_state = get_next_state(e_program_state, true);
            switch (e_program_state) {
                case (e_init):
                    red.off(); green.off(); blue.off();
                    break;
                case (e_red):
                    red.on(); green.off(); blue.off();
                    break;
                case (e_blue):
                    blue.on(); red.off(); green.off();
                    break;
                case (e_green):
                    green.on(); red.off(); blue.off();
                    break;
                default: 
                    red.off(); green.off(); blue.off();
            }
        }
};

class Pot_Rotation {
    private:
        Potentiometer *pot;
        Led_Sequencer *sequencer;
        const float v_min, v_max, f_min, f_max;
        float gradient, intercept;
        float flash_period; // the flash period is in terms of a cycle red->green->blue
        Timeout led_flash;


    public:
        Pot_Rotation(Potentiometer *potentiometer, Led_Sequencer *seq, float v1, float v2, float f1, float f2)
            : pot(potentiometer), sequencer(seq), v_min(v1), v_max(v2), f_min(f1), f_max(f2) { 
                // calculates the flash frequency in terms of normalised voltage
                gradient = (f_max - f_min) / (1.0f);
                intercept = f_max - gradient * 1.0f;
                led_flash.attach(callback(this, &Pot_Rotation::rotation_update), GLOBAL_REFRESH_PERIOD);
            }
        
        void rotation_update() {
            sequencer->sequence();
            flash_period = 1.0f / (pot->amplitudeNorm() * gradient + intercept);
            led_flash.attach(callback(this, &Pot_Rotation::rotation_update), flash_period / 3.0f);
        }
};

int main() {
    Led_Sequencer *seq = new Led_Sequencer(D5, D9, D8);
    Potentiometer *pot_right = new Potentiometer(A1, 3.3);
    Pot_Rotation pot_right_rotation(pot_right, seq, 0, 3.3, 0.5, 5);

    while(1) {}
}
