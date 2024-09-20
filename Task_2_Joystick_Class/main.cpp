#include "mbed.h"
//#include "mbed2/299/TARGET_NUCLEO_F401RE/TARGET_STM/TARGET_STM32F4/TARGET_NUCLEO_F401RE/PinNames.h"

class Joystick {

    private:
        DigitalIn up, down, left, right, fire;
    
    public:
        Joystick(PinName u, PinName d, PinName l, PinName r, PinName f)
            :up(u), down(d), left(l), right(r), fire(f) {}
        bool upPressed(void)    {return (up == 1)   ?true:false;}
        bool downPressed(void)  {return (down == 1) ?true:false;}
        bool leftPressed(void)  {return (left == 1) ?true:false;}
        bool rightPressed(void) {return (right == 1)?true:false;}
        bool firePressed(void)  {return (fire == 1) ?true:false;}
};

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
};

int main() {

    Joystick joystick(A2, A3, A4, A5, D4);
    LED redLed(D5), greenLed(D9), blueLed(D8); 

    while(1) {
        // LEDs off by default
        redLed.off();
        greenLed.off();
        blueLed.off();

        if (joystick.upPressed()/*red*/)             {redLed.on();}
        else if (joystick.downPressed()/*yellow*/)   {redLed.on(), greenLed.on();}
        else if (joystick.leftPressed()/*blue*/)     {blueLed.on();}
        else if (joystick.rightPressed()/*green*/)   {greenLed.on();}
        else if (joystick.firePressed()/*white*/)    {redLed.on(), greenLed.on(), blueLed.on();}
    }

    return 0;
}
