#include "mbed.h"
//#include "mbed2/299/TARGET_NUCLEO_F401RE/TARGET_STM/TARGET_STM32F4/TARGET_NUCLEO_F401RE/PinNames.h"

class Speaker {
    private: 
        DigitalOut outputSignal;
        bool state;
    public: 
        Speaker(PinName pin): outputSignal(pin) {};
        void on(void) {outputSignal = 1; state = true;}
        void off(void) {outputSignal = 0; state = false;}
        void toggle(void) {
            if (state == true) {
                state = false;
                outputSignal = 0;
            } else {
                state = true;
                outputSignal = 1;
            }
        }
};

int main() {

    Speaker speaker(D6);
    int freq_hz = 500; // unit: Hz 
    int wait_time_us = 1000000 / freq_hz; // unit: us

    while(1) {
        speaker.toggle();
        wait_us(wait_time_us);
        //speaker.off();
        //wait_us(wait_time_us);
    }

    return 0;
}
