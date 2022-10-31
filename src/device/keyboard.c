#include "keyboard.h"
#include "interrupt.h"
#include "io.h"
#include "stdint.h"
#include "printk.h"
#include "debug.h"
#include "ioqueue.h"

/* 键盘缓冲区 */
struct ioqueue KBD_BUFFER;
/* 状态记录 */
static bool CTRL_STATUS         = false;        // CTRL键状态
static bool SHIFT_STATUS        = false;       // SHIFT键状态
static bool ALT_STATUS          = false;         // ALT键状态
static bool CAPS_LOCK_STATUS    = false;   // 大写字母键状态
static bool NUM_LOCK_STATUS     = false;    //小键盘数字锁状态
static bool EXTEND_SCANCODE     = false;    // 记录通码是否以0xe0开始

/* 以通码为索引的二维数组，保存键盘映射
 * 
 */
static char KEY_MAP[][2] = {
/* 0x00 */      {0, 0},
/* 0x01 */      {ESC, ESC},
/* 0x02 */      {'1', '!'},
/* 0x03 */      {'2', '@'},
/* 0x04 */      {'3', '#'},
/* 0x05 */      {'4', '$'},
/* 0x06 */      {'5', '%'},
/* 0x07 */      {'6', '^'},
/* 0x08 */      {'7', '&'},
/* 0x09 */      {'8', '*'},
/* 0x0A */      {'9', '('},
/* 0x0B */      {'0', ')'},
/* 0x0C */      {'-', '_'},
/* 0x0D */      {'=', '+'},
/* 0x0E */      {BACKSPACE, BACKSPACE},
/* 0x0F */      {TAB, TAB},
/* 0x10 */      {'q', 'Q'},
/* 0x11 */      {'w', 'W'},
/* 0x12 */      {'e', 'E'},
/* 0x13 */      {'r', 'R'},
/* 0x14 */      {'t', 'T'},
/* 0x15 */      {'y', 'Y'},
/* 0x16 */      {'u', 'U'},
/* 0x17 */      {'i', 'I'},
/* 0x18 */      {'o', 'O'},
/* 0x19 */      {'p', 'P'},
/* 0x1A */      {'[', '{'},
/* 0x1B */      {']', '}'},
/* 0x1C */      {ENTER, ENTER},
/* 0x1D */      {CTRL_L_CAHR, CTRL_L_CAHR},
/* 0x1E */      {'a', 'A'},
/* 0x1F */      {'s', 'S'},
/* 0x20 */      {'d', 'D'},
/* 0x21 */      {'f', 'F'},
/* 0x22 */      {'g', 'G'},
/* 0x23 */      {'h', 'H'},
/* 0x24 */      {'j', 'J'},
/* 0x25 */      {'k', 'K'},
/* 0x26 */      {'l', 'L'},
/* 0x27 */      {';', ':'},
/* 0x28 */      {'\'', '"'},
/* 0x29 */      {'`', '~'},
/* 0x2A */      {SHIFT_L_CHAR, SHIFT_L_CHAR},
/* 0x2B */      {'\\', '|'},
/* 0x2C */      {'z', 'Z'},
/* 0x2D */      {'x', 'X'},
/* 0x2E */      {'c', 'C'},
/* 0x2F */      {'v', 'V'},
/* 0x30 */      {'b', 'B'},
/* 0x31 */      {'n', 'N'},
/* 0x32 */      {'m', 'M'},
/* 0x33 */      {',', '<'},
/* 0x34 */      {'.', '>'},
/* 0x35 */      {'/', '?'},
/* 0x36 */      {SHIFT_R_CHAR, SHIFT_R_CHAR},
/* 0x37 */      {'*', '*'},
/* 0x38 */      {ALT_L_CHAR, ALT_L_CHAR},
/* 0x39 */      {' ', ' '},
/* 0x3A */      {CAPS_LOCK_CHAR, CAPS_LOCK_CHAR}
/* 其他按键暂不做处理 */
};
static void intrKeyboardHandler(){
    bool ctrlDownLast = CTRL_STATUS;
    bool shiftDownLast = SHIFT_STATUS;
    bool capsLockDownLast = CAPS_LOCK_STATUS;
    bool breakCode = false;
    // 读取缓冲区，以便8042继续工作
    uint16 scanCode = inb(KBD_BUFFER_PORT);
    // 如果以0xe0开头，设置EXTEND_SCANCODE为true
    if(scanCode == 0xE0){
        EXTEND_SCANCODE = true;
        return;
    }
    // 如果上次是扩展的，需要合成扫描码
    if(EXTEND_SCANCODE){
        scanCode = 0xE000 | scanCode;
        EXTEND_SCANCODE = false;
    }
    // 查看是否是断码
    breakCode = ((scanCode & 0x0080) != 0);
    if (breakCode){
        uint16 makeCode = (scanCode &=0xFF7F);
        switch (makeCode)
        {
        case CTRL_L_MAKE:
        case CTRL_R_MAKE:
            CTRL_STATUS = false;
            break;
        case SHIFT_L_MAKE:
        case SHIFT_R_MAKE:
            SHIFT_STATUS = false;
            break;
        case ALT_L_MAKE:
        case ALT_R_MAKE:
            ALT_STATUS = false;
            break;
        default:
            break;
        }
        // 结束此次中断处理
        return;
    }else if((scanCode > 0x00 && scanCode < 0x3B) || scanCode == ALT_L_MAKE || scanCode == CTRL_L_MAKE){
        bool shift = false; // 判断SHIFT是否被按下
            /* 小于0x0E是数字键 */
        if((scanCode < 0x0E) || (scanCode == 0x29)  ||
            /* */
           (scanCode == 0x1A) || (scanCode == 0x1B) ||
           /* */
           (scanCode == 0x2B) || (scanCode == 0x27) ||
           /* */
           (scanCode == 0x28) || (scanCode == 0x33) ||
           /* */
           (scanCode == 0x34) || (scanCode == 0x35)){
            if(shiftDownLast) shift = true;
        }else{
            if(shiftDownLast && capsLockDownLast){
                shift = false;
            }else if(shiftDownLast || capsLockDownLast){
                shift = true;
            }else{
                shift = false;
            }
            uint8 index = (scanCode &=0x00FF); //将扫描码高8位置0，针对高字节是0xE0的扫描码
            char currentChar = KEY_MAP[index][shift];
            // 如果字符可见，输出
            if(currentChar){
                if(!ioqIsFull(&KBD_BUFFER)){
                    //putChar(currentChar);
                    ioqPutChar(&KBD_BUFFER, currentChar);
                }
                return;
            }
            switch (scanCode)
            {
            case CTRL_L_MAKE:
            case CTRL_R_MAKE:
                CTRL_STATUS = true;
                break;
            case SHIFT_L_MAKE:
            case SHIFT_R_MAKE:
                SHIFT_STATUS = true;
                break;
            case ALT_L_MAKE:
            case ALT_R_MAKE:
                ALT_STATUS = true;
                break;
            case CAPS_LOCK_MAKE:
            CAPS_LOCK_STATUS = !CAPS_LOCK_STATUS;
                break;
            default:
                logWarning("UnSupport CTRL key\n");
                break;
            }
        }
    }else{
        logWarning("UnSupport key\n");
    }
    return;
}
void initKeyboard(int (* step)(void)){
    printkf("[%02d] init keyboard\n", step());
    initIoqueue(&KBD_BUFFER);
    registerHandler(INTR_0x21_KEYBOARD, intrKeyboardHandler);
}