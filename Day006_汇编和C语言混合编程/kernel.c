#include "print.h"
void kernelMain(void)
{
    cPutChar(0x07,'\n');
    cPutChar(0x07,'\n');
    cPutChar(0x07,'\n');
    cPutChar(0x07,'\n');
    cPutChar(4,'A');
    cPutChar(4,'B');
    cPutChar(4,'C');
    cPutChar(4,'\b');
    cPutChar(4,'D');
    while (1);
}
