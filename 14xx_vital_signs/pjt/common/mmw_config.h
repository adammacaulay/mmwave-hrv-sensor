/**
 *   @file  mmw_config.h
 *
 *   @brief
 *      This is the header file that describes configurations for the Millimeter
 *   Wave Demo.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2018 Texas Instruments, Inc.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef MMW_CONFIG_H
#define MMW_CONFIG_H

/* MMWAVE library Include Files */
#include <ti/control/mmwave/mmwave.h>
#include <ti/common/sys_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum number of 1D FFT bins in DC range antenna signature compensation */
#define DC_RANGE_SIGNATURE_COMP_MAX_BIN_SIZE 32

/**
 * @brief
 *  Millimeter Wave Demo CFAR Configuration
 *
 * @details
 *  The structure contains the cfar configuration used in data path
 */
typedef struct MmwDemo_CfarCfg_t
{
    /*! @brief    CFAR threshold scale */
    uint16_t       thresholdScale;

    /*! @brief    CFAR averagining mode 0-CFAR_CA, 1-CFAR_CAGO, 2-CFAR_CASO */
    uint8_t        averageMode;

    /*! @brief    CFAR noise avraging window length */
    uint8_t        winLen;

    /*! @brief    CFAR guard length*/
    uint8_t        guardLen;

    /*! @brief    CFAR cumulative noise sum divisor */
    uint8_t        noiseDivShift;

    /*! @brief    CFAR 0-cyclic mode disabled, 1-cyclic mode enabled */
    uint8_t        cyclicMode;

} MmwDemo_CfarCfg;

/**
 * @brief
 *  Clutter removal configuration
 *
 * @details
 *  The structure contains clutter removal configuration
 */
typedef struct MmwDemo_ClutterRemovalCfg_t
{
    /*! @brief    enabled flag:  1-enabled 0-disabled */
    uint8_t     enabled;

} MmwDemo_ClutterRemovalCfg;

/**
 * @brief
 *  Millimeter Wave Demo DC range signature compensation
 *
 * @details
 *  The structure contains the DC range antenna signature removeal configuration used in data path
 */
typedef struct MmwDemo_CalibDcRangeSigCfg_t
{
    /*! @brief    enabled flag:  1-enabled 0-disabled */
    uint16_t    enabled;

    /*! @brief    maximum negative range bin (1D FFT index) to be compensated */
    int16_t     negativeBinIdx;

    /*! @brief    maximum positive range bin (1D FFT index) to be compensated */
    int16_t     positiveBinIdx;

    /*! @brief    number of chirps in the averaging phase */
    uint16_t    numAvgChirps;
} MmwDemo_CalibDcRangeSigCfg;

/**
 * @brief
 *  ADCBUF configuration
 *
 * @details
 *  The structure is used to hold all the relevant configuration
 *  which is used to configure ADCBUF.
 */
typedef struct MmwDemo_ADCBufCfg_t
{
    /*! ADCBUF out format:
        0-Complex,
        1-Real */
    uint8_t     adcFmt;

    /*! ADCBUF IQ swap selection:
        0-I in LSB, Q in MSB,
        1-Q in LSB, I in MSB */
    uint8_t     iqSwapSel;

    /*! ADCBUF channel interleave configuration:
        0-interleaved(not supported on XWR16xx),
        1- non-interleaved */
    uint8_t     chInterleave;

    /**
     * @brief   Chirp Threshold configuration used for ADCBUF buffer
     */
    uint8_t     chirpThreshold;
}MmwDemo_ADCBufCfg;


/**
 * @brief
 *  Vital Signs Measurement Demo Gui Monitor Selection
 *
 * @details
 *  The structure contains the flags which select what information is sent to gui
 *
 */
typedef struct VitalSignsDemo_GuiMonSel_t
{


    /*! @brief   Flag that can be set from the GUI */
    uint8_t      guiFlag_Param1;

    /*! @brief   Flag that can be set from the GUI   */
    uint8_t      guiFlag_Param2;

    /*! @brief   Flag that can be set from the GUI   */
    uint8_t      guiFlag_ClutterRemoval;

    /*! @brief   Flag Set when the Refresh buttom is pressed on the GUI */
    uint8_t      guiFlag_Reset;

    uint8_t      statsInfo;
} VitalSignsDemo_GuiMonSel;


typedef struct VitalSignsDemo_MotionDetection_t
{
    /*! @brief   Flag that can be set from the GUI */
    uint16_t      enabled;

    /*! @brief   Flag that can be set from the GUI   */
    uint16_t      blockSize;

    /*! @brief   Flag indicating the spectral method to be used for vital Signs Measurement */
    float         threshold;

    uint16_t      gainControl;

} VitalSignsDemo_MotionDetection;


/**
 * @brief
 *  Vital Signs Demo Parameters Configuration
 *
 * @details
 *  The structure contains the configurations specific to the vital signs Measurement algorithm
 *
 */
