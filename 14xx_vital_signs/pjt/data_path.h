/**
 *   @file  data_path.h
 *
 *   @brief
 *      This is the data path processing header.
 *
 *  \par

 *  NOTE:
 * Copyright (c) 2018 Texas Instruments Incorporated
 *
 * All rights reserved not granted herein.
 * Limited License.
 *
 * Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive license
 * under copyrights and patents it now or hereafter owns or controls to make, have made, use,
 * import, offer to sell and sell ("Utilize") this software subject to the terms herein.
 *
 * With respect to the foregoing patent license, such license is granted  solely to the extent
 * that any such patent is necessary to Utilize the software alone.  The patent license shall
 * not apply to any combinations which include this software, other than combinations with
 * devices manufactured by or for TI (“TI Devices”). No hardware patent is licensed hereunder.
 *
 * Redistributions must preserve existing copyright notices and reproduce this license
 * (including the above copyright notice and the disclaimer and (if applicable) source code
 * license limitations below) in the documentation and/or other materials provided with the
 * distribution.
 *
 * Redistribution and use in binary form, without modification, are permitted provided that
 * the following conditions are met:
 *
 * No reverse engineering, decompilation, or disassembly of this software is permitted with
 * respect to any software provided in binary form. Any redistribution and use are licensed
 * by TI for use only with TI Devices. Nothing shall obligate TI to provide you with source
 * code for the software licensed and provided to you in object code.
 *
 * If software source code is provided to you, modification and redistribution of the source
 * code are permitted provided that the following conditions are met:
 *
 * Any redistribution and use of the source code, including any resulting derivative works,
 * are licensed by TI for use only with TI Devices.
 * Any redistribution and use of any object code compiled from the source code and any
 * resulting derivative works, are licensed by TI for use only with TI Devices.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be
 * used to endorse or promote products derived from this software without specific prior
 * written permission.
 *
 * DISCLAIMER.
 *
 * THIS SOFTWARE IS PROVIDED BY TI AND TI’S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TI AND TI’S
 * LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef DATA_PATH_H
#define DATA_PATH_H

#include <ti/sysbios/knl/Semaphore.h>

#include <ti/common/sys_common.h>
#include <ti/common/mmwave_error.h>
#include <ti/drivers/adcbuf/ADCBuf.h>
#include <ti/drivers/edma/edma.h>
#include <ti/drivers/hwa/hwa.h>

#include "common/mmw_config.h"
#include "common/mmw_output.h"


#ifdef __cplusplus
extern "C" {
#endif


#define SPEED_OF_LIGHT_IN_METERS_PER_SEC (3.0e8)     // Speed of light
#define PI    3.14159265359

/*! @brief Human Vital Signs Parameters */
#define PHASE_FFT_SIZE                (1024)               // FFT size for each of the Breathing and Cardiac waveform
#define BREATHING_SPECTRUM_SIZE       (PHASE_FFT_SIZE/2)   // Spectrum size after the FFT of the Breathing waveform  (Only keeping the positive half of the spectrum)
#define HEARTRATE_SPECTRUM_SIZE       (PHASE_FFT_SIZE/2)   // Spectrum size after the FFT of the Heart Rate waveform (Only keeping the positive half of the spectrum)
#define MEDIAN_WINDOW_LENGTH          (20)                 // Median window length (for the heart-rate picks up the median of the last MEDIAN_WINDOW_LENGTH measurements)
#define MAX_PEAKS_ALLOWED_WFM         (128)                // Maximum number of peaks allowed in the breathing and cardiac waveforms
#define WAVELENGTH_MM                 (3.9)                // Wavelength in millimeter
#define CONVERT_HZ_BPM                (60.0)               // Converts Hz to Beats per minute
#define RESET_LOCAL_COUNT_VAL         (128)                // Resets the local frame count after RESET_LOCAL_COUNT_VAL frames
#define MAX_NUM_PEAKS_SPECTRUM        (4)                  // Maximum number of peaks selected in the Vital Signs Spectrum
#define MAX_ALLOWED_PEAKS_SPECTRUM    (128)                // Maximum allowed peaks in the Vital Signs Spectrum
#define MAX_HEART_RATE_BPM            (120)                // Maximum Heart-rate allowed
#define XCORR_NUM_LAGS                (200)                // Number of Lags for which the auto-correlation is computed
#define BREATHING_HARMONIC_NUM        (2)                  // Breathing Harmonic
#define HEART_HAMRONIC_THRESH_BPM     (4.0)                // Threshold for determining if a peak is a harmonic
#define BREATHING_HAMRONIC_THRESH_BPM (4.0)                // Threshold for determining if a peak is a harmonic

