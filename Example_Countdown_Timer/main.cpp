#include "mbed.h"
#include "C12832.h"
//#include "mbed2/299/TARGET_NUCLEO_F401RE/TARGET_STM/TARGET_STM32F4/TARGET_NUCLEO_F401RE/PinNames.h"

#define COUNTDOWN_FLASH_FREQ 2
#define COUNTDOWN_TIME_DEFAULT 5.0

class Led {
    private:
        DigitalOut output_signal;
        bool led_status;

    public:
        Led(PinName pin): output_signal(pin) {}
        void on()  {output_signal = 0; led_status = true;}
        void off() {output_signal = 1; led_status = false;}
        void toggle() {
            output_signal = !output_signal;
            led_status = !led_status;
        }
        bool get_led_status() const {return led_status;}
};

class Countdown_Timer: public Led {
    private:
        float countdown_time;
        Timeout countdown;
        Ticker led_ticker;
        bool timer_status;

    public:
        Countdown_Timer(PinName led_pin, float period)
            : Led(led_pin), countdown_time(period), timer_status(false) {}
        void timer_stop() {
            led_ticker.detach();
            countdown.detach();
            timer_status = false;
            on();
        }
        void ISR_end_timer() {
            timer_stop();
        }
        void start() {
            countdown.attach(callback(this, &Countdown_Timer::ISR_end_timer), countdown_time);
            led_ticker.attach(callback(this, &Led::toggle), 1.0/(COUNTDOWN_FLASH_FREQ*2));
            //off();
            timer_status = true;
        }
        void stop() {
            led_ticker.detach();
            countdown.detach();
            timer_status = false;
            on();
        }
        bool get_timer_status() {return timer_status;}
}; 

C12832 lcd_screen(D11, D13, D12, D7, D10);
InterruptIn joystick_fire(D4);
Countdown_Timer countdown_timer(D9, COUNTDOWN_TIME_DEFAULT); // green LED

void ISR_joystick_fire_pressed() {
    if(countdown_timer.get_timer_status() == false) countdown_timer.start();
    else countdown_timer.stop();
}

int main() {

    joystick_fire.rise(&ISR_joystick_fire_pressed);

    while(1) {}

}
