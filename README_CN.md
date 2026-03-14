# AD5940 例程说明（中文）

[AD5940/AD5941](https://www.analog.com/en/products/ad5940.html) 是 Analog Devices 推出的高精度阻抗与电化学前端芯片，通过 SPI 总线与外部 MCU 通信。

本仓库提供了丰富的例程，涵盖基础硬件控制、模拟前端配置、阻抗测量、电化学分析以及生物医学应用等多个方向。

---

## 例程分类总览

| 类别 | 数量 | 例程列表 |
|------|------|----------|
| 硬件基础控制 | 3 | Reset、SPI、Sequencer |
| 模拟前端与信号发生 | 4 | Temperature、LPLoop、LPDAC、WG（4种波形）、HSDACCal |
| ADC 与信号处理 | 2 | ADC（3种模式）、DFT |
| 阻抗测量 | 7 | Impedance、Impedance（可调频率）、BATImpedance、BIA、BIA-HiZ、BIOZ-2Wire、ECSns_EIS |
| 电化学分析 | 4 | Amperometric、ChronoAmperometric、SqrWaveVoltammetry、Ramp |
| 生物医学应用 | 3 | ECG、EDA、BioElec（多模态） |

---

## 一、硬件基础控制例程

### 1. AD5940_Reset — 芯片复位

演示 AD5940 的三种复位方式：
- **硬件复位**：通过拉低 RESET 引脚实现
- **软件复位**：通过向 MMR 寄存器写入特定值实现
- **上电复位（POR）**：芯片上电时自动触发

例程同时展示如何通过读取 `RSTSTA` 寄存器来判断复位原因。

---

### 2. AD5940_SPI — SPI 通信测试

演示通过 SPI 总线与 AD5940 进行寄存器读写通信的基本方法：
- 使用 `AD5940_ReadReg` 读取寄存器
- 使用 `AD5940_WriteReg` 写入寄存器

可用于验证 SPI 总线连接是否正常。

---

### 3. AD5940_Sequencer — 序列发生器操作

演示 AD5940 内置序列发生器（Sequencer）的使用方法：
- 将命令序列预加载到片内 SRAM
- 通过 MMR 写入、GPIO 触发、定时器触发等方式启动序列
- 支持 4 条独立序列，用于自动化控制模拟前端（AFE）

---

## 二、模拟前端与信号发生例程

### 4. AD5940_Temperature — 温度测量

使用 AD5940 片内温度传感器进行温度测量：
- 支持 4Hz 采样率（可配置）
- 配置 SINC3/SINC2 滤波器
- 支持 FIFO 阈值中断，可结合序列发生器使用

---

### 5. AD5940_LPLoop — 低功耗回路

演示低功耗（LP）回路放大器和 LPDAC 的使用：
- 使用 12 位 LPDAC 以片内 2.5V 基准输出电压
- 通过 LP 电位器放大器（LP PA）将电压缓冲输出至 RE0 引脚

---

### 6. AD5940_LPDAC — 低功耗 DAC

专门演示低功耗 DAC 的配置与使用：
- 启用低功耗带隙基准
- 使用 LP 参考缓冲器（2.5V 输出）
- 通过 LPDAC 输出稳定电压

---

### 7. AD5940_WG — 波形发生器

演示 AD5940 内置波形发生器（Waveform Generator）的多种输出模式：

| 子例程 | 功能 |
|--------|------|
| `AD5940_WGSin.c` | 输出 25kHz 正弦波 |
| `AD5940_WGSin_LPDAC.c` | 正弦波叠加 LPDAC 直流偏置输出 |
| `AD5940_WGArbitrary.c` | 输出任意波形（用户自定义） |
| `AD5940_WGTrapezoid.c` | 输出梯形波 |

同时展示开关矩阵（Switch Matrix）的配置方法。

---

### 8. AD5940_HSDACCal — 高速 DAC 校准

对高速 DAC（HSDAC）进行偏置校准，支持多种增益档位：
- ±607 mV、±75 mV、±15.14 mV、±121.2 mV

**注意**：每种功耗模式和增益档位均需单独校准。

---

## 三、ADC 与信号处理例程

### 9. AD5940_ADC — 模数转换

提供三种 ADC 工作模式：

| 子例程 | 功能 |
|--------|------|
| `AD5940_ADCPolling.c` | 轮询模式：基本 ADC 采样，包含 PGA 校准 |
| `AD5940_ADCMeanFIFO.c` | FIFO 均值模式：ADC 数据写入 FIFO 后取均值 |
| `AD5940_ADCNotchTest.c` | 陷波测试：验证陷波滤波器的工频抑制效果 |

---

### 10. AD5940_DFT — 离散傅里叶变换

使用 AD5940 片内 DFT 引擎进行频域分析：
- 需同时使能正弦波发生器作为激励信号
- 轮询方式读取 DFT 运算结果
- 可用于阻抗计算和频率分量提取

---

## 四、阻抗测量例程

### 11. AD5940_Impedance — 基础阻抗测量

标准阻抗测量例程，支持 2 线制和 4 线制配置：
- 可配置激励频率、RTIA（跨阻放大器增益电阻）、DAC 电压、偏置电压
- 包含 RCAL 参考校准
- 适用于通用阻抗测量场景

---

### 12. AD5940_Impedance_Adjustable_with_frequency — 可调频率阻抗测量

在基础阻抗例程基础上增强：
- 支持运行时动态调整激励频率
- 可进行频率扫描，实现电化学阻抗谱（EIS）测量

---

### 13. AD5940_BATImpedance — 电池阻抗测量

测量电池内阻：
- 状态机流程：初始化 → 测量 RCAL → 测量电池
- 支持 RCAL 通道、电池通道、放大器通道预充电

---

### 14. AD5940_BIA — 人体阻抗分析

用于生物医学的 4 线制人体阻抗分析（BIA）：
- 多频率阻抗测量，可分析人体成分（水分、脂肪、肌肉比例）
- 包含 RTIA 自动校准

---

### 15. AD5940_BIA_HiZ_Electrodes — 高阻抗电极人体阻抗分析

针对高阻抗电极优化的 BIA 例程：
- 适用于皮肤直接接触的干电极
- 解决高阻抗电极带来的信号衰减问题

---

### 16. AD5940_BIOZ-2Wire — 2 线制生物阻抗测量

使用 2 线制（而非 4 线制）进行生物阻抗（BIOZ）测量：
- 适用于电极放置受限的场景
- 简化连线，降低系统复杂度

---

### 17. AD5940_ECSns_EIS — 电化学传感器阻抗谱

基于阻抗例程，专为电化学传感器应用设计：
- 通过测量传感器阻抗变化检测化学或生物物质
- 适用于葡萄糖传感器、生物传感器等应用场景

---

## 五、电化学分析例程

### 18. AD5940_Amperometric — 安培法测量

在恒定施加电压下测量电流响应：
- 可配置偏置电压和采样率
- 适用于电化学传感器（如氧气传感器、气体传感器）

---

### 19. AD5940_ChronoAmperometric — 计时安培法

施加电位阶跃，记录电流随时间的变化曲线：
- 用于分析电化学反应动力学
- 可用于传感器响应特性表征

---

### 20. AD5940_SqrWaveVoltammetry — 方波伏安法

施加方波电压信号，测量电流交流分量：
- 提供频率分辨的电化学信息
- 相比线性扫描，灵敏度更高、分析速度更快

---

### 21. AD5940_Ramp — 斜坡测试（线性扫描伏安法）

线性扫描电压并测量电流响应：
- 可用于循环伏安法（CV）实验
- 适用于电化学体系基础表征

---

## 六、生物医学应用例程

### 22. AD5940_ECG — 心电图采集

采集人体心脏电信号（心电图，ECG）：
- 高速 ADC 采样，最高支持 1500 Hz 采样率
- 配置可编程增益放大器（PGA）和 SINC 滤波器
- 适用于医疗级心电信号采集

---

### 23. AD5940_EDA — 皮肤电活动测量

测量皮肤电特性（电导肤反应，即 EDA/GSR）：
- 可用于情绪与压力状态检测
- 包含 UART 命令接口，支持实时参数调整
- 内置低通滤波器稳定时间支持

---

### 24. AD5940_BioElec — 综合生物电测量

集成 BIA、ECG、EDA 三种生物电测量功能的综合例程：
- **BIA**：分析人体成分（水分、脂肪、肌肉）
- **ECG**：采集心电信号
- **EDA**：检测皮肤电活动（情绪/压力指标）
- UART 命令接口，支持在三种模式间实时切换

---

## 开发环境说明

### 支持的 MCU 平台

| 平台 | 说明 |
|------|------|
| **ADICUP3029**（ADuCM3029） | 主推平台（Analog Devices ARM MCU） |
| **NUCLEO-F411RE**（STM32F4） | 备选平台 |

### IDE 支持

- **Keil MDK**（推荐，所有例程均已验证）
- **IAR Embedded Workbench**

> 注意：Keil 配合 ADuCM3029 使用时需安装 ARM Compiler 5 版本。

### 评估板

| 评估板 | 适用场景 |
|--------|----------|
| [EVAL-AD5940BIOZ](https://www.analog.com/cn/design-center/evaluation-hardware-and-software/evaluation-boards-kits/EVAL-AD5940BIOZ.html) | 医疗健康应用（EDA / BIA / ECG） |
| [EVAL-AD5940ELCZ](https://www.analog.com/cn/design-center/evaluation-hardware-and-software/evaluation-boards-kits/EVAL-AD5940ELCZ.html) | 工业电化学应用（气体传感器、水质检测等） |

---

## 相关资源

- [AD5940 Wiki](https://wiki.analog.com/resources/eval/user-guides/ad5940)
- [AD5940 数据手册](https://www.analog.com/media/en/technical-documentation/data-sheets/AD5940.pdf)
- [AD5940 常见问题](https://ez.analog.com/data_converters/precision_adcs/w/documents/14012/ad5940-faqs)
- [SensorPal 上位机工具](https://wiki.analog.com/resources/eval/user-guides/eval-ad5940/tools/sensorpal_setup_guide)

---

版权所有 © 2017–2019 Analog Devices, Inc. 保留所有权利。