#define CONF_METRIC_BANDWIDTH_PEAK_HEART_HZ    0.1   // Bandwidth around the max peak to include in the signal power estimation
#define CONF_METRIC_BANDWIDTH_PEAK_BREATH_HZ   0.2   // Bandwidth around the max peak to include in the signal power estimation


/* 1D */
#define VITALSIGNS_HWA_1D_ADCBUF_INP              (&gMmwHwaMemBuf[0])

/* 1D -ping */
#define VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PING  0
#define VITALSIGNS_HWA_DMA_DEST_CHANNEL_1D_PING    0
#define VITALSIGNS_HWA_1D_OUT_PING                (&gMmwHwaMemBuf[2])
#define VITALSIGNS_EDMA_1D_PING_CH_ID              EDMA_TPCC0_REQ_HWACC_0
#define VITALSIGNS_EDMA_1D_PING_SHADOW_LINK_CH_ID  EDMA_SHADOW_LNK_PARAM_BASE_ID
#define VITALSIGNS_EDMA_1D_PING_CHAIN_CH_ID        EDMA_TPCC0_REQ_FREE_0
#define VITALSIGNS_EDMA_1D_PING_ONE_HOT_SHADOW_LINK_CH_ID (EDMA_SHADOW_LNK_PARAM_BASE_ID + 2)

/* 1D - pong */
#define VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PONG   1
#define VITALSIGNS_HWA_DMA_DEST_CHANNEL_1D_PONG     1
#define VITALSIGNS_HWA_1D_OUT_PONG                 (&gMmwHwaMemBuf[3])
#define VITALSIGNS_EDMA_1D_PONG_CH_ID               EDMA_TPCC0_REQ_HWACC_1
#define VITALSIGNS_EDMA_1D_PONG_SHADOW_LINK_CH_ID  (EDMA_SHADOW_LNK_PARAM_BASE_ID + 1)
#define VITALSIGNS_EDMA_1D_PONG_CHAIN_CH_ID         EDMA_TPCC0_REQ_FREE_1
#define VITALSIGNS_EDMA_1D_PONG_ONE_HOT_SHADOW_LINK_CH_ID (EDMA_SHADOW_LNK_PARAM_BASE_ID + 3)

#define VITALSIGNSDEMO_EDMA_TRANSFER_COMPLETION_CODE_1D_DONE   VITALSIGNS_EDMA_1D_PONG_CHAIN_CH_ID

/* Phase Processing */
#define VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_PHASE      4
#define VITALSIGNS_HWA_DMA_DEST_CHANNEL_PHASE        4
#define VITALSIGNS_HWA_PHASE_INP                    (&gMmwHwaMemBuf[2])
#define VITALSIGNS_HWA_PHASE_OUT                    (&gMmwHwaMemBuf[1])
#define VITALSIGNS_EDMA_PHASE_INP_CH_ID              EDMA_TPCC0_REQ_FREE_8
#define VITALSIGNS_EDMA_PHASE_INP_SHADOW_LINK_CH_ID1 (EDMA_SHADOW_LNK_PARAM_BASE_ID + 12)
#define VITALSIGNS_EDMA_PHASE_INP_CHAIN_CH_ID        EDMA_TPCC0_REQ_FREE_9
#define VITALSIGNS_EDMA_PHASE_INP_SHADOW_LINK_CH_ID2 (EDMA_SHADOW_LNK_PARAM_BASE_ID + 13)
#define VITALSIGNS_EDMA_PHASE_OUT_CH_ID              EDMA_TPCC0_REQ_HWACC_4
#define VITALSIGNS_EDMA_PHASE_OUT_SHADOW_LINK_CH_ID1 (EDMA_SHADOW_LNK_PARAM_BASE_ID + 14)
#define VITALSIGNS_EDMA_PHASE_OUT_CHAIN_CH_ID        EDMA_TPCC0_REQ_HWACC_4

