/***************************************************************************** 
 * File Name          : Main.c
 * Author             : Jancgk
 * Version            : V1.1
 * Date               : 2022/01/25
 * Description        : 模拟兼容HID设备
 *********************************************************************************
  * Modified from WCH CH583 source code
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "CH58x_common.h"
#include "ws2812b.h"

#define DevEP0SIZE 0x40
uint8_t RGBSET[2] = {0, 0};
// 设备描述符
const uint8_t MyDevDescr[] = {0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, DevEP0SIZE, 0x3d, 0x41, 0x07, 0x21, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01};
// 配置描述符
const uint8_t MyCfgDescr[] = {
    0x09, 0x02, 0x29, 0x00, 0x01, 0x01, 0x04, 0xA0, 0x64, // 配置描述符
    0x09, 0x04, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x05, // 接口描述符
    0x09, 0x21, 0x00, 0x01, 0x00, 0x01, 0x22, 0x22, 0x00, // HID类描述符
    0x07, 0x05, 0x81, 0x03, 0x40, 0x00, 0x01,             // 端点描述符
    0x07, 0x05, 0x01, 0x03, 0x40, 0x00, 0x01              // 端点描述符
};
/*字符串描述符略*/
/*HID类报表描述符*/
const uint8_t HIDDescr[] = {0x06, 0x00, 0xff,
                            0x09, 0x01,
                            0xa1, 0x01,       // 集合开始
                            0x09, 0x02,       // Usage Page  用法
                            0x15, 0x00,       // Logical  Minimun
                            0x26, 0x00, 0xff, // Logical  Maximun
                            0x75, 0x08,       // Report Size
                            0x95, 0x0A,       // Report Counet
                            0x81, 0x06,       // Input
                            0x09, 0x02,       // Usage Page  用法
                            0x15, 0x00,       // Logical  Minimun
                            0x26, 0x00, 0xff, // Logical  Maximun
                            0x75, 0x08,       // Report Size
                            0x95, 0x0A,       // Report Counet
                            0x91, 0x06,       // Output
                            0xC0};
// 语言描述符
const uint8_t MyLangDescr[] = {0x04, 0x03, 0x09, 0x04};
// 厂家信息
const uint8_t MyManuInfo[] = {0x30, 0x03, 'M', 0, 'o', 0, 'y', 0, 'u', 0, ' ', 0, 'a', 0, 't', 0, ' ', 0, 'w', 0, 'o', 0, 'r', 0, 'k', 0, ' ', 0, 'T', 0, 'e', 0, 'c', 0, 'h', 0, 'n', 0, 'o', 0, 'l', 0, 'o', 0, 'g', 0, 'y', 0}; // MoYu at work Technology
// 产品信息
const uint8_t MyProdInfo[] = {0x1C, 0x03, 'K', 0, 'V', 0, 'M', 0, ' ', 0, 'C', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'M', 0, 'i', 0, 'n', 0, 'i', 0};

/**********************************************************/
uint8_t DevConfig, Ready = 0;
uint8_t SetupReqCode;
uint16_t SetupReqLen;
const uint8_t *pDescr;
uint8_t Report_Value = 0x00;
uint8_t Idle_Value = 0x00;
uint8_t USB_SleepStatus = 0x00; /* USB睡眠状态 */

// HID设备中断传输中上传给主机4字节的数据
uint8_t HID_Buf[10] = {0x0};

/*HID上下行数据*/
// uint8_t HIDInData[10] = {0x0};
uint8_t HIDOutData[10] = {0x0};
// uint8_t HIDOutDataTrigger = 0;
uint8_t HIDKeyLightsCode = 0;
// 检查是否支持usb switch
uint8_t USBSwitchSupportStatus = 0;

uint8_t buf[3] = {0};

/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(4))) uint8_t EP0_Databuf[64 + 64 + 64]; // ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4))) uint8_t EP1_Databuf[64 + 64];      // ep1_out(64)+ep1_in(64)
__attribute__((aligned(4))) uint8_t EP2_Databuf[64 + 64];      // ep2_out(64)+ep2_in(64)
__attribute__((aligned(4))) uint8_t EP3_Databuf[64 + 64];      // ep3_out(64)+ep3_in(64)

// USB2
//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#define U2DevEP0SIZE 0x40
// 设备描述符
const uint8_t U2MyDevDescr[] =
    {0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, U2DevEP0SIZE, 0x3d, 0x41,
     0x08, 0x21, 0x00, 0x01, 0x01, 0x02, 0x00, 0x01};
/*HID类报表描述符*/
const uint8_t U2KeyRepDesc[] = {0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x05, 0x07,
                                0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08,
                                0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0x95, 0x03, 0x75, 0x01,
                                0x05, 0x08, 0x19, 0x01, 0x29, 0x03, 0x91, 0x02, 0x95, 0x05, 0x75, 0x01,
                                0x91, 0x01, 0x95, 0x06, 0x75, 0x08, 0x26, 0xff, 0x00, 0x05, 0x07, 0x19,
                                0x00, 0x29, 0x91, 0x81, 0x00, 0xC0};
const uint8_t U2MouseRepDesc[] = {
    // Relative
    //   0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09,
    //   0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25,
    //   0x01, 0x75, 0x01, 0x95, 0x03, 0x81, 0x02, 0x75, 0x05, 0x95, 0x01, 0x81,
    //   0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81, 0x25,
    //   0x7f, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06, 0xC0, 0xC0

    // Absolute mouse
    0x05, 0x01, // Usage Page (Generic Desktop)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, // Usage (Pointer)
    0xA1, 0x00, // Collection (Physical)
    // 0x85, 0x0B,    // Report ID  [11 is SET at RUNTIME]
    // Buttons
    0x05, 0x09, // Usage Page (Button)
    0x19, 0x01, // Usage Minimum (0x01)
    0x29, 0x05, // Usage Maximum (0x05)
    0x15, 0x00, // Logical Minimum (0)
    0x25, 0x01, // Logical Maximum (1)
    0x95, 0x05, // Report Count (5)
    0x75, 0x01, // Report Size (1)
    0x81, 0x02, // Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x03, // Report Size (3)
    0x95, 0x01, // Report Count (1)
    0x81, 0x03, // Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    // Movement
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,       // Usage (X)
    0x09, 0x31,       // Usage (Y)
    0x15, 0x00,       // LOGICAL_MINIMUM (0)       ; Note: 0x15 = 1 Byte; 0x16 = 2 Byte; 0x17 = 4 Byte
    0x26, 0xFF, 0x7F, // LOGICAL_MAXIMUM (32767)   ; Note: 0x25 = 1 Byte, 0x26 = 2 Byte; 0x27 = 4 Byte Report
    0x35, 0x00,       // Physical Minimum (0)
    0x46, 0xff, 0x7f, // Physical Maximum (32767)
    0x75, 0x10,       // REPORT_SIZE (16)
    0x95, 0x02,       // REPORT_COUNT (2)
    0x81, 0x02,       // Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    // Wheel
    0x09, 0x38, // Usage (Wheel)
    0x15, 0x81, // Logical Minimum (-127)
    0x25, 0x7F, // Logical Maximum (127)
    0x35, 0x81, // Physical Minimum (same as logical)
    0x45, 0x7f, // Physical Maximum (same as logical)
    0x75, 0x08, // Report Size (8)
    0x95, 0x01, // Report Count (1)
    0x81, 0x06, // Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,       //   End Collection
    0xC0        // End Collection

};
// 配置描述符
const uint8_t U2MyCfgDescr[] = {
    0x09, 0x02, 0x3b, 0x00, 0x02, 0x01, 0x00, 0xE0,
    0x19,                                                 // 配置描述符
    0x09, 0x04, 0x00, 0x00, 0x01, 0x03, 0x01, 0x01, 0x00, // 接口描述符,键盘
    0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x3e, 0x00, // HID类描述符
    0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x0a,             // 端点描述符
    0x09, 0x04, 0x01, 0x00, 0x01, 0x03, 0x01, 0x02, 0x00, // 接口描述符,鼠标
    0x09, 0x21, 0x10, 0x01, 0x00, 0x01, 0x22, sizeof(U2MouseRepDesc) & 0xFF, sizeof(U2MouseRepDesc) >> 8, // HID类描述符
    0x07, 0x05, 0x82, 0x03, 0x06, 0x00, 0x0a                                                              // 端点描述符

};
/* USB速度匹配描述符 */
const uint8_t U2My_QueDescr[] = {0x0A, 0x06, 0x00, 0x02, 0xFF, 0x00, 0xFF,
                                 0x40, 0x01, 0x00};

