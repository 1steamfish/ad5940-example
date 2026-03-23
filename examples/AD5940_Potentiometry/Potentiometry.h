/*!
 *****************************************************************************
 @file:    Potentiometry.h
 @author:  Electrochemical Workstation Team
 @brief:   Open Circuit Potentiometry header file.
 @details  Potentiometry measures open-circuit potential for ion-selective
           electrodes (ISE) - ideal for K+, Na+, pH measurements in sweat.
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#ifndef _POTENTIOMETRY_H_
#define _POTENTIOMETRY_H_
#include "ad5940.h"
#include <stdio.h>
#include "string.h"
#include "math.h"

/**
 * Potentiometry application parameter structure
 * Used for ion-selective electrodes (K+, Na+, pH, etc.)
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
  float     ADCRefVolt;           /**< ADC reference voltage (mV) */
  BoolFlag  bTestFinished;        /**< Test finished flag */

  /* Potentiometry parameters */
  float     SampleRate;           /**< Sampling rate (Hz), typical 1-10 Hz */
  uint32_t  NumSamples;           /**< Number of samples to collect */
  uint32_t  AveragingCount;       /**< Number of readings to average */

  /* Ion type configuration */
  enum _IonType{
    ION_TYPE_K_PLUS = 0,    /**< Potassium ion (K+) */
    ION_TYPE_NA_PLUS,       /**< Sodium ion (Na+) */
    ION_TYPE_H_PLUS,        /**< Hydrogen ion (pH) */
    ION_TYPE_CL_MINUS,      /**< Chloride ion (Cl-) */
    ION_TYPE_CUSTOM         /**< User-defined ion */
  } IonType;

  /* Calibration parameters */
  float     CalibSlope;           /**< Nernst slope (mV/decade), e.g., 59.16 for pH at 25°C */
  float     CalibOffset;          /**< Calibration offset (mV) */
  float     Temperature;          /**< Temperature (°C) for Nernst correction */

  /* Measurement configuration */
  uint32_t  AdcPgaGain;           /**< PGA gain setting */
  uint8_t   ADCSinc3Osr;          /**< SINC3 OSR */
  uint8_t   ADCSinc2Osr;          /**< SINC2 OSR */
  uint32_t  DataFifoSrc;          /**< FIFO data source */

  /* Input impedance - important for ISE */
  BoolFlag  HighImpedanceMode;    /**< Enable high-Z input (>1GΩ for ISE) */

  /* Digital configuration */
  uint32_t  FifoThresh;           /**< FIFO threshold */

/* Private variables */
  BoolFlag  POTInited;            /**< Initialization flag */
  SEQInfo_Type  InitSeqInfo;
  SEQInfo_Type  MeasSeqInfo;
  uint32_t  SampleCount;          /**< Current sample count */
  float     AccumVoltage;         /**< Accumulated voltage for averaging */
  BoolFlag  StopRequired;         /**< Stop flag */

  enum _POTState{
    POT_STATE_IDLE = 0,
    POT_STATE_INIT,
    POT_STATE_STABILIZE,    /**< Allow electrode to stabilize */
    POT_STATE_MEASURE,      /**< Active measurement */
    POT_STATE_COMPLETE,
    POT_STATE_STOP
  } POTState;

}AppPOTCfg_Type;

/* Control commands */
#define POTCTRL_START          0
#define POTCTRL_STOPNOW        1
#define POTCTRL_SHUTDOWN       2
#define POTCTRL_CALIBRATE      3

/* Function prototypes */
AD5940Err AppPOTInit(uint32_t *pBuffer, uint32_t BufferSize);
AD5940Err AppPOTGetCfg(void *pCfg);
AD5940Err AppPOTISR(void *pBuff, uint32_t *pCount);
AD5940Err AppPOTCtrl(uint32_t Command, void *pPara);
float AppPOTCalcVoltage(uint32_t ADCcode);
float AppPOTCalcConcentration(float voltage_mV);

#endif