/* EDMA resource partitioning */
#define EDMA_SHADOW_LNK_PARAM_BASE_ID             EDMA_NUM_DMA_CHANNELS

#define VITALSIGNSDEMO_EDMA_TRANSFER_COMPLETION_CODE_PHASE_DONE  VITALSIGNS_EDMA_PHASE_OUT_CHAIN_CH_ID

#define VITALSIGNS_HWA_START_POS_PARAMSETS_1D       0
#define VITALSIGNS_HWA_START_POS_PARAMSETS_2D      (VITALSIGNS_HWA_START_POS_PARAMSETS_1D + HWAUTIL_NUM_PARAM_SETS_1D)
#define VITALSIGNS_HWA_START_POS_PARAMSETS_PHASE   (VITALSIGNS_HWA_START_POS_PARAMSETS_2D + HWAUTIL_NUM_PARAM_SETS_PHASE)

#define VITALSIGNS_HWA_WINDOWRAM_1D_OFFSET         0 //In samples

/* FFT Window */
/*! Hanning window */
#define VITALSIGNS_WIN_HANNING  0
/*! Blackman window */
#define VITALSIGNS_WIN_BLACKMAN 1
/*! Rectangular window */
#define VITALSIGNS_WIN_RECT     2

#define ONE_Q17 (1U << 17)

/* Number of Waveforms (Breathing + Heart Rate) processed by the HWACC per iteration (during FFT processing of the waveforms) */
#define VITALSIGNS_NUM_RANGE_BINS_PER_TRANSFER  2

/*! @brief Version of the Vital Signs Measurement Demo. */
#define VITALSIGNS_VERSION        "0.0.0.1"

/*! @brief Number of IIR Filter stages for the Breathing bandpass filter */
#define IIR_FILTER_BREATH_NUM_STAGES        2

/*! @brief Number of IIR Filter stages for the Heart bandpass filter */
#define IIR_FILTER_HEART_NUM_STAGES         4

/*! @brief Second-order IIR-Filter contains 6 coefficeints per stage */
#define IIR_FILTER_COEFS_SECOND_ORDER       6

/*! @brief  Size should at least equal to  "Coefficeints per stage * Number of stages + 2"  */
#define BREATHING_WFM_IIR_FILTER_TAPS_LENGTH   (IIR_FILTER_COEFS_SECOND_ORDER * IIR_FILTER_BREATH_NUM_STAGES) + 2

/*! @brief  Size should be equal to Coefficeints per stage * Number of stages + 2 */
#define HEART_WFM_IIR_FILTER_TAPS_LENGTH       (IIR_FILTER_COEFS_SECOND_ORDER * IIR_FILTER_HEART_NUM_STAGES) + 2

/*! @brief  Filter size for the Moving average filter  */
#define FIR_FILTER_SIZE                    10

/*! @brief  Window Size  */
#define DOPPLER_WINDOW_SIZE                16



/*! simple memory pool structure for flexible partitioning of L3 RAM */
typedef struct {
    /*! base address of memory */
    uint8_t *base;

    /*! size of memory in bytes */
    uint32_t size;

    /*! index into the pool to track where is next free */
    uint32_t indx;
} VitalSignsDemoMemPool_t;

