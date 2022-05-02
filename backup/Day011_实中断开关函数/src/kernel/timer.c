#include "timer.h"
#include "io.h"
#include "print.h"

static void setFrequency(uint8 counterPort, uint8 counterNo, uint8 rwl, uint8 mode, uint16 countValue)
{
    outb(CONTROL_PORT, (uint8)(counterNo << 6 | rwl << 4 | mode << 1 | BCD_FALSE));
    outb(counterPort, (uint8)countValue);
    outb(counterPort, (uint8)countValue >> 8);
}

void initTimer()
{
    putStr("[08] init timer\n");
    uint16 countVal = FREQUENCY_MAX / FREQUENCY_24Hz;
    setFrequency(PORT_COUNTER0, SC_COUNTER0, RWL_LOW_HIGH, COUNTOR_MODE2, countVal);
}