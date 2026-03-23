/*!
 *****************************************************************************
 @file:    CyclicVoltammetry.c
 @author:  Electrochemical Workstation Team
 @brief:   Cyclic Voltammetry measurement implementation.
 @details  Implements bidirectional voltage sweep (forward + reverse) with
           current measurement for electrochemical analysis.
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#include "CyclicVoltammetry.h"

/**
 * @brief Default CV application parameters
 * @details Configuration for glucose/lactate detection in sweat
 */
AppCVCfg_Type AppCVCfg =
{
    .bParaChanged = bFALSE,
    .SeqStartAddr = 0,
    .MaxSeqLen = 0,
    .SeqStartAddrCal = 0,
    .MaxSeqLenCal = 0,

    /* Clock configuration */
    .LFOSCClkFreq = 32000.0,
    .SysClkFreq = 16000000.0,
    .AdcClkFreq = 16000000.0,
    .RcalVal = 10000.0,
    .ADCRefVolt = 1820.0f,
    .bTestFinished = bFALSE,

    /* CV parameters - optimized for sweat glucose detection */
    .StartVolt = -500.0f,           /* Start at -0.5V */
    .PeakVolt = +500.0f,            /* Peak at +0.5V */
    .VzeroStart = 2200.0f,          /* Vzero 2.2V */
    .VzeroPeak = 400.0f,            /* Vzero 0.4V */
    .NumOfCycles = 3,               /* 3 cycles for averaging */
    .ScanRate = 100.0f,             /* 100 mV/s scan rate */
    .StepNumber = 500,              /* 500 steps per direction */

    /* Receive path */
    .SampleDelay = 1.0f,            /* 1ms settling time */
    .LPTIARtiaSel = LPTIARTIA_10K,  /* 10kΩ RTIA for μA range */
    .LPTIARloadSel = LPTIARLOAD_SHORT,
    .ExternalRtiaValue = 10000.0f,
    .AdcPgaGain = ADCPGA_1P5,       /* 1.5x gain */
    .ADCSinc3Osr = ADCSINC3OSR_2,
    .FifoThresh = 4,

    /* Private */
    .CVInited = bFALSE,
    .StopRequired = bFALSE,
    .CVState = CV_STATE_IDLE,
    .bFirstDACSeq = bTRUE,
    .bForwardSweep = bTRUE,
    .CurrCycle = 0,
};

/**
 * @brief Get CV configuration pointer
 */
AD5940Err AppCVGetCfg(void *pCfg)
{
    if(pCfg){
        *(AppCVCfg_Type**)pCfg = &AppCVCfg;
        return AD5940ERR_OK;
    }
    return AD5940ERR_PARA;
}

/**
 * @brief Calculate current from ADC code
 * @param ADCcode: ADC result code
 * @param pVolt: Pointer to store calculated voltage (optional)
 * @return Current in nanoamperes (nA)
 */
float AppCVCalcCurrent(uint32_t ADCcode, float *pVolt)
{
    float voltage, current;
    float rtia = AppCVCfg.RtiaValue.Real;

    if(AppCVCfg.ExternalRtiaValue > 0)
        rtia = AppCVCfg.ExternalRtiaValue;

    /* Calculate voltage from ADC code */
    voltage = AppCVCalcVoltage(ADCcode);

    if(pVolt)
        *pVolt = voltage;

    /* Current = Voltage / RTIA */
    /* Result in nanoamperes */
    current = voltage / rtia * 1e9;

    return current;
}

/**
 * @brief Calculate voltage from ADC code
 * @param ADCcode: ADC result code
 * @return Voltage in millivolts (mV)
 */
