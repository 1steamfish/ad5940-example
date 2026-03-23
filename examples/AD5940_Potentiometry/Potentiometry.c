/*!
 *****************************************************************************
 @file:    Potentiometry.c
 @author:  Electrochemical Workstation Team
 @brief:   Open Circuit Potentiometry implementation.
 @details  Measures open-circuit potential for K+, Na+ ion detection in sweat.
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

This software is proprietary to Analog Devices, Inc. and its licensors.
By using this software you agree to the terms of the associated
Analog Devices Software License Agreement.

*****************************************************************************/
#include "Potentiometry.h"

/**
 * @brief Default Potentiometry configuration
 * @details Configured for sodium (Na+) detection in sweat
 */
AppPOTCfg_Type AppPOTCfg =
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
    .ADCRefVolt = 1820.0f,
    .bTestFinished = bFALSE,

    /* Potentiometry parameters - for Na+ ISE */
    .SampleRate = 2.0f,             /* 2 Hz sampling */
    .NumSamples = 100,              /* 100 samples (50 seconds) */
    .AveragingCount = 10,           /* Average 10 readings */
    .IonType = ION_TYPE_NA_PLUS,

    /* Calibration - Nernst equation parameters */
    .CalibSlope = 59.16f,           /* mV/decade at 25°C */
    .CalibOffset = 0.0f,            /* To be calibrated */
    .Temperature = 25.0f,           /* 25°C default */

    /* ADC configuration - high resolution for mV accuracy */
    .AdcPgaGain = ADCPGA_1,         /* No gain for full range */
    .ADCSinc3Osr = ADCSINC3OSR_4,
    .ADCSinc2Osr = ADCSINC2OSR_1333,  /* Maximum filtering */
    .DataFifoSrc = DATATYPE_SINC2,

    /* High impedance mode for ISE */
    .HighImpedanceMode = bTRUE,     /* >1GΩ input impedance */

    .FifoThresh = 4,

    /* Private */
    .POTInited = bFALSE,
    .StopRequired = bFALSE,
    .POTState = POT_STATE_IDLE,
    .SampleCount = 0,
    .AccumVoltage = 0.0f,
};

AD5940Err AppPOTGetCfg(void *pCfg)
{
    if(pCfg){
        *(AppPOTCfg_Type**)pCfg = &AppPOTCfg;
        return AD5940ERR_OK;
    }
    return AD5940ERR_PARA;
}

/**
 * @brief Calculate voltage from ADC code
 * @param ADCcode: Raw ADC value
 * @return Voltage in mV
 */
float AppPOTCalcVoltage(uint32_t ADCcode)
{
    float voltage;
    uint32_t tmp;
    float kFactor = 1.0;

    /* PGA gain factor */
    switch(AppPOTCfg.AdcPgaGain)
    {
        case ADCPGA_1:      kFactor = 1.0; break;
        case ADCPGA_1P5:    kFactor = 1.5; break;
        case ADCPGA_2:      kFactor = 2.0; break;
        case ADCPGA_4:      kFactor = 4.0; break;
        case ADCPGA_9:      kFactor = 9.0; break;
        default:            kFactor = 1.0; break;
    }

    tmp = ADCcode & 0xffff;
    if(tmp & (1L << 15))  /* Negative voltage */
    {
        tmp = (tmp ^ 0xffff) + 1;
        voltage = -(float)tmp / 32768.0 * AppPOTCfg.ADCRefVolt / kFactor;
    }
    else
    {
        voltage = (float)tmp / 32768.0 * AppPOTCfg.ADCRefVolt / kFactor;
    }

    return voltage;
}

/**
 * @brief Calculate ion concentration from measured voltage
 * @param voltage_mV: Measured open-circuit potential (mV)
 * @return Concentration in mM (millimolar)
 * @note Uses Nernst equation: E = E0 + (RT/nF) * ln([ion])
 */
