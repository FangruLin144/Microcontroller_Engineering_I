#include "mbed.h"
#include "C12832.h"
#include <cstdint>

// macro definition

#define NUMBER_OF_STATES 5 // states that can be accessed via up or down press
#define POT_SAMPLING_FREQ 100 // unit: Hz
#define LCD_SCREEN_REFRESH_PERIOD 0.1 // unit: s
#define COUNTDOWN_FLASH_FREQ 1 // unit: Hz
#define SPEAKER_FREQ 500 // unit: Hz

// class and type definition

typedef enum {e_init, 
              e_current_time, e_set_time, 
              e_world_time, 
              e_stopwatch_inactive, e_stopwatch_active, 
              e_countdown_timer_inactive, e_countdown_timer_active, e_countdown_timer_elapsed} Program_State;
Program_State e_program_state;

class LED {                                           //Begin LED class definition
    protected:                                          //Protected (Private) data member declaration
        DigitalOut outputSignal;                        //Declaration of DigitalOut object
        bool status;                                    //Variable to recall the state of the LED
    public:                                             //Public declarations
        LED(PinName pin) : outputSignal(pin) {off();}
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

class SamplingPotentiometer : public Potentiometer {
    private:
        float samplingFrequency, samplingPeriod;
        Ticker sampler;

    public:
        SamplingPotentiometer(PinName p, float v, float fs)
            : Potentiometer(p, v), samplingFrequency(fs) {
                samplingPeriod = 1.0f / samplingFrequency;
                sampler.attach(callback(this, &Potentiometer::sample), samplingPeriod);
            }
};

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

class Clock {
    private:
        int hour, min, sec;
        Ticker clock_ticker;

    public:
        Clock(): hour(0), min(0), sec(0) {
            clock_ticker.attach(callback(this, &Clock::tick), 1.0f);
        }
        void reset() {hour = 0; min = 0; sec = 0;}
        void tick() {
            sec++; 
            if (sec == 60) {
                sec = 0;
                min++;
                if (min == 60) {
                    min = 0;
                    hour++;
                    if (hour == 25) hour = 0;
                }
            }
        }
        void set_clock(int h, int m) {hour = h; min = m;}
        int get_hour() {return hour;}
        int get_min() {return min;}
        int get_sec() {return sec;}
};

class Stopwatch {
    private:
        LED led; // blue led
        Timer stopwatch_timer;
        bool stopwatch_status; // default: false, not running

    public:
        Stopwatch(PinName pin): led(pin), stopwatch_status(false) {

        }
        bool get_stopwatch_status() {return stopwatch_status;}
        void led_on() {led.on();}
        void led_off() {led.off();}
        void stopwatch_start() {
            stopwatch_timer.start();
            stopwatch_status = true;
        }
        void stopwatch_stop() {
            stopwatch_timer.stop();
            stopwatch_status = false;
        }
        void stopwatch_reset() {stopwatch_timer.reset();}
        float stopwatch_read() {return stopwatch_timer.read();}
};

class Countdown_Timer: public LED, public Speaker {
    private:
        float countdown_time;
        float current_time;
        bool timer_status;
        bool timer_elapsed;
        Timeout countdown;
        Ticker led_ticker;
        Ticker current_timer_ticker;
        Ticker speaker_ticker;

        void count_down() {current_time -= 1.0f;}
        void ISR_end_timer() {
            timer_stop();
            LED::on();
            speaker_ticker.attach(callback(this, &Speaker::toggle), 1.0f/(SPEAKER_FREQ*2));
            timer_elapsed = true;
            e_program_state = e_countdown_timer_elapsed;
        }