typedef struct VitalSignsDemo_ParamsCfg_t
{
    /*! @brief    End range in meters */
    float         startRange_m;

    /*! @brief    Start Range in meters */
    float         endRange_m;

    /*! @brief    window Length for Breathing */
    uint16_t      winLen_breathing;

    /*! @brief    window Length for Heart Rate Estimate*/
    uint16_t      winLen_heartRate;

    /*! @brief    Threshold for Gain Control*/
    float         rxAntennaProcess;

    /*! @brief    Alpha factor for exponential smoothing  of the vital signs waveforms
     *            Used in making a decision if a person is breathing or not */
    float         alpha_breathingWfm;
    float         alpha_heartWfm;

    /*! @brief    Scaling factors for the Breathing and Heart rate waveforms before the HWA - FFT */
    float         scale_breathingWfm;
    float         scale_heartWfm;
} VitalSignsDemo_ParamsCfg;

/**
 * @brief
 *  Vital Signs Monitoring Demo CLI related configuration
 */
typedef struct MmwDemo_CliCfg_t_
{
    /*! @brief   ADCBUF Configuration. */
    MmwDemo_ADCBufCfg   adcBufCfg;

#ifdef SOC_XWR14XX
    MmwDemo_CfarCfg     cfarCfg;
#else
    /*! @brief   CFAR configuration for range */
    MmwDemo_CfarCfg     cfarCfgRange;
    
    /*! @brief   CFAR configuration for doppler */
    MmwDemo_CfarCfg     cfarCfgDoppler;

#endif

    /*! @brief   Calibrate DC (zero) range signature */
    MmwDemo_CalibDcRangeSigCfg calibDcRangeSigCfg;

    /*! @brief   Clutter removal configuration */
    MmwDemo_ClutterRemovalCfg clutterRemovalCfg;

    VitalSignsDemo_ParamsCfg  vitalSignsParamsCfg;

    VitalSignsDemo_GuiMonSel  vitalSigns_GuiMonSel;

    VitalSignsDemo_MotionDetection   motionDetectionParamsCfg;

} MmwDemo_CliCfg_t;

/**
 * @brief Range Bias and rx channel gain/phase compensation configuration.
 *
 */
typedef struct MmwDemo_compRxChannelBiasCfg_t_
{

    /*! @brief  Compensation for range estimation bias */
    float rangeBias;

    /*! @brief  Compensation for Rx channel phase bias in Q15 format */
    cmplx16ImRe_t rxChPhaseComp[SYS_COMMON_NUM_TX_ANTENNAS*SYS_COMMON_NUM_RX_CHANNEL];

} MmwDemo_compRxChannelBiasCfg_t;

/**
 * @brief Range Bias and rx channel gain/phase measurement configuration.
 *
 */
typedef struct MmwDemo_measureRxChannelBiasCfg_t_
{

    /*! @brief  1-enabled 0-disabled */
    uint8_t enabled;

    /*! @brief  Target distance during measurement (in meters) */
    float targetDistance;

    /*! @brief  Search window size (in meters), the search is done in range
     *          [-searchWinSize/2 + targetDistance, targetDistance + searchWinSize/2] */
    float searchWinSize;

} MmwDemo_measureRxChannelBiasCfg_t;


/**
 * @brief
 *  Millimeter Wave Demo CLI related configuration common across all subframes
 */
typedef struct MmwDemo_CliCommonCfg_t_
{
    /*! @brief   Configuration for compensation for range bias
                 and Rx channel phase offset */
    MmwDemo_compRxChannelBiasCfg_t compRxChanCfg;

    /*! @brief   Configuration for measurement of range bias
                 and Rx channel phase offset */
    MmwDemo_measureRxChannelBiasCfg_t measureRxChanCfg;
} MmwDemo_CliCommonCfg_t;

/**
 * @brief
 *  Millimeter Wave Demo configuration
 *
 * @details
 *  The structure is used to hold all the relevant configuration
 *  which is used to execute the Millimeter Wave Demo.
 */
typedef struct MmwDemo_Cfg_t
{
    /*! @brief   CPU Clock Frequency. */
    uint32_t            sysClockFrequency;

    /*! @brief   UART Logging Baud Rate. */
    uint32_t            loggingBaudRate;

    /*! @brief   UART Command Baud Rate. */
    uint32_t            commandBaudRate;

    /*! @brief   mmWave Control Configuration. */
    MMWave_CtrlCfg      ctrlCfg;

    /*! @brief   mmWave Open Configuration. */
    MMWave_OpenCfg      openCfg;

    /*! @brief   Datapath output loggerSetting
                 0 (default): MSS UART logger
                 1: DSS UART logger
     */
    uint8_t              dataLogger;
} MmwDemo_Cfg;
#ifdef __cplusplus
}
#endif

#endif /* MMW_CONFIG_H */
