/*!
 *****************************************************************************
 @file:    SweatAnalysis.h
 @author:  Electrochemical Workstation Team
 @brief:   Sweat analysis configuration and utilities
 @details  Unified interface for detecting glucose, lactate, K+, Na+ in sweat
 -----------------------------------------------------------------------------

Copyright (c) 2017-2019 Analog Devices, Inc. All Rights Reserved.

*****************************************************************************/
#ifndef _SWEAT_ANALYSIS_H_
#define _SWEAT_ANALYSIS_H_

#include "ad5940.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Analyte types */
typedef enum {
    ANALYTE_GLUCOSE = 0,    /**< Glucose (enzymatic detection) */
    ANALYTE_LACTATE,        /**< Lactate (enzymatic detection) */
    ANALYTE_K_ION,          /**< Potassium ion (ISE) */
    ANALYTE_NA_ION,         /**< Sodium ion (ISE) */
    ANALYTE_CL_ION,         /**< Chloride ion (ISE) */
    ANALYTE_PH,             /**< pH (ISE) */
    ANALYTE_CUSTOM
} AnalyteType_t;

/* Detection method for each analyte */
typedef enum {
    METHOD_CHRONOAMPEROMETRY = 0,   /**< Fixed potential, measure current */
    METHOD_DPV,                     /**< Differential Pulse Voltammetry */
    METHOD_CV,                      /**< Cyclic Voltammetry */
    METHOD_POTENTIOMETRY,           /**< Open circuit potential (ISE) */
    METHOD_IMPEDANCE                /**< Impedance spectroscopy */
} DetectionMethod_t;

/* Sweat analyte configuration */
typedef struct {
    AnalyteType_t   analyte;        /**< Target analyte */
    DetectionMethod_t method;       /**< Detection method */
    BoolFlag        enabled;        /**< Enable this analyte measurement */

    /* Sensor configuration */
    float           rtia_value;     /**< Transimpedance resistor (Ohm) */
    uint32_t        rtia_sel;       /**< RTIA selection code */
    float           sensor_bias;    /**< Sensor bias voltage (mV) */

    /* Measurement range */
    float           range_min;      /**< Minimum concentration (mM or mV) */
    float           range_max;      /**< Maximum concentration (mM or mV) */

    /* Calibration */
    float           calib_slope;    /**< Calibration slope */
    float           calib_offset;   /**< Calibration offset */
    BoolFlag        calib_valid;    /**< Calibration data valid */

} SweatAnalyteConfig_t;

/* Sweat detection system configuration */
typedef struct {
    /* Multi-analyte configuration */
    SweatAnalyteConfig_t glucose;
    SweatAnalyteConfig_t lactate;
    SweatAnalyteConfig_t k_ion;
    SweatAnalyteConfig_t na_ion;

    /* System parameters */
    float           sample_rate;        /**< Sampling rate (Hz) */
    uint32_t        samples_per_run;    /**< Samples per measurement run */
    float           temperature;        /**< Operating temperature (°C) */

    /* Sequential measurement timing */
    float           meas_interval;      /**< Interval between analytes (ms) */

    /* Data processing */
    BoolFlag        enable_filtering;   /**< Enable moving average filter */
    uint32_t        filter_window;      /**< Filter window size */

} SweatSystemConfig_t;

/* Measurement result */
typedef struct {
    AnalyteType_t   analyte;
    float           concentration;      /**< Concentration (mM for ions, mg/dL for glucose) */
    float           raw_value;          /**< Raw ADC voltage or current */
    uint32_t        timestamp;          /**< Timestamp (ms) */
    BoolFlag        valid;              /**< Result valid flag */
    uint8_t         quality;            /**< Quality metric (0-100) */
} SweatMeasResult_t;

/* Function prototypes */
AD5940Err SweatAnalysis_Init(SweatSystemConfig_t *pConfig);
AD5940Err SweatAnalysis_ConfigureAnalyte(SweatAnalyteConfig_t *pAnalyte);
AD5940Err SweatAnalysis_StartMeasurement(AnalyteType_t analyte);
AD5940Err SweatAnalysis_StopMeasurement(void);
AD5940Err SweatAnalysis_GetResult(SweatMeasResult_t *pResult);
AD5940Err SweatAnalysis_Calibrate(AnalyteType_t analyte, float *standards, uint32_t num_points);
const char* SweatAnalysis_GetAnalyteName(AnalyteType_t analyte);
const char* SweatAnalysis_GetMethodName(DetectionMethod_t method);

/* Typical configurations for common analytes */
extern const SweatAnalyteConfig_t SweatConfig_Glucose_Default;
extern const SweatAnalyteConfig_t SweatConfig_Lactate_Default;
extern const SweatAnalyteConfig_t SweatConfig_KIon_Default;
extern const SweatAnalyteConfig_t SweatConfig_NaIon_Default;

#endif /* _SWEAT_ANALYSIS_H_ */
