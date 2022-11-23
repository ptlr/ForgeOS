#include "init.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall-init.h"
#include "ide.h"
#include "fs.h"

static int step = 6;
static int currentStep(void){
    step += 1;
    return step - 1;
}
void init(void)
{
    printkf("[%02d] start init\n", currentStep());
    initIdt(currentStep); // IDT需要最先初始化
    initTimer(currentStep);
    initMem(currentStep);
    initThreadEnv(currentStep);
    consoleInit(currentStep);
    initKeyboard(currentStep);
    initTss(currentStep);
    syscallInit(currentStep);
    initIDE(currentStep);
    initFileSystem(currentStep);
}