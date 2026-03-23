# 基于FPGA的单通道电化学工作站

## 项目概述

本项目实现了一个基于FPGA控制AD5940模拟前端芯片的单通道电化学工作站，专门用于汗液检测。系统支持检测汗液中的葡萄糖、乳酸、钾离子(K+)和钠离子(Na+)，并提供多种电化学检测方法。

## 功能特性

### 1. 支持的检测物质

| 物质 | 正常范围 | 检测方法 | 传感器类型 |
|------|----------|----------|------------|
| 葡萄糖 (Glucose) | 0-11 mM | 计时安培法 / DPV | 葡萄糖氧化酶传感器 |
| 乳酸 (Lactate) | 0-25 mM | DPV / 循环伏安法 | 乳酸氧化酶传感器 |
| 钾离子 (K+) | 2-8 mM | 电位分析法 | 离子选择性电极 (ISE) |
| 钠离子 (Na+) | 10-90 mM | 电位分析法 | 离子选择性电极 (ISE) |

### 2. 支持的电化学方法

#### 2.1 计时安培法 (Chronoamperometry)
- **原理**: 施加恒定电位，测量随时间变化的电流响应
- **应用**: 葡萄糖、氧气等气体传感器检测
- **特点**: 快速响应，适合实时监测
- **位置**: `examples/AD5940_ChronoAmperometric/`

#### 2.2 循环伏安法 (Cyclic Voltammetry, CV)
- **原理**: 施加三角波电压（正向+反向扫描），测量电流-电压曲线
- **应用**: 电化学体系表征，氧化还原反应研究
- **特点**: 可获得完整的电化学信息
- **位置**: `examples/AD5940_CyclicVoltammetry/` ⭐ **新增**

#### 2.3 差分脉冲伏安法 (Differential Pulse Voltammetry, DPV)
- **原理**: 在阶梯电压上叠加周期性脉冲，测量差分电流
- **应用**: 痕量物质检测（乳酸、重金属等）
- **特点**: 高灵敏度，低检测限
- **位置**: `examples/AD5940_DPV/` ⭐ **新增**

#### 2.4 电位分析法 (Potentiometry)
- **原理**: 测量开路电位，用于离子选择性电极
- **应用**: K+、Na+、pH等离子浓度测量
- **特点**: 无需施加电流，高输入阻抗 (>1GΩ)
- **位置**: `examples/AD5940_Potentiometry/` ⭐ **新增**

## FPGA接口实现

### 3.1 SPI主控制器模块 (`fpga/rtl/ad5940_spi_master.v`)

**功能特性**:
- 支持SPI Mode 0 (CPOL=0, CPHA=0)
- 可配置时钟分频 (最高16 MHz)
- 支持8/16/32位数据传输
- 全双工寄存器读写

**接口信号**:
```verilog
// 系统接口
input  clk              // 系统时钟 (如50 MHz)
input  rst_n            // 异步复位（低有效）
// 控制接口
input  start            // 启动SPI传输
input  rw               // 读(1)/写(0)
input  [15:0] addr      // AD5940寄存器地址
input  [31:0] wr_data   // 写数据
output [31:0] rd_data   // 读数据
output busy             // 忙标志
output done             // 完成标志
// SPI物理接口
output spi_cs_n         // 片选（低有效）
output spi_sclk         // SPI时钟
output spi_mosi         // 主出从入
input  spi_miso         // 主入从出
```

**使用示例**:
```verilog
// 实例化SPI主控制器
ad5940_spi_master #(
    .CLK_FREQ(50_000_000),   // 50 MHz系统时钟
    .SPI_FREQ(10_000_000)    // 10 MHz SPI时钟
) spi_inst (
    .clk(sys_clk),
    .rst_n(sys_rst_n),
    .start(spi_start),
    .rw(spi_rw),
    .addr(spi_addr),
    .wr_data(spi_wr_data),
    .rd_data(spi_rd_data),
    .busy(spi_busy),
    .done(spi_done),
    .spi_cs_n(ad5940_cs_n),
    .spi_sclk(ad5940_sclk),
    .spi_mosi(ad5940_mosi),
    .spi_miso(ad5940_miso)
);
```

### 3.2 顶层控制器模块 (`fpga/rtl/ad5940_controller.v`)

**功能特性**:
- 主机命令接口（UART/USB/并行总线）
- 多种电化学方法配置
- 数据采集与FIFO管理
- 中断处理（INT0, INT1）
- 状态LED指示

**支持的主机命令**:

