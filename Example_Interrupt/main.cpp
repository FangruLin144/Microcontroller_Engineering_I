#include "mbed.h"

InterruptIn joystick_centre(D4);

class LED {
    protected: 
        DigitalOut outputSignal;
        bool status;

    public:
        LED(PinName pin): outputSignal(pin) {off();}
        void on(void) {
            outputSignal = 0;
            status = true;
        }
        void off(void) {
            outputSignal = 1;
            status = false;
        }
        void toggle(void) {
            if (status) off();
            else on();
        }
        bool getStatus(void) {
            return status;
        }
}green_led(D9), blue_led(D8);

void joystick_centre_pressed() {
    blue_led.toggle();
}

int main() {

    joystick_centre.rise(&joystick_centre_pressed);

    green_led.on();
    blue_led.off();

    while(1) {
        green_led.toggle();
        //blue_led.toggle();
        wait(0.5);
    }


}
