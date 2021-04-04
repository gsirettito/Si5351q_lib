#include "Arduino.h"
#include "si5351q.h"

Si5351q::Si5351q(uint32_t xtal)
{
    if (xtal != XTAL_25 && xtal != XTAL_27)
        return;
    _xtal = xtal;
    Wire.begin();
}

void Si5351q::Begin()
{
    writeRegister(0x09, 0xFF); // disable OEB pin
    writeRegister(0x0F, 0x00); // XTAL as input for both PLLs
    writeRegister(0xB7, 0xC0); // 10pF	XTAL load capacitance
    writeRegister(0x03, 0xFF); // disable all outputs
}

void Si5351q::PLL(uint8_t pll, uint8_t multiplier)
{
    PLL(pll, multiplier, 0, 1);
}

void Si5351q::PLL(uint8_t pll, uint8_t multiplier, uint32_t numerator, uint32_t denominator)
{
    uint32_t MSNx_P1 = 128 * multiplier - 512;
    uint32_t MSNx_P2 = 0;
    uint32_t MSNx_P3 = denominator;

    if (multiplier < 15 || multiplier > 90)
        return;
    if (denominator < 1)
        return;
    if (numerator > 0xFFFFF)
        return;
    if (denominator > 0xFFFFF)
        return;

    if (numerator > 0)
    {
        MSNx_P1 = (uint32_t)(128 * multiplier + floor(128 * ((float)numerator / (float)denominator)) - 512);
        MSNx_P2 = (uint32_t)(128 * numerator - denominator * floor(128 * ((float)numerator / (float)denominator)));
    }

    uint8_t config[] = {
        (MSNx_P3 & 0x0000FF00) >> 8,
        MSNx_P3 & 0x000000FF,
        (MSNx_P1 & 0x00030000) >> 16,
        (MSNx_P1 & 0x0000FF00) >> 8,
        MSNx_P1 & 0x000000FF,
        ((MSNx_P3 & 0x000F0000) >> 12) | ((MSNx_P2 & 0x000F0000) >> 16),
        (MSNx_P2 & 0x0000FF00) >> 8,
        MSNx_P2 & 0x000000FF,
    };

    writeBlock(pll, config, sizeof(config));
    writeRegister(0xB1, 0xA0);
}

void Si5351q::Frequency(uint8_t pll, uint8_t clk, uint32_t frequency)
{
    float k, divider = _xtal / 1e6;
    uint32_t numerator = 0, denominator = 1;
    float multiplier = (float)frequency / 1.0e6;

    while (multiplier < 15)
    {
        multiplier *= 2;
        divider *= 2;
    }
    while (multiplier > 90)
    {
        multiplier /= 2;
        divider /= 2;
    }

    k = multiplier - (int)multiplier;
    multiplier = (int)multiplier;

    farey(k, MaxFarey, numerator, denominator);
    PLL(pll, multiplier, numerator, denominator);

    float fvco = _xtal * (multiplier + ((float)numerator / (float)denominator));

    k = divider - (int)divider;
    divider = (int)divider;

    if (k == 0)
    {
        writeRegister(MS_0 + clk, readRegister(MS_0 + clk) | 0b01000000);
        denominator = 1;
        numerator = 0;
    }
    else
    {
        writeRegister(MS_0 + clk, readRegister(MS_0 + clk) & 0b10111111);
        farey(k, MaxFarey, numerator, denominator);
    }

    uint32_t MSNx_P1 = 128 * divider - 512;
    uint32_t MSNx_P2 = 0;
    uint32_t MSNx_P3 = denominator;

    if (numerator > 0)
    {
        MSNx_P1 = (uint32_t)(128 * divider + floor(128 * ((float)numerator / (float)denominator)) - 512);
        MSNx_P2 = (uint32_t)(128 * numerator - denominator * floor(128 * ((float)numerator / (float)denominator)));
    }

    uint8_t config[] = {
        (MSNx_P3 & 0x0000FF00) >> 8,
        MSNx_P3 & 0x000000FF,
        (MSNx_P1 & 0x00030000) >> 16,
        (MSNx_P1 & 0x0000FF00) >> 8,
        MSNx_P1 & 0x000000FF,
        ((MSNx_P3 & 0x000F0000) >> 12) | ((MSNx_P2 & 0x000F0000) >> 16),
        (MSNx_P2 & 0x0000FF00) >> 8,
        MSNx_P2 & 0x000000FF,
    };

    writeBlock(MS_0 + clk * 8, config, sizeof(config));

    uint8_t _current_config = readRegister(CLK0_REG + clk);
    _current_config = pll == PLL_B ? _current_config | 0x20 : _current_config & (0x20 ^ 0xFF);
    _current_config = numerator == 0 ? _current_config | 0x40 : _current_config & (0x40 ^ 0xFF);

    writeRegister(CLK0_REG + clk, _current_config);
}

void Si5351q::Enable(uint8_t clk)
{
    writeRegister(CLK_EN, readRegister(CLK_EN) & ((1 << clk) ^ 0xFF));
}

void Si5351q::Disable(uint8_t clk)
{
    writeRegister(CLK_EN, readRegister(CLK_EN) | (1 << clk));
}

#pragma region Private methods

uint8_t Si5351q::writeRegister(uint8_t address, uint8_t value)
{
    Wire.beginTransmission(Si5351_Address);
    Wire.write(address);
    Wire.write(value);
    return Wire.endTransmission();
}

uint8_t Si5351q::writeBlock(uint8_t address, uint8_t *data, uint8_t lenght)
{
    Wire.beginTransmission(Si5351_Address);
    Wire.write(address);
    Wire.write(data, lenght);
    return Wire.endTransmission();
}

uint8_t Si5351q::readRegister(uint8_t address)
{
    Wire.beginTransmission(Si5351_Address);
    Wire.write(address);
    Wire.requestFrom(Si5351_Address, (uint8_t)1);

    return Wire.available() ? Wire.read() : -1;
}

void Si5351q::farey(float alpha, uint32_t maximum, uint32_t &x, uint32_t &y)
{
    uint32_t a = 0, b = 1;
    uint32_t c = 1, d = 1;

    while (b <= maximum && d <= maximum)
    {
        if (alpha * (b + d) == (a + c))
        {
            if ((b + d) <= maximum)
            {
                x = a + c;
                y = b + d;
                return;
            }
            else if (d > b)
            {
                x = c;
                y = d;
                return;
            }
            else
            {
                x = a;
                y = b;
                return;
            }
        }
        else if (alpha * (b + d) > (a + c))
        {
            a += c;
            b += d;
        }
        else
        {
            c += a;
            d += b;
        }
    }
    if (b > maximum)
    {
        x = c;
        y = d;
        return;
    }
    else
    {
        x = a;
        y = b;
        return;
    }
}

#pragma endregion