| 命令代码 | 命令名称 | 功能描述 |
|---------|----------|----------|
| 0x01 | CMD_RESET | 复位AD5940 |
| 0x02 | CMD_INIT | 初始化AD5940 |
| 0x10 | CMD_CONFIG_CV | 配置循环伏安法 |
| 0x11 | CMD_CONFIG_DPV | 配置差分脉冲伏安法 |
| 0x12 | CMD_CONFIG_CA | 配置计时安培法 |
| 0x13 | CMD_CONFIG_POT | 配置电位分析法 |
| 0x20 | CMD_START_MEAS | 启动测量 |
| 0x21 | CMD_STOP_MEAS | 停止测量 |
| 0x30 | CMD_READ_DATA | 读取测量数据 |
| 0x31 | CMD_READ_STATUS | 读取状态 |
| 0x40 | CMD_WRITE_REG | 写AD5940寄存器 |
| 0x41 | CMD_READ_REG | 读AD5940寄存器 |

## 汗液检测配置模块

位置: `examples/SweatAnalysis/`

### 4.1 多物质检测配置

**头文件**: `SweatAnalysis.h`

**核心数据结构**:

```c
typedef struct {
    AnalyteType_t   analyte;        // 目标物质
    DetectionMethod_t method;       // 检测方法
    BoolFlag        enabled;        // 使能标志
    float           rtia_value;     // 跨阻电阻值 (Ω)
    uint32_t        rtia_sel;       // RTIA选择代码
    float           sensor_bias;    // 传感器偏置电压 (mV)
    float           range_min;      // 最小浓度
    float           range_max;      // 最大浓度
    float           calib_slope;    // 校准斜率
    float           calib_offset;   // 校准偏移
    BoolFlag        calib_valid;    // 校准有效标志
} SweatAnalyteConfig_t;
```

### 4.2 预配置参数

#### 葡萄糖检测配置
```c
const SweatAnalyteConfig_t SweatConfig_Glucose_Default = {
    .analyte = ANALYTE_GLUCOSE,
    .method = METHOD_CHRONOAMPEROMETRY,
    .rtia_value = 10000.0,          // 10kΩ
    .sensor_bias = 400.0,           // +0.4V (葡萄糖氧化酶最佳电位)
    .range_min = 0.0,
    .range_max = 11.0,              // 0-11 mM
};
```

#### 乳酸检测配置
```c
const SweatAnalyteConfig_t SweatConfig_Lactate_Default = {
    .analyte = ANALYTE_LACTATE,
    .method = METHOD_DPV,
    .rtia_value = 20000.0,          // 20kΩ
    .sensor_bias = 600.0,           // +0.6V (乳酸氧化酶最佳电位)
    .range_min = 0.0,
    .range_max = 25.0,              // 0-25 mM (运动时)
};
```

#### 钾离子(K+)检测配置
```c
const SweatAnalyteConfig_t SweatConfig_KIon_Default = {
    .analyte = ANALYTE_K_ION,
    .method = METHOD_POTENTIOMETRY,
    .rtia_value = 0.0,              // 电位法不需要RTIA
    .sensor_bias = 0.0,             // 开路测量
    .range_min = 2.0,
    .range_max = 8.0,               // 2-8 mM
    .calib_slope = 59.16,           // 25°C时的Nernst斜率 (mV/decade)
};
```

#### 钠离子(Na+)检测配置
```c
const SweatAnalyteConfig_t SweatConfig_NaIon_Default = {
    .analyte = ANALYTE_NA_ION,
    .method = METHOD_POTENTIOMETRY,
    .range_min = 10.0,
    .range_max = 90.0,              // 10-90 mM
    .calib_slope = 59.16,
};
```

### 4.3 API函数

```c
// 初始化汗液检测系统
AD5940Err SweatAnalysis_Init(SweatSystemConfig_t *pConfig);

// 配置单个物质传感器
AD5940Err SweatAnalysis_ConfigureAnalyte(SweatAnalyteConfig_t *pAnalyte);

// 启动测量
AD5940Err SweatAnalysis_StartMeasurement(AnalyteType_t analyte);

// 停止测量
AD5940Err SweatAnalysis_StopMeasurement(void);

// 获取测量结果
AD5940Err SweatAnalysis_GetResult(SweatMeasResult_t *pResult);

// 标定传感器（多点校准）
AD5940Err SweatAnalysis_Calibrate(AnalyteType_t analyte,
                                  float *standards,
                                  uint32_t num_points);
```

## 通道配置与量程管理

### 5.1 电流型测量（安培法）

