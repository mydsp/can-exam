# can-exam —— 软3：CAN 通信（双板主从）

对应《G308 电控组 2026 夏季考核题》软件第三题。**同一份 `can.c/can.h`（模块化通用 CAN 接口），两个角色由构建预设切换。**

- 主机：检测按键按下 → CAN 发送数据 `0x11` → 收到的数据全部打印到串口
- 从机：收到 `0x11` → 回数据 `0x22` → 翻转 LED

## 交付物

| 题面提交要求 | 位置 |
| --- | --- |
| （1）项目文件 | 本仓库源码（`master` / `slave` 两个预设） |
| （2）开发文档 | [`docs/03-开发文档.md`](docs/03-开发文档.md) |
| （3）实操视频 | [`docs/video/soft3-demo.mp4`](docs/video/soft3-demo.mp4) |
| 补充证据：两板串口日志截图 | [`docs/img/soft3-master-log.jpg`](docs/img/soft3-master-log.jpg)、[`docs/img/soft3-slave-log.jpg`](docs/img/soft3-slave-log.jpg) |
| 补充证据：硬件与接线实拍 | [`docs/img/soft3-hardware.jpg`](docs/img/soft3-hardware.jpg) |

```
E:\can-exam\
├── Core\                     C8T6 (STM32F103C8T6) 双板工程
│   ├── Inc\can.h             通用 CAN 接口（题目评分点：模块化）
│   ├── Src\can.c             HAL bxCAN 驱动：init/send/recv/flush
│   ├── Src\main.c            master / slave 两种角色逻辑
│   ├── Src\bsp.c             串口打印 / LED / 按键
│   └── Src\stm32f1xx_hal_msp.c   引脚与时钟底层
├── esp32-selftest\           ESP32-S3 单板自测（联调前先验证收发器链路）
├── CMakePresets.json         master / slave 两个构建预设
└── STM32F103C8Tx_FLASH.ld
```

## 关键参数

| 项 | 值 |
|---|---|
| MCU | STM32F103C8T6（学校蓝 pill / 最小系统板，片上 bxCAN 控制器） |
| CAN 引脚 | PA11 = CAN_RX，PA12 = CAN_TX（F103 默认映射，无需 remap） |
| 波特率 | **500 kbit/s**（APB1 36MHz ÷ 9 = 4MHz tq，8 tq/bit，采样点 75%） |
| 串口 | USART1 PA9/PA10，115200 |
| LED / 按键 | 从机 PC13（板载灯，低点亮）/ 主机 PB0（内部上拉，按键接 PB0↔GND） |
| 收发器 | TJA1050 模块 ×2（**5V 供电**，见下方电平注意） |

> 为什么用 C8T6：F401 片上没有 CAN 控制器（本机 `stm32f401xe.h` 实测 0 处 CAN 定义）。
> CAN 节点 = 控制器（芯片内）+ 收发器（TJA1050 只干这个）+ 总线（CANH/CANL 双绞线）。

## 接线

### TJA1050 模块 ↔ C8T6（两块板各一套，接法相同）

```
C8T6                  TJA1050 模块
────                  ────────────
3.3V                  (不用)
5V    ──────────────  VCC          ← 模块必须 5V
GND   ──────────────  GND          ← 必须共地
PA12 ────────直连────  TXD          ← 3.3V 输出 meets TJA1050 输入阈值(~2V)，直连
PA11 <──1k┬┬2k──<────  RXD          ← 5V 不能直灌 3.3V 引脚，必须分压
          │
        GND(2k 下端)
CANH  ←──模块A CANH────────模块B CANH
CANL  ←──模块A CANL────────模块B CANL
```

- **终端电阻**：模块自带 120Ω 跳线。双板联调时总线两端各保留一个；ESP32 单板经 TJA1050 自测时，CANH 与 CANL 之间也必须有约 120Ω 负载。CANH/CANL **绝不可用导线直接短接**，否则会让收发器的差分输出互相对冲，存在过流和损坏风险。
- 板间连线：CANH↔CANH、CANL↔CANL，不要交叉。

### ESP32-S3 自测接线（GPIO4=TX，GPIO5=RX）

```
ESP32-S3              TJA1050 模块
GPIO4  ──直连───────  TXD
GPIO5  <──1k┬┬2k──<──  RXD
GND    ──────────────  GND
5V     ──────────────  VCC
CANH ──┬（CANH 与 CANL 之间接 120Ω；绝不可直接短接）
CANL ──┘
```

### ESP32-S3 双节点预验收（两块 ESP32 + 两个 TJA1050，推荐）

两块 ESP32 各接一个 TJA1050，GPIO4/5 和 RXD 分压接法与上图相同；两块模块之间连接 CANH↔CANH、CANL↔CANL、GND↔GND。总线两端各保留一个 120Ω 终端电阻，整条总线断电测量 CANH-CANL 应约为 60Ω。**CANH 与 CANL 绝不可直接短接。**

