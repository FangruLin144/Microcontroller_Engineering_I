#include "mbed.h"
#include "C12832.h" // URL: http://os.mbed.com/users/askksa12543/code/C12832/

C12832 lcd(D11, D13, D12, D7, D10);
AnalogIn analog_source(A0);

int main() {
    float adc_in;
    float adc_voltage;

    while(1) {
        adc_in = analog_source.read();
        adc_voltage = adc_in * 3300; // unit: mV

        lcd.locate(0, 0);
        lcd.printf("adc_in: %f\nadc_voltage: %f mV", adc_in, adc_voltage);
        wait(0.1);
    }

}