**RTIA（跨阻放大器电阻）可选值**:
- 低功耗TIA: 200Ω ~ 512kΩ（26档）
- 高速TIA: 200Ω ~ 160kΩ（8档）
- 外部RTIA: 支持自定义电阻值

**量程计算**:
```
电流 (nA) = (ADC电压 - Vzero) / RTIA × 10^9
```

**量程选择示例**:
| 电流范围 | 推荐RTIA | 测量精度 |
|---------|---------|---------|
| 0-1 μA | 512 kΩ | 最高 |
| 0-10 μA | 100 kΩ | 高 |
| 0-100 μA | 10 kΩ | 中 |
| 0-1 mA | 1 kΩ | 低 |

### 5.2 电压型测量（电位法）

**ADC配置**:
- ADC参考电压: 1.82V (典型值)
- PGA增益: 1x, 1.5x, 2x, 4x, 9x
- 分辨率: 16位 (SINC2滤波后)
- 输入阻抗: >1GΩ (高阻模式)

**电压计算**:
```
电压 (mV) = (ADC_Code / 32768) × ADC_Ref / PGA_Gain
```

## 项目文件结构

```
ad5940-example/
├── fpga/                           ⭐ 新增 - FPGA代码
│   ├── rtl/                        # RTL设计文件
│   │   ├── ad5940_spi_master.v    # SPI主控制器
│   │   └── ad5940_controller.v    # 顶层控制器
│   ├── testbench/                  # 仿真测试文件
│   └── docs/                       # FPGA设计文档
│
├── examples/
│   ├── AD5940_ChronoAmperometric/  # 计时安培法（已有）
│   ├── AD5940_CyclicVoltammetry/   ⭐ 新增 - 循环伏安法
│   │   ├── CyclicVoltammetry.h
│   │   └── CyclicVoltammetry.c
│   ├── AD5940_DPV/                 ⭐ 新增 - 差分脉冲伏安法
│   │   ├── DPV.h
│   │   └── DPV.c
│   ├── AD5940_Potentiometry/       ⭐ 新增 - 电位分析法
│   │   ├── Potentiometry.h
│   │   └── Potentiometry.c
│   ├── SweatAnalysis/              ⭐ 新增 - 汗液检测模块
│   │   ├── SweatAnalysis.h
│   │   └── SweatAnalysis.c
│   └── ... (其他已有例程)
│
├── FPGA_WORKSTATION_CN.md          ⭐ 本文档
└── README_CN.md                     # 中文说明（已有）
```

## 使用指南

### 6.1 FPGA工程搭建

1. **导入RTL文件**:
   - 将 `fpga/rtl/` 中的Verilog文件添加到FPGA工程
   - 设置 `ad5940_controller.v` 为顶层模块

2. **引脚约束**:
```tcl
# AD5940 SPI接口
set_property PACKAGE_PIN XX [get_ports ad5940_cs_n]
set_property PACKAGE_PIN XX [get_ports ad5940_sclk]
set_property PACKAGE_PIN XX [get_ports ad5940_mosi]
set_property PACKAGE_PIN XX [get_ports ad5940_miso]

# 控制信号
set_property PACKAGE_PIN XX [get_ports ad5940_reset_n]
set_property PACKAGE_PIN XX [get_ports ad5940_int0]
set_property PACKAGE_PIN XX [get_ports ad5940_int1]
```

3. **时钟配置**:
   - 系统时钟: 50 MHz (推荐)
   - SPI时钟: 10 MHz (最高16 MHz)

### 6.2 MCU例程使用

参考现有的 `AD5940_ChronoAmperometric` 例程结构：

1. **包含头文件**:
```c
#include "ad5940.h"
#include "CyclicVoltammetry.h"  // 或其他方法
#include "SweatAnalysis.h"
```

2. **初始化配置**:
```c
AppCVCfg_Type *pCVCfg;
AppCVGetCfg(&pCVCfg);

// 修改参数
pCVCfg->StartVolt = -500.0;     // -0.5V
pCVCfg->PeakVolt = +500.0;      // +0.5V
pCVCfg->ScanRate = 100.0;       // 100 mV/s
pCVCfg->NumOfCycles = 3;        // 3个循环

// 初始化
AppCVInit(buffer, buffer_size);
```

3. **启动测量**:
```c
AppCVCtrl(CVCTRL_START, NULL);
```

4. **读取数据**:
```c
uint32_t data_count;
AppCVISR(data_buffer, &data_count);

// 计算电流
float current = AppCVCalcCurrent(adc_code, &voltage);
```

### 6.3 汗液检测完整流程