/* USB全速模式,其他速度配置描述符 */
uint8_t U2USB_FS_OSC_DESC[sizeof(U2MyCfgDescr)] = {
    0x09, 0x07, /* 其他部分通过程序复制 */
};

// 产品信息
const uint8_t U2MyProdInfo[] = {0x1E, 0x03, 'K', 0, 'V', 0, 'M', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 't', 0, 'r', 0, 'o', 0, 'l', 0, 'l', 0, 'e', 0, 'd', 0};

/**********************************************************/
uint8_t U2DevConfig, U2Ready;
uint8_t U2SetupReqCode;
uint16_t U2SetupReqLen;
const uint8_t *pU2Descr;
uint8_t U2Report_Value = 0x00;
uint8_t U2Idle_Value = 0x00;
uint8_t U2USB_SleepStatus = 0x00; /* USB睡眠状态 */

/*鼠标键盘数据*/
uint8_t U2HIDMouse[6] = {0x0};
uint8_t U2HIDKey[8] = {0x0};
/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(4))) uint8_t U2EP0_Databuf[64 + 64 + 64]; // ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4))) uint8_t U2EP1_Databuf[64 + 64];      // ep1_out(64)+ep1_in(64)
__attribute__((aligned(4))) uint8_t U2EP2_Databuf[64 + 64];      // ep2_out(64)+ep2_in(64)
__attribute__((aligned(4))) uint8_t U2EP3_Databuf[64 + 64];      // ep3_out(64)+ep3_in(64)

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   USB 传输处理函数
 *
 * @return  none
 */
