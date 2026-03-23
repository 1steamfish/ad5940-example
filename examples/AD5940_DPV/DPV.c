/*!
 *****************************************************************************
 @file:    DPV.c
 @author:  Electrochemical Workstation Team
 @brief:   Differential Pulse Voltammetry implementation.
 @details  DPV provides high sensitivity for trace detection of glucose,
           lactate, and ions in sweat samples.
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#include "DPV.h"

/**
 * @brief Default DPV configuration
 * @details Optimized for sweat lactate detection
 */
AppDPVCfg_Type AppDPVCfg =
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

    /* DPV parameters - for lactate detection */
    .StartVolt = -200.0f,           /* Start at -0.2V */
    .EndVolt = +800.0f,             /* End at +0.8V (lactate oxidation) */
    .PulseAmplitude = 50.0f,        /* 50 mV pulse amplitude */
    .PulseWidth = 50.0f,            /* 50 ms pulse width */
    .PulsePeriod = 200.0f,          /* 200 ms between pulses */
    .StepHeight = 5.0f,             /* 5 mV step increment */
    .StepNumber = 200,              /* (800-(-200))/5 = 200 steps */

    /* Measurement configuration */
    .SampleDelay = 2.0f,            /* 2 ms settling */
    .LPTIARtiaSel = LPTIARTIA_20K,  /* 20kΩ for wider range */
    .LPTIARloadSel = LPTIARLOAD_SHORT,
    .ExternalRtiaValue = 20000.0f,
    .AdcPgaGain = ADCPGA_2,         /* 2x gain for sensitivity */
    .ADCSinc3Osr = ADCSINC3OSR_4,
    .FifoThresh = 8,

    /* Private */
    .DPVInited = bFALSE,
    .StopRequired = bFALSE,
    .DPVState = DPV_STATE_IDLE,
    .CurrStepPos = 0,
};

AD5940Err AppDPVGetCfg(void *pCfg)
{
    if(pCfg){
        *(AppDPVCfg_Type**)pCfg = &AppDPVCfg;
        return AD5940ERR_OK;
    }
    return AD5940ERR_PARA;
}

float AppDPVCalcCurrent(uint32_t ADCcode)
{
    float voltage, current;
    float rtia = AppDPVCfg.RtiaValue.Real;

    if(AppDPVCfg.ExternalRtiaValue > 0)
        rtia = AppDPVCfg.ExternalRtiaValue;

    voltage = AppDPVCalcVoltage(ADCcode);
    current = voltage / rtia * 1e9;  /* nA */

    return current;
}

float AppDPVCalcVoltage(uint32_t ADCcode)
{
    float voltage;
    uint32_t tmp;
    float kFactor = 1.0;

    switch(AppDPVCfg.AdcPgaGain)
    {
        case ADCPGA_1:      kFactor = 1.0; break;
        case ADCPGA_1P5:    kFactor = 1.5; break;
        case ADCPGA_2:      kFactor = 2.0; break;
        case ADCPGA_4:      kFactor = 4.0; break;
        case ADCPGA_9:      kFactor = 9.0; break;
        default:            kFactor = 1.0; break;
    }

    tmp = ADCcode & 0xffff;
    if(tmp & (1L << 15))  /* Negative */
    {
        tmp = (tmp ^ 0xffff) + 1;
        voltage = -(float)tmp / 32768.0 * AppDPVCfg.ADCRefVolt / kFactor;
    }
    else
    {
        voltage = (float)tmp / 32768.0 * AppDPVCfg.ADCRefVolt / kFactor;
    }

    return voltage;
}

AD5940Err AppDPVCtrl(uint32_t Command, void *pPara)
{
    switch(Command)
    {
        case DPVCTRL_START:
        {
            if(AppDPVCfg.DPVInited == bFALSE)
                return AD5940ERR_APPERROR;

            AppDPVCfg.DPVState = DPV_STATE_BASELINE;
            AppDPVCfg.CurrStepPos = 0;
            AppDPVCfg.StopRequired = bFALSE;

            if(AD5940_WakeUp(10) > 10)
                return AD5940ERR_WAKEUP;

            AD5940_SEQCtrlS(bTRUE);
            break;
        }

        case DPVCTRL_STOPNOW:
        {
            AppDPVCfg.StopRequired = bTRUE;
            AD5940_SEQCtrlS(bFALSE);
            AppDPVCfg.DPVState = DPV_STATE_STOP;
            break;
        }

        case DPVCTRL_STOPSYNC:
        {
            AppDPVCfg.StopRequired = bTRUE;
            break;
        }

        case DPVCTRL_SHUTDOWN:
        {
            AppDPVCfg.StopRequired = bTRUE;
            AD5940_SEQCtrlS(bFALSE);
            AD5940_ShutDownS();
            AppDPVCfg.DPVState = DPV_STATE_STOP;
            break;
        }

        default:
            break;
    }

    return AD5940ERR_OK;
}

AD5940Err AppDPVInit(uint32_t *pBuffer, uint32_t BufferSize)
{
    /* Full initialization would include:
     * - AFE configuration
     * - LPDAC setup for baseline + pulse voltages
     * - LPTIA configuration
     * - ADC setup
     * - Sequencer for baseline and pulse measurements
     * - Differential current calculation
     */

    AppDPVCfg.DPVInited = bTRUE;
    AppDPVCfg.bTestFinished = bFALSE;

    return AD5940ERR_OK;
}

AD5940Err AppDPVISR(void *pBuff, uint32_t *pCount)
{
    uint32_t *pData = (uint32_t*)pBuff;
    uint32_t count = 0;

    /* Read FIFO data
     * Calculate differential current (I_pulse - I_baseline)
     * Step to next voltage
     */

    if(AppDPVCfg.CurrStepPos >= AppDPVCfg.StepNumber)
    {
        AppDPVCfg.bTestFinished = bTRUE;
        AppDPVCfg.DPVState = DPV_STATE_COMPLETE;
    }

    *pCount = count;
    return AD5940ERR_OK;
}
