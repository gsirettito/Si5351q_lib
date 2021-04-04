
#ifndef si5351q_h
#define si5351q_h

#include <Wire.h>

#define Si5351_Address 0x60

#define XTAL_25 25000000
#define XTAL_27 27000000

#define PLL_A 26
#define PLL_B 34

#define CLK0_REG
#define MS_0 42
#define CLK_EN 3

#define CLK_0 0
#define CLK_1 1
#define CLK_2 2

#define MaxFarey 8999

class Si5351q
{
private:
	uint8_t _xtal = XTAL_25;
	uint8_t writeRegister(uint8_t address, uint8_t value);
	uint8_t writeBlock(uint8_t address, uint8_t *value, uint8_t lenght);
	uint8_t readRegister(uint8_t address);

	void PLL(uint8_t pll, uint8_t multiplier);
	void PLL(uint8_t pll, uint8_t multiplier, uint32_t numerator, uint32_t denominator);
	void farey(float alpha, uint32_t maximum, uint32_t &x, uint32_t &y);

public:
	Si5351q(uint32_t xtal);
	void Begin();
	void Frequency(uint8_t pll, uint8_t clk, uint32_t frequency);
	void Enable(uint8_t clk);
	void Disable(uint8_t clk);
};

#endif
