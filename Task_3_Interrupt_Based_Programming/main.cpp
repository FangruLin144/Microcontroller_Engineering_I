#include "mbed.h"
#include "C12832.h"

C12832 lcd_screen(D11, D13, D12, D7, D10);
Ticker lcd_ticker;

class RGBLED {
    private:
        DigitalOut red, green, blue;
        int status;
    
    public:
        RGBLED(PinName r, PinName g, PinName b)
            : red(r),  green(g), blue(b) {status = -1;}
        void off() {
            red = 1, green = 1, blue = 1;
            status = -1;
        }
        void red_on() {
            red = 0, green = 1, blue = 1;
            status = 0;
        }
        void green_on() {
            red = 1, green = 0, blue = 1;
            status = 1;
        }
        void blue_on() {
            red = 1, green = 1, blue = 0;
            status = 2;
        }
        void yellow_on() {
            red = 0, green = 0, blue = 1;
            status = 3;
        }
        int get_status() {return status;}
}rgb_led(D5, D9, D8);

class Equation {
    private:

    public:
        double num_1, num_2, ans;
        unsigned int current_num; 
        // current_num == 1: num_1
        // current_num == 2: num_2
        // current_num == 3: sign
        bool flag_NA; 
        // indicate whether the equation is valid
        // for the equation 3/0 the flag_NA will be false
        unsigned int sign;
        Equation()
            : num_1(0.0), num_2(0.0), ans(0.0), current_num(1), flag_NA(true), sign(0) {}
        void refresh_ans() {
            flag_NA = true;
            if (Equation::sign_table() == '+') ans = num_1 + num_2;
            else if (Equation::sign_table() == '-') ans = num_1 - num_2;
            else if (Equation::sign_table() == 'x') ans = num_1 * num_2;
            else if (Equation::sign_table() == '/') {
                if (num_2 == 0) flag_NA = false;
                else ans = num_1 / num_2;
            } else if (Equation::sign_table() == '^') ans = pow(num_1, num_2);
        }
        char sign_table() {
            if (Equation::sign == 0) return '+';
            else if (Equation::sign == 1) return '-'; 
            else if (Equation::sign == 2) return 'x';
            else if (Equation::sign == 3) return '/';
            else return '^';
        }
}equation;

void lcd_init() {
    lcd_screen.cls();        //Clear the screen
    lcd_screen.locate(20,0); //Locate at (20,0)
    lcd_screen.printf("%.0lf %c %.0lf = %.2lf", equation.num_1, equation.sign_table(), equation.num_2, equation.ans);
}

void lcd_cursor_print() {
    static bool flag_cursor = false;  // indicate if the flag is shown on the screen
    if (flag_cursor == false) {    // the cursor is not currently appearing on the screen
        flag_cursor = true;       // toggle the flag 
        if (equation.current_num == 1) {        // cursor under the first number
            lcd_screen.locate(20,8);
            lcd_screen.printf("-");
        } else if (equation.current_num == 2){ // cursor under the second number
            lcd_screen.locate(40,8);
            lcd_screen.printf("-");
        } else {                               // cursor under the sign
            lcd_screen.locate(30,8);
            lcd_screen.printf("-");
        }
    } else {
        flag_cursor = false;
        lcd_screen.locate(20,10);
        lcd_screen.printf("                         "); // print 25 spaces
    }
}

void lcd_equation_print() {
    lcd_screen.locate(20,0);
    lcd_screen.printf("                              "); // print 30 spaces
    if (equation.flag_NA == false) {
        lcd_screen.locate(20,0); //Locate at (20,0)
        lcd_screen.printf("%.0lf %c %.0lf = N/A", equation.num_1, equation.sign_table(), equation.num_2);
    } else {
        lcd_screen.locate(20,0); //Locate at (20,0)
        lcd_screen.printf("%.0lf %c %.0lf = %.2lf", equation.num_1, equation.sign_table(), equation.num_2, equation.ans);
    }
}

void joystick_up_pressed() {
    if (equation.current_num == 1) {
        equation.num_1 = int(equation.num_1 + 1) % 10;
    } else if (equation.current_num == 2) {
        equation.num_2 = int(equation.num_2 + 1) % 10;
    } else {
        equation.sign = (equation.sign + 1) % 5;
    }
    equation.refresh_ans();
    lcd_equation_print();
}

void joystick_down_pressed() {
    if (equation.current_num == 1) {
        equation.num_1 = int(equation.num_1 + 9)  % 10;
    } else if (equation.current_num == 2) {
        equation.num_2 = int(equation.num_2 + 9) % 10;
    } else {
        equation.sign = (equation.sign + 4) % 5;
    }
    equation.refresh_ans();
    lcd_equation_print();
}

void joystick_fire_pressed() {
    equation.current_num = equation.current_num % 3 + 1;
    lcd_cursor_print();
    
    if (equation.current_num == 1)      rgb_led.red_on(); 
    else if (equation.current_num == 2) rgb_led.yellow_on();
    else if (equation.current_num == 3) rgb_led.blue_on();
    else rgb_led.off();
}

int main() {

    InterruptIn joystick_up(A2);
    InterruptIn joystick_down(A3);
    InterruptIn joystick_fire(D4);

    lcd_init();
    rgb_led.red_on();

    lcd_ticker.attach(&lcd_cursor_print, 0.5);

    joystick_up.rise(&joystick_up_pressed);
    joystick_down.rise(&joystick_down_pressed);
    joystick_fire.rise(&joystick_fire_pressed);

    while(1) {}

}
