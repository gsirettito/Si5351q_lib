#include "si5351q.h"

#include <Wire.h>

Si5351q siq = Si5351q(XTAL_25);

void setup(void)
{
    Serial.begin(9600);
    Serial.println("Si5351q Clockgen Test");
    Serial.println("");

    /* Initialise the sensor */
    siq.Begin();

    Serial.println("OK!");

    Serial.println("Set Output #0 to 14.5MHz");
    siq.Frequency(PLL_A, CLK_0, 14500000);

    /* Enable the clocks */
    siq.Enable(CLK_0);
}

void loop(void)
{
}