/*! Partition HWA memory into 4 equal parts - M0,M1,M2,M3. */
#define VITALSIGNS_HWA_NUM_MEM_BUFS 4
typedef struct {
    uint8_t buf[SOC_XWR14XX_MSS_HWA_MEM_SIZE / VITALSIGNS_HWA_NUM_MEM_BUFS];
} vitalSignsHwaBuf_t;


/*!
 *  @brief Timing information
 */
typedef struct MmwDemo_timingInfo
{
    /*! @brief number of processor cycles between frames excluding
           processing time to transmit output on UART */
    uint32_t interFrameProcCycles;

     /*! @brief number of processor cycles to transmit detected object information
           on the output UART port, which is not presently back-grounded. */
    uint32_t transmitOutputCycles;

    /*! @brief Inter frame processing end time */
    uint32_t interFrameProcessingEndTime;

    /*! @brief Inter frame processing end margin in number of cycles before
     * due time to start processing first chirp of the next frame */
    uint32_t interFrameProcessingEndMargin;

    /*! @brief CPU Load during active frame period - i.e. chirping */
    uint32_t activeFrameCPULoad;

    /*! @brief CPU Load during inter frame period - i.e. after chirps
     *  are done and before next frame starts */
    uint32_t interFrameCPULoad;

} MmwDemo_timingInfo_t;

/**
 * @brief
 *  Vital Signs Measurement Demo configuration

 * @details
 *  The structure is used to hold all the relevant configuration
 *  which is used to execute the Vital Signs Measurement Demo.
 */
typedef struct VitalSignsDemo_Cfg_t
{
    /*! @brief   CPU Clock Frequency. */
    uint32_t        sysClockFrequency;

    /*! @brief   UART Logging Baud Rate. */
    uint32_t        loggingBaudRate;

    /*! @brief   UART Command Baud Rate. */
    uint32_t        commandBaudRate;

    /*! @brief   mmWave Control Configuration. */
    MMWave_CtrlCfg      ctrlCfg;

    /*! @brief   mmWave Open Configuration. */
    MMWave_OpenCfg      openCfg;

    /*! @brief   Gui Monitor Selection */
    VitalSignsDemo_GuiMonSel guiMonSel;

    VitalSignsDemo_ParamsCfg  vitalSignsParamsCfg;

} VitalSignsDemo_Cfg;

/**
 * @brief
 *  Vital Signs Measurement Demo Data Path Information.
 *
 * @details
 *  The structure is used to hold all the relevant information for
 *  the data path.
 */