float AppPOTCalcConcentration(float voltage_mV)
{
    float concentration;
    float adjusted_voltage;

    /* Apply calibration */
    adjusted_voltage = voltage_mV - AppPOTCfg.CalibOffset;

    /* Nernst equation (simplified for monovalent ions):
     * E = E0 + slope * log10([ion])
     * [ion] = 10^((E - E0) / slope)
     */

    /* Temperature correction for slope (optional) */
    float slope = AppPOTCfg.CalibSlope;
    if(AppPOTCfg.Temperature != 25.0f)
    {
        /* Theoretical: slope = (RT/F) * ln(10) * 1000 */
        /* At 25°C: 59.16 mV/decade */
        slope = slope * (AppPOTCfg.Temperature + 273.15) / 298.15;
    }

    /* Calculate concentration (mM) */
    concentration = pow(10.0, adjusted_voltage / slope);

    return concentration;
}

AD5940Err AppPOTCtrl(uint32_t Command, void *pPara)
{
    switch(Command)
    {
        case POTCTRL_START:
        {
            if(AppPOTCfg.POTInited == bFALSE)
                return AD5940ERR_APPERROR;

            AppPOTCfg.POTState = POT_STATE_STABILIZE;
            AppPOTCfg.SampleCount = 0;
            AppPOTCfg.AccumVoltage = 0.0f;
            AppPOTCfg.StopRequired = bFALSE;

            if(AD5940_WakeUp(10) > 10)
                return AD5940ERR_WAKEUP;

            AD5940_SEQCtrlS(bTRUE);
            break;
        }

        case POTCTRL_STOPNOW:
        {
            AppPOTCfg.StopRequired = bTRUE;
            AD5940_SEQCtrlS(bFALSE);
            AppPOTCfg.POTState = POT_STATE_STOP;
            break;
        }

        case POTCTRL_SHUTDOWN:
        {
            AppPOTCfg.StopRequired = bTRUE;
            AD5940_SEQCtrlS(bFALSE);
            AD5940_ShutDownS();
            AppPOTCfg.POTState = POT_STATE_STOP;
            break;
        }

        case POTCTRL_CALIBRATE:
        {
            /* Perform two-point calibration with standard solutions */
            /* This would be called externally with known concentrations */
            if(pPara)
            {
                float *calib_data = (float*)pPara;
                /* calib_data[0] = voltage1, calib_data[1] = conc1
                 * calib_data[2] = voltage2, calib_data[3] = conc2
                 */
                AppPOTCfg.CalibSlope = (calib_data[2] - calib_data[0]) /
                                       log10(calib_data[3] / calib_data[1]);
                AppPOTCfg.CalibOffset = calib_data[0] -
                                        AppPOTCfg.CalibSlope * log10(calib_data[1]);
            }
            break;
        }

        default:
            break;
    }

    return AD5940ERR_OK;
}

AD5940Err AppPOTInit(uint32_t *pBuffer, uint32_t BufferSize)
{
    /* Full initialization would include:
     * - AFE power configuration
     * - High-impedance input buffer setup
     * - ADC configuration with maximum filtering
     * - No LPTIA needed (direct voltage measurement)
     * - Sequencer for periodic sampling
     */

    AppPOTCfg.POTInited = bTRUE;
    AppPOTCfg.bTestFinished = bFALSE;

    return AD5940ERR_OK;
}

AD5940Err AppPOTISR(void *pBuff, uint32_t *pCount)
{
    uint32_t *pData = (uint32_t*)pBuff;
    uint32_t count = 0;

    /* Read FIFO data
     * Average readings
     * Convert to concentration using Nernst equation
     */

    if(AppPOTCfg.SampleCount >= AppPOTCfg.NumSamples)
    {
        AppPOTCfg.bTestFinished = bTRUE;
        AppPOTCfg.POTState = POT_STATE_COMPLETE;
    }

    *pCount = count;
    return AD5940ERR_OK;
}