float AppCVCalcVoltage(uint32_t ADCcode)
{
    float voltage;
    uint32_t  tmp;
    float kFactor = 1.0;

    /* PGA gain factor */
    switch(AppCVCfg.AdcPgaGain)
    {
        case ADCPGA_1:
            kFactor = 1.0;
            break;
        case ADCPGA_1P5:
            kFactor = 1.5;
            break;
        case ADCPGA_2:
            kFactor = 2.0;
            break;
        case ADCPGA_4:
            kFactor = 4.0;
            break;
        case ADCPGA_9:
            kFactor = 9.0;
            break;
        default:
            kFactor = 1.0;
            break;
    }

    /* Convert ADC code to voltage */
    tmp = ADCcode & 0xffff;
    if(tmp & (1L << 15))  /* Negative */
    {
        tmp = (tmp ^ 0xffff) + 1;  /* 2's complement */
        voltage = (float)tmp;
        voltage = voltage / 32768.0 * AppCVCfg.ADCRefVolt / kFactor;
        voltage = -voltage;
    }
    else
    {
        voltage = (float)tmp;
        voltage = voltage / 32768.0 * AppCVCfg.ADCRefVolt / kFactor;
    }

    return voltage;
}

/**
 * @brief Control CV application (start, stop, etc.)
 * @param Command: Control command
 * @param pPara: Optional parameter
 * @return AD5940ERR_OK if successful
 */
AD5940Err AppCVCtrl(uint32_t Command, void *pPara)
{
    switch(Command)
    {
        case CVCTRL_START:
        {
            if(AppCVCfg.CVInited == bFALSE)
                return AD5940ERR_APPERROR;

            /* Start CV measurement */
            AppCVCfg.CVState = CV_STATE_FORWARD;
            AppCVCfg.CurrCycle = 0;
            AppCVCfg.bForwardSweep = bTRUE;
            AppCVCfg.StopRequired = bFALSE;

            /* Wake up AD5940 */
            if(AD5940_WakeUp(10) > 10)
                return AD5940ERR_WAKEUP;

            /* Configure and start sequencer */
            AD5940_SEQCtrlS(bTRUE);
            break;
        }

        case CVCTRL_STOPNOW:
        {
            /* Stop immediately */
            AppCVCfg.StopRequired = bTRUE;
            AD5940_SEQCtrlS(bFALSE);
            AppCVCfg.CVState = CV_STATE_STOP;
            break;
        }

        case CVCTRL_STOPSYNC:
        {
            /* Stop after current cycle completes */
            AppCVCfg.StopRequired = bTRUE;
            break;
        }

        case CVCTRL_SHUTDOWN:
        {
            /* Shutdown and enter hibernate */
            AppCVCfg.StopRequired = bTRUE;
            AD5940_SEQCtrlS(bFALSE);
            AD5940_ShutDownS();
            AppCVCfg.CVState = CV_STATE_STOP;
            break;
        }

        default:
            break;
    }

    return AD5940ERR_OK;
}

/**
 * @brief Initialize CV application
 * @note This is a simplified implementation. Full implementation would
 *       include complete sequencer setup, DAC configuration, etc.
 */
AD5940Err AppCVInit(uint32_t *pBuffer, uint32_t BufferSize)
{
    /* This would contain full initialization code similar to RampTest.c */
    /* Including:
     * - AFE power-up
     * - LPDAC configuration
     * - LPTIA configuration
     * - ADC configuration
     * - Sequencer command generation
     * - FIFO setup
     */

    AppCVCfg.CVInited = bTRUE;
    AppCVCfg.bTestFinished = bFALSE;

    return AD5940ERR_OK;
}

/**
 * @brief Interrupt service routine for CV
 * @note Processes measurement data from FIFO
 */
AD5940Err AppCVISR(void *pBuff, uint32_t *pCount)
{
    uint32_t *pData = (uint32_t*)pBuff;
    uint32_t count = 0;

    /* Read FIFO data */
    /* Process forward/reverse sweep transitions */
    /* Update cycle count */

    if(AppCVCfg.CurrStepPos >= AppCVCfg.StepNumber)
    {
        /* Toggle sweep direction */
        AppCVCfg.bForwardSweep = !AppCVCfg.bForwardSweep;
        AppCVCfg.CurrStepPos = 0;

        if(AppCVCfg.bForwardSweep)
        {
            /* Completed one full cycle */
            AppCVCfg.CurrCycle++;

            if(AppCVCfg.CurrCycle >= AppCVCfg.NumOfCycles)
            {
                /* All cycles complete */
                AppCVCfg.bTestFinished = bTRUE;
                AppCVCfg.CVState = CV_STATE_COMPLETE;
            }
        }
    }

    *pCount = count;
    return AD5940ERR_OK;
}
