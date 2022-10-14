#ifndef DEVICE_TIMER_H
#define DEVICE_TIMER_H

#define FREQUENCY_MAX       1193180 // 8253最大频率
#define FREQUENCY_100Hz      100
#define FREQUENCY_24Hz       24
#define CONTROL_PORT        0x43   // 控制端口

#define PORT_COUNTER0       0x40    // 计时器0的端口地址
#define PORT_COUNTER1       0x41    // 计时器1的端口地址
#define PORT_COUNTER2       0x42    // 计时器2的端口地址

#define SC_COUNTER0     0               // 使用
#define SC_COUNTER1     1               // 不使用
#define SC_COUNTER2     2               // 不使用

#define RWL_LATCH                      0           // 锁存
#define RWL_ONLY_LOW             1          // 只读写低字节
#define RWL_ONLY_HIGH            2          // 只读写高字节 
#define RWL_LOW_HIGH             3          // 先写低字节，再写高字节

#define COUNTOR_MODE0          0
#define COUNTOR_MODE1          1
#define COUNTOR_MODE2          2
#define COUNTOR_MODE3          3
#define COUNTOR_MODE4          4
#define COUNTOR_MODE5          5

#define BCD_FALSE            0
#define BCD_TRUE            1
void initTimer(int step);
#endif