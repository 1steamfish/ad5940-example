/*!
 *****************************************************************************
 @file:    DPV.h
 @author:  Electrochemical Workstation Team
 @brief:   Differential Pulse Voltammetry header file.
 @details  DPV applies voltage steps with periodic pulses and measures
           differential current for high-sensitivity trace analysis.
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#ifndef _DPV_H_
#define _DPV_H_
#include "ad5940.h"
#include <stdio.h>
#include "string.h"
#include "math.h"

#define DAC12BITVOLT_1LSB   (2200.0f/4095)  //mV
#define DAC6BITVOLT_1LSB    (DAC12BITVOLT_1LSB*64)  //mV

/**
 * Differential Pulse Voltammetry (DPV) application parameter structure
 * DPV is highly sensitive for trace analysis of glucose, lactate, ions
 */
typedef struct
{
/* Common configurations */
  BoolFlag  bParaChanged;         /**< Indicate to regenerate sequence */
  uint32_t  SeqStartAddr;         /**< Initialization sequence start address */
  uint32_t  MaxSeqLen;            /**< Maximum sequence length */
  uint32_t  SeqStartAddrCal;      /**< Calibration sequence start address */
  uint32_t  MaxSeqLenCal;         /**< Maximum calibration sequence length */

/* Application related parameters */
  float     LFOSCClkFreq;         /**< Wakeup Timer clock frequency (Hz) */
  float     SysClkFreq;           /**< System clock frequency (Hz) */
  float     AdcClkFreq;           /**< ADC clock frequency (Hz) */
  float     RcalVal;              /**< Rcal value (Ohm) */
  float     ADCRefVolt;           /**< ADC reference voltage (mV) */
  BoolFlag  bTestFinished;        /**< Test finished flag */

  /* DPV waveform parameters */
  float     StartVolt;            /**< Start voltage (mV), e.g., -500 */
  float     EndVolt;              /**< End voltage (mV), e.g., +500 */
  float     PulseAmplitude;       /**< Pulse amplitude (mV), typical 25-100 mV */
  float     PulseWidth;           /**< Pulse width (ms), typical 20-100 ms */
  float     PulsePeriod;          /**< Period between pulses (ms) */
  float     StepHeight;           /**< Voltage step increment (mV), typical 2-10 mV */
  uint32_t  StepNumber;           /**< Number of voltage steps */

  /* Measurement configuration */
  float     SampleDelay;          /**< Delay before sampling (ms) */
  uint32_t  LPTIARtiaSel;         /**< RTIA selection */
  uint32_t  LPTIARloadSel;        /**< Rload selection */
  float     ExternalRtiaValue;    /**< External RTIA value (Ohm) */
  uint32_t  AdcPgaGain;           /**< PGA gain setting */
  uint8_t   ADCSinc3Osr;          /**< SINC3 OSR */

  /* Digital configuration */
  uint32_t  FifoThresh;           /**< FIFO threshold */

/* Private variables */
  BoolFlag  DPVInited;            /**< Initialization flag */
  fImpPol_Type  RtiaValue;        /**< Calibrated RTIA value */
  SEQInfo_Type  InitSeqInfo;
  SEQInfo_Type  MeasSeqInfo;
  uint32_t  CurrStepPos;          /**< Current step position */
  float     CurrVoltage;          /**< Current base voltage */
  BoolFlag  StopRequired;         /**< Stop flag */

  enum _DPVState{
    DPV_STATE_IDLE = 0,
    DPV_STATE_INIT,
    DPV_STATE_BASELINE,     /**< Measure baseline current (before pulse) */
    DPV_STATE_PULSE,        /**< Measure current during pulse */
    DPV_STATE_STEP,         /**< Step to next voltage */
    DPV_STATE_COMPLETE,
    DPV_STATE_STOP
  } DPVState;

}AppDPVCfg_Type;

/* Control commands */
#define DPVCTRL_START          0
#define DPVCTRL_STOPNOW        1
#define DPVCTRL_STOPSYNC       2
#define DPVCTRL_SHUTDOWN       3

/* Function prototypes */
AD5940Err AppDPVInit(uint32_t *pBuffer, uint32_t BufferSize);
AD5940Err AppDPVGetCfg(void *pCfg);
AD5940Err AppDPVISR(void *pBuff, uint32_t *pCount);
AD5940Err AppDPVCtrl(uint32_t Command, void *pPara);
float AppDPVCalcCurrent(uint32_t ADCcode);
float AppDPVCalcVoltage(uint32_t ADCcode);

#endif
