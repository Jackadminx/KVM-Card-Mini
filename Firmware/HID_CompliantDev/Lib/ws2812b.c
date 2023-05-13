#include "ws2812b.h"


// 发0码
void Set0Code(void)
{
    GPIOA_SetBits(GPIO_Pin_13); // 发送帧复位信号
    __nop();
    __nop();
    GPIOA_ResetBits(GPIO_Pin_13); // 发送帧复位信号
    //       NOP();
}
// 发1码
void Set1Code(void)
{
    GPIOA_SetBits(GPIO_Pin_13); // 发送帧复位信号
    __nop();
    __nop();
    __nop();
    __nop();
    __nop();
    __nop();
    __nop();
    __nop();
    GPIOA_ResetBits(GPIO_Pin_13); // 发送帧复位信号
}
// 发一个像素
void SendOnePix(unsigned char buf[])
{
    unsigned char i, j;
    unsigned char temp;

    for (j = 0; j < 3; j++) {
        temp = buf[j];
        for (i = 0; i < 8; i++) {
            if (temp & 0x80)        //从高位开始发送
                    {
                Set1Code();
            } else                //发送“0”码
            {
                Set0Code();
            }
            temp = (temp << 1);      //左移位
        }
    }
}