void USB_DevTransProcess(void) // USB设备传输中断处理
{
    uint8_t len, chtype;          // len用于拷贝函数，chtype用于存放数据传输方向、命令的类型、接收的对象等信息
    uint8_t intflag, errflag = 0; // intflag用于存放标志寄存器值，errflag用于标记是否支持主机的指令

    intflag = R8_USB_INT_FG; // 取得中断标识寄存器的值

    if (intflag & RB_UIF_TRANSFER) // 判断_INT_FG中的USB传输完成中断标志位。若有传输完成中断了，进if语句
    {
        if ((R8_USB_INT_ST & MASK_UIS_TOKEN) != MASK_UIS_TOKEN) // 非空闲   //判断中断状态寄存器中的5:4位，查看令牌的PID标识。若这两位不是11（表示空闲），进if语句
        {
            switch (R8_USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) // 取得令牌的PID标识和设备模式下的3:0位的端点号。主机模式下3:0位是应答PID标识位
            // 分析操作令牌和端点号
            {                         // 端点0用于控制传输。以下的端点0的IN和OUT令牌相应程序，对应控制传输的数据阶段和状态阶段。
            case UIS_TOKEN_IN:        // 令牌包的PID为IN，5:4位为10。3:0位的端点号为0。IN令牌：设备给主机发数据。_UIS_：USB中断状态
            {                         // 端点0为双向端点，用作控制传输。 “|0”运算省略了
                switch (SetupReqCode) // 这个值会在收到SETUP包时赋值。在后面会有SETUP包处理程序，对应控制传输的设置阶段。
                {
                case USB_GET_DESCRIPTOR:                                        // USB标准命令，主机从USB设备获取描述
                    len = SetupReqLen >= DevEP0SIZE ? DevEP0SIZE : SetupReqLen; // 本次包传输长度。最长为64字节，超过64字节的分多次处理，前几次要满包。
                    memcpy(pEP0_DataBuf, pDescr, len);                          // memcpy:内存拷贝函数，从(二号位)地址拷贝(三号位)字符串长度到(一号位)地址中
                    // DMA直接与内存相连，会检测到内存的改写，而后不用单片机控制就可以将内存中的数据发送出去。如果只是两个数组互相赋值，不涉及与DMA匹配的物理内存，就无法触发DMA。
                    SetupReqLen -= len;           // 记录剩下的需要发送的数据长度
                    pDescr += len;                // 更新接下来需要发送的数据的起始地址,拷贝函数用
                    R8_UEP0_T_LEN = len;          // 端点0发送长度寄存器中写入本次包传输长度
                    R8_UEP0_CTRL ^= RB_UEP_T_TOG; // 同步切换。IN方向（对于单片机就是T方向）的PID中的DATA0和DATA1切换
                    break;                        // 赋值完端点控制寄存器的握手包响应（ACK、NAK、STALL），由硬件打包成符合规范的包，DMA自动发送
                case USB_SET_ADDRESS:             // USB标准命令，主机为设备设置一个唯一地址，范围0～127，0为默认地址
                    R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) | SetupReqLen;
                    // 7位地址+最高位的用户自定义地址（默认为1），或上“包传输长度”（这里的“包传输长度”在后面赋值成了地址位）
                    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                    // R响应OUT事务ACK，T响应IN事务NAK。这个CASE分支里是IN方向，当DMA相应内存中，单片机没有数据更新时，回NAK握手包。
                    break; // 一般程序里的OUT事务，设备会回包给主机，不响应NAK。

                case USB_SET_FEATURE: // USB标准命令，主机要求启动一个在设备、接口或端点上的特征
                    break;

                default:
                    R8_UEP0_T_LEN = 0; // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输（数据字段长度为0的数据包，包里SYNC、PID、EOP字段都有）
                    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                    // R响应OUT事务ACK，T响应IN事务NAK。这个CASE分支里是OUT方向，当DMA相应内存中更新了数据且单片机验收正常时，回ACK握手包。
                    Ready = 1;
                    printf("Ready_STATUS = %d\n", Ready);
                    break;
                }
            }
            break;

            case UIS_TOKEN_OUT:      // 令牌包的PID为OUT，5:4位为00。3:0位的端点号为0。OUT令牌：主机给设备发数据。
            {                        // 端点0为双向端点，用作控制传输。 “|0”运算省略了
                len = R8_USB_RX_LEN; // 读取当前USB接收长度寄存器中存储的接收的数据字节数 //接收长度寄存器为各个端点共用，发送长度寄存器各有各的
            }
            break;

            case UIS_TOKEN_OUT | 1: // 令牌包的PID为OUT，端点号为1
            {
                if (R8_USB_INT_ST & RB_UIS_TOG_OK) // 硬件会判断是否正确的同步切换数据包，同步切换正确，这一位自动置位
                {                                  // 不同步的数据包将丢弃
                    R8_UEP1_CTRL ^= RB_UEP_R_TOG;  // OUT事务的DATA同步切换。设定一个期望值。
                    len = R8_USB_RX_LEN;           // 读取接收数据的字节数
                    DevEP1_OUT_Deal(len);          // 发送长度为len的字节，自动回ACK握手包。自定义的程序。
                }
            }
            break;

            case UIS_TOKEN_IN | 1:                                               // 令牌包的PID为IN，端点号为1
                R8_UEP1_CTRL ^= RB_UEP_T_TOG;                                    // IN事务的DATA切换一下。设定将要发送的包的PID。
                R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK; // 当DMA中没有由单片机更新数据时，将T响应IN事务置为NAK。更新了就发出数据。
                Ready = 1;
                printf("Ready_IN_EP1 = %d\n", Ready);
                break;
            }
            R8_USB_INT_FG = RB_UIF_TRANSFER; // 写1清零中断标志
        }

        if (R8_USB_INT_ST & RB_UIS_SETUP_ACT) // Setup包处理
        {
            R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
            // R响应OUT事务期待为DATA1（DMA收到的数据包的PID要为DATA1，否则算数据错误要重传）和ACK（DMA相应内存中收到了数据，单片机验收正常）
            // T响应IN事务设定为DATA1（单片机有数据送入DMA相应内存，以DATA1发送出去）和NAK（单片机没有准备好数据）。
            SetupReqLen = pSetupReqPak->wLength;   // 数据阶段的字节数      //pSetupReqPak：将端点0的RAM地址强制转换成一个存放结构体的地址，结构体成员依次紧凑排列
            SetupReqCode = pSetupReqPak->bRequest; // 命令的序号
            chtype = pSetupReqPak->bRequestType;   // 包含数据传输方向、命令的类型、接收的对象等信息

            len = 0;
            errflag = 0;
            if ((pSetupReqPak->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) // 判断命令的类型，如果不是标准请求，进if语句
            {
                /* 非标准请求 */
                /* 其它请求,如类请求，产商请求等 */
                if (pSetupReqPak->bRequestType & 0x40) // 取得命令中的某一位，判断是否为0，不为零进if语句
                {
                    /* 厂商请求 */
                }
                else if (pSetupReqPak->bRequestType & 0x20) // 取得命令中的某一位，判断是否为0，不为零进if语句
                {                                           // 判断为HID类请求
                    switch (SetupReqCode)                   // 判断命令的序号
                    {
                    case DEF_USB_SET_IDLE: /* 0x0A: SET_IDLE */ // 主机想设置HID设备特定输入报表的空闲时间间隔
                        Idle_Value = EP0_Databuf[3];
                        break; // 这个一定要有

                    case DEF_USB_SET_REPORT: /* 0x09: SET_REPORT */ // 主机想设置HID设备的报表描述符
                        break;

                    case DEF_USB_SET_PROTOCOL: /* 0x0B: SET_PROTOCOL */ // 主机想设置HID设备当前所使用的协议
                        Report_Value = EP0_Databuf[2];
                        break;

                    case DEF_USB_GET_IDLE: /* 0x02: GET_IDLE */ // 主机想读取HID设备特定输入报表的当前的空闲比率
                        EP0_Databuf[0] = Idle_Value;
                        len = 1;
                        break;

                    case DEF_USB_GET_PROTOCOL: /* 0x03: GET_PROTOCOL */ // 主机想获得HID设备当前所使用的协议
                        EP0_Databuf[0] = Report_Value;
                        len = 1;
                        break;

                    default:
                        errflag = 0xFF;
                    }
                }
            }
            else // 判断为标准请求
            {
                switch (SetupReqCode) // 判断命令的序号
                {
                case USB_GET_DESCRIPTOR: // 主机想获得标准描述符
                {
                    switch (((pSetupReqPak->wValue) >> 8)) // 右移8位，看原来的高8位是否为0，为1表示方向为IN方向，则进s-case语句
                    {
                    case USB_DESCR_TYP_DEVICE: // 不同的值代表不同的命令。主机想获得设备描述符
                    {
                        pDescr = MyDevDescr; // 将设备描述符字符串放在pDescr地址中，“获得标准描述符”这个case末尾会用拷贝函数发送
                        len = MyDevDescr[0]; // 协议规定设备描述符的首字节存放字节数长度。拷贝函数会用到len参数
                    }
                    break;

                    case USB_DESCR_TYP_CONFIG: // 主机想获得配置描述符
                    {
                        pDescr = MyCfgDescr; // 将配置描述符字符串放在pDescr地址中，之后会发送
                        len = MyCfgDescr[2]; // 协议规定配置描述符的第三个字节存放配置信息的总长
                    }
                    break;

                    case USB_DESCR_TYP_HID:                    // 主机想获得人机接口类描述符。此处结构体中的wIndex与配置描述符不同，意为接口号。
                        switch ((pSetupReqPak->wIndex) & 0xff) // 取低八位，高八位抹去
                        {
                        /* 选择接口 */
                        case 0:
                            pDescr = (uint8_t *)(&MyCfgDescr[18]); // 接口1的类描述符存放位置，待发送
                            len = 9;
                            break;

                        default:
                            /* 不支持的字符串描述符 */
                            errflag = 0xff;
                            break;
                        }
                        break;

                    case USB_DESCR_TYP_REPORT: // 主机想获得设备报表描述符
                    {
                        if (((pSetupReqPak->wIndex) & 0xff) == 0) // 接口0报表描述符
                        {
                            pDescr = HIDDescr; // 数据准备上传
                            len = sizeof(HIDDescr);
                        }
                        else
                            len = 0xff; // 本程序只有2个接口，这句话正常不可能执行
                    }
                    break;

                    case USB_DESCR_TYP_STRING: // 主机想获得设备字符串描述符
                    {
                        switch ((pSetupReqPak->wValue) & 0xff) // 根据wValue的值传递字符串信息
                        {
                        case 1:
                            pDescr = MyManuInfo;
                            len = MyManuInfo[0];
                            break;
                        case 2:
                            pDescr = MyProdInfo;
                            len = MyProdInfo[0];
                            break;
                        case 0:
                            pDescr = MyLangDescr;
                            len = MyLangDescr[0];
                            break;
                        default:
                            errflag = 0xFF; // 不支持的字符串描述符
                            break;
                        }
                    }
                    break;

                    default:
                        errflag = 0xff;
                        break;
                    }
                    if (SetupReqLen > len)
                        SetupReqLen = len;                                        // 实际需上传总长度
                    len = (SetupReqLen >= DevEP0SIZE) ? DevEP0SIZE : SetupReqLen; // 最大长度为64字节
                    memcpy(pEP0_DataBuf, pDescr, len);                            // 拷贝函数
                    pDescr += len;
                }
                break;

                case USB_SET_ADDRESS:                            // 主机想设置设备地址
                    SetupReqLen = (pSetupReqPak->wValue) & 0xff; // 将主机分发的位设备地址暂存在SetupReqLen中
                    break;                                       // 控制阶段会赋值给设备地址参数

                case USB_GET_CONFIGURATION:      // 主机想获得设备当前配置
                    pEP0_DataBuf[0] = DevConfig; // 将设备配置放进RAM
                    if (SetupReqLen > 1)
                        SetupReqLen = 1; // 将数据阶段的字节数置1。因为DevConfig只有一个字节
                    break;

                case USB_SET_CONFIGURATION:                    // 主机想设置设备当前配置
                    DevConfig = (pSetupReqPak->wValue) & 0xff; // 取低八位，高八位抹去
                    break;

                case USB_CLEAR_FEATURE: // 关闭USB设备的特征/功能。可以是设备或是端点层面上的。
                {
                    if ((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // 判断是不是端点特征（清除端点停止工作的状态）
                    {
                        switch ((pSetupReqPak->wIndex) & 0xff) // 取低八位，高八位抹去。判断索引
                        {                                      // 16位的最高位判断数据传输方向，0为OUT，1为IN。低位为端点号。
                        case 0x81:                             // 清零_TOG和_T_RES这三位，并将后者写成_NAK，响应IN事务NAK表示无数据返回
                            R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                            break;
                        case 0x01: // 清零_TOG和_R_RES这三位，并将后者写成_ACK，响应OUT事务ACK表示接收正常
                            R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                            break;
                        default:
                            errflag = 0xFF; // 不支持的端点
                            break;
                        }
                    }
                    else if ((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) // 判断是不是设备特征（用于设备唤醒）
                    {
                        if (pSetupReqPak->wValue == 1) // 唤醒标志位为1
                        {
                            USB_SleepStatus &= ~0x01; // 最低位清零
                        }
                    }
                    else
                    {
                        errflag = 0xFF;
                    }
                }
                break;

                case USB_SET_FEATURE:                                                            // 开启USB设备的特征/功能。可以是设备或是端点层面上的。
                    if ((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // 判断是不是端点特征（使端点停止工作）
                    {
                        /* 端点 */
                        switch (pSetupReqPak->wIndex) // 判断索引
                        {                             // 16位的最高位判断数据传输方向，0为OUT，1为IN。低位为端点号。
                        case 0x81:                    // 清零_TOG和_T_RES三位，并将后者写成_STALL，根据主机指令停止端点的工作
                            R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                            break;
                        case 0x01: // 清零_TOG和_R_RES三位，并将后者写成_STALL，根据主机指令停止端点的工作
                            R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                            break;
                        default:
                            /* 不支持的端点 */
                            errflag = 0xFF; // 不支持的端点
                            break;
                        }
                    }
                    else if ((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) // 判断是不是设备特征（使设备休眠）
                    {
                        if (pSetupReqPak->wValue == 1)
                        {
                            USB_SleepStatus |= 0x01; // 设置睡眠
                        }
                    }
                    else
                    {
                        errflag = 0xFF;
                    }
                    break;

                case USB_GET_INTERFACE: // 主机想获得接口当前工作的选择设置值
                    pEP0_DataBuf[0] = 0x00;
                    if (SetupReqLen > 1)
                        SetupReqLen = 1; // 将数据阶段的字节数置1。因为待传数据只有一个字节
                    break;

                case USB_SET_INTERFACE: // 主机想激活设备的某个接口
                    break;

                case USB_GET_STATUS:                                                             // 主机想获得设备、接口或是端点的状态
                    if ((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // 判断是否为端点状态
                    {
                        /* 端点 */
                        pEP0_DataBuf[0] = 0x00;
                        switch (pSetupReqPak->wIndex)
                        {          // 16位的最高位判断数据传输方向，0为OUT，1为IN。低位为端点号。
                        case 0x81: // 判断_TOG和_T_RES三位，若处于STALL状态，进if语句
                            if ((R8_UEP1_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                            {
                                pEP0_DataBuf[0] = 0x01; // 返回D0为1，表示端点被停止工作了。该位由SET_FEATURE和CLEAR_FEATURE命令配置。
                            }
                            break;

                        case 0x01: // 判断_TOG和_R_RES三位，若处于STALL状态，进if语句
                            if ((R8_UEP1_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                            {
                                pEP0_DataBuf[0] = 0x01;
                            }
                            break;
                        }
                    }
                    else if ((pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE) // 判断是否为设备状态
                    {
                        pEP0_DataBuf[0] = 0x00;
                        if (USB_SleepStatus) // 如果设备处于睡眠状态
                        {
                            pEP0_DataBuf[0] = 0x02; // 最低位D0为0，表示设备由总线供电，为1表示设备自供电。 D1位为1表示支持远程唤醒，为0表示不支持。
                        }
                        else
                        {
                            pEP0_DataBuf[0] = 0x00;
                        }
                    }
                    pEP0_DataBuf[1] = 0; // 返回状态信息的格式为16位数，高八位保留为0
                    if (SetupReqLen >= 2)
                    {
                        SetupReqLen = 2; // 将数据阶段的字节数置2。因为待传数据只有2个字节
                    }
                    break;

                default:
                    errflag = 0xff;
                    break;
                }
            }
            if (errflag == 0xff) // 错误或不支持
            {
                //                  SetupReqCode = 0xFF;
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
                Ready = 1;
                printf("Ready_Stall = %d\n", Ready);
            }
            else
            {
                if (chtype & 0x80) // 上传。最高位为1，数据传输方向为设备向主机传输。
                {
                    len = (SetupReqLen > DevEP0SIZE) ? DevEP0SIZE : SetupReqLen;
                    SetupReqLen -= len;
                }
                else
                    len = 0; // 下传。最高位为0，数据传输方向为主机向设备传输。
                R8_UEP0_T_LEN = len;
                R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // 默认数据包是DATA1
            }

            R8_USB_INT_FG = RB_UIF_TRANSFER; // 写1清中断标识
        }
    }

    else if (intflag & RB_UIF_BUS_RST) // 判断_INT_FG中的总线复位标志位，为1触发
    {
        R8_USB_DEV_AD = 0;                            // 设备地址写成0，待主机重新分配给设备一个新地址
        R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK; // 把端点0的控制寄存器，写成：接收响应响应ACK表示正常收到，发送响应NAK表示没有数据要返回
        R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_USB_INT_FG = RB_UIF_BUS_RST; // 写1清中断标识
    }
    else if (intflag & RB_UIF_SUSPEND) // 判断_INT_FG中的总线挂起或唤醒事件中断标志位。挂起和唤醒都会触发此中断
    {
        if (R8_USB_MIS_ST & RB_UMS_SUSPEND) // 取得杂项状态寄存器中的挂起状态位，为1表示USB总线处于挂起态，为0表示总线处于非挂起态
        {
            Ready = 0;
            printf("Ready_Sleep = %d\n", Ready);
        }    // 挂起     //当设备处于空闲状态超过3ms，主机会要求设备挂起（类似于电脑休眠）
        else // 挂起或唤醒中断被触发，又没有被判断为挂起
        {
            Ready = 1;
            printf("Ready_WeakUp = %d\n", Ready);
        }                               // 唤醒
        R8_USB_INT_FG = RB_UIF_SUSPEND; // 写1清中断标志
    }
    else
    {
        R8_USB_INT_FG = intflag; //_INT_FG中没有中断标识，再把原值写回原来的寄存器
    }
}

/*********************************************************************
 * @fn      DevHIDReport
 *
 * @brief   上报HID数据
 *
 * @return  0：成功
 *          1：出错
 */
void DevHIDReport(void)
{
    memcpy(pEP1_IN_DataBuf, HID_Buf, sizeof(HID_Buf));
    DevEP1_IN_Deal(sizeof(HID_Buf));
}

/*********************************************************************
 * @fn      DevWakeup
 *
 * @brief   设备模式唤醒主机
 *
 * @return  none
 */
void DevWakeup(void)
{
    R16_PIN_ANALOG_IE &= ~(RB_PIN_USB_DP_PU);
    R8_UDEV_CTRL |= RB_UD_LOW_SPEED;
    mDelaymS(2);
    R8_UDEV_CTRL &= ~RB_UD_LOW_SPEED;
    R16_PIN_ANALOG_IE |= RB_PIN_USB_DP_PU;
}

/*********************************************************************
 * @fn      DebugInit
 *
 * @brief   调试初始化
 *
 * @return  none
 */
void DebugInit(void)
{
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
}

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   USB2 传输处理函数
 *
 * @return  none
 */
void USB2_DevTransProcess(void)
{
    uint8_t len, chtype;
    uint8_t intflag, errflag = 0;

    intflag = R8_USB2_INT_FG;
    if (intflag & RB_UIF_TRANSFER)
    {
        if ((R8_USB2_INT_ST & MASK_UIS_TOKEN) != MASK_UIS_TOKEN) // 非空闲
        {
            switch (R8_USB2_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
            // 分析操作令牌和端点号
            {
            case UIS_TOKEN_IN:
            {
                switch (U2SetupReqCode)
                {
                case USB_GET_DESCRIPTOR:
                    len = U2SetupReqLen >= U2DevEP0SIZE ? U2DevEP0SIZE : U2SetupReqLen; // 本次传输长度
                    memcpy(pU2EP0_DataBuf, pU2Descr, len);                              /* 加载上传数据 */
                    U2SetupReqLen -= len;
                    pU2Descr += len;
                    R8_U2EP0_T_LEN = len;
                    R8_U2EP0_CTRL ^= RB_UEP_T_TOG; // 翻转
                    break;
                case USB_SET_ADDRESS:
                    R8_USB2_DEV_AD = (R8_USB2_DEV_AD & RB_UDA_GP_BIT) | U2SetupReqLen;
                    R8_U2EP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                    break;

                case USB_SET_FEATURE:
                    break;

                default:
                    R8_U2EP0_T_LEN = 0; // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                    R8_U2EP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                    break;
                }
            }
            break;

            case UIS_TOKEN_OUT:
            {
                len = R8_USB2_RX_LEN;
                if (U2SetupReqCode == 0x09)
                {
                    HIDKeyLightsCode = pU2EP0_DataBuf[0];
                    printf("\r\n[%d] ", pU2EP0_DataBuf[0]);
                    printf("[%s] Num Lock\t",
                           (pU2EP0_DataBuf[0] & (1 << 0)) ? "*" : " ");
                    printf("[%s] Caps Lock\t",
                           (pU2EP0_DataBuf[0] & (1 << 1)) ? "*" : " ");
                    printf("[%s] Scroll Lock\r\n",
                           (pU2EP0_DataBuf[0] & (1 << 2)) ? "*" : " ");
                }
            }
            break;

            case UIS_TOKEN_OUT | 1:
            {
                if (R8_USB2_INT_ST & RB_UIS_TOG_OK)
                { // 不同步的数据包将丢弃
                    R8_U2EP1_CTRL ^= RB_UEP_R_TOG;
                    len = R8_USB2_RX_LEN;
                    U2DevEP1_OUT_Deal(len);
                }
            }
            break;

            case UIS_TOKEN_IN | 1:
                R8_U2EP1_CTRL ^= RB_UEP_T_TOG;
                R8_U2EP1_CTRL = (R8_U2EP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                break;

            case UIS_TOKEN_OUT | 2:
            {
                if (R8_USB2_INT_ST & RB_UIS_TOG_OK)
                { // 不同步的数据包将丢弃
                    R8_U2EP2_CTRL ^= RB_UEP_R_TOG;
                    len = R8_USB2_RX_LEN;
                    U2DevEP2_OUT_Deal(len);
                }
            }
            break;

            case UIS_TOKEN_IN | 2:
                R8_U2EP2_CTRL ^= RB_UEP_T_TOG;
                R8_U2EP2_CTRL = (R8_U2EP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                break;

            case UIS_TOKEN_OUT | 3:
            {
                if (R8_USB2_INT_ST & RB_UIS_TOG_OK)
                { // 不同步的数据包将丢弃
                    R8_U2EP3_CTRL ^= RB_UEP_R_TOG;
                    len = R8_USB2_RX_LEN;
                    U2DevEP3_OUT_Deal(len);
                }
            }
            break;

            case UIS_TOKEN_IN | 3:
                R8_U2EP3_CTRL ^= RB_UEP_T_TOG;
                R8_U2EP3_CTRL = (R8_U2EP3_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                break;

            case UIS_TOKEN_OUT | 4:
            {
                if (R8_USB2_INT_ST & RB_UIS_TOG_OK)
                {
                    R8_U2EP4_CTRL ^= RB_UEP_R_TOG;
                    len = R8_USB2_RX_LEN;
                    U2DevEP4_OUT_Deal(len);
                }
            }
            break;

            case UIS_TOKEN_IN | 4:
                R8_U2EP4_CTRL ^= RB_UEP_T_TOG;
                R8_U2EP4_CTRL = (R8_U2EP4_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
                break;

            default:
                break;
            }
            R8_USB2_INT_FG = RB_UIF_TRANSFER;
        }
        if (R8_USB2_INT_ST & RB_UIS_SETUP_ACT) // Setup包处理
        {
            R8_U2EP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
            U2SetupReqLen = pU2SetupReqPak->wLength;
            U2SetupReqCode = pU2SetupReqPak->bRequest;
            chtype = pU2SetupReqPak->bRequestType;

            len = 0;
            errflag = 0;
            if ((pU2SetupReqPak->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
            {
                /* 非标准请求 */
                /* 其它请求,如类请求，产商请求等 */
                if (pU2SetupReqPak->bRequestType & 0x40)
                {
                    /* 厂商请求 */
                }
                else if (pU2SetupReqPak->bRequestType & 0x20)
                {
                    switch (U2SetupReqCode)
                    {
                    case DEF_USB_SET_IDLE: /* 0x0A: SET_IDLE */
                        U2Idle_Value = EP0_Databuf[3];
                        break; // 这个一定要有

                    case DEF_USB_SET_REPORT: /* 0x09: SET_REPORT */
                        break;

                    case DEF_USB_SET_PROTOCOL: /* 0x0B: SET_PROTOCOL */
                        U2Report_Value = U2EP0_Databuf[2];
                        break;

                    case DEF_USB_GET_IDLE: /* 0x02: GET_IDLE */
                        U2EP0_Databuf[0] = U2Idle_Value;
                        len = 1;
                        break;

                    case DEF_USB_GET_PROTOCOL: /* 0x03: GET_PROTOCOL */
                        U2EP0_Databuf[0] = U2Report_Value;
                        len = 1;
                        break;

                    default:
                        errflag = 0xFF;
                    }
                }
            }
            else /* 标准请求 */
            {
                switch (U2SetupReqCode)
                {
                case USB_GET_DESCRIPTOR:
                {
                    switch (((pU2SetupReqPak->wValue) >> 8))
                    {
                    case USB_DESCR_TYP_DEVICE:
                    {
                        pU2Descr = U2MyDevDescr;
                        len = U2MyDevDescr[0];
                    }
                    break;

                    case USB_DESCR_TYP_CONFIG:
                    {
                        pU2Descr = U2MyCfgDescr;
                        len = U2MyCfgDescr[2];
                    }
                    break;

                    case USB_DESCR_TYP_HID:
                        switch ((pU2SetupReqPak->wIndex) & 0xff)
                        {
                        /* 选择接口 */
                        case 0:
                            pU2Descr = (uint8_t *)(&U2MyCfgDescr[18]);
                            len = 9;
                            break;

                        case 1:
                            pU2Descr = (uint8_t *)(&U2MyCfgDescr[43]);
                            len = 9;
                            break;

                        default:
                            /* 不支持的字符串描述符 */
                            errflag = 0xff;
                            break;
                        }
                        break;

                    case USB_DESCR_TYP_REPORT:
                    {
                        if (((pU2SetupReqPak->wIndex) & 0xff) == 0) // 接口0报表描述符
                        {
                            pU2Descr = U2KeyRepDesc; // 数据准备上传
                            len = sizeof(U2KeyRepDesc);
                        }
                        else if (((pU2SetupReqPak->wIndex) & 0xff) == 1) // 接口1报表描述符
                        {
                            pU2Descr = U2MouseRepDesc; // 数据准备上传
                            len = sizeof(U2MouseRepDesc);
                            U2Ready = 1; // 如果有更多接口，该标准位应该在最后一个接口配置完成后有效
                        }
                        else
                            len = 0xff; // 本程序只有2个接口，这句话正常不可能执行
                    }
                    break;

                    case USB_DESCR_TYP_STRING:
                    {
                        switch ((pU2SetupReqPak->wValue) & 0xff)
                        {
                        case 1:
                            pU2Descr = MyManuInfo;
                            len = MyManuInfo[0];
                            break;
                        case 2:
                            pU2Descr = U2MyProdInfo;
                            len = U2MyProdInfo[0];
                            break;
                        case 0:
                            pU2Descr = MyLangDescr;
                            len = MyLangDescr[0];
                            break;
                        default:
                            errflag = 0xFF; // 不支持的字符串描述符
                            break;
                        }
                    }
                    break;

                    case 0x06:
                        pU2Descr = (uint8_t *)(&U2My_QueDescr[0]);
                        len = sizeof(U2My_QueDescr);
                        break;

                    case 0x07:
                        memcpy(&U2USB_FS_OSC_DESC[2], &U2MyCfgDescr[2],
                               sizeof(U2MyCfgDescr) - 2);
                        pU2Descr = (uint8_t *)(&U2USB_FS_OSC_DESC[0]);
                        len = sizeof(U2USB_FS_OSC_DESC);
                        break;

                    default:
                        errflag = 0xff;
                        break;
                    }
                    if (U2SetupReqLen > len)
                        U2SetupReqLen = len; // 实际需上传总长度
                    len = (U2SetupReqLen >= U2DevEP0SIZE) ? U2DevEP0SIZE : U2SetupReqLen;
                    memcpy(pU2EP0_DataBuf, pU2Descr, len);
                    pU2Descr += len;
                }
                break;

                case USB_SET_ADDRESS:
                    U2SetupReqLen = (pU2SetupReqPak->wValue) & 0xff;
                    break;

                case USB_GET_CONFIGURATION:
                    pU2EP0_DataBuf[0] = U2DevConfig;
                    if (U2SetupReqLen > 1)
                        U2SetupReqLen = 1;
                    break;

                case USB_SET_CONFIGURATION:
                    U2DevConfig = (pU2SetupReqPak->wValue) & 0xff;
                    break;

                case USB_CLEAR_FEATURE:
                {
                    if ((pU2SetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) // 端点
                    {
                        switch ((pU2SetupReqPak->wIndex) & 0xff)
                        {
                        case 0x83:
                            R8_U2EP3_CTRL = (R8_U2EP3_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                            break;
                        case 0x03:
                            R8_U2EP3_CTRL = (R8_U2EP3_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                            break;
                        case 0x82:
                            R8_U2EP2_CTRL = (R8_U2EP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                            break;
                        case 0x02:
                            R8_U2EP2_CTRL = (R8_U2EP2_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                            break;
                        case 0x81:
                            R8_U2EP1_CTRL = (R8_U2EP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                            break;
                        case 0x01:
                            R8_U2EP1_CTRL = (R8_U2EP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                            break;
                        default:
                            errflag = 0xFF; // 不支持的端点
                            break;
                        }
                    }
                    else if ((pU2SetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                    {
                        if (pU2SetupReqPak->wValue == 1)
                        {
                            U2USB_SleepStatus &= ~0x01;
                        }
                    }
                    else
                    {
                        errflag = 0xFF;
                    }
                }
                break;

                case USB_SET_FEATURE:
                    if ((pU2SetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                    {
                        /* 端点 */
                        switch (pU2SetupReqPak->wIndex)
                        {
                        case 0x83:
                            R8_U2EP3_CTRL = (R8_U2EP3_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                            break;
                        case 0x03:
                            R8_U2EP3_CTRL = (R8_U2EP3_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                            break;
                        case 0x82:
                            R8_U2EP2_CTRL = (R8_U2EP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                            break;
                        case 0x02:
                            R8_U2EP2_CTRL = (R8_U2EP2_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                            break;
                        case 0x81:
                            R8_U2EP1_CTRL = (R8_U2EP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_STALL;
                            break;
                        case 0x01:
                            R8_U2EP1_CTRL = (R8_U2EP1_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_STALL;
                            break;
                        default:
                            /* 不支持的端点 */
                            errflag = 0xFF; // 不支持的端点
                            break;
                        }
                    }
                    else if ((pU2SetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                    {
                        if (pU2SetupReqPak->wValue == 1)
                        {
                            /* 设置睡眠 */
                            U2USB_SleepStatus |= 0x01;
                        }
                    }
                    else
                    {
                        errflag = 0xFF;
                    }
                    break;

                case USB_GET_INTERFACE:
                    pU2EP0_DataBuf[0] = 0x00;
                    if (U2SetupReqLen > 1)
                        U2SetupReqLen = 1;
                    break;

                case USB_SET_INTERFACE:
                    break;

                case USB_GET_STATUS:
                    if ((pU2SetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                    {
                        /* 端点 */
                        pU2EP0_DataBuf[0] = 0x00;
                        switch (pU2SetupReqPak->wIndex)
                        {
                        case 0x83:
                            if ((R8_U2EP3_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                            {
                                pU2EP0_DataBuf[0] = 0x01;
                            }
                            break;

                        case 0x03:
                            if ((R8_U2EP3_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                            {
                                pU2EP0_DataBuf[0] = 0x01;
                            }
                            break;

                        case 0x82:
                            if ((R8_U2EP2_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                            {
                                pU2EP0_DataBuf[0] = 0x01;
                            }
                            break;

                        case 0x02:
                            if ((R8_U2EP2_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                            {
                                pU2EP0_DataBuf[0] = 0x01;
                            }
                            break;

                        case 0x81:
                            if ((R8_U2EP1_CTRL & (RB_UEP_T_TOG | MASK_UEP_T_RES)) == UEP_T_RES_STALL)
                            {
                                pU2EP0_DataBuf[0] = 0x01;
                            }
                            break;

                        case 0x01:
                            if ((R8_U2EP1_CTRL & (RB_UEP_R_TOG | MASK_UEP_R_RES)) == UEP_R_RES_STALL)
                            {
                                pU2EP0_DataBuf[0] = 0x01;
                            }
                            break;
                        }
                    }
                    else if ((pU2SetupReqPak->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
                    {
                        pU2EP0_DataBuf[0] = 0x00;
                        if (U2USB_SleepStatus)
                        {
                            pU2EP0_DataBuf[0] = 0x02;
                        }
                        else
                        {
                            pU2EP0_DataBuf[0] = 0x00;
                        }
                    }
                    pU2EP0_DataBuf[1] = 0;
                    if (U2SetupReqLen >= 2)
                    {
                        U2SetupReqLen = 2;
                    }
                    break;

                default:
                    errflag = 0xff;
                    break;
                }
            }
            if (errflag == 0xff) // 错误或不支持
            {
                //                  U2SetupReqCode = 0xFF;
                R8_U2EP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
            }
            else
            {
                if (chtype & 0x80) // 上传
                {
                    len = (U2SetupReqLen > U2DevEP0SIZE) ? U2DevEP0SIZE : U2SetupReqLen;
                    U2SetupReqLen -= len;
                }
                else
                    len = 0; // 下传
                R8_U2EP0_T_LEN = len;
                R8_U2EP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // 默认数据包是DATA1
            }

            R8_USB2_INT_FG = RB_UIF_TRANSFER;
        }
    }
    else if (intflag & RB_UIF_BUS_RST)
    {
        R8_USB2_DEV_AD = 0;
        R8_U2EP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_U2EP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_U2EP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_U2EP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        R8_USB2_INT_FG = RB_UIF_BUS_RST;
    }
    else if (intflag & RB_UIF_SUSPEND)
    {
        if (R8_USB2_MIS_ST & RB_UMS_SUSPEND)
        {
            ;
        } // 挂起
        else
        {
            ;
        } // 唤醒
        R8_USB2_INT_FG = RB_UIF_SUSPEND;
    }
    else
    {
        R8_USB2_INT_FG = intflag;
    }
}

/*********************************************************************
 * @fn      DevHIDMouseReport
 *
 * @brief   上报鼠标数据
 *
 * @return  none
 */
void U2DevHIDMouseReport(void)
{
    memcpy(pU2EP2_IN_DataBuf, U2HIDMouse, sizeof(U2HIDMouse));
    U2DevEP2_IN_Deal(sizeof(U2HIDMouse));
}

/*********************************************************************
 * @fn      U2DevHIDKeyReport
 *
 * @brief   上报键盘数据
 *
 * @return  none
 */
void U2DevHIDKeyReport(void)
{
    memcpy(pU2EP1_IN_DataBuf, U2HIDKey, sizeof(U2HIDKey));
    U2DevEP1_IN_Deal(sizeof(U2HIDKey));
}

/*********************************************************************
 * @fn      U2DevWakeup
 *
 * @brief   设备模式唤醒主机
 *
 * @return  none
 */
void U2DevWakeup(void)
{
    R16_PIN_ANALOG_IE &= ~(RB_PIN_USB2_DP_PU);
    R8_U2DEV_CTRL |= RB_UD_LOW_SPEED;
    mDelaymS(2);
    R8_U2DEV_CTRL &= ~RB_UD_LOW_SPEED;
    R16_PIN_ANALOG_IE |= RB_PIN_USB2_DP_PU;
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函数
 *
 * @return  none
 */
int main()
{
    uint8_t s;
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    DebugInit(); // 配置串口1用来prinft来debug
    printf("start\n");

    /* 配置USB Switch GPIO */

    USBSwitchSupportStatus = GPIOB_ReadPortPin(GPIO_Pin_4) ? 1 : 0;

    GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_20mA); // NMOS
    GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);  // IN
    GPIOA_ModeCfg(GPIO_Pin_12, GPIO_ModeOut_PP_5mA); // EN#
    //切换至上位机端，电源接通
    GPIOB_SetBits(GPIO_Pin_4); // pull down
    GPIOB_ResetBits(GPIO_Pin_7);
    GPIOA_ResetBits(GPIO_Pin_12);

    /* 配置WS2812B GPIO */
    GPIOA_ModeCfg(GPIO_Pin_13, GPIO_ModeOut_PP_5mA);
    printf("RGB ON\n");
    // GRB WS2812B
    buf[0] = 0x00;
    buf[1] = 0x05;
    buf[2] = 0x00;
    SendOnePix(buf);

    mDelaymS(100);

    pEP0_RAM_Addr = EP0_Databuf; // 配置缓存区64字节。
    pEP1_RAM_Addr = EP1_Databuf;
    pU2EP0_RAM_Addr = U2EP0_Databuf;
    pU2EP1_RAM_Addr = U2EP1_Databuf;
    pU2EP2_RAM_Addr = U2EP2_Databuf;
    pU2EP3_RAM_Addr = U2EP3_Databuf;

    USB_DeviceInit();
    USB2_DeviceInit();

    PFIC_EnableIRQ(USB_IRQn); // 启用中断向量
    PFIC_EnableIRQ(USB2_IRQn);

    uint8_t i;
    while (1)
    {
        mDelayuS(1000);
    }
}

/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   端点1数据处理，收到数据后取反再发出去。用户自行更改。
 *
 * @return  none
 */
void DevEP1_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */

    uint8_t i;

    memcpy(HIDOutData, pEP1_OUT_DataBuf, 10);

    if (HIDOutData[0] == 1)
    {

        memcpy(pU2EP1_IN_DataBuf, HIDOutData + 2, 8);
        U2DevEP1_IN_Deal(8);

    }

    else if (HIDOutData[0] == 2)
    {
        memcpy(pU2EP2_IN_DataBuf, HIDOutData + 2, 6);
        U2DevEP2_IN_Deal(6);
    }

    else if (HIDOutData[0] == 3)
    {
        printf("HIDKeyLightsCode: ");
        HID_Buf[0] = 3;
        HID_Buf[2] = HIDKeyLightsCode;
        DevHIDReport();
    }
    else if (HIDOutData[0] == 4)
    {
        printf("SYS_ResetExecute: ");
        SYS_ResetExecute();
    }

    else if (HIDOutData[0] == 5)
    {
        printf("SET WS2812B: ");
        // GRB
        buf[0] = HIDOutData[2];
        buf[1] = HIDOutData[3];
        buf[2] = HIDOutData[4];
        SendOnePix(buf);
    }
    else if (HIDOutData[0] == 6)
    {
        printf("USB SWITCH: ");
        if (HIDOutData[2] == 0) // 信号线悬空，电源断电
        {
            GPIOB_ResetBits(GPIO_Pin_4); // pull up
            GPIOB_SetBits(GPIO_Pin_7);
            GPIOA_SetBits(GPIO_Pin_12);
            printf("0");
        }
        else if (HIDOutData[2] == 1) // 切换至上位机端，电源接通
        {
            GPIOB_SetBits(GPIO_Pin_4); // pull down
            GPIOB_ResetBits(GPIO_Pin_7);
            GPIOA_ResetBits(GPIO_Pin_12);
            printf("1");
        }
        else if (HIDOutData[2] == 2) // 切换至被控端，电源接通
        {
            GPIOB_SetBits(GPIO_Pin_4); // pull down
            GPIOB_SetBits(GPIO_Pin_7);
            GPIOA_ResetBits(GPIO_Pin_12);
            printf("2");
        }
        else if (HIDOutData[2] == 3) // 状态查询
        {
            HID_Buf[0] = 6;
            HID_Buf[2] = 3;
            HID_Buf[3] = GPIOB_ReadPortPin(GPIO_Pin_4) ? 1 : 0;  // nmos
            HID_Buf[4] = GPIOB_ReadPortPin(GPIO_Pin_7) ? 1 : 0;  // IN
            HID_Buf[5] = GPIOA_ReadPortPin(GPIO_Pin_12) ? 1 : 0; // EN#
            HID_Buf[6] = USBSwitchSupportStatus;
            DevHIDReport();
            printf("3");
        }

        printf("\r\n");
    }
}

/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB中断函数
 *
 * @return  none
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode"))) void
USB_IRQHandler(void) /* USB中断服务程序,使用寄存器组1 */
{
    USB_DevTransProcess();
}

/*********************************************************************
 * @fn      U2DevEP1_OUT_Deal
 *
 * @brief   端点1数据处理
 *
 * @return  none
 */
void U2DevEP1_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
    uint8_t i;

    for (i = 0; i < l; i++)
    {
        pU2EP1_IN_DataBuf[i] = ~pU2EP1_OUT_DataBuf[i];
    }
    U2DevEP1_IN_Deal(l);
}

/*********************************************************************
 * @fn      U2DevEP2_OUT_Deal
 *
 * @brief   端点2数据处理
 *
 * @return  none
 */
void U2DevEP2_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
    uint8_t i;

    for (i = 0; i < l; i++)
    {
        pU2EP2_IN_DataBuf[i] = ~pU2EP2_OUT_DataBuf[i];
    }
    U2DevEP2_IN_Deal(l);
}

/*********************************************************************
 * @fn      U2DevEP3_OUT_Deal
 *
 * @brief   端点3数据处理
 *
 * @return  none
 */
void U2DevEP3_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
    uint8_t i;

    for (i = 0; i < l; i++)
    {
        pU2EP3_IN_DataBuf[i] = ~pU2EP3_OUT_DataBuf[i];
    }
    U2DevEP3_IN_Deal(l);
}

/*********************************************************************
 * @fn      U2DevEP4_OUT_Deal
 *
 * @brief   端点4数据处理
 *
 * @return  none
 */
void U2DevEP4_OUT_Deal(uint8_t l)
{ /* 用户可自定义 */
    uint8_t i;

    for (i = 0; i < l; i++)
    {
        pU2EP4_IN_DataBuf[i] = ~pU2EP4_OUT_DataBuf[i];
    }
    U2DevEP4_IN_Deal(l);
}

/*********************************************************************
 * @fn      USB2_IRQHandler
 *
 * @brief   USB2中断函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void USB2_IRQHandler(void) /* USB中断服务程序,使用寄存器组1 */
{
    USB2_DevTransProcess();
}