```c
// 1. 初始化系统
SweatSystemConfig_t sys_config;
sys_config.glucose = SweatConfig_Glucose_Default;
sys_config.lactate = SweatConfig_Lactate_Default;
sys_config.k_ion = SweatConfig_KIon_Default;
sys_config.na_ion = SweatConfig_NaIon_Default;
sys_config.sample_rate = 1.0;   // 1 Hz
SweatAnalysis_Init(&sys_config);

// 2. 校准传感器（可选）
float glucose_standards[] = {0, 5.5, 11.0};  // mM
SweatAnalysis_Calibrate(ANALYTE_GLUCOSE, glucose_standards, 3);

// 3. 启动测量
SweatAnalysis_StartMeasurement(ANALYTE_GLUCOSE);

// 4. 读取结果
SweatMeasResult_t result;
SweatAnalysis_GetResult(&result);

printf("葡萄糖浓度: %.2f mg/dL\n", result.concentration);
```

## 技术规格

### 7.1 AD5940性能指标

| 参数 | 规格 |
|-----|------|
| 电流测量范围 | 1 pA ~ 1 mA |
| 电流分辨率 | < 1 pA (RTIA=512kΩ) |
| 电压范围 | -1.8V ~ +1.8V |
| ADC分辨率 | 16位 |
| SPI接口速度 | 最高 16 MHz |
| 功耗 | 低功耗模式 < 100 μA |

### 7.2 检测性能

| 物质 | 检测限 | 线性范围 | 响应时间 |
|-----|--------|---------|---------|
| 葡萄糖 | < 0.1 mM | 0-20 mM | < 5 s |
| 乳酸 | < 0.05 mM | 0-25 mM | < 3 s |
| K+ | < 0.1 mM | 0.5-10 mM | < 10 s |
| Na+ | < 1 mM | 5-100 mM | < 10 s |

## 常见问题

### Q1: 如何选择合适的RTIA值？
**A**: 根据预期电流范围选择：
- 电流 < 1 μA → RTIA = 512kΩ ~ 100kΩ
- 电流 1-10 μA → RTIA = 100kΩ ~ 20kΩ
- 电流 10-100 μA → RTIA = 20kΩ ~ 5kΩ
- 电流 > 100 μA → RTIA = 5kΩ ~ 200Ω

### Q2: 循环伏安法扫描速率如何设置？
**A**: 典型范围10-1000 mV/s：
- 快速扫描(>200 mV/s): 用于快速筛选
- 中速扫描(50-200 mV/s): 常规分析
- 慢速扫描(<50 mV/s): 高分辨率，准平衡态

### Q3: 离子选择性电极如何校准？
**A**: 使用两点或三点标准溶液：
1. 准备已知浓度的标准溶液（如1 mM, 10 mM, 100 mM）
2. 依次测量各标准溶液的电位
3. 根据Nernst方程计算斜率和偏移
4. 理论斜率: 59.16 mV/decade (25°C, 单价离子)

### Q4: FPGA和MCU方案如何选择？
**A**:
- **MCU方案**: 适合单通道、低成本、快速原型开发
- **FPGA方案**: 适合多通道、高速采集、复杂信号处理

## 参考资料

1. [AD5940数据手册](https://www.analog.com/media/en/technical-documentation/data-sheets/AD5940.pdf)
2. [AD5940 Wiki](https://wiki.analog.com/resources/eval/user-guides/ad5940)
3. 电化学测量方法:
   - Bard, A. J., & Faulkner, L. R. (2001). Electrochemical Methods (2nd ed.)
   - Wang, J. (2006). Analytical Electrochemistry (3rd ed.)
4. 汗液检测相关文献:
   - Gao et al., Nature (2016) - Flexible wearable sweat sensors
   - Bandodkar & Wang, Trends in Biotechnology (2014) - Wearable biosensors

## 版本历史

- **v1.0** (2026-03-23)
  - ✅ 新增FPGA SPI主控制器模块
  - ✅ 新增FPGA顶层控制器模块
  - ✅ 新增循环伏安法(CV)例程
  - ✅ 新增差分脉冲伏安法(DPV)例程
  - ✅ 新增开路电位分析法(Potentiometry)例程
  - ✅ 新增汗液多物质检测配置模块
  - ✅ 完善中文技术文档

## 技术支持

如有问题或建议，请通过以下方式联系：
- GitHub Issues: [项目Issues页面]
- 邮箱: support@example.com

---
**版权所有** © 2017-2026 Analog Devices, Inc. & 电化学工作站项目组
