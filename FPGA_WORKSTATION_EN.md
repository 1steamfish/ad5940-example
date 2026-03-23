# FPGA-Based Single-Channel Electrochemical Workstation

## Overview

This project implements an FPGA-controlled single-channel electrochemical workstation using the AD5940 analog front-end chip, specifically designed for sweat analysis. The system supports detection of glucose, lactate, potassium ions (K+), and sodium ions (Na+) in sweat samples using multiple electrochemical methods.

## Key Features

### Supported Analytes

| Analyte | Normal Range | Detection Method | Sensor Type |
|---------|--------------|------------------|-------------|
| Glucose | 0-11 mM | Chronoamperometry / DPV | Glucose Oxidase |
| Lactate | 0-25 mM | DPV / Cyclic Voltammetry | Lactate Oxidase |
| K+ Ion | 2-8 mM | Potentiometry | Ion-Selective Electrode |
| Na+ Ion | 10-90 mM | Potentiometry | Ion-Selective Electrode |

### Supported Electrochemical Methods

1. **Chronoamperometry** (CA) - `examples/AD5940_ChronoAmperometric/`
   - Fixed potential, time-dependent current measurement
   - Fast response for real-time monitoring

2. **Cyclic Voltammetry** (CV) - `examples/AD5940_CyclicVoltammetry/` ⭐ **NEW**
   - Bidirectional voltage sweep with current measurement
   - Complete electrochemical characterization

3. **Differential Pulse Voltammetry** (DPV) - `examples/AD5940_DPV/` ⭐ **NEW**
   - High sensitivity for trace analysis
   - Ideal for lactate and heavy metal detection

4. **Potentiometry** - `examples/AD5940_Potentiometry/` ⭐ **NEW**
   - Open-circuit potential measurement
   - For ion-selective electrodes (K+, Na+, pH)

## FPGA Implementation

### SPI Master Controller (`fpga/rtl/ad5940_spi_master.v`)

Features:
- SPI Mode 0 (CPOL=0, CPHA=0)
- Configurable clock frequency (up to 16 MHz)
- 8/16/32-bit data transfer support
- Full-duplex register read/write

### Top-Level Controller (`fpga/rtl/ad5940_controller.v`)

Features:
- Host command interface (UART/USB/parallel)
- Multiple electrochemical method configuration
- Data acquisition with FIFO management
- Interrupt handling (INT0, INT1)
- Status LED indicators

### Supported Commands

| Command Code | Name | Function |
|--------------|------|----------|
| 0x01 | CMD_RESET | Reset AD5940 |
| 0x02 | CMD_INIT | Initialize AD5940 |
| 0x10 | CMD_CONFIG_CV | Configure Cyclic Voltammetry |
| 0x11 | CMD_CONFIG_DPV | Configure DPV |
| 0x12 | CMD_CONFIG_CA | Configure Chronoamperometry |
| 0x13 | CMD_CONFIG_POT | Configure Potentiometry |
| 0x20 | CMD_START_MEAS | Start measurement |
| 0x21 | CMD_STOP_MEAS | Stop measurement |
| 0x30 | CMD_READ_DATA | Read measurement data |

## Sweat Analysis Module

Location: `examples/SweatAnalysis/`

### Pre-configured Settings

**Glucose Detection**:
```c
- Method: Chronoamperometry
- RTIA: 10kΩ
- Bias: +0.4V (optimal for glucose oxidase)
- Range: 0-11 mM
```

**Lactate Detection**:
```c
- Method: DPV
- RTIA: 20kΩ
- Bias: +0.6V (optimal for lactate oxidase)
- Range: 0-25 mM
```

**K+ Ion Detection**:
```c
- Method: Potentiometry
- Input impedance: >1GΩ
- Nernst slope: 59.16 mV/decade at 25°C
- Range: 2-8 mM
```

**Na+ Ion Detection**:
```c
- Method: Potentiometry
- Range: 10-90 mM
```

## File Structure

```
ad5940-example/
├── fpga/                           ⭐ NEW - FPGA code
│   ├── rtl/
│   │   ├── ad5940_spi_master.v    # SPI master controller
│   │   └── ad5940_controller.v    # Top-level controller
│   ├── testbench/
│   └── docs/
│
├── examples/
│   ├── AD5940_CyclicVoltammetry/   ⭐ NEW
│   ├── AD5940_DPV/                 ⭐ NEW
│   ├── AD5940_Potentiometry/       ⭐ NEW
│   ├── SweatAnalysis/              ⭐ NEW
│   └── ... (existing examples)
│
├── FPGA_WORKSTATION_CN.md          ⭐ NEW - Chinese documentation
└── FPGA_WORKSTATION_EN.md          ⭐ THIS FILE
```

## Quick Start

### FPGA Implementation

1. Import RTL files from `fpga/rtl/` into your FPGA project
2. Set `ad5940_controller.v` as top module
3. Configure pin constraints for SPI interface
4. Set system clock to 50 MHz (recommended)

### MCU Examples

1. Include headers:
```c
#include "ad5940.h"
#include "CyclicVoltammetry.h"
#include "SweatAnalysis.h"
```

2. Configure measurement:
```c
AppCVCfg_Type *pCVCfg;
AppCVGetCfg(&pCVCfg);
pCVCfg->StartVolt = -500.0;  // -0.5V
pCVCfg->PeakVolt = +500.0;   // +0.5V
AppCVInit(buffer, buffer_size);
```

3. Start measurement:
```c
AppCVCtrl(CVCTRL_START, NULL);
```

## Performance Specifications

### AD5940 Specifications

| Parameter | Specification |
|-----------|---------------|
| Current Range | 1 pA ~ 1 mA |
| Current Resolution | < 1 pA (RTIA=512kΩ) |
| Voltage Range | -1.8V ~ +1.8V |
| ADC Resolution | 16-bit |
| SPI Speed | Up to 16 MHz |

### Detection Performance

| Analyte | Detection Limit | Linear Range | Response Time |
|---------|----------------|--------------|---------------|
| Glucose | < 0.1 mM | 0-20 mM | < 5 s |
| Lactate | < 0.05 mM | 0-25 mM | < 3 s |
| K+ | < 0.1 mM | 0.5-10 mM | < 10 s |
| Na+ | < 1 mM | 5-100 mM | < 10 s |

## References

1. [AD5940 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/AD5940.pdf)
2. [AD5940 Wiki](https://wiki.analog.com/resources/eval/user-guides/ad5940)
3. Bard, A. J., & Faulkner, L. R. (2001). Electrochemical Methods
4. Gao et al., Nature (2016) - Flexible wearable sweat sensors

## Version History

- **v1.0** (2026-03-23)
  - ✅ Added FPGA SPI master controller
  - ✅ Added FPGA top-level controller
  - ✅ Added Cyclic Voltammetry (CV) example
  - ✅ Added Differential Pulse Voltammetry (DPV) example
  - ✅ Added Potentiometry example
  - ✅ Added Sweat Analysis module
  - ✅ Added comprehensive documentation

## License

Copyright (c) 2017-2026 Analog Devices, Inc. All Rights Reserved.

---
For detailed Chinese documentation, see [FPGA_WORKSTATION_CN.md](FPGA_WORKSTATION_CN.md)