typedef struct VitalSignsDemo_DataPathObj_t
{

    /*! @brief Pointer to cli config */
     MmwDemo_CliCfg_t *cliCfg;

     /*! @brief Pointer to cli config common to all subframes*/
     MmwDemo_CliCommonCfg_t *cliCommonCfg;

     /*! @brief   Element size (in number of bytes) used to configure DMA channel */
     uint16_t dmaElemSize;

     /*! @brief   Frame count used to configure DMA channel */
     uint8_t dmaFrameCnt;

     /*! @brief   Chirp Threshold configuration used for ADCBUF driver */
     uint8_t chirpThreshold;

     /*! @brief   Chirp Available Semaphore: */
     Semaphore_Handle  chirpSemHandle;

     /*! @brief   Counter which tracks the number of chirps detected */
     uint32_t chirpInterruptCounter;

     /*! @brief   ADCBUF handle. */
     ADCBuf_Handle adcbufHandle;

     /*! @brief   Handle of the EDMA driver. */
     EDMA_Handle edmaHandle;

     /*! @brief   EDMA error Information when there are errors like missing events */
     EDMA_errorInfo_t  EDMA_errorInfo;

     /*! @brief EDMA transfer controller error information. */
     EDMA_transferControllerErrorInfo_t EDMA_transferControllerErrorInfo;

     /*! @brief Semaphore handle for 1D EDMA completion. */
     Semaphore_Handle EDMA_1Ddone_semHandle;

     /*! @brief Semaphore handle for 2D EDMA completion. */
     Semaphore_Handle EDMA_2Ddone_semHandle;

     /*! @brief Semaphore handle for FFT of the Phase completion. */
     Semaphore_Handle EDMA_FFTphaseDone_semHandle;

     /*! @brief Handle to hardware accelerator driver. */
     HWA_Handle  hwaHandle;

     /*! @brief Semaphore handle for Hardware accelerator completion. */
     Semaphore_Handle HWA_done_semHandle;

     /*! @brief Hardware accelerator Completion Isr count for debug purposes. */
     uint32_t hwaDoneIsrCounter;

     /*! @brief valid Profile index */
     uint32_t validProfileIdx;

     /*! @brief Timing information */
     MmwDemo_timingInfo_t timingInfo;

     /*! @brief Rx channel gain/phase offset compensation coefficients */
     MmwDemo_compRxChannelBiasCfg_t compRxChanCfg;

     /*! @brief Doppler Window Coefficients */
     float pDopplerWindow[DOPPLER_WINDOW_SIZE];

     /*! @brief Frame counter incremented in frame start interrupt handler*/
     uint32_t frameStartIntCounter;

     /*! @brief Semaphore handle for Frame start indication. */
     Semaphore_Handle frameStart_semHandle;

     /*! @brief  Used for checking that inter frame processing finshed on time */
     int32_t interFrameProcToken;

     /*! @brief number of transmit antennas */
     uint32_t numTxAntennas;

     /*! @brief number of Recieve antennas */
     uint32_t numRxAntennas;

     /*! @brief number of virtual antennas */
     uint32_t numVirtualAntennas;

     /*! @brief number of virtual azimuth antennas */
     uint32_t numVirtualAntAzim;

     /*! @brief number of virtual elevation antennas */
     uint32_t numVirtualAntElev;

     /*! @brief number of ADC samples */
     uint32_t numAdcSamples;

     /*! @brief number of range bins */
     uint32_t numRangeBins;

     /*! @brief number of chirps per frame */
     uint32_t numChirpsPerFrame;

     /*! @brief number of doppler bins */
     uint32_t numDopplerBins;

     /*! @brief Frame Repetition time. Determines the frame rate */
     uint32_t framePeriodicity;

     /*! @brief range resolution in meters */
     float rangeResolution;

     /*! @brief number of processor cycles between frames excluding
            processing time to transmit output on UART */
     uint32_t interFrameProcCycles;

      /*! @brief number of processor cycles to transmit detected object information
            on the output UART port, which is not presently back-grounded. */
     uint32_t transmitOutputProcCycles;


     /* Vital Signs Parameters */

     uint32_t *radarCube;              // Contains the Range-FFT output
     uint16_t *pRangeProfileCplx;      // The Complex Range Profile extracted from the Radar Cube
     uint32_t  maxIndexRangeBin;       // Index of the range bin with the Max Value
     uint16_t rangeBinStartIndex;      // Start Index based on the range specified by the user through the configuration file
     uint16_t rangeBinEndIndex;        // End Index based on the range specified by the user through the configuration file
     uint16_t numRangeBinProcessed;    // Number of Range-bins Processed (determined from the StartRange and EndRange in the configuration file)
     uint16_t  rxAntennaProcess;       // Data from this reciever antenna is used for vital signs processing
     uint32_t framePeriodicity_ms;     // Frame Periodicity in ms
     float maxValueRangeBin;           // Magnitude of the range bin with the Max Value
     float rangeBinSize_meter;         // Size of the Range-bin in Meters
     float chirpDuration_us;           // Chirp Duration in microseconds
     float chirpBandwidth_kHz;         // Chirp Bandwidth in MHz
     float rangeMaximum;               // Maximum Unambigous Range
     float scaleFactor_PhaseToDisp;    // Scaling factor to convert phase values to displacement (mm)

     // Motion Detection
     float *pMotionCircularBuffer;    // Circular Buffer for storing the data segment that is checked for motion corruption
     float motionDetected;            // Flag to indicate that the data segment is corrupted by Motion
     float motionDetection_Thresh;    // Energy Threshold over which a segment is classified as motion corrupted
     float noiseImpulse_Thresh;       // Threshold for impule noise removal
     uint16_t motionDetection_BlockSize;    // Size of the data segment checked for motion corruption

     // Clutter Removal Params
     float    *pRangeProfileClutterRemoved;  // Contains the Clutter removed range profile
     float    *pTempReal_Prev;               // Temporary pointer
     float    *pTempImag_Prev;               // Temporary pointer

     // Auto-Correlation
     float *pXcorr;                   // Auto-correlation Output
     uint16_t xCorr_minLag;           // Minimum Lag
     uint16_t xCorr_maxLag;           // Maximum Lag
     uint16_t xCorr_Breath_minLag;
     uint16_t xCorr_Breath_maxLag;

     // IIR-Filtering
     float pFilterCoefs[FIR_FILTER_SIZE];
     float pDelayHeart[HEART_WFM_IIR_FILTER_TAPS_LENGTH];          // IIR-Filter delay lines
     float pDelayBreath[BREATHING_WFM_IIR_FILTER_TAPS_LENGTH];     // IIR-Filter delay lines
     float pFilterCoefsBreath[IIR_FILTER_BREATH_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER];
     float pFilterCoefsHeart[IIR_FILTER_HEART_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER];
     float pFilterCoefsHeart_4Hz[IIR_FILTER_HEART_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER];
     float pScaleValsHeart[IIR_FILTER_HEART_NUM_STAGES + 1];
     float pScaleValsHeart_4Hz[IIR_FILTER_HEART_NUM_STAGES + 1];
     float pScaleValsBreath[IIR_FILTER_BREATH_NUM_STAGES + 1];

     // Time-domain Waveform Buffers and sizes
     int32_t  *pVitalSignsCircularBuff; // Circular Buffer for Chest Displacement Waveforms
     float    *pTempCircularBuff;       // Temporary Circular Buffer for Pre-processing the Cardiac Waveform
     float    *pVitalSigns_Breath_CircularBuffer;   // Circular Buffer for Breathing Waveform
     uint16_t circularBufferSizeBreath, circularBufferSizeHeart;
     float scale_breathingWfm;   // Scale factor for Breathing Wfm
     float scale_heartWfm;       // Scale factor for Heart Wfm

     // Frequency-domain Waveform Buffers and params
     uint32_t *pBreathingSpectrum;      // FFT spectrum of the Breathing waveform Spectrum
     uint32_t *pHeartbeatSpectrum;      // FFT spectrum of the HeartBeat waveform Spectrum
     uint32_t *pVitalSignsSpectrum;     // FFT spectrum obtained from the HWA
     float breath_startFreq_Hz;       // Lower cuttoff Frequency for the Breathing band-pass filter
     float breath_endFreq_Hz;         // Higher cuttoff Frequency for the Breathing band-pass filter
     float heart_startFreq_Hz;        // Lower cuttoff Frequency  for the Heart beat band-pass filter
     float heart_endFreq_Hz;          // Higher cuttoff Frequency  for the Breathing band-pass filter
     float samplingFreq_Hz;           // Slow-Axis sampling rate. Will be equivalent to the Frame rate
     float freqIncrement_Hz;  // Frequency Increment after the FFT of the Vital Signs Waveforms

     uint16_t breathingWfm_Spectrum_FftSize;     // FFT size for Breathing Waveform
     uint16_t heartWfm_Spectrum_FftSize;         // FFT size for Cardiac Waveform
     uint16_t heart_startFreq_Index_1p6Hz;
     uint16_t heart_endFreq_Index_4Hz;

     // Confidence Metric
     uint16_t confMetric_spectrumHeart_IndexStart;
     uint16_t confMetric_spectrumHeart_IndexEnd;
     uint16_t confMetric_spectrumBreath_IndexStart;
     uint16_t confMetric_spectrumBreath_IndexEnd;
     uint16_t confMetric_spectrumHeart_IndexStart_1p6Hz;
     uint16_t confMetric_spectrumHeart_IndexStart_4Hz;
     uint16_t confMetric_numIndexAroundPeak_heart;
     uint16_t confMetric_numIndexAroundPeak_breath;
     uint16_t heart_startFreq_Index, heart_endFreq_Index;   // Index corresponding to the lower and higher cutoff frequencies for the band-pass filter
     uint16_t breath_startFreq_Index, breath_endFreq_Index; //Index corresponding to the lower and higher cutoff frequencies for the band-pass filter

     uint16_t pPeakIndex[MAX_ALLOWED_PEAKS_SPECTRUM];        // Indices of the Peaks in the Cardiac/Breathing spectrum
     uint16_t pPeakIndexSorted[MAX_ALLOWED_PEAKS_SPECTRUM];  // Sorted Indices of the Peaks in the Cardiac/Breathing spectrum
     float pPeakValues[MAX_ALLOWED_PEAKS_SPECTRUM];          // Values of the Peaks in the Cardiac/Breathing spectrum
     uint16_t pPeakSortTempIndex[MEDIAN_WINDOW_LENGTH];      // For Median Sorting

     uint16_t peakDistanceBreath_Min;
     uint16_t peakDistanceBreath_Max;
     uint16_t peakDistanceHeart_Min;
     uint16_t peakDistanceHeart_Max;

     float   alpha_breathing;    // Alpha factor for exponential smoothing of the breathing Wfm
     float   alpha_heart;        // Alpha factor for exponential smoothing of the heart Wfm

     float pBufferHeartRate[MEDIAN_WINDOW_LENGTH];       // Maintains a history of the Previous "MEDIAN_WINDOW_LENGTH" heart rate measurements
     float pBufferBreathingRate[MEDIAN_WINDOW_LENGTH];   // Maintains a history of the Previous "MEDIAN_WINDOW_LENGTH" heart rate measurements
     float pBufferHeartRate_4Hz[MEDIAN_WINDOW_LENGTH];   // Maintains a history of the Previous "MEDIAN_WINDOW_LENGTH" heart rate measurements

     VitalSignsDemo_OutputStats  VitalSigns_Output;     // VitalSigns Output Structure

} VitalSignsDemo_DataPathObj;