    public:
        Countdown_Timer(PinName led_pin, float period)
            : LED(led_pin), Speaker(D6), countdown_time(period), current_time(period), timer_status(false), timer_elapsed(false) {}
        void set_countdown_period(float t) {
            countdown_time = t; 
            current_time = int(t);
            timer_elapsed = false;
            speaker_ticker.detach();
            LED::off();
        }
        void timer_start() {
            countdown.attach(callback(this, &Countdown_Timer::ISR_end_timer), countdown_time);
            led_ticker.attach(callback(this, &LED::toggle), 1.0f/(COUNTDOWN_FLASH_FREQ*2));
            current_timer_ticker.attach(callback(this, &Countdown_Timer::count_down), 1.0);
            timer_status = true;
            timer_elapsed = false;
        }
        void timer_stop() {
            countdown.detach();
            led_ticker.detach();
            current_timer_ticker.detach();
            timer_status = false;
            timer_elapsed = false;
            LED::off();
        }
        bool get_countdown_timer_status() {return timer_status;}
        bool get_countdown_timer_elapsed_status() {return timer_elapsed;}
        float get_current_time() {return current_time;}
        float get_countdown_period() {return countdown_time;}
}; 

// function prototype definition

void joystick_up_pressed();
void joystick_down_pressed();
void joystick_fire_pressed();
void state_machine_init();
void state_machine_set_time();
void state_machine_current_time();
void state_machine_world_time();
void state_machine_stopwatch_inactive();
void state_machine_stopwatch_active();
void state_machine_countdown_timer_inactive();
void state_machine_countdown_timer_active();
void state_machine_countdown_timer_elapsed();

// global variable definition

C12832 lcd_screen(D11, D13, D12, D7, D10);
Clock *system_clock = new Clock;
SamplingPotentiometer *pot_left  = new SamplingPotentiometer(A0, 3.3f, POT_SAMPLING_FREQ);
SamplingPotentiometer *pot_right = new SamplingPotentiometer(A1, 3.3f, POT_SAMPLING_FREQ);
Stopwatch stopwatch(D8); // blue led
Countdown_Timer countdown_timer(D9, 0.0f); // green led

// functions

void joystick_down_pressed() {
    switch (e_program_state) {
        case (e_init): e_program_state = e_current_time; break;
        case (e_current_time): e_program_state = e_world_time; break;
        case (e_world_time): 
            if (stopwatch.get_stopwatch_status() == true) e_program_state = e_stopwatch_active;
            else e_program_state = e_stopwatch_inactive;
            break;
        case (e_stopwatch_inactive): 
        case (e_stopwatch_active):
            if (countdown_timer.get_countdown_timer_status() == true) e_program_state = e_countdown_timer_active;
            else e_program_state = e_countdown_timer_inactive;
            break;
        case (e_countdown_timer_inactive):
        case (e_countdown_timer_active):
        case (e_countdown_timer_elapsed):
            e_program_state = e_init;
            break;
        default:
            e_program_state = e_init;
    }
}

void joystick_up_pressed() {
    switch (e_program_state) {
        case (e_init): 
            if (countdown_timer.get_countdown_timer_status() == true) e_program_state = e_countdown_timer_active;
            else e_program_state = e_countdown_timer_inactive;
            break;
        case (e_countdown_timer_inactive):
        case (e_countdown_timer_active):
        case (e_countdown_timer_elapsed):
            if (stopwatch.get_stopwatch_status() == true) e_program_state = e_stopwatch_active;
            else e_program_state = e_stopwatch_inactive;
            break;
        case (e_stopwatch_inactive):
        case (e_stopwatch_active):
            e_program_state = e_world_time;
            break;
        case (e_world_time):
            e_program_state = e_current_time;
            break;
        case (e_current_time):
            e_program_state = e_init;
            break;
        default:
            e_program_state = e_init;
    }
}

void joystick_fire_pressed() {
    // avoid poor connection of joystick
    // that one actual press leads to several rising edges
    static Timer fire_timer;
    static bool first_enter = true;
    if (first_enter) fire_timer.start();
    else {
        if (fire_timer.read() <= 0.5f) return;
        fire_timer.reset();
    }

    switch (e_program_state) {
        case (e_init):
            e_program_state = e_set_time;
            break;
        case (e_set_time):
            e_program_state = e_init;
            break;
        case (e_stopwatch_inactive):
            e_program_state = e_stopwatch_active;
            break;
        case (e_stopwatch_active):
            e_program_state = e_stopwatch_inactive;
            break;
        case (e_countdown_timer_inactive):
            e_program_state = e_countdown_timer_active;
            break;
        case (e_countdown_timer_active):
            e_program_state = e_countdown_timer_inactive;
            break;
        case (e_countdown_timer_elapsed):
            e_program_state = e_countdown_timer_inactive;
            break;
        default:
            break;
    }

    first_enter = false;
}

void state_machine_init() {
    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Press Fire to set time:\n");
    lcd_screen.printf("%02d:%02d:%02d\n", system_clock->get_hour(), system_clock->get_min(), system_clock->get_sec());
}

void state_machine_set_time() {
    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Set new time (HH:MM)\n");
    int hour = int(pot_left->amplitudeNorm() * 24); 
    int min  = int(pot_right->amplitudeNorm() * 60);
    // theoretically it should be *23 or *59
    // however, the normalised voltage will never reach VDD due to internal circuitry design 
    // so here it is modified to *24 or *60 to make sure the highest value reaches 23 or 59
    system_clock->set_clock(hour, min);
    lcd_screen.printf("%02d:%02d\n", hour, min);
}

void state_machine_current_time() {
    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Current time:\n");
    lcd_screen.printf("%02d:%02d:%02d", system_clock->get_hour(), system_clock->get_min(), system_clock->get_sec());
}

void state_machine_world_time() {
    int time_zone_index = int(pot_left->amplitudeNorm() * 21); // 0 <= time_zone_index <= 20
    int new_hour = system_clock->get_hour();
    int new_min  = system_clock->get_min();
    int new_sec  = system_clock->get_sec();

    lcd_screen.cls();
    lcd_screen.locate(0, 0);

    switch (time_zone_index) {
        case 0: // GMT-11
            new_hour = (new_hour + 24 - 11) % 24;
            lcd_screen.printf("Pago Pago (GMT-11)\n");
            break;
        case 1: // GMT-10
            new_hour = (new_hour + 24 - 10) % 24;
            lcd_screen.printf("Papeete (GMT-10)\n");
            break;
        case 2: // GMT-9
            new_hour = (new_hour + 24 - 9) % 24;
            lcd_screen.printf("Sitka (GMT-9)\n");
            break;
        case 3: // GMT-8
            new_hour = (new_hour + 24 - 8) % 24;
            lcd_screen.printf("Los Angeles (GMT-8)\n");
            break;
        case 4: // GMT-7
            new_hour = (new_hour + 24 - 7) % 24;
            lcd_screen.printf("El Paso (GMT-7)\n");
            break;
        case 5: // GMT-6
            new_hour = (new_hour + 24 - 6) % 24;
            lcd_screen.printf("San Salvador (GMT-6)\n");
            break;
        case 6: // GMT-5
            new_hour = (new_hour + 24 - 5) % 24;
            lcd_screen.printf("Havana (GMT-5)\n");
            break;
        case 7: // GMT-4
            new_hour = (new_hour + 24 - 4) % 24;
            lcd_screen.printf("Valencia (GMT-4)\n");
            break;
        case 8: // GMT-3
            new_hour = (new_hour + 24 - 3) % 24;
            lcd_screen.printf("Buenos Aires (GMT-3)\n");
            break;
        case 9: // GMT-2
            new_hour = (new_hour + 24 - 2) % 24;
            lcd_screen.printf("Grytviken (GMT-2)\n");
            break;
        case 10: // GMT-1
            new_hour = (new_hour + 24 - 1) % 24;
            lcd_screen.printf("Praia (GMT-1)\n");
            break;
        case 11: // GMT+0
            lcd_screen.printf("London (GMT+0)\n");
            break;
        case 12: // GMT+1
            new_hour = (new_hour + 1) % 24;
            lcd_screen.printf("Melilla (GMT+1)\n");
            break;
        case 13: // GMT+2
            new_hour = (new_hour + 2) % 24;
            lcd_screen.printf("Juba (GMT+2)\n");
            break;
        case 14: // GMT+3
            new_hour = (new_hour + 3) % 24;
            lcd_screen.printf("Amman (GMT+3)\n");
            break;
        case 15: // GMT+3:30
            new_min = (new_min + 30) % 60;
            new_hour = ((new_min + 30) / 60 + 3) % 24;
            lcd_screen.printf("Tehran (GMT+3:30)\n");
            break;
        case 16: // GMT+4
            new_hour = (new_hour + 4) % 24;
            lcd_screen.printf("Dubai (GMT+4)\n");
            break;
        case 17: // GMT+8
            new_hour = (new_hour + 8) % 24;
            lcd_screen.printf("Shanghai (GMT+8)\n");
            break;
        case 18: // GMT+10
            new_hour = (new_hour + 10) % 24;
            lcd_screen.printf("Sydney (GMT+10)\n");
            break;
        case 19: // GMT+11
            new_hour = (new_hour + 11) % 24;
            lcd_screen.printf("Tofol (GMT+11)\n");
            break;
        case 20: // GMT+12
            new_hour = (new_hour + 12) % 24;
            lcd_screen.printf("Auckland (GMT+12)\n");
            break;
        default:
            lcd_screen.printf("London (GMT+0)\n");
    }

    lcd_screen.printf("%02d:%02d:%02d\n", new_hour, new_min, new_sec);
    lcd_screen.printf("%02d:%02d:%02d  (Manchester)", system_clock->get_hour(), system_clock->get_min(), system_clock->get_sec());
}

void state_machine_stopwatch_inactive() {
    if (stopwatch.get_stopwatch_status() == true) {
        stopwatch.led_off();
        stopwatch.stopwatch_stop();
    }
    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Stopwatch: inactive\n");
    lcd_screen.printf("Last time: %.2f", stopwatch.stopwatch_read());
}

void state_machine_stopwatch_active() {
    if (stopwatch.get_stopwatch_status() == false) {
        stopwatch.stopwatch_reset();
        stopwatch.stopwatch_start();
        stopwatch.led_on();
    }
    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Stopwatch: running\n");
    lcd_screen.printf("Time: %.2f s", stopwatch.stopwatch_read());
}

void state_machine_countdown_timer_inactive() {
    if (countdown_timer.get_countdown_timer_status() == true) {
        countdown_timer.timer_stop();
    }

    int min = int(pot_left->amplitudeNorm() * 100);
    int sec = int(pot_right->amplitudeNorm() * 60);
    countdown_timer.set_countdown_period(float(min*60+sec));

    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Set countdown period:\n");
    lcd_screen.printf("%02d:%02d", min, sec);
}

void state_machine_countdown_timer_active() {
    if (countdown_timer.get_countdown_timer_status() == false) {
        countdown_timer.timer_start();
    } 
    if (countdown_timer.get_countdown_timer_elapsed_status() == true) {
        e_program_state = e_countdown_timer_elapsed;
        state_machine_countdown_timer_elapsed();
    }

    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Countdown timer running:\n");
    lcd_screen.printf("%d / %d s", int(countdown_timer.get_current_time()), int(countdown_timer.get_countdown_period()));
}

void state_machine_countdown_timer_elapsed() {
    lcd_screen.cls();
    lcd_screen.locate(0, 0);
    lcd_screen.printf("Time period elapsed!");
}

int main() {

    InterruptIn joystick_up(A2);
    InterruptIn joystick_down(A3);
    InterruptIn joystick_fire(D4);

    joystick_up.rise(&joystick_up_pressed);
    joystick_down.rise(&joystick_down_pressed);
    joystick_fire.rise(&joystick_fire_pressed);

    e_program_state = e_init;

    while(1) {

        switch (e_program_state) {
            case (e_init):
                state_machine_init();
                break;
            case (e_set_time):
                state_machine_set_time();
                break;
            case (e_current_time):
                state_machine_current_time();
                break;
            case (e_world_time):
                state_machine_world_time();
                break;
            case (e_stopwatch_inactive):
                state_machine_stopwatch_inactive();
                break;
            case (e_stopwatch_active):
                state_machine_stopwatch_active();
                break;
            case (e_countdown_timer_inactive):
                state_machine_countdown_timer_inactive();
                break;
            case (e_countdown_timer_active):
                state_machine_countdown_timer_active();
                break;
            case (e_countdown_timer_elapsed):
                state_machine_countdown_timer_elapsed();
                break;
            default:
                state_machine_init();
        }

        wait(LCD_SCREEN_REFRESH_PERIOD);

    }

}
