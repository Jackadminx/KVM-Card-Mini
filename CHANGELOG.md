## HV2.6 

USB设备主从切换功能

U盘等设备可以实现控制端和被控端之间无缝切换，实现跨设备文件传输

 

### 硬件分析

![hKpa4u4PxegGPVCr2JAKcyb8GBJ3i4sHnqIIH5An](.\Document\Images\hKpa4u4PxegGPVCr2JAKcyb8GBJ3i4sHnqIIH5An.jpg)

![img](.\Document\Images\OqkMmGFWqzc19HXRmTqjMmWJ7LFyIBar70Tsb4bh.jpg)

简单的系统结构框图

 

### 软件部分

![bo5bQ83DBd2kRrmJnJrOaYuYEQluwcdC7EkIYT1T](.\Document\Images\bo5bQ83DBd2kRrmJnJrOaYuYEQluwcdC7EkIYT1T.jpg)

\- Master    USB切换接口连接至主控设备

\- Float     USB切换接口断开连接（悬空）

\- Controlled USB切换接口连接至被控设备

 

![8m1CQe2m8CiBCmADa5pUIVrscxnp87DDQDJAnUvU](.\Document\Images\8m1CQe2m8CiBCmADa5pUIVrscxnp87DDQDJAnUvU.jpg)

Web版客户端