void MmwDemo_hwaOpen(VitalSignsDemo_DataPathObj *obj, SOC_Handle socHandle);
void MmwDemo_edmaOpen(VitalSignsDemo_DataPathObj *obj);
void edmaInit(VitalSignsDemo_DataPathObj *obj);

int32_t VitalSignsDemo_config1D_EDMA(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_config1D_HWA(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_dataPathTrigger1D(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_dataPathWait1D(VitalSignsDemo_DataPathObj *obj);

int32_t VitalSignsDemo_configPhase_EDMA(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_configPhase_HWA(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_dataPathTriggerPhase(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_dataPathWaitPhase(VitalSignsDemo_DataPathObj *obj);

void VitalSignsDemo_EDMA_errorCallbackFxn(EDMA_Handle handle, EDMA_errorInfo_t *errorInfo);
void VitalSignsDemo_EDMA_transferControllerErrorCallbackFxn(EDMA_Handle handle,EDMA_transferControllerErrorInfo_t *errorInfo);
void VitalSignsDemo_dataPathHwaDoneIsrCallback(void * arg);
void VitalSignsDemo_EDMA_transferCompletionCallbackFxn(uintptr_t arg, uint8_t transferCompletionCode);



void VitalSignsDemo_memPoolReset(VitalSignsDemoMemPool_t *pool);
uint8_t *VitalSignsDemo_memPoolAlloc(VitalSignsDemoMemPool_t *pool, uint32_t size);
uint32_t pow2roundup(uint32_t x);

#ifdef __cplusplus
}
#endif

#endif /* DATA_PATH_H */

