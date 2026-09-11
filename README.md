# can-exam —— CAN 双板主从通信（STM32F103C8T6）

基于 bxCAN 的双板主从通信工程：同一份模块化 `can.c` / `can.h` 通用接口，
主机与从机两种角色由 CMake 预设切换。主机按键按下后发送 `0x11` 并将收到的数据打印至串口；
从机收到 `0x11` 后回传 `0x22` 并翻转板载 LED。500 kbit/s。
对应《G308 电控组 2026 夏季考核题》软件第三题。

## 功能特性

- `can.c` / `can.h` 通用接口：`CAN_Init` / `CAN_Send` / `CAN_Recv` / `CAN_FlushRx`，与应用层完全解耦
- 主从角色经 `CAN_ROLE_MASTER` / `CAN_ROLE_SLAVE` 宏注入，驱动层与角色无关
- 500 kbit/s：APB1 36MHz ÷ 9 = 4MHz tq，8 tq/位，采样点 75%
- 全接收过滤器（Mask 全 0，FIFO0）；`AutoBusOff` / `AutoRetransmission` 使能

## 硬件连接

| C8T6 引脚 | 连接 | 说明 |
| --- | --- | --- |
| PA12（CAN_TX） | TJA1050 TXD | 3.3V 直连 |
| PA11（CAN_RX） | TJA1050 RXD | 1kΩ 串联 + 2kΩ 下拉分压（5V→3.3V） |
| 5V / GND | TJA1050 VCC / GND | 模块 5V 供电，全系统共地 |
| PC13 | 板载 LED | 从机翻转指示 |
| PB0 | 按键 → GND | 主机发送触发 |
| CANH / CANL | 总线 | 两端各 120Ω 终端电阻 |

## 构建与烧录

```bat
cmake --preset master
cmake --build --preset master
pyocd flash -t stm32f103c8 build\master\can-master.bin
```
从机将预设替换为 `slave`。

## 验收现象

- 上电后两板串口分别打印 `role=master` / `role=slave`
- 按一次主机按键：主机打印 `TX ID=0x11 DATA=0x11` 与 `RX ID=0x022 DLC=1 DATA=0x22`；
  从机打印 `RX ID=0x011 DLC=1 -> toggle LED, reply 0x22`，PC13 翻转一次

## 文档与交付物

| 材料 | 文件 |
| --- | --- |
| 开发文档 | [docs/03-开发文档.md](docs/03-开发文档.md) |
| 实操视频 | [docs/video/soft3-demo.mp4](docs/video/soft3-demo.mp4) |
| 串口日志截图 | [docs/img/soft3-master-log.jpg](docs/img/soft3-master-log.jpg) / [soft3-slave-log.jpg](docs/img/soft3-slave-log.jpg) |
| 接线实拍 | [docs/img/soft3-hardware.jpg](docs/img/soft3-hardware.jpg) |

## 已知限制

- 单帧请求-应答协议，无多帧分包与重传（题面未要求）
- 波特率固定 500 kbit/s；从机按 ID 判定，未校验载荷内容
