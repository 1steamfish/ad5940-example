/*!
 *****************************************************************************
 @file:    CyclicVoltammetry.h
 @author:  Electrochemical Workstation Team
 @brief:   Cyclic Voltammetry header file.
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#ifndef _CYCLICVOLTAMMETRY_H_
#define _CYCLICVOLTAMMETRY_H_
#include "ad5940.h"
#include <stdio.h>
#include "string.h"
#include "math.h"

/* Do not modify following parameters */
#define ALIGIN_VOLT2LSB     0   /* Set it to 1 to align each voltage step to 1LSB of DAC. 0: step code is fractional. */
#define DAC12BITVOLT_1LSB   (2200.0f/4095)  //mV
#define DAC6BITVOLT_1LSB    (DAC12BITVOLT_1LSB*64)  //mV

/**
 * Cyclic Voltammetry (CV) application parameter structure
 * CV performs a triangular voltage sweep - ramping up from start to peak,
 * then ramping back down to start voltage, measuring current throughout.
 */
typedef struct
{
/* Common configurations for all kinds of Application. */
  BoolFlag  bParaChanged;         /**< Indicate to generate sequence again. It's auto cleared by AppCVInit */
  uint32_t  SeqStartAddr;         /**< Initialization sequence start address in SRAM of AD5940  */
  uint32_t  MaxSeqLen;            /**< Limit the maximum sequence.   */
  uint32_t  SeqStartAddrCal;      /**< Calibration sequence start address in SRAM of AD5940 */
  uint32_t  MaxSeqLenCal;         /**< Maximum calibration sequence length */

/* Application related parameters */
  float     LFOSCClkFreq;         /**< The clock frequency of Wakeup Timer in Hz. Typically it's 32kHz. */
  float     SysClkFreq;           /**< The real frequency of system clock */
  float     AdcClkFreq;           /**< The real frequency of ADC clock */
  float     RcalVal;              /**< Rcal value in Ohm */
  float     ADCRefVolt;           /**< The real ADC voltage in mV. */
  BoolFlag  bTestFinished;        /**< Variable to indicate CV test has finished */

  /* CV waveform parameters */
  float     StartVolt;            /**< The start voltage of CV sweep in mV (e.g., -500 to +500) */
  float     PeakVolt;             /**< The peak voltage of CV sweep in mV */
  float     VzeroStart;           /**< The start voltage of Vzero in mV. Set it to 2400mV by default */
  float     VzeroPeak;            /**< The peak voltage of Vzero in mV. Set it to 200mV by default */
  uint32_t  NumOfCycles;          /**< Number of CV cycles to perform (typically 1-10) */
  float     ScanRate;             /**< Scan rate in mV/s (typical range: 10-1000 mV/s) */
  uint32_t  StepNumber;           /**< Total number of steps per sweep direction. Limited to 4095. */

  /* Receive path configuration */
  float     SampleDelay;          /**< The time delay between update DAC and start ADC in ms */
  uint32_t  LPTIARtiaSel;         /**< Select RTIA (transimpedance amplifier resistor) */
  uint32_t  LPTIARloadSel;        /**< Select Rload */
  float     ExternalRtiaValue;    /**< Optional external RTIA value in Ohm */
  uint32_t  AdcPgaGain;           /**< PGA Gain select from GNPGA_1, GNPGA_1_5, GNPGA_2, GNPGA_4, GNPGA_9 */
  uint8_t   ADCSinc3Osr;          /**< SINC3 filter oversampling rate */

  /* Digital related */
  uint32_t  FifoThresh;           /**< FIFO Threshold value */

/* Private variables for internal usage */
  BoolFlag  CVInited;             /**< If the program run firstly, generated initialization sequence commands */
  fImpPol_Type  RtiaValue;        /**< Calibrated Rtia value */
  SEQInfo_Type  InitSeqInfo;
  SEQInfo_Type  ADCSeqInfo;
  BoolFlag      bFirstDACSeq;     /**< Init DAC sequence */
  SEQInfo_Type  DACSeqInfo;       /**< The first DAC update sequence info */
  uint32_t  CurrStepPos;          /**< Current position in sweep */
  uint32_t  CurrCycle;            /**< Current cycle number */
  float     DACCodePerStep;       /**< DAC code increment per step */
  float     CurrDACCode;          /**< Current DAC code */
  uint32_t  CurrVzeroCode;        /**< Current Vzero code */
  BoolFlag  bForwardSweep;        /**< TRUE = forward sweep (start->peak), FALSE = reverse (peak->start) */
  BoolFlag  StopRequired;         /**< After FIFO is ready, stop the measurement sequence */

  enum _CVState{
    CV_STATE_IDLE = 0,
    CV_STATE_INIT,
    CV_STATE_FORWARD,       /**< Forward sweep (start voltage -> peak voltage) */
    CV_STATE_REVERSE,       /**< Reverse sweep (peak voltage -> start voltage) */
    CV_STATE_COMPLETE,
    CV_STATE_STOP
  } CVState;

}AppCVCfg_Type;

/* Control commands */
#define CVCTRL_START          0
#define CVCTRL_STOPNOW        1
#define CVCTRL_STOPSYNC       2
#define CVCTRL_SHUTDOWN       3   /**< Note: shutdown here means turn off everything and put AFE to hibernate mode. */

/* Function prototypes */
AD5940Err AppCVInit(uint32_t *pBuffer, uint32_t BufferSize);
AD5940Err AppCVGetCfg(void *pCfg);
AD5940Err AppCVISR(void *pBuff, uint32_t *pCount);
AD5940Err AppCVCtrl(uint32_t Command, void *pPara);
float AppCVCalcCurrent(uint32_t ADCcode, float *pVolt);
float AppCVCalcVoltage(uint32_t ADCcode);

#endif