同一份工程按角色生成两个固件：

```powershell
cd E:\can-exam\esp32-selftest
idf.py -B build-master -D CAN_DUAL_NODE=1 -D CAN_ROLE=master build
idf.py -B build-slave  -D CAN_DUAL_NODE=1 -D CAN_ROLE=slave  build
```

主机周期发送标准帧 `ID=0x11 DATA=0x11`，从机收到后回复 `ID=0x22 DATA=0x22`。分别烧录：

```powershell
idf.py -B build-master -p 主机COM口 flash monitor
idf.py -B build-slave  -p 从机COM口 flash monitor
```

主机串口应出现 `DUAL CAN PASS: 5/5 ACK frames OK`，从机串口应出现 5 次收到 `0x11` 并发送 `0x22`。

## 电平铁律（踩坑高发区）

1. TJA1050 是 **5V 芯片**，模块 VCC 接 5V；C8T6/ESP32-S3 是 3.3V 芯片。
2. 方向 MCU→收发器（TXD）：3.3V 直连没问题。
3. 方向 收发器→MCU（RXD）：**必须 1k/2k 分压**，直连是在给芯片灌 5V。
4. 两板、收发器、串口适配器必须**共地**。

## 构建与烧录

### C8T6（本机 VSCode + CMake + Ninja + arm-gcc 已验证编译）

```powershell
# 主机
cmake --preset master
cmake --build --preset master
# 从机
cmake --preset slave
cmake --build --preset slave
# 产物：build\master\can-master.bin / build\slave\can-slave.bin
```

烧录（Horco DAPLink 用 pyOCD；F103 目标可能要先装 pack）：

```powershell
pyocd pack update; pyocd pack install stm32f103c8
pyocd flash -t stm32f103c8 build\master\can-master.bin
pyocd flash -t stm32f103c8 build\slave\can-slave.bin
```

### ESP32-S3 自测（本机 IDF v6.0.2 已验证编译）

```powershell
# 关键：本机 IDF_TOOLS_PATH 在 E:\Espressif
set IDF_TOOLS_PATH=E:\Espressif
call E:\Espressif\v6.0.2\esp-idf\export.bat
cd E:\can-exam\esp32-selftest
idf.py build
idf.py -p COM口 flash monitor
```

看到 `SELF-TEST PASS: 5/5 frames round-trip OK` 即收发器链路完好。

## 验证流程（先自测后联机，逐步缩小问题域）

1. **ESP32-S3 双节点预验收**（现在就能做）：验证两个 TJA1050、两块 ESP32 和 CANH/CANL 总线；若只有一个模块，再使用单板自测；
2. **C8T6 单板回环**：把 `Core\Src\main.c` 里 `CAN_Init(CAN_WORK_NORMAL)` 临时改成
   `CAN_Init(CAN_WORK_LOOPBACK)` 重编译烧录——主机模式下按一次键，
   串口应打印出发出去的那帧（`RX ID=0x11 DATA=0x11`）。验证 C8T6 控制器代码 OK；
   测完**改回 NORMAL**。
3. **双板联调**：两块 C8T6 + 两个 TJA1050 按上图接好，串口分别看日志：
   - 主机：按键后打印 `TX ID=0x11`，随后打印 `RX ID=0x22 DATA=0x22`
   - 从机：打印 `RX ID=0x11` 且 PC13 灯翻转

## 软3 验收点对照

| 题目要求 | 实现位置 |
|---|---|
| 模块化思路，通用 can.c/can.h | `Core\Inc\can.h` + `Core\Src\can.c`（两板共用） |
| 两块板分主从，按键发 0x11 / 从机回 0x22 | `main.c` RunMaster / RunSlave，构建预设区分 |
| 主机收到数据打印到串口助手 | `RunMaster` 里每次 CAN_Recv 后 bsp_printf |
| 从机每收一帧翻转 LED | `RunSlave` 里 BSP_LED_Toggle（PC13） |

提交材料记得留：项目文件（本仓库）+ 开发文档 + 实操视频（按键→灯翻转→串口日志）。

## 常见故障速查

| 现象 | 先查 |
|---|---|
| 主机 TX 打印但 RX 永远无 | 两端波特率 / 共地 / RXD 分压 / 120Ω 是否恰当 |
| 一直打 error passive / bus-off | 只有一个节点且没开自测模式（真总线至少两个节点） |
| 发送成功但对方收不到 | CANH/CANL 是否接反 / 模块 5V 有没有 / TXD 是否接对脚 |
| 板子直接不动 | RXD 5V 直灌 3.3V 引脚已损伤 PA11（换脚或换板） |
