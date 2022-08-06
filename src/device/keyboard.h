#ifndef DEVICE_KEYBOARD_H
#define DEVICE_KEYBOARD_H

// 8042缓冲端口
#define KBD_BUFFER_PORT 0x60

/* 使用转义字符定义控制按键 */
#define ESC '\x1b' // 16进制表示
#define BACKSPACE '\b'
#define TAB '\t'
#define ENTER '\r'
#define DELETE  '\x7f'
/* 以上的不可见字符定义为0*/
#define CHAR_INVISIABLE 0
#define CTRL_L_CAHR     CHAR_INVISIABLE
#define CTRL_R_CHAR     CHAR_INVISIABLE
#define SHIFT_L_CHAR    CHAR_INVISIABLE
#define SHIFT_R_CHAR    CHAR_INVISIABLE
#define ALT_L_CHAR      CHAR_INVISIABLE
#define ALT_R_CHAR      CHAR_INVISIABLE
#define CAPS_LOCK_CHAR  CHAR_INVISIABLE

/* 定义控制字符的通码和断码 */
#define SHIFT_L_MAKE    0x2a
#define SHIFT_R_MAKE    0x36
#define ALT_L_MAKE      0x38
#define ALT_R_MAKE      0xe038
#define ALT_R_BREAK     0xe0b8
#define CTRL_L_MAKE     0x1d
#define CTRL_R_MAKE     0xe01d
#define CTRL_R_BREAK     0xe09d
#define CAPS_LOCK_MAKE  0x3a
// 导出键盘缓冲区
extern struct ioqueue KBD_BUFFER;
void initKeyboard();
#endif