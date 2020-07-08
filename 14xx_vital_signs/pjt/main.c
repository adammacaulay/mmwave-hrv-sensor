/**
  @file  main.c
 *
 *   @brief
 *      This is the main file which implements the Vital Signs Monitoring Demo
 *
 *  \par
 *
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

 /** @mainpage Human Vital Signs Measurement Demo
 *
 *  @section intro_sec Introduction
 * - This lab exercise demonstrates the ability of the millimeter wave sensor to measure small-scale chest displacements
 * due to breathing and heart beat.
 * - The lab GUI will display the chest displacements from a person sitting in front of the sensor and will process this data to estimate the Breathing-rate and Heart-rate
 *
 *  @section tasks Software Tasks
 *    The demo consists of the following four (SYSBIOS) tasks:
 *    - @ref vitalSignsDemo_initTask. This task is created/launched by @ref main and is a
 *      one-time active task that performs the following sequence:
 *      -# Initializes drivers (\<driver\>_init).
 *      -# Initializes the MMWave module (MMWave_init)
 *      -# Creates/launches following three tasks (the @ref CLI_task is launched indirectly by
 *          calling @ref CLI_open).
 *    - @ref CLI_task. This command line interface task provides a simplified 'shell' interface
 *      which allows the configuration of the BSS via the mmWave interface (MMWave_config).
 *      It parses input CLI configuration commands like chirp profile and GUI configuration.
 *      When sensor start CLI command is parsed, it performs the following steps:
 *      -# Issues MMWave_config using the
 *         previously parsed configurations to setup the BSS.
 *      -# Configures the only-once (@ref vitalSignsDemo_dataPathConfigCommon) and
 *         first-time (@ref vitalSignsDemo_config1D_HWA, @ref vitalSignsDemo_dataPathTrigger1D)
 *         data path processing configurations, so that the processing chain is ready
 *         to do the first step of 1D processing related to chirps (see @ref datapath).
 *      -# Issues MMWave_start to command the BSS to start chirping.
 *    - @ref vitalSignsDemo_mmWaveCtrlTask. This task is used to provide an execution
 *      context for the mmWave control, it calls in an endless loop the MMWave_execute API.
 *    - @ref vitalSignsDemo_dataPathTask. The task performs in real-time:
 *      - Data path processing chain control and (re-)configuration
 *        of the hardware entities involved in the processing chain, namely HWA and EDMA.
 *      - Transmits the processed output through the UART output port.
 *        For format of the data on UART output port, see @ref VitalSignsDemo_vitalSignProcess.
 *        The UART transmission is done in the data path processing task
 *        itself although it could be done in a separate thread to potially parallelize
 *        data path processing with transmission on UART.
 *        Separation of the transmit task may be done in future versions of the demo
 *
 * @section  Processing Processing Steps
 *   Processing steps can be seen in the block diagram below:
 *
 *   @image html vitalSignsProcessing_BuildingBlocks.png
 *
 *   - The input is taken from the receive antennae from the ADC buffer
 *     for every chirp (corresponding to the chirping pattern defined in the configuration file)
 *     and a 1D (Range) FFT is performed using the HWA. The output is transfered to the L3 RAM using the EDMA.
 *
 *     The following processing steps happen during the "interframe processing" time. These are implemented in
 *      @ref VitalSignsDemo_vitalSignProcess
 *
 *   - The range profile corresponding to the user defined RX antenna is extracted from the L3 Memory
 *   - The range-bin index corresponding to the maximum reflectivity target is found and the phase at this particular
 *     range-bin is computed.
 *   - Pre-processing on these phase values is done. The pre-processing steps consist of phase unwrapping,
 *     phase differences between successive phase samples and Impulse noise removal
 *   - These phase values are passed through two band-pass filters. These band-pass filters operate in real-time input data
 *     to generate a continuous stream of output data. The IIR filters implemented are cascaded bi-Quad IIR Filters
 *   - Band-pass filter outputs are stored in two circular buffers of size M. One circular buffer stores the
 *     breathing-waveform while the other the cardiac-waveform.
 *   - Circular buffers are passed on to the spectral estimation block. The current implementation provides
 *     a FFT (using the HWA). The configuration of the EDMA and HWA for the FFT on the breathing and
 *     cardiac waveforms can be found @ref VitalSignsDemo_configPhase_EDMA and @ref VitalSignsDemo_configPhase_HWA
 *   - Peak counting and other Post processing steps using R4F.
*/


/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* BIOS/XDC Include Files. */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/heaps/HeapBuf.h>
#include <ti/sysbios/heaps/HeapMem.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/family/arm/v7a/Pmu.h>
#include <ti/sysbios/family/arm/v7r/vim/Hwi.h>
#include <ti/sysbios/utils/Load.h>

/* mmWave SDK Include Files: */
#include <ti/common/sys_common.h>
#include <ti/common/mmwave_sdk_version.h>
#include <ti/drivers/soc/soc.h>
#include <ti/drivers/esm/esm.h>
#include <ti/drivers/crc/crc.h>
#include <ti/drivers/gpio/gpio.h>
#include <ti/drivers/mailbox/mailbox.h>
#include <ti/control/mmwave/mmwave.h>
#include <ti/drivers/osal/DebugP.h>
#include <ti/drivers/uart/UART.h>
#include <ti/utils/cli/cli.h>

/* Demo Include Files */
#include "vitalSigns.h"
#include "vitalSignsDemo_utilsFunc.h"
#include "config_edma_util.h"
#include "config_hwa_util.h"

#include "data_path.h"

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/*! Enable a Simulated Test Tone */
//#define TEST_TONE

#define MAX_NUM_RANGE_BINS_PROCESS    100

/*! Finds the max Range-bin every RESET_LOCAL_COUNT_VAL frames */
#define RANGE_BIN_TRACKING    1

/*! Perform Automatic Gain Control for the Cardiac Waveform prior to FFT-based Spectral Estimation */
#define FLAG_PERFORM_AGC      1

/*! Apply Hanning Window prior to FFT-based Spectral Estimation */
#define FLAG_APPLY_WINDOW     1

/*! Breathing Harmonic Cancellation (only the 2nd Harmonic is cancelled in the current implementation) */
#define FLAG_HARMONIC_CANCELLATION         1

/*! Perform Median Filtering on the Breathing-Rate and Heart-Rate values estimated using the FFT */
#define FLAG_MEDIAN_FILTER                     1

/* Compute the phase differences between the successive phase values */
#define FLAG_COMPUTE_PHASE_DIFFERENCE          1

/* Remove impulse-like noise before Spectral estimation */
#define FLAG_REMOVE_IMPULSE_NOISE              1

/* Length of Magic Word that's sent at the start of each frame for synchronization purposes */
#define LENGTH_MAGIC_WORD                      8

/* Perform AutoCorrelation of the time-domain waveform */
#define PERFORM_XCORR                          1

/*! L3 RAM buffer */
uint8_t gMmwL3[SOC_XWR14XX_MSS_L3RAM_SIZE];
#pragma DATA_SECTION(gMmwL3, ".l3ram");

extern vitalSignsHwaBuf_t gMmwHwaMemBuf[VITALSIGNS_HWA_NUM_MEM_BUFS];

extern uint32_t log2Approx(uint32_t x);

/*! L3 heap for convenience of partitioning L3 RAM */
VitalSignsDemoMemPool_t gMmwL3heap = { &gMmwL3[0], SOC_XWR14XX_MSS_L3RAM_SIZE, 0 };

/**************************************************************************
 *************************** Global Definitions ***************************
 **************************************************************************/
/*! Global Variable for tracking information required by the vital Signs Demo  */
VitalSignsDemo_MCB gMmwMCB;

/*! Global Frame Count */
static uint32_t gFrameCount;

/**************************************************************************
 *************************** Extern Definitions ***************************
 **************************************************************************/

extern void VitalSignsDemo_CLIInit (void);

/**************************************************************************
 ************************* Vital Signs Measurements Demo Functions ********
 **************************************************************************/

void VitalSignsDemo_mmWaveCtrlTask(UArg arg0, UArg arg1);
void VitalSignsDemo_dataPathConfigCommon(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_dataPathInit(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_dataPathConfig(void);
void VitalSignsDemo_dataPathOpen(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_vitalSignProcess(VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_transmitProcessedOutput(UART_Handle uartHandle, VitalSignsDemo_DataPathObj *obj);
void VitalSignsDemo_initTask(UArg arg0, UArg arg1);
void VitalSignsDemo_dataPathTask(UArg arg0, UArg arg1);
void VitalSignsDemo_dataPathCfgBuffers(VitalSignsDemo_DataPathObj *obj, VitalSignsDemoMemPool_t *pool);
bool VitalSignsDemo_parseProfileAndChirpConfig(VitalSignsDemo_DataPathObj *dataPathObj);

/* external sleep function when in idle (used in .cfg file) */
void MmwDemo_sleep(void);

/**
 *  @b Description
 *  @n
 *      Send assert information through CLI.
 */
void _MmwDemo_debugAssert(int32_t expression, const char *file, int32_t line)
{
    if (!expression) {
        CLI_write ("Exception: %s, line %d.\n",file,line);
    }
}

/**
 *  @b Description
 *  @n
 *      Get a handle for ADCBuf.
 */
void MmwDemo_ADCBufOpen(VitalSignsDemo_DataPathObj *obj)
{
    ADCBuf_Params       ADCBufparams;
    /*****************************************************************************
     * Start ADCBUF driver:
     *****************************************************************************/
    /* ADCBUF Params initialize */
    ADCBuf_Params_init(&ADCBufparams);
    ADCBufparams.chirpThreshold = 1;
    ADCBufparams.continousMode  = 0;

    /* Open ADCBUF driver */
    obj->adcbufHandle = ADCBuf_open(0, &ADCBufparams);
    if (obj->adcbufHandle == NULL)
    {
        //System_printf("Error: Unable to open the ADCBUF driver\n");
        MmwDemo_debugAssert (0);
        return;
    }
    //System_printf("Debug: ADCBUF Instance(0) %p has been opened successfully\n", obj->adcbufHandle);
}


/**
 *  @b Description
 *  @n
 *      Configures ADCBuf and returns the number of RxAntennas
 */
uint8_t MmwDemo_ADCBufConfig(VitalSignsDemo_DataPathObj *dataPathObj)
{

    ADCBuf_dataFormat   dataFormat;
    ADCBuf_RxChanConf   rxChanConf;
    uint8_t             channel;
    int32_t             retVal = 0;
    uint8_t             numBytePerSample = 0;
    MmwDemo_ADCBufCfg*  ptrAdcbufCfg;
    uint32_t            chirpThreshold;
    uint32_t            rxChanMask = 0xF;

    ptrAdcbufCfg = &dataPathObj->cliCfg->adcBufCfg;

    /*****************************************************************************
     * Disable all ADCBuf channels
     *****************************************************************************/
    if ((retVal = ADCBuf_control(dataPathObj->adcbufHandle, ADCBufMMWave_CMD_CHANNEL_DISABLE, (void *)&rxChanMask)) < 0)
    {
       System_printf("Error: Disable ADCBuf channels failed with [Error=%d]\n", retVal);
       DebugP_assert (0);
       goto exit;
    }

    /* Calculate the DMA transfer parameters */
    if (ptrAdcbufCfg->adcFmt == 0)
    {
        /* Complex dataFormat has 4 bytes */
        numBytePerSample =  4;
    }
    else
    {
        /* Real dataFormat has 2 bytes */
        numBytePerSample =  2;
    }

    /* Configure ADC buffer data format */
    dataFormat.adcOutFormat       = ptrAdcbufCfg->adcFmt;
    dataFormat.sampleInterleave   = ptrAdcbufCfg->iqSwapSel;
    dataFormat.channelInterleave  = ptrAdcbufCfg->chInterleave;

    /* Debug Message: */
    System_printf("Debug: Start ADCBuf driver dataFormat=%d, sampleSwap=%d, interleave=%d, chirpThreshold=%d\n",
                   dataFormat.adcOutFormat, dataFormat.sampleInterleave, dataFormat.channelInterleave,
                   ptrAdcbufCfg->chirpThreshold);

    retVal = ADCBuf_control(dataPathObj->adcbufHandle, ADCBufMMWave_CMD_CONF_DATA_FORMAT, (void *)&dataFormat);
    if (retVal < 0)
    {
        DebugP_assert (0);
        goto exit;
    }

    memset((void*)&rxChanConf, 0, sizeof(ADCBuf_RxChanConf));
    /* Enable Rx Channel indicated in channel configuration */
    for (channel = 0; channel < SYS_COMMON_NUM_RX_CHANNEL; channel++)
    {
        if(gMmwMCB.cfg.openCfg.chCfg.rxChannelEn & 0x1<<channel)
        {
            /* Populate the receive channel configuration: */
            rxChanConf.channel = channel;
            retVal = ADCBuf_control(dataPathObj->adcbufHandle, ADCBufMMWave_CMD_CHANNEL_ENABLE, (void *)&rxChanConf);
            if (retVal < 0)
            {
                DebugP_assert (0);
                goto exit;
            }

        rxChanConf.offset  += dataPathObj->numAdcSamples * numBytePerSample;

        }
    }

    chirpThreshold = ptrAdcbufCfg->chirpThreshold;
    /* Set the chirp threshold: */
    retVal = ADCBuf_control(dataPathObj->adcbufHandle, ADCBufMMWave_CMD_SET_CHIRP_THRESHHOLD,
                            (void *)&chirpThreshold);
    if(retVal < 0)
    {
        DebugP_assert (0);
    }

exit:
     return retVal;
}

/** @brief Processing for vital Signs Measurement.
 *
 *   @param[in] obj          Pointer data path object VitalSignsDemo_DataPathObj
 */
void VitalSignsDemo_vitalSignProcess(VitalSignsDemo_DataPathObj *obj)
{
    static uint16_t frameCountLocal;       // Local circular count
    uint32_t processingCyclesOut;          // Keeps track of the cycle counts
    uint16_t loopIndexBuffer;              // Index that loops over the buffers

    /* "obj->radarCube" contains the Range FFT Output after processing in the HWA */
    uint32_t *mmwRadarCube = obj->radarCube;

    /* Obtain GUI-related Flags sent through the CLI */
    VitalSignsDemo_GuiMonSel *pGuiMonSel;
    pGuiMonSel = (VitalSignsDemo_GuiMonSel *) &(gMmwMCB.cliCfg.vitalSigns_GuiMonSel);

    /* Variables for Impulse Noise Removal */
    static float dataCurr, dataPrev2, dataPrev1;

    /* Variables for Clutter Removal */
    uint16_t guiFlag_ClutterRemoval  = pGuiMonSel->guiFlag_ClutterRemoval;
    float maxValClutter;
    uint16_t rangeBinMaxClutter;

    /* Variables for Phase Unwrapping */
    static float phasePrevFrame;              // Phase value of Previous frame (For phase unwrapping)
    static float diffPhaseCorrectionCum;      // Phase correction cumulative (For phase unwrapping)
    static float phaseUsedComputationPrev;    // Phase values used for the Previous frame
    float unwrapPhasePeak,unwrapPhasePeak_mm; // Unwrapped Phase values at the target range-bin
    float phaseUsedComputation;               // Unwrapped Phase value used for computation

    /* Variables for Detecting Motion Corrupted Segments */
    uint16_t guiFlag_MotionDetection = obj->cliCfg->motionDetectionParamsCfg.enabled;       // GUI Flag. Set from the configuration File
    uint16_t guiFlag_GainControl     = obj->cliCfg->motionDetectionParamsCfg.gainControl;   // GUI Flag. Set from the configuration File
    uint16_t indexMotionDetection;      // Temporary Index
    float sumEnergy;                    // Energy in the data-segment checked for presence of large-scale motion

    /* Vital Signs Waveform */
    float outputFilterBreathOut;              // Breathing waveform after the IIR-Filter
    float outputFilterHeartOut;               // Cardiac waveform after the IIR-Filter
    int32_t *pCircularBufferHeart;            // Pointer to the Cardiac Waveform
    int32_t *pCircularBufferBreath;           // Pointer to the Breathing Waveform

    /* Variables for FFT-based Spectral Estimation */
    uint16_t pPeakSortOutIndex[MAX_NUM_PEAKS_SPECTRUM];   // Sorted Peaks in the Spectrum
    uint16_t numPeaks_BreathSpectrum;                     // Number of Peaks in the Breathing Spectrum
    uint16_t numPeaks_heartSpectrum;                      // Number of Peaks in the Cardiac Spectrum
    uint16_t maxIndexHeartBeatSpect, maxIndexBreathSpect; // Indices corresponding to the max peak in the Breathing and Cardiac Spectrum
    uint16_t maxIndexHeartBeatSpect_4Hz;                  // Indices corresponding to the max peak from [1.6 - 4.0] Hz
    float breathingRateEst_FFT, heartRateEst_FFT;         // Vital Signs Estimate based on the FFT
    float heartRateEst_FFT_4Hz;                           // Vital Signs Estimate based on the FFT

    /* Confidence Metric associated with the estimates */
    float confidenceMetricBreath[MAX_NUM_PEAKS_SPECTRUM];         // Confidence Metric associated with each Breathing Spectrum Peak
    float confidenceMetricHeart[MAX_NUM_PEAKS_SPECTRUM];          // Confidence Metric associated with each Cardiac Spectrum Peak
    float confidenceMetricBreathOut, confidenceMetricHeartOut;    // Confidence Metric associated with the estimates
    float confidenceMetricHeartOut_4Hz;                           // Confidence Metric for the 1st Heart beat Harmonic
    float confidenceMetricHeartOut_xCorr;                         // Confidence Metric for the Autocorrelation
    float confidenceMetricBreathOut_xCorr;

    /* Variables for peak-counting */
    uint16_t pPeakLocsHeart[MAX_PEAKS_ALLOWED_WFM];    // Peak locations (indices) of the Cardiac Waveform
    uint16_t pPeakLocsBreath[MAX_PEAKS_ALLOWED_WFM];   // Peak locations (indices) of the Breathing Waveform
    uint16_t pPeakLocsValid[MAX_PEAKS_ALLOWED_WFM];    // Peak locations after only retaining the valid peaks
    uint16_t numPeaksBreath, numPeaksHeart;                       // Number of peaks in the time-domain filtered waveform
    float breathingRateEst_peakCount, heartRateEst_peakCount;     // Vital Signs Estimate based on peak-Interval
    float heartRateEst_peakCount_filtered;                        // Heart-rate peak-interval based estimate after filtering

    /* Exponential smoothing filter */
    static float breathWfmOutUpdated, heartWfmOutUpdated;    // Updated values after exponential smoothing
    float breathWfmOutPrev, heartWfmOutPrev;                 // Exponential smoothing values at time instance (t-1)
    float sumEnergyBreathWfm, sumEnergyHeartWfm;             // These values are used to make a decision if the energy in the waveform is sufficient for it to be classfied as a valid waveform

    /* Variables for Auto-Correlation */
    float heartRateEst_xCorr;         // Heart-rate estimate from the Autocorrelation Method
    float breathRateEst_xCorr;        // Breath-rate estimate from the Autocorrelation Method

    /* For FIR Filtering */
    static float pDataIn[FIR_FILTER_SIZE];

    /* Variables for Extracting the Range-FFT output  */
    uint16_t rangeBinIndex;             // Range-bin Index
    float rangeBinPhase;                // Phase of the Range-bin selected for processing
    static uint16_t rangeBinIndexPhase; // Index of the Range Bin for which the phase is computed
    uint16_t rangeBinMax;               // Index of the Strongest Range-Bin
    uint16_t indexTemp, indexNumPeaks;  // Temporary Indices
    int16_t temp_real, temp_imag;       // Temporary variables storing the Real and Imaginary part of a range-bin in the Range-FFT Output
    float absVal;                       // Absolute value based on the Real and Imaginary values
    float maxVal;                       // Maximum Value of the Range-Bin in the current range-profile
    uint16_t *pTempIndex;                // Temporary Index pointer


    // Refresh Pushbutton on the GUI Pressed
    if (pGuiMonSel->guiFlag_Reset  == 1)
       {
           gFrameCount = 1;
           pGuiMonSel->guiFlag_Reset = 0;
           breathWfmOutUpdated = 0;
           heartWfmOutUpdated = 0;
       }

    if (RANGE_BIN_TRACKING)
      {
          frameCountLocal = gFrameCount % RESET_LOCAL_COUNT_VAL;
      }
      else
      {
          frameCountLocal = gFrameCount;
      }
     rangeBinMax = 0;
     maxVal = 0;
     rangeBinMaxClutter = 0;
     maxValClutter = 0;

      //  tempPtr points towards the RX-Channel to process
    uint32_t *tempPtr = mmwRadarCube + (obj->rxAntennaProcess - 1);

      //  Range-FFT is stored in L3 memory in a transpose fashion
    tempPtr += (obj->rangeBinStartIndex) * (obj->numChirpsPerFrame * obj->numRxAntennas);  // Chirps per Frame takes into account the numTX antennas

    for (rangeBinIndex = obj->rangeBinStartIndex; rangeBinIndex < obj->rangeBinEndIndex; rangeBinIndex++)
      {
          // Points towards the real part of the current range-bin i.e. rangeBinIndex
          pTempIndex = (uint16_t *) tempPtr;
          obj->pRangeProfileCplx[2 * (rangeBinIndex - obj->rangeBinStartIndex)] = *pTempIndex;
          temp_real = (int16_t) * pTempIndex;

          // Points towards the imaginary part of the current range-bin i.e. rangeBinIndex
          obj->pRangeProfileCplx[2 * (rangeBinIndex - obj->rangeBinStartIndex) + 1] = *(++pTempIndex);
          temp_imag = (int16_t) * pTempIndex;

         if (guiFlag_ClutterRemoval == 1)
         {
          // Clutter Removed Range Profile
          float tempReal_Curr,tempImag_Curr;
          float alphaClutter = 0.1;
          float currVal;

          tempReal_Curr = (float) temp_real;
          tempImag_Curr = (float) temp_imag;
          uint16_t currRangeIndex;
          currRangeIndex = rangeBinIndex - obj->rangeBinStartIndex;

          obj->pTempReal_Prev[currRangeIndex]  = alphaClutter*tempReal_Curr + (1-alphaClutter)*obj->pTempReal_Prev[currRangeIndex];
          obj->pTempImag_Prev[currRangeIndex]  = alphaClutter*tempImag_Curr + (1-alphaClutter)*obj->pTempImag_Prev[currRangeIndex];

          currVal = sqrt((tempReal_Curr - obj->pTempReal_Prev[currRangeIndex])*(tempReal_Curr - obj->pTempReal_Prev[currRangeIndex]) + (tempImag_Curr - obj->pTempImag_Prev[currRangeIndex])*(tempImag_Curr - obj->pTempImag_Prev[currRangeIndex]));
          obj->pRangeProfileClutterRemoved[rangeBinIndex - obj->rangeBinStartIndex] = currVal;

   // Based on the Max value Range-bin
          if (currVal > maxValClutter)
           {
              maxValClutter = currVal;
              rangeBinMaxClutter = rangeBinIndex;
           }
      }
      else
      {
          // Magnitude of the current range-bin
           absVal = (float) temp_real * (float) temp_real + (float) temp_imag * (float) temp_imag;
          // Maximum value range-bin of the current range-profile
          if (absVal > maxVal)
          {
              maxVal = absVal;
              rangeBinMax = rangeBinIndex;
          }
      }
       // If the Refresh button in the GUI is pressed
       if (frameCountLocal == 1)
          {
              if (guiFlag_ClutterRemoval ==1)
              {
              rangeBinIndexPhase = rangeBinMaxClutter;
              }
              else
              {
              rangeBinIndexPhase = rangeBinMax;
              }
          }

       // Computes the phase of the selected range-bin i.e. rangeBinIndexPhase
       if (rangeBinIndex == (rangeBinIndexPhase))
          {
                  rangeBinPhase = atan2(temp_imag, temp_real);
          }
          // Points towards the next range-bin
          tempPtr += obj->numChirpsPerFrame * obj->numRxAntennas;  // Chirps per Frame takes into account the numTX antennas
      }  // For Loop ends

    // Phase-Unwrapping
    unwrapPhasePeak = unwrap(rangeBinPhase, phasePrevFrame, &diffPhaseCorrectionCum);
    phasePrevFrame = rangeBinPhase;

    // Sends out a test-tone
#ifdef TEST_TONE
    float testTone_ampBreath_mm  = 2.0;
    float testTone_freqBreath_Hz = 0.4;
    float testTone_ampHeart_mm   = 0.3;
    float testTone_freqHeart_Hz  = 1.8;

    unwrapPhasePeak = (1/obj->scaleFactor_PhaseToDisp)
                      *(testTone_ampBreath_mm  * sin(2 * PI * testTone_freqBreath_Hz * gFrameCount/obj->samplingFreq_Hz)
                      + testTone_ampHeart_mm * sin(2*PI*testTone_freqHeart_Hz*gFrameCount/obj->samplingFreq_Hz));
#endif
    // Computes the phase differences between successive phase samples
    if(FLAG_COMPUTE_PHASE_DIFFERENCE)
      {
         phaseUsedComputation = unwrapPhasePeak - phaseUsedComputationPrev;
         phaseUsedComputationPrev = unwrapPhasePeak;
      }
    else
      {
         phaseUsedComputation = unwrapPhasePeak;
      }

    // Removes impulse like noise from the waveforms
    if(FLAG_REMOVE_IMPULSE_NOISE)
      {
        float threshNoise = 1.5;
        dataPrev2 = dataPrev1;
        dataPrev1 = dataCurr;
        dataCurr = phaseUsedComputation;
        phaseUsedComputation = filter_RemoveImpulseNoise(dataPrev2, dataPrev1, dataCurr, threshNoise);
      }

     //  Performs Bandpass filtering on the Phase values to separate the signal into "Breathing Waveform" and "Cardiac Waveform"
    outputFilterBreathOut = filter_IIR_BiquadCascade(phaseUsedComputation,  obj->pFilterCoefsBreath, obj->pScaleValsBreath, obj->pDelayBreath, IIR_FILTER_BREATH_NUM_STAGES);
    outputFilterHeartOut  = filter_IIR_BiquadCascade(phaseUsedComputation,  obj->pFilterCoefsHeart_4Hz,  obj->pScaleValsHeart_4Hz,  obj->pDelayHeart,   IIR_FILTER_HEART_NUM_STAGES);

    if (guiFlag_MotionDetection ==1)
       {
      for (loopIndexBuffer = 1; loopIndexBuffer < obj->motionDetection_BlockSize; loopIndexBuffer++)
      {
          obj->pMotionCircularBuffer[loopIndexBuffer - 1] = obj->pMotionCircularBuffer[loopIndexBuffer] ;
      }
      obj->pMotionCircularBuffer[obj->motionDetection_BlockSize-1] = outputFilterHeartOut;
      indexMotionDetection = gFrameCount % obj->motionDetection_BlockSize;

      // Only perform these steps for every (obj->motionDetection_BlockSize) samples
      if (indexMotionDetection == 0)
      {
      // Check if the current segment is "Noisy"
    sumEnergy = 0;
    for (loopIndexBuffer = 0; loopIndexBuffer < obj->motionDetection_BlockSize; loopIndexBuffer++)
       {
         sumEnergy +=  (obj->pMotionCircularBuffer[loopIndexBuffer]*obj->pMotionCircularBuffer[loopIndexBuffer]);
       }

    if (sumEnergy > obj->motionDetection_Thresh)
       {
         obj->motionDetected = 1;   // Temporary variable to send to the GUI
       }
    else
       {
         obj->motionDetected = 0;   // Temporary variable to send to the GUI
       }

    // If NO motion detected in the current segment
        if ( obj->motionDetected == 0)
           {
             uint16_t tempEndIndex;
           //  Shift the current contents of the circular Buffer
          for (loopIndexBuffer = obj->motionDetection_BlockSize; loopIndexBuffer < obj->circularBufferSizeHeart; loopIndexBuffer++)
                 {
                  obj->pTempCircularBuff[loopIndexBuffer - obj->motionDetection_BlockSize] = obj->pTempCircularBuff[loopIndexBuffer] ;
                  }
           // Copy the current data segment to the end of the Circular Buffer
           for (loopIndexBuffer = 0; loopIndexBuffer < obj->motionDetection_BlockSize; loopIndexBuffer++)
                  {
                  tempEndIndex = obj->circularBufferSizeHeart - obj->motionDetection_BlockSize;
                  obj->pTempCircularBuff[ tempEndIndex + loopIndexBuffer] = obj->pMotionCircularBuffer[loopIndexBuffer] ;
                  }
          }
      }
     // If Motion DETECTED then don't UPDATE or SHIFT the values in the buffer
      }
  else
     {
      // Copies the "Cardiac Waveform" in a temporary circular Buffer for further Pre-processing
          for (loopIndexBuffer = 1; loopIndexBuffer < obj->circularBufferSizeHeart; loopIndexBuffer++)
          {
              obj->pTempCircularBuff[loopIndexBuffer - 1] = obj->pTempCircularBuff[loopIndexBuffer] ;
          }
          obj->pTempCircularBuff[obj->circularBufferSizeHeart-1] = outputFilterHeartOut;
       }

      // Pointers to the Breathing and Cardiac Waveform Circular Buffers
      pCircularBufferBreath     = obj->pVitalSignsCircularBuff;
      pCircularBufferHeart      = obj->pVitalSignsCircularBuff + obj->circularBufferSizeBreath;

      // Copies the "Breathing Waveform" in a circular Buffer
      for (loopIndexBuffer = 1; loopIndexBuffer < obj->circularBufferSizeBreath; loopIndexBuffer++)
      {
        pCircularBufferBreath[loopIndexBuffer - 1] = pCircularBufferBreath[loopIndexBuffer];
        obj->pVitalSigns_Breath_CircularBuffer[loopIndexBuffer - 1] = obj->pVitalSigns_Breath_CircularBuffer[loopIndexBuffer];

      }
        pCircularBufferBreath[obj->circularBufferSizeBreath - 1] = (int32_t) obj->scale_breathingWfm * outputFilterBreathOut;
        obj->pVitalSigns_Breath_CircularBuffer[obj->circularBufferSizeBreath - 1] = outputFilterBreathOut;

      /* Spectral Estimation based on the Number of Peaks */
      numPeaksHeart  = find_Peaks(obj->pTempCircularBuff, float_type, pPeakLocsHeart,obj->pPeakValues, 0, obj->circularBufferSizeHeart  - 1);
      if (numPeaksHeart != 0)
      {
          numPeaksHeart  =  filterPeaksWfm(pPeakLocsHeart, pPeakLocsValid, numPeaksHeart, obj->peakDistanceHeart_Min, obj->peakDistanceHeart_Max);
      }
      heartRateEst_peakCount = CONVERT_HZ_BPM * ((numPeaksHeart * obj->samplingFreq_Hz) / obj->circularBufferSizeHeart);

          for (loopIndexBuffer = 1; loopIndexBuffer < FIR_FILTER_SIZE; loopIndexBuffer++)
          {
              pDataIn[loopIndexBuffer - 1] = pDataIn[loopIndexBuffer];
          }
          pDataIn[FIR_FILTER_SIZE - 1] = heartRateEst_peakCount;
          heartRateEst_peakCount_filtered = filter_FIR(pDataIn, obj->pFilterCoefs, FIR_FILTER_SIZE);

      numPeaksBreath = find_Peaks(obj->pVitalSignsCircularBuff, int32_type, pPeakLocsBreath, obj->pPeakValues, 0, obj->circularBufferSizeBreath - 1);
      if (numPeaksBreath != 0)
      {
          numPeaksBreath  =  filterPeaksWfm(pPeakLocsBreath, pPeakLocsValid, numPeaksBreath, obj->peakDistanceBreath_Min, obj->peakDistanceBreath_Max);
      }

      breathingRateEst_peakCount = CONVERT_HZ_BPM  * ((numPeaksBreath * obj->samplingFreq_Hz) / obj->circularBufferSizeBreath);
      heartRateEst_peakCount     = CONVERT_HZ_BPM  * ((numPeaksHeart * obj->samplingFreq_Hz) / obj->circularBufferSizeHeart);

      // Pre-Processing Steps for the Cardiac Waveform
      // Perform Automatic Gain Control
     if (guiFlag_GainControl)
     {
      computeAGC ( obj->pTempCircularBuff, obj->circularBufferSizeHeart, obj->motionDetection_BlockSize, obj->motionDetection_Thresh);
     }

     if (guiFlag_MotionDetection ==1)
     {
        outputFilterHeartOut =  obj->pMotionCircularBuffer[obj->motionDetection_BlockSize-1];
     }
     else
     {
        outputFilterHeartOut = obj->pTempCircularBuff[obj->circularBufferSizeHeart-1];
     }

     if (PERFORM_XCORR)
     {
         uint16_t xCorr_numPeaks;
         uint16_t maxIndex_lag;
         computeAutoCorrelation ( obj->pTempCircularBuff,  obj->circularBufferSizeHeart , obj->pXcorr, obj->xCorr_minLag, obj->xCorr_maxLag);
         xCorr_numPeaks  = find_Peaks(obj->pXcorr, float_type, obj->pPeakIndex, obj->pPeakValues, obj->xCorr_minLag, obj->xCorr_maxLag);
         maxIndex_lag    = computeMaxIndex((float*) obj->pXcorr, obj->xCorr_minLag, obj->xCorr_maxLag);
         float temp = (float) (1.0)/(maxIndex_lag/obj->samplingFreq_Hz);
         heartRateEst_xCorr = (float) CONVERT_HZ_BPM *temp;

         if (xCorr_numPeaks == 0 )
         {
             confidenceMetricHeartOut_xCorr = 0;
         }
         else
         {
             confidenceMetricHeartOut_xCorr = obj->pXcorr[maxIndex_lag];
         }

         // Auto-correlation on the Breathing Waveform
         computeAutoCorrelation ( obj->pVitalSigns_Breath_CircularBuffer,  obj->circularBufferSizeBreath,
                                  obj->pXcorr, obj->xCorr_Breath_minLag, obj->xCorr_Breath_maxLag);
         xCorr_numPeaks  = find_Peaks(obj->pXcorr, float_type, obj->pPeakIndex, obj->pPeakValues, obj->xCorr_Breath_minLag, obj->xCorr_Breath_maxLag);
         maxIndex_lag    = computeMaxIndex((float*) obj->pXcorr, obj->xCorr_Breath_minLag, obj->xCorr_Breath_maxLag);
         temp = (float) (1.0)/(maxIndex_lag/obj->samplingFreq_Hz);
         breathRateEst_xCorr = (float) CONVERT_HZ_BPM *temp;

         if (xCorr_numPeaks == 0 )
         {
             confidenceMetricBreathOut_xCorr = 0;
         }
         else
         {
             confidenceMetricBreathOut_xCorr = obj->pXcorr[maxIndex_lag];
         }
     }

     // Apply Window on the Cardiac Waveform prior to FFT-based spectral estimation
     // and copies the pre-processed data to pCircularBufferHeart
     if (FLAG_APPLY_WINDOW)
     {
      uint16_t index_win;
      uint16_t index_WinEnd;

      float tempFloat;
      index_WinEnd =  obj->circularBufferSizeHeart - 1;
      for (index_win = 0; index_win < DOPPLER_WINDOW_SIZE; index_win++)
      {
        tempFloat = obj->pDopplerWindow[index_win];
        pCircularBufferHeart[index_win]    = (int32_t) obj->scale_heartWfm * tempFloat * obj->pTempCircularBuff[index_win];
        pCircularBufferHeart[index_WinEnd] = (int32_t) obj->scale_heartWfm * tempFloat * obj->pTempCircularBuff[index_WinEnd];
        index_WinEnd --;
      }

      for (loopIndexBuffer = DOPPLER_WINDOW_SIZE; loopIndexBuffer < obj->circularBufferSizeHeart - DOPPLER_WINDOW_SIZE ; loopIndexBuffer++)
      {
           pCircularBufferHeart[loopIndexBuffer]     = (int32_t)  (obj->scale_heartWfm)*obj->pTempCircularBuff[loopIndexBuffer];
      }
     }
     else
     {
       for (loopIndexBuffer = 0; loopIndexBuffer < obj->circularBufferSizeHeart; loopIndexBuffer++)
       {
           pCircularBufferHeart[loopIndexBuffer] = (int32_t) ( obj->scale_heartWfm)*obj->pTempCircularBuff[loopIndexBuffer];
       }
     }

    /*  Performs the Spectral analysis using the HWA
         obj->vitalSignsSpectrum contains the breathing and heart rate spectrum after the FFT on the HWA */
    VitalSignsDemo_dataPathTriggerPhase(obj);  // Trigger the EDMA For data transfer from L3 to HWA
    VitalSignsDemo_dataPathWaitPhase(obj);     // Wait for the HWA + EDMA to complete the FFT on the waveforms and transer the output to L3

    // Pointers to the Breathing spectrum and Cardiac spectrum
    obj->pBreathingSpectrum = obj->pVitalSignsSpectrum;
    obj->pHeartbeatSpectrum = obj->pVitalSignsSpectrum + BREATHING_SPECTRUM_SIZE;

    // Pick the Peaks in the Breathing Spectrum
    numPeaks_BreathSpectrum = find_Peaks(obj->pBreathingSpectrum, float_type, obj->pPeakIndex, obj->pPeakValues,
                                           obj->breath_startFreq_Index, obj->breath_endFreq_Index);
    indexNumPeaks = (numPeaks_BreathSpectrum < MAX_NUM_PEAKS_SPECTRUM) ? numPeaks_BreathSpectrum : MAX_NUM_PEAKS_SPECTRUM;

    if (indexNumPeaks != 0)
        {
         heapsort_index(obj->pPeakValues, numPeaks_BreathSpectrum, obj->pPeakIndexSorted);
         for(indexTemp = 0; indexTemp < indexNumPeaks; indexTemp++ )
            {
            pPeakSortOutIndex[indexTemp] = obj->pPeakIndex [obj->pPeakIndexSorted[numPeaks_BreathSpectrum-indexTemp-1] ];
            confidenceMetricBreath[indexTemp]  = computeConfidenceMetric(obj->pBreathingSpectrum,
                                                                         obj->confMetric_spectrumBreath_IndexStart,
                                                                         obj->confMetric_spectrumBreath_IndexEnd,
                                                                         pPeakSortOutIndex[indexTemp],
                                                                         obj->confMetric_numIndexAroundPeak_breath);
            }
         maxIndexBreathSpect       = pPeakSortOutIndex[0]; // The maximum peak
         confidenceMetricBreathOut = confidenceMetricBreath[0];
         }
     else
      {
         maxIndexBreathSpect    = computeMaxIndex((float*) obj->pBreathingSpectrum, obj->breath_startFreq_Index, obj->breath_endFreq_Index);
         confidenceMetricBreathOut  = computeConfidenceMetric(obj->pBreathingSpectrum,
                                                              0,
                                                              PHASE_FFT_SIZE/4,
                                                              maxIndexBreathSpect,
                                                              obj->confMetric_numIndexAroundPeak_breath);
      }

    // Pick the Peaks in the Heart Spectrum [1.6 - 4.0 Hz]
    numPeaks_heartSpectrum = find_Peaks(obj->pHeartbeatSpectrum, uint32_type, obj->pPeakIndex, obj->pPeakValues,
                                         obj->heart_startFreq_Index_1p6Hz, obj->heart_endFreq_Index_4Hz);
    indexNumPeaks = (numPeaks_heartSpectrum < MAX_NUM_PEAKS_SPECTRUM) ? numPeaks_heartSpectrum : MAX_NUM_PEAKS_SPECTRUM;
    if (indexNumPeaks != 0)
        {
          heapsort_index(obj->pPeakValues, numPeaks_heartSpectrum, obj->pPeakIndexSorted);
          for(indexTemp = 0; indexTemp < indexNumPeaks; indexTemp++ )
             {
              pPeakSortOutIndex[indexTemp] = obj->pPeakIndex [obj->pPeakIndexSorted[numPeaks_heartSpectrum-indexTemp-1] ];
             }
     maxIndexHeartBeatSpect_4Hz    = pPeakSortOutIndex[0]; // The maximum peak
     confidenceMetricHeartOut_4Hz  = computeConfidenceMetric(obj->pHeartbeatSpectrum,
                                                             obj->confMetric_spectrumHeart_IndexStart_1p6Hz,
                                                             obj->confMetric_spectrumHeart_IndexStart_4Hz,
                                                             maxIndexHeartBeatSpect_4Hz,
                                                             obj->confMetric_numIndexAroundPeak_heart);
         }
     else
         {
            maxIndexHeartBeatSpect_4Hz = computeMaxIndex((float*) obj->pHeartbeatSpectrum, obj->heart_startFreq_Index_1p6Hz, obj->heart_endFreq_Index_4Hz);
            confidenceMetricHeartOut_4Hz = computeConfidenceMetric(obj->pHeartbeatSpectrum,
                                                                   0,
                                                                   PHASE_FFT_SIZE/4,
                                                                   maxIndexHeartBeatSpect_4Hz,
                                                                   obj->confMetric_numIndexAroundPeak_heart);
         }
        heartRateEst_FFT_4Hz  = (float) CONVERT_HZ_BPM * maxIndexHeartBeatSpect_4Hz * (obj->freqIncrement_Hz);

        // If a peak is within [1.6 2.0] Hz then check if a harmonic is present is the cardiac spectrum region [0.8 - 2.0] Hz
        if (heartRateEst_FFT_4Hz < MAX_HEART_RATE_BPM)
        {
           for (indexTemp =1; indexTemp<numPeaks_heartSpectrum;indexTemp++)

               if(abs(heartRateEst_FFT_4Hz - CONVERT_HZ_BPM *(obj->freqIncrement_Hz)*pPeakSortOutIndex[indexTemp]) < HEART_HAMRONIC_THRESH_BPM)
               {
                   heartRateEst_FFT_4Hz = CONVERT_HZ_BPM *(obj->freqIncrement_Hz)*pPeakSortOutIndex[indexTemp];
                   break;
               }
        }

        // Pick the Peaks in the Cardiac Spectrum
         numPeaks_heartSpectrum = find_Peaks(obj->pHeartbeatSpectrum, float_type, obj->pPeakIndex, obj->pPeakValues, obj->heart_startFreq_Index, obj->heart_endFreq_Index);
         indexNumPeaks = (numPeaks_heartSpectrum < MAX_NUM_PEAKS_SPECTRUM) ? numPeaks_heartSpectrum : MAX_NUM_PEAKS_SPECTRUM;

         if (indexNumPeaks != 0)
         {
         heapsort_index(obj->pPeakValues, numPeaks_heartSpectrum, obj->pPeakIndexSorted);
         for(indexTemp = 0; indexTemp < indexNumPeaks; indexTemp++ )
                   {
                    pPeakSortOutIndex[indexTemp] = obj->pPeakIndex [obj->pPeakIndexSorted[numPeaks_heartSpectrum-indexTemp-1] ];
                    confidenceMetricHeart[indexTemp]  = computeConfidenceMetric(obj->pHeartbeatSpectrum,
                                                                                           obj->confMetric_spectrumHeart_IndexStart,
                                                                                           obj->confMetric_spectrumHeart_IndexEnd,
                                                                                           pPeakSortOutIndex[indexTemp],
                                                                                           obj->confMetric_numIndexAroundPeak_heart);
                   }
             maxIndexHeartBeatSpect   = pPeakSortOutIndex[0]; // The maximum peak
             confidenceMetricHeartOut = confidenceMetricHeart[0];
         }
         else
         {
          maxIndexHeartBeatSpect    = computeMaxIndex((float*) obj->pHeartbeatSpectrum, obj->heart_startFreq_Index, obj->heart_endFreq_Index);
          confidenceMetricHeartOut  = computeConfidenceMetric(obj->pHeartbeatSpectrum,
                                                                         0,
                                                                         PHASE_FFT_SIZE/4,
                                                                         maxIndexHeartBeatSpect,
                                                                         obj->confMetric_numIndexAroundPeak_heart);
         }

         // Remove the First Breathing Harmonic (if present in the cardiac Spectrum)
         if(FLAG_HARMONIC_CANCELLATION)
         {
             float diffIndex = abs(maxIndexHeartBeatSpect - BREATHING_HARMONIC_NUM*maxIndexBreathSpect);
             if ( diffIndex*(obj->freqIncrement_Hz)*CONVERT_HZ_BPM < BREATHING_HAMRONIC_THRESH_BPM ) // Only cancel the 2nd Breathing Harmonic
             {
                 maxIndexHeartBeatSpect = pPeakSortOutIndex[1]; // Pick the 2nd Largest peak in the cardiac-spectrum
                 confidenceMetricHeartOut = confidenceMetricHeart[1];
             }
         }

    heartRateEst_FFT     = (float) CONVERT_HZ_BPM * maxIndexHeartBeatSpect * (obj->freqIncrement_Hz);
    breathingRateEst_FFT = (float) CONVERT_HZ_BPM * maxIndexBreathSpect  * (obj->freqIncrement_Hz);

    //  Median Value for Heart Rate and Breathing Rate based on 'MEDIAN_WINDOW_LENGTH' previous estimates
     if(FLAG_MEDIAN_FILTER)
     {
        for (loopIndexBuffer = 1; loopIndexBuffer < MEDIAN_WINDOW_LENGTH; loopIndexBuffer++)
        {
         obj->pBufferHeartRate[loopIndexBuffer - 1]     = obj->pBufferHeartRate[loopIndexBuffer];
         obj->pBufferBreathingRate[loopIndexBuffer - 1] = obj->pBufferBreathingRate[loopIndexBuffer];
         obj->pBufferHeartRate_4Hz[loopIndexBuffer - 1] = obj->pBufferHeartRate_4Hz[loopIndexBuffer];
        }
     obj->pBufferHeartRate[MEDIAN_WINDOW_LENGTH - 1]     = heartRateEst_FFT;
     obj->pBufferBreathingRate[MEDIAN_WINDOW_LENGTH - 1] = breathingRateEst_FFT;
     obj->pBufferHeartRate_4Hz[MEDIAN_WINDOW_LENGTH - 1] = heartRateEst_FFT_4Hz;

     heapsort_index(obj->pBufferHeartRate, MEDIAN_WINDOW_LENGTH,  obj->pPeakSortTempIndex);
     heartRateEst_FFT = obj->pBufferHeartRate[ obj->pPeakSortTempIndex[MEDIAN_WINDOW_LENGTH/2+1]];

     heapsort_index(obj->pBufferHeartRate_4Hz, MEDIAN_WINDOW_LENGTH,  obj->pPeakSortTempIndex);
     heartRateEst_FFT_4Hz = obj->pBufferHeartRate_4Hz[ obj->pPeakSortTempIndex[MEDIAN_WINDOW_LENGTH/2+1]];

     heapsort_index(obj->pBufferBreathingRate, MEDIAN_WINDOW_LENGTH,  obj->pPeakSortTempIndex);
     breathingRateEst_FFT = obj->pBufferBreathingRate[ obj->pPeakSortTempIndex[MEDIAN_WINDOW_LENGTH/2+1]];
     }

    breathWfmOutPrev = breathWfmOutUpdated;
    breathWfmOutUpdated = (obj->alpha_breathing)*(outputFilterBreathOut*outputFilterBreathOut) + (1 - (obj->alpha_breathing))*breathWfmOutPrev; // Exponential Smoothing
    sumEnergyBreathWfm = breathWfmOutUpdated*10000;

    heartWfmOutPrev = heartWfmOutUpdated;
    heartWfmOutUpdated = (obj->alpha_heart)*(outputFilterHeartOut*outputFilterHeartOut) + (1 - (obj->alpha_heart))*heartWfmOutPrev;  // Exponential Smoothing
    sumEnergyHeartWfm = heartWfmOutUpdated*10000;

    unwrapPhasePeak_mm = unwrapPhasePeak*(obj->scaleFactor_PhaseToDisp);
    processingCyclesOut =  obj->timingInfo.interFrameProcCycles;

    // Output Values
    obj->VitalSigns_Output.unwrapPhasePeak_mm    = unwrapPhasePeak_mm;
    obj->VitalSigns_Output.outputFilterBreathOut = outputFilterBreathOut;
    obj->VitalSigns_Output.outputFilterHeartOut  = outputFilterHeartOut;
    obj->VitalSigns_Output.rangeBinIndexPhase = rangeBinIndexPhase;
    obj->VitalSigns_Output.maxVal  = maxVal;
    obj->VitalSigns_Output.sumEnergyHeartWfm  = sumEnergyHeartWfm;
    obj->VitalSigns_Output.sumEnergyBreathWfm = sumEnergyBreathWfm;

    obj->VitalSigns_Output.confidenceMetricBreathOut = confidenceMetricBreathOut;
    obj->VitalSigns_Output.confidenceMetricBreathOut_xCorr    = confidenceMetricBreathOut_xCorr;//breathRateEst_HarmonicEnergy;//;//heartRateEst_HarmonicEnergy;
    obj->VitalSigns_Output.confidenceMetricHeartOut = confidenceMetricHeartOut;    // Confidence Metric associated with the estimates
    obj->VitalSigns_Output.confidenceMetricHeartOut_4Hz = confidenceMetricHeartOut_4Hz;
    obj->VitalSigns_Output.confidenceMetricHeartOut_xCorr = confidenceMetricHeartOut_xCorr;

    obj->VitalSigns_Output.breathingRateEst_FFT = breathingRateEst_FFT;
    obj->VitalSigns_Output.breathingRateEst_peakCount = breathingRateEst_peakCount;
    obj->VitalSigns_Output.breathingRateEst_xCorr  = breathRateEst_xCorr;//breathRateEst_HarmonicEnergy;

    obj->VitalSigns_Output.heartRateEst_peakCount_filtered = heartRateEst_peakCount_filtered;
    obj->VitalSigns_Output.heartRateEst_xCorr = heartRateEst_xCorr;
    obj->VitalSigns_Output.heartRateEst_FFT_4Hz = heartRateEst_FFT_4Hz;
    obj->VitalSigns_Output.heartRateEst_FFT = heartRateEst_FFT;

    obj->VitalSigns_Output.processingCyclesOut = processingCyclesOut;
    obj->VitalSigns_Output.rangeBinStartIndex = obj->rangeBinStartIndex;
    obj->VitalSigns_Output.rangeBinEndIndex  = obj->rangeBinEndIndex;
    obj->VitalSigns_Output.motionDetectedFlag = obj->motionDetected;

}


void VitalSignsDemo_transmitProcessedOutput(UART_Handle uartHandle,
                                            VitalSignsDemo_DataPathObj *obj)
{

    MmwDemo_output_message_header header;
    uint32_t tlvIdx = 0;
    uint32_t numPaddingBytes;
    uint32_t packetLen;
    uint32_t itemPayloadLen;
    uint8_t padding[MMWDEMO_OUTPUT_MSG_SEGMENT_LEN];

    MmwDemo_output_message_tl   tl[MMWDEMO_OUTPUT_MSG_MAX];

    /* Clear message header */
    memset((void *)&header, 0, sizeof(MmwDemo_output_message_header));
    /* Header: */
    header.platform = 0xA1443;
    header.magicWord[0] = 0x0102;
    header.magicWord[1] = 0x0304;
    header.magicWord[2] = 0x0506;
    header.magicWord[3] = 0x0708;
    header.numDetectedObj = 99;//obj->numObjOut;
    header.version =    MMWAVE_SDK_VERSION_BUILD |   //DEBUG_VERSION
                        (MMWAVE_SDK_VERSION_BUGFIX << 8) |
                        (MMWAVE_SDK_VERSION_MINOR << 16) |
                        (MMWAVE_SDK_VERSION_MAJOR << 24);

    packetLen = sizeof(MmwDemo_output_message_header);

    // Vital Signs Stats
    itemPayloadLen = sizeof(VitalSignsDemo_OutputStats);
    tl[tlvIdx].type = MMWDEMO_OUTPUT_MSG_STATS;
    tl[tlvIdx].length = itemPayloadLen;
    packetLen += sizeof(MmwDemo_output_message_tl) + tl[tlvIdx].length;
    tlvIdx++;

    // Complex Range-Profile Output
    tl[tlvIdx].type = MMWDEMO_OUTPUT_MSG_RANGE_PROFILE;
    tl[tlvIdx].length = obj->numRangeBinProcessed *sizeof(uint32_t);
    packetLen += sizeof(MmwDemo_output_message_tl) + tl[tlvIdx].length;
    tlvIdx++;

    header.numTLVs = tlvIdx;
     /* Round up packet length to multiple of MMWDEMO_OUTPUT_MSG_SEGMENT_LEN */
    header.totalPacketLen = MMWDEMO_OUTPUT_MSG_SEGMENT_LEN *
             ((packetLen + (MMWDEMO_OUTPUT_MSG_SEGMENT_LEN-1))/MMWDEMO_OUTPUT_MSG_SEGMENT_LEN);
    header.timeCpuCycles =  Pmu_getCount(0);
    header.frameNumber = obj->frameStartIntCounter;

    /* Send Header */
    tlvIdx = 0;
    // Header
    UART_writePolling (uartHandle,
                        (uint8_t*)&header,
                        sizeof(MmwDemo_output_message_header));

    /* Send Vital Signs Stats */
    // Message Header
    UART_writePolling (uartHandle,
                         (uint8_t*)&tl[tlvIdx],
                         sizeof(MmwDemo_output_message_tl));

    UART_writePolling (uartHandle,
                        (uint8_t*)&(obj->VitalSigns_Output),
                        tl[tlvIdx].length);
    tlvIdx++;

    /* Send RangeProfile bytes */
    UART_writePolling (uartHandle,
                        (uint8_t*)&tl[tlvIdx],
                        sizeof(MmwDemo_output_message_tl));

    UART_writePolling(uartHandle,
                      (uint8_t*) obj->pRangeProfileCplx,
                       obj->numRangeBinProcessed*sizeof(uint32_t));

    /* Send padding bytes */
    numPaddingBytes = MMWDEMO_OUTPUT_MSG_SEGMENT_LEN - (packetLen & (MMWDEMO_OUTPUT_MSG_SEGMENT_LEN-1));
    if (numPaddingBytes<MMWDEMO_OUTPUT_MSG_SEGMENT_LEN)
       {
        UART_writePolling (uartHandle,
                          (uint8_t*)padding,
                          numPaddingBytes);
       }

}
/**
 *  @b Description
 *  @n
 *      The function is used to trigger the Front end to start generating chirps.
 *
 *  @retval
 *      Not Applicable.
 */
int32_t VitalSignsDemo_dataPathStart (void)
{

    MMWave_CalibrationCfg   calibrationCfg;
    int32_t                 errCode = 0;
    VitalSignsDemo_DataPathObj *dataPathObj = &gMmwMCB.dataPathObj;

    dataPathObj->frameStartIntCounter = 0;
    dataPathObj->interFrameProcToken = 0;

    /* Initialize the calibration configuration: */
    memset ((void *)&calibrationCfg, 0, sizeof(MMWave_CalibrationCfg));

    /* Populate the calibration configuration: */
    calibrationCfg.dfeDataOutputMode = MMWave_DFEDataOutputMode_FRAME;
    calibrationCfg.u.chirpCalibrationCfg.enableCalibration    = true;
    calibrationCfg.u.chirpCalibrationCfg.enablePeriodicity    = true;
    calibrationCfg.u.chirpCalibrationCfg.periodicTimeInFrames = 10U;

    /* Start the mmWave module: The configuration has been applied successfully. */
    if (MMWave_start (gMmwMCB.ctrlHandle, &calibrationCfg, &errCode) < 0)
    {
        /* Error: Unable to start the mmWave control */
        //System_printf ("Error: mmWave Control Start failed [Error code %d]\n", errCode);
        MmwDemo_debugAssert (0);
    }
    return errCode;
}


/**
 *  @b Description
 *  @n
 *      The function is used to configure the data path based on the chirp profile.
 *      After this function is executed, the data path processing will ready to go
 *      when the ADC buffer starts receiving samples corresponding to the chirps.
 *
 *  @retval
 *      Not Applicable.
 */

  void VitalSignsDemo_dataPathConfig(void)
 {
    VitalSignsDemo_DataPathObj *dataPathObj = &gMmwMCB.dataPathObj;

      /* Configure ADCBuf Config and get the valid number of RX antennas
         do this first as we need the numRxAntennas in MmwDemo_parseProfileAndChirpConfig
         to get the Virtual Antennas */
      /* Parse the profile and chirp configs and get the valid number of TX Antennas */
      if (VitalSignsDemo_parseProfileAndChirpConfig(dataPathObj) == true)
      {

          if (MmwDemo_ADCBufConfig(dataPathObj) < 0)
          {
              //System_printf("Error: ADCBuf config failed \n");
              MmwDemo_debugAssert (0);
          }

          /* Now we are ready to allocate and config the data buffers */
          VitalSignsDemo_dataPathCfgBuffers(dataPathObj, &gMmwL3heap);
          /* Configure one-time EDMA and HWA parameters */
          VitalSignsDemo_dataPathConfigCommon(dataPathObj);
          /* Config HWA for 1D processing and keep it ready for immediate processingh
             as soon as Front End starts generating chirps */
          VitalSignsDemo_config1D_HWA(dataPathObj);
          VitalSignsDemo_dataPathTrigger1D(dataPathObj);
      }
      else
      {
          /* no valid profile found - assert! */
          MmwDemo_debugAssert(0);
      }
      return;

}


void VitalSignsDemo_dataPathCfgBuffers(VitalSignsDemo_DataPathObj *obj, VitalSignsDemoMemPool_t *pool)

{
    VitalSignsDemo_memPoolReset(pool);

    /* Obtain Parameters from the Command Line Interface */
    VitalSignsDemo_ParamsCfg  vitalSignsParamCLI;
    vitalSignsParamCLI = gMmwMCB.cliCfg.vitalSignsParamsCfg;
    obj->cliCfg->motionDetectionParamsCfg = gMmwMCB.cliCfg.motionDetectionParamsCfg;

    obj->circularBufferSizeHeart    = vitalSignsParamCLI.winLen_heartRate;
    obj->circularBufferSizeBreath   = vitalSignsParamCLI.winLen_breathing;
    obj->rxAntennaProcess           = vitalSignsParamCLI.rxAntennaProcess;
    obj->alpha_breathing            = vitalSignsParamCLI.alpha_breathingWfm;
    obj->alpha_heart                = vitalSignsParamCLI.alpha_heartWfm;
    obj->scale_breathingWfm         = vitalSignsParamCLI.scale_breathingWfm;
    obj->scale_heartWfm             = vitalSignsParamCLI.scale_heartWfm;

    // Make sure that the RX-channel to process is valid
    obj->rxAntennaProcess = (obj->rxAntennaProcess < 1) ? 1 : obj->rxAntennaProcess;
    obj->rxAntennaProcess = (obj->rxAntennaProcess > obj->numRxAntennas) ? obj->numRxAntennas : obj->rxAntennaProcess;

    obj->samplingFreq_Hz = (float) 1000 / (obj->framePeriodicity_ms); // 1000 to convert from ms to seconds
    obj->freqIncrement_Hz = obj->samplingFreq_Hz  / PHASE_FFT_SIZE;

    obj->noiseImpulse_Thresh   = 1.5;
    obj->motionDetection_Thresh = obj->cliCfg->motionDetectionParamsCfg.threshold;      //1.0;   // Threshold for Large Motions
    obj->motionDetection_BlockSize = obj->cliCfg->motionDetectionParamsCfg.blockSize;   //20;    // Block Size for Large Motion

    float rangeStart_meter = vitalSignsParamCLI.startRange_m;    // Get the Start Range. Read from the configuration file
    float rangeEnd_meter   = vitalSignsParamCLI.endRange_m;      // Get the End Range. Read from the configuration file

    obj->rangeBinStartIndex = floor(rangeStart_meter/(obj->rangeBinSize_meter));    // Range-bin index corresponding to the Starting range in meters
    obj->rangeBinEndIndex   = floor(rangeEnd_meter/(obj->rangeBinSize_meter));      // Range-bin index corresponding to the Ending range in meters
    obj->numRangeBinProcessed = obj->rangeBinEndIndex - obj->rangeBinStartIndex + 1;          // Number of range bins that can be processed

    obj->breathingWfm_Spectrum_FftSize = PHASE_FFT_SIZE;
    obj->heartWfm_Spectrum_FftSize = PHASE_FFT_SIZE;

    obj->scale_breathingWfm = (obj->scale_breathingWfm == 0) ? 10000 : obj->scale_breathingWfm;
    obj->scale_heartWfm     = (obj->scale_heartWfm == 0) ? 10000 : obj->scale_heartWfm;

    // Vital-signs peak search frequencies may be different from the Band-pass filter cut-off frequencies
    obj->breath_startFreq_Hz = 0.1;   // Breathing-Rate peak search Start-Frequency
    obj->breath_endFreq_Hz   = 0.6;   // Breathing-Rate peak search End-Frequency

    obj->heart_startFreq_Hz = 0.8;    // Heart-Rate peak search Start-Frequency
    obj->heart_endFreq_Hz   = 2.0;    // Heart-Rate peak search End-Frequency

    obj->heart_startFreq_Index = floor(obj->heart_startFreq_Hz / obj->freqIncrement_Hz);
    obj->heart_endFreq_Index   = ceil(obj->heart_endFreq_Hz / obj->freqIncrement_Hz);

    obj->breath_startFreq_Index = floor(obj->breath_startFreq_Hz / obj->freqIncrement_Hz);
    obj->breath_endFreq_Index   = ceil(obj->breath_endFreq_Hz / obj->freqIncrement_Hz);
    obj->heart_startFreq_Index_1p6Hz = floor(1.6/obj->freqIncrement_Hz);
    obj->heart_endFreq_Index_4Hz   = ceil(4.0/obj->freqIncrement_Hz);

    obj->confMetric_spectrumHeart_IndexStart  = floor(obj->heart_startFreq_Hz/obj->freqIncrement_Hz);
    obj->confMetric_spectrumHeart_IndexEnd    = ceil( obj->heart_endFreq_Hz/obj->freqIncrement_Hz);
    obj->confMetric_spectrumBreath_IndexStart = floor(obj->breath_startFreq_Hz/obj->freqIncrement_Hz);
    obj->confMetric_spectrumBreath_IndexEnd   = ceil( obj->breath_endFreq_Hz/obj->freqIncrement_Hz);
    obj->confMetric_spectrumHeart_IndexStart_1p6Hz  = obj->heart_startFreq_Index_1p6Hz;
    obj->confMetric_spectrumHeart_IndexStart_4Hz  = obj->heart_endFreq_Index_4Hz;
    obj->confMetric_numIndexAroundPeak_heart  = floor(CONF_METRIC_BANDWIDTH_PEAK_HEART_HZ/obj->freqIncrement_Hz);
    obj->confMetric_numIndexAroundPeak_breath = floor(CONF_METRIC_BANDWIDTH_PEAK_BREATH_HZ/obj->freqIncrement_Hz);

    obj->scaleFactor_PhaseToDisp = WAVELENGTH_MM/(4*PI);

    // Auto-Correlation
    obj->xCorr_minLag = (uint16_t) obj->samplingFreq_Hz/2.1;    // (Fs/Freq)  Corresponding to f = 2.1 Hz at a sampling Frequency of 20 Hz
    obj->xCorr_maxLag = (uint16_t) obj->samplingFreq_Hz/0.8;    // (Fs/Freq)  Corresponding to f = 0.8 Hz at a sampling Frequency of 20 Hz
    obj->xCorr_Breath_minLag = (uint16_t) obj->samplingFreq_Hz/obj->breath_endFreq_Hz;    // (Fs/Freq)  Corresponding to f = 0.6 Hz at a sampling Frequency of 20 Hz
    obj->xCorr_Breath_maxLag = (uint16_t) obj->samplingFreq_Hz/obj->breath_startFreq_Hz;    // (Fs/Freq)  Corresponding to f = 0.1 Hz at a sampling Frequency of 20 Hz


    obj->peakDistanceBreath_Max = (uint16_t) obj->samplingFreq_Hz/(obj->breath_startFreq_Hz);
    obj->peakDistanceBreath_Min = (uint16_t) obj->samplingFreq_Hz/(obj->breath_endFreq_Hz);
    obj->peakDistanceHeart_Max  = (uint16_t) obj->samplingFreq_Hz/(obj->heart_startFreq_Hz);
    obj->peakDistanceHeart_Min  = (uint16_t) obj->samplingFreq_Hz/(obj->heart_endFreq_Hz);

//////////////  Parameters :  IIR-Cascade Bandpass Filter  //////////////////////////////////////

    // Heart Beat Rate    0.8 - 4.0 Hz
    float pFilterCoefsHeart_4Hz[IIR_FILTER_HEART_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER] = {
                                           1.0000, 0, -1.0000, 1.0000, -0.5306, 0.5888,
                                           1.0000, 0, -1.0000, 1.0000, -1.8069, 0.8689,
                                           1.0000, 0, -1.0000, 1.0000, -1.4991, 0.5887,
                                           1.0000, 0, -1.0000, 1.0000, -0.6654, 0.2099 };
    float pScaleValsHeart_4Hz[IIR_FILTER_HEART_NUM_STAGES + 1] = {0.4188, 0.4188, 0.3611, 0.3611, 1.0000};
    memcpy(obj->pFilterCoefsHeart_4Hz, pFilterCoefsHeart_4Hz, sizeof(pFilterCoefsHeart_4Hz));
    memcpy(obj->pScaleValsHeart_4Hz, pScaleValsHeart_4Hz, sizeof(pScaleValsHeart_4Hz));

    // Heart Beat Rate    0.8 - 2 Hz
    float pFilterCoefsHeart[IIR_FILTER_HEART_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER] = {
                                           1.0000, 0, -1.0000, 1.0000, -1.4907, 0.8216,
                                           1.0000, 0, -1.0000, 1.0000, -1.8530, 0.9166,
                                           1.0000, 0, -1.0000, 1.0000, -1.6588, 0.7512,
                                           1.0000, 0, -1.0000, 1.0000, -1.4616, 0.6555 };
    float pScaleValsHeart[IIR_FILTER_HEART_NUM_STAGES + 1] = {0.1754, 0.1754, 0.1619, 0.1619, 1.0000};
    memcpy(obj->pFilterCoefsHeart_4Hz, pFilterCoefsHeart, sizeof(pFilterCoefsHeart));
    memcpy(obj->pScaleValsHeart_4Hz, pScaleValsHeart, sizeof(pScaleValsHeart));

    // 0.1 Hz to 0.5 Hz Bandpass Filter coefficients
    float pFilterCoefsBreath[IIR_FILTER_BREATH_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER] = {
                                           1.0000, 0, -1.0000, 1.0000, -1.9632, 0.9644,
                                           1.0000, 0, -1.0000, 1.0000, -1.8501, 0.8681 };
    float pScaleValsBreath[IIR_FILTER_BREATH_NUM_STAGES + 1] = {0.0602, 0.0602, 1.0000};
    memcpy(obj->pFilterCoefsBreath, pFilterCoefsBreath, sizeof(pFilterCoefsBreath));
    memcpy(obj->pScaleValsBreath, pScaleValsBreath, sizeof(pScaleValsBreath));

    float pFilterCoefs[FIR_FILTER_SIZE] = { 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1 };
    memcpy(obj->pFilterCoefs, pFilterCoefs, FIR_FILTER_SIZE*sizeof(float));

    float DopplerWindowCoefs [DOPPLER_WINDOW_SIZE] =  { 0.0800, 0.0894, 0.1173, 0.1624,
                                                        0.2231, 0.2967, 0.3802, 0.4703,
                                                        0.5633, 0.6553, 0.7426, 0.8216,
                                                        0.8890, 0.9422, 0.9789, 0.9976 };
    memcpy(obj->pDopplerWindow, DopplerWindowCoefs, sizeof(DopplerWindowCoefs));

    obj->radarCube = (uint32_t *) VitalSignsDemo_memPoolAlloc(pool, obj->numRangeBins * obj->numDopplerBins * obj->numVirtualAntennas * sizeof(uint32_t));
    if (obj->radarCube == NULL) {
        System_printf("Failed to allocate radar Cube memory from L3 RAM\n");
    }

    obj->pTempCircularBuff = (float *) VitalSignsDemo_memPoolAlloc(&gMmwL3heap, (obj->circularBufferSizeHeart) * sizeof(float));
    if (obj->pTempCircularBuff == NULL) {
        System_printf( "Failed to allocate Circular Buffer for Cardiac Waveform from L3 RAM\n");
    }

    obj->pRangeProfileClutterRemoved = (float *) VitalSignsDemo_memPoolAlloc(&gMmwL3heap, MAX_NUM_RANGE_BINS_PROCESS * sizeof(float));
    if (obj->pRangeProfileClutterRemoved == NULL) {
        System_printf( "Failed to allocate Circular Buffer for Cardiac Waveform from L3 RAM\n");
    }

    obj->pTempReal_Prev = (float *) VitalSignsDemo_memPoolAlloc(&gMmwL3heap, MAX_NUM_RANGE_BINS_PROCESS * sizeof(float));
    if (obj->pTempReal_Prev == NULL) {
        System_printf( "Failed to allocate Circular Buffer for Cardiac Waveform from L3 RAM\n");
    }

    obj->pTempImag_Prev = (float *) VitalSignsDemo_memPoolAlloc(&gMmwL3heap, MAX_NUM_RANGE_BINS_PROCESS * sizeof(float));
    if (obj->pTempImag_Prev == NULL) {
        System_printf( "Failed to allocate Circular Buffer for Cardiac Waveform from L3 RAM\n");
    }

    obj->pMotionCircularBuffer = (float *) VitalSignsDemo_memPoolAlloc(&gMmwL3heap, obj->motionDetection_BlockSize* sizeof(float));
    if (obj->pMotionCircularBuffer == NULL) {
        System_printf( "Failed to allocate Circular Buffer for Cardiac Waveform from L3 RAM\n");
    }

    obj->pXcorr = (float *) VitalSignsDemo_memPoolAlloc(&gMmwL3heap, XCORR_NUM_LAGS* sizeof(float));
    if (obj->pXcorr == NULL) {
        System_printf( "Failed to allocate Buffer for Auto-correlation function \n");
    }

    obj->pVitalSignsCircularBuff = (int32_t *) VitalSignsDemo_memPoolAlloc(pool, (2*obj->circularBufferSizeHeart + obj->circularBufferSizeBreath)  * sizeof(int32_t));
    if (obj->pVitalSignsCircularBuff == NULL) {
        System_printf( "Failed to allocate Circular Buffer for Vital signs from L3 RAM\n");
    }

    obj->pVitalSigns_Breath_CircularBuffer = (float *) VitalSignsDemo_memPoolAlloc(pool, obj->circularBufferSizeBreath  * sizeof(float));
    if (obj->pVitalSigns_Breath_CircularBuffer == NULL) {
        System_printf( "Failed to allocate Circular Buffer for Breathing Waveform from L3 RAM\n");
    }

    obj->pVitalSignsSpectrum = (uint32_t *) VitalSignsDemo_memPoolAlloc(pool, 1 * PHASE_FFT_SIZE * sizeof(uint32_t));
    if (obj->pVitalSignsSpectrum == NULL) {
        System_printf("Failed to allocate Memory for Vital Signs Spectrum from L3 RAM\n");
    }

    obj->pRangeProfileCplx = (uint16_t *) VitalSignsDemo_memPoolAlloc(pool, (obj->numRangeBins) * sizeof(uint32_t));
    if (obj->pRangeProfileCplx == NULL) {
        System_printf("Failed to allocate memory from L3 RAM\n");
    }
}


/**
 *  @b Description
 *  @n
 *      parses Profile, Chirp and Frame config and extracts parameters
 *      needed for processing chain configuration
 */
bool VitalSignsDemo_parseProfileAndChirpConfig(VitalSignsDemo_DataPathObj *dataPathObj)
{
    uint16_t    frameChirpStartIdx;
    uint16_t    frameChirpEndIdx;
    int16_t     frameTotalChirps;
    int32_t     errCode;
    uint32_t    profileLoopIdx, chirpLoopIdx;
    bool        foundValidProfile = false;
    uint16_t    channelTxEn = gMmwMCB.cfg.openCfg.chCfg.txChannelEn;
    uint8_t     channel;
    uint8_t     numRxAntennas = 0;
    uint8_t     rxAntOrder [SYS_COMMON_NUM_RX_CHANNEL];
    uint8_t     txAntOrder [SYS_COMMON_NUM_TX_ANTENNAS];
    int32_t     i;
    int32_t     txIdx, rxIdx;
    float numTemp, denTemp;

    /* Find number of enabled channels */
    for (channel = 0; channel < SYS_COMMON_NUM_RX_CHANNEL; channel++)
      {
          rxAntOrder[channel] = 0;
          if(gMmwMCB.cfg.openCfg.chCfg.rxChannelEn & (0x1<<channel))
          {
              rxAntOrder[numRxAntennas] = channel;
              /* Track the number of receive channels: */
              numRxAntennas++;
          }
      }
    dataPathObj->numRxAntennas = numRxAntennas;
    dataPathObj->framePeriodicity_ms= (5e-6)*(gMmwMCB.cfg.ctrlCfg.u.frameCfg.frameCfg.framePeriodicity);  // 1 LSB = 5 ns ; (1e3)(5e-9) = 5e-6 ; 1e3 to convert sec to ms

    /* read frameCfg chirp start/stop*/
    frameChirpStartIdx = gMmwMCB.cfg.ctrlCfg.u.frameCfg.frameCfg.chirpStartIdx;
    frameChirpEndIdx = gMmwMCB.cfg.ctrlCfg.u.frameCfg.frameCfg.chirpEndIdx;
    frameTotalChirps = frameChirpEndIdx - frameChirpStartIdx + 1;

      /* loop for profiles and find if it has valid chirps */
      /* we support only one profile in this processing chain */
      for (profileLoopIdx=0;
          ((profileLoopIdx<MMWAVE_MAX_PROFILE)&&(foundValidProfile==false));
          profileLoopIdx++)
      {
          uint32_t    mmWaveNumChirps = 0;
          bool        validProfileHasElevation=false;
          bool        validProfileHasOneTxPerChirp=false;
          uint16_t    validProfileTxEn = 0;
          uint16_t    validChirpTxEnBits[32]={0};
          MMWave_ProfileHandle profileHandle;

          profileHandle = gMmwMCB.cfg.ctrlCfg.u.frameCfg.profileHandle[profileLoopIdx];
          if (profileHandle == NULL)
              continue; /* skip this profile */

          /* get numChirps for this profile; skip error checking */
          MMWave_getNumChirps(profileHandle,&mmWaveNumChirps,&errCode);
          /* loop for chirps and find if it has valid chirps for the frame
             looping around for all chirps in a profile, in case
             there are duplicate chirps
           */
          for (chirpLoopIdx=1;chirpLoopIdx<=mmWaveNumChirps;chirpLoopIdx++)
          {
              MMWave_ChirpHandle chirpHandle;
              /* get handle and read ChirpCfg */
              if (MMWave_getChirpHandle(profileHandle,chirpLoopIdx,&chirpHandle,&errCode)==0)
              {
                  rlChirpCfg_t chirpCfg;
                  if (MMWave_getChirpCfg(chirpHandle,&chirpCfg,&errCode)==0)
                  {
                      uint16_t chirpTxEn = chirpCfg.txEnable;
                      /* do chirps fall in range and has valid antenna enabled */
                      if ((chirpCfg.chirpStartIdx >= frameChirpStartIdx) &&
                          (chirpCfg.chirpEndIdx <= frameChirpEndIdx) &&
                          ((chirpTxEn & channelTxEn) > 0))
                      {
                          uint16_t idx = 0;
                          for (idx=(chirpCfg.chirpStartIdx-frameChirpStartIdx);idx<=(chirpCfg.chirpEndIdx-frameChirpStartIdx);idx++)
                          {
                              validChirpTxEnBits[idx] = chirpTxEn;
                              foundValidProfile = true;
                          }

                      }
                  }
              }
          }
          /* now loop through unique chirps and check if we found all of the ones
             needed for the frame and then determine the azimuth/elevation antenna
             configuration
           */
          if (foundValidProfile) {
              int16_t nonElevFirstChirpIdx = -1;
              for (chirpLoopIdx=0;chirpLoopIdx<frameTotalChirps;chirpLoopIdx++)
              {
                  bool validChirpHasElevation=false;
                  bool validChirpHasOneTxPerChirp=false;
                  uint16_t chirpTxEn = validChirpTxEnBits[chirpLoopIdx];
                  if (chirpTxEn == 0) {
                      /* this profile doesnt have all the needed chirps */
                      foundValidProfile = false;
                      break;
                  }
                  /* check if this is an elevation TX chirp */
                  validChirpHasElevation = (chirpTxEn==0x2);
                  validProfileHasElevation |= validChirpHasElevation;
                  /* if not, then check the MIMO config */
                  if (!validChirpHasElevation)
                  {
                      validChirpHasOneTxPerChirp = ((chirpTxEn==0x1) || (chirpTxEn==0x4));
                      /* if this is the first chirp without elevation, record the chirp's
                         MIMO config as profile's MIMO config. We dont handle intermix
                         at this point */
                      if (nonElevFirstChirpIdx==-1) {
                          validProfileHasOneTxPerChirp = validChirpHasOneTxPerChirp;
                          nonElevFirstChirpIdx = chirpLoopIdx;
                      }
                      /* check the chirp's MIMO config against Profile's MIMO config */
                      if (validChirpHasOneTxPerChirp != validProfileHasOneTxPerChirp)
                      {
                          /* this profile doesnt have all chirps with same MIMO config */
                          foundValidProfile = false;
                          break;
                      }
                  }
                  /* save the antennas actually enabled in this profile */
                  validProfileTxEn |= chirpTxEn;
              }
          }

          /* found valid chirps for the frame; mark this profile valid */
          if (foundValidProfile==true) {
              rlProfileCfg_t  profileCfg;
              uint32_t        numTxAntAzim = 0;
              uint32_t        numTxAntElev = 0;

              dataPathObj->validProfileIdx = profileLoopIdx;
              dataPathObj->numTxAntennas = 0;
              if (validProfileHasElevation)
              {
                  numTxAntElev = 1;
              }
              if (!validProfileHasOneTxPerChirp)
              {
                  numTxAntAzim=1;
              }
              else
              {
                  if (validProfileTxEn & 0x1)
                  {
                      numTxAntAzim++;
                  }
                  if (validProfileTxEn & 0x4)
                  {
                      numTxAntAzim++;
                  }
              }
              /*System_printf("Azimuth Tx: %d (MIMO:%d), Elev Tx:%d\n",
                              numTxAntAzim,validProfileHasMIMO,numTxAntElev);*/
              dataPathObj->numTxAntennas = numTxAntAzim + numTxAntElev;
              dataPathObj->numVirtualAntAzim = numTxAntAzim * dataPathObj->numRxAntennas;
              dataPathObj->numVirtualAntElev = numTxAntElev * dataPathObj->numRxAntennas;
              dataPathObj->numVirtualAntennas = dataPathObj->numVirtualAntAzim + dataPathObj->numVirtualAntElev;

              /* Copy the Rx channel compensation coefficients from common area to data path structure */
              if (validProfileHasOneTxPerChirp)
              {
                  for (i = 0; i < dataPathObj->numTxAntennas; i++)
                  {
                      txAntOrder[i] = log2Approx(validChirpTxEnBits[i]);
                  }
                  for (txIdx = 0; txIdx < dataPathObj->numTxAntennas; txIdx++)
                  {
                      for (rxIdx = 0; rxIdx < dataPathObj->numRxAntennas; rxIdx++)
                      {
                          dataPathObj->compRxChanCfg.rxChPhaseComp[txIdx*dataPathObj->numRxAntennas + rxIdx] =
                          dataPathObj->cliCommonCfg->compRxChanCfg.rxChPhaseComp[txAntOrder[txIdx]*SYS_COMMON_NUM_RX_CHANNEL + rxAntOrder[rxIdx]];

                      }

                  }
              }
              else
              {
                  cmplx16ImRe_t one;
                  one.imag = 0;
                  one.real = 0x7fff;
                  for (txIdx = 0; txIdx < dataPathObj->numTxAntennas; txIdx++)
                  {
                      for (rxIdx = 0; rxIdx < dataPathObj->numRxAntennas; rxIdx++)
                      {
                          dataPathObj->compRxChanCfg.rxChPhaseComp[txIdx*dataPathObj->numRxAntennas + rxIdx] = one;
                      }

                  }
              }

              /* Get the profile configuration: */
              if (MMWave_getProfileCfg(profileHandle,&profileCfg, &errCode) < 0)
              {
                  MmwDemo_debugAssert(0);
                  return false;
              }

  #ifndef MMW_ENABLE_NEGATIVE_FREQ_SLOPE
              /* Check frequency slope */
              if (profileCfg.freqSlopeConst < 0)
              {
                  System_printf("Frequency slope must be positive\n");
                  MmwDemo_debugAssert(0);
              }
  #endif

              dataPathObj->numAdcSamples = profileCfg.numAdcSamples;
              dataPathObj->numRangeBins = pow2roundup(dataPathObj->numAdcSamples);
              dataPathObj->numChirpsPerFrame = frameTotalChirps *
                                                gMmwMCB.cfg.ctrlCfg.u.frameCfg.frameCfg.numLoops;

              dataPathObj->numDopplerBins = dataPathObj->numChirpsPerFrame/dataPathObj->numTxAntennas;
              dataPathObj->rangeResolution = SPEED_OF_LIGHT_IN_METERS_PER_SEC * profileCfg.digOutSampleRate * 1e3 /
                      (2 * profileCfg.freqSlopeConst * ((3.6*1e3*900)/(1U << 26)) * 1e12 * dataPathObj->numRangeBins);

              dataPathObj->chirpDuration_us   = (1e3*profileCfg.numAdcSamples)/(profileCfg.digOutSampleRate);
              dataPathObj->chirpBandwidth_kHz = (48*profileCfg.freqSlopeConst)*(dataPathObj->chirpDuration_us );

              numTemp = (float) (dataPathObj->chirpDuration_us)*(profileCfg.digOutSampleRate)*SPEED_OF_LIGHT_IN_METERS_PER_SEC ;
              denTemp = (float)  2*(dataPathObj->chirpBandwidth_kHz);
              dataPathObj->rangeMaximum =  (numTemp/(denTemp*1e6));  //  Rmax = (T_chirp*fbeatMax*C)/(2*Bandwidth);
              dataPathObj->rangeBinSize_meter = (dataPathObj->rangeMaximum)/(dataPathObj->numRangeBins);

              dataPathObj->frameStartIntCounter = 0;
              dataPathObj->interFrameProcToken = 0;
          }
      }
      return foundValidProfile;
}


/**
 *  @b Description
 *  @n
 *  This function is called at the init time from @ref VitalSignsDemo_initTask.
 *  It initializes drivers: ADCBUF, HWA, EDMA, and semaphores used
 *  by  @ref VitalSignsDemo_dataPathTask
 *  @retval
 *      Not Applicable.
 */
void VitalSignsDemo_dataPathInit(VitalSignsDemo_DataPathObj *obj)
{

    /* Initialize the ADCBUF */
    ADCBuf_init();

    /* Initialize the HWA */
    HWA_init();

    /* Debug Message: */
    System_printf("Debug: HWA has been initialized\n");

    edmaInit(obj);
}

void VitalSignsDemo_dataPathOpen(VitalSignsDemo_DataPathObj *obj)
{
    /*****************************************************************************
     * Start HWA, EDMA and ADCBUF drivers:
     *****************************************************************************/
    MmwDemo_hwaOpen(obj, gMmwMCB.socHandle);
    MmwDemo_edmaOpen(obj);
    MmwDemo_ADCBufOpen(obj);
}


/**
 *  @b Description
 *  @n
 *      The task is used to provide an execution context for the mmWave
 *      control task
 *
 *  @retval
 *      Not Applicable.
 */
void VitalSignsDemo_mmWaveCtrlTask(UArg arg0, UArg arg1) {
    int32_t errCode;

    while (1) {
        /* Execute the mmWave control module: */
        if (MMWave_execute(gMmwMCB.ctrlHandle, &errCode) < 0)
            System_printf(
                    "Error: mmWave control execution failed [Error code %d]\n",
                    errCode);
    }
}

/**
 *  @b Description
 *  @n
 *      Registered event function to mmwave which is invoked when an event from the
 *      BSS is received.
 *
 *  @param[in]  msgId
 *      Message Identifier
 *  @param[in]  sbId
 *      Subblock identifier
 *  @param[in]  sbLen
 *      Length of the subblock
 *  @param[in]  payload
 *      Pointer to the payload buffer
 *
 *  @retval
 *      Always return 0
 */
int32_t MmwDemo_eventCallbackFxn(uint16_t msgId, uint16_t sbId, uint16_t sbLen, uint8_t *payload)
{
    uint16_t asyncSB = RL_GET_SBID_FROM_UNIQ_SBID(sbId);

    /* Process the received message: */
    switch (msgId)
    {
        case RL_RF_ASYNC_EVENT_MSG:
        {
            /* Received Asychronous Message: */
            switch (asyncSB)
            {
                case RL_RF_AE_CPUFAULT_SB:
                {
                    MmwDemo_debugAssert(0);
                    break;
                }
                case RL_RF_AE_ESMFAULT_SB:
                {
                    MmwDemo_debugAssert(0);
                    break;
                }
                case RL_RF_AE_INITCALIBSTATUS_SB:
                {
                    rlRfInitComplete_t*  ptrRFInitCompleteMessage;
                    uint32_t            calibrationStatus;

                    /* Get the RF-Init completion message: */
                    ptrRFInitCompleteMessage = (rlRfInitComplete_t*)payload;
                    calibrationStatus = ptrRFInitCompleteMessage->calibStatus & 0xFFFU;

                    /* Display the calibration status: */
                    CLI_write ("Debug: Init Calibration Status = 0x%x\n", calibrationStatus);
                    break;
                }
                case RL_RF_AE_FRAME_TRIGGER_RDY_SB:
                {
                    gMmwMCB.stats.frameTriggerReady++;
                    break;
                }
                case RL_RF_AE_MON_TIMING_FAIL_REPORT_SB:
                {
                    gMmwMCB.stats.failedTimingReports++;
                    break;
                }
                case RL_RF_AE_RUN_TIME_CALIB_REPORT_SB:
                {
                    gMmwMCB.stats.calibrationReports++;
                    break;
                }
                case RL_RF_AE_FRAME_END_SB:
                {
                    /*Received Frame Stop async event from BSS.
                      No action required.*/
                    break;
                }
                default:
                {
                    System_printf ("Error: Asynchronous Event SB Id %d not handled\n", asyncSB);
                    break;
                }
            }
            break;
        }
        default:
        {
            System_printf ("Error: Asynchronous message %d is NOT handled\n", msgId);
            break;
        }
    }
    return 0;
}

/**
 *  @b Description
 *  @n
 *      The task is used for data path processing and to transmit the
 *      detected objects through the UART output port.
 *
 *  @retval
 *      Not Applicable.
 */
void VitalSignsDemo_dataPathTask(UArg arg0, UArg arg1) {
    VitalSignsDemo_DataPathObj *dataPathObj = &gMmwMCB.dataPathObj;
    uint32_t startTime, endTime, transmitOutStartTime;

    while (1) {
        gFrameCount = gFrameCount + 1;
        Semaphore_pend(dataPathObj->frameStart_semHandle, BIOS_WAIT_FOREVER);
        Load_update();
        dataPathObj->timingInfo.interFrameCPULoad=Load_getCPULoad();
        startTime = Pmu_getCount(0);

        VitalSignsDemo_dataPathWait1D(dataPathObj);
        /* 1st Dimension FFT done! */
        VitalSignsDemo_configPhase_HWA(dataPathObj);

        transmitOutStartTime = Pmu_getCount(0);
        Load_update();
        dataPathObj->timingInfo.activeFrameCPULoad=Load_getCPULoad();

        /* Starting the vital Signs Processing */
        VitalSignsDemo_vitalSignProcess(dataPathObj);
        VitalSignsDemo_transmitProcessedOutput(gMmwMCB.loggingUartHandle,dataPathObj);

        /* Processing cycles for vital signs processing including the sending out of data */
        dataPathObj->transmitOutputProcCycles = Pmu_getCount(0) - transmitOutStartTime;

        /* Prepare for next frame */
        VitalSignsDemo_config1D_HWA(dataPathObj);
        VitalSignsDemo_dataPathTrigger1D(dataPathObj);

        /* Inter-Frame Processing cycles including sending out of data */
        dataPathObj->timingInfo.interFrameProcessingEndTime = Pmu_getCount(0);
        endTime = dataPathObj->timingInfo.interFrameProcessingEndTime;
        dataPathObj->timingInfo.interFrameProcCycles = endTime - startTime;
        dataPathObj->interFrameProcToken--;
    }
}



/**
 *  @b Description
 *  @n
 *      Frame start interrupt handler
 *
 *  @retval
 *      Not Applicable.
 */
static void MmwDemo_frameStartIntHandler(uintptr_t arg)
{
    VitalSignsDemo_DataPathObj * dpObj = &gMmwMCB.dataPathObj;

    /* Increment interrupt counter for debugging purpose */
    dpObj->frameStartIntCounter++;

    /* Note: this is valid after the first frame */
    dpObj->timingInfo.interFrameProcessingEndMargin =
            Pmu_getCount(0) - dpObj->timingInfo.interFrameProcessingEndTime;

    /* Check if previous chirp processing has completed */
    MmwDemo_debugAssert(dpObj->interFrameProcToken == 0);
    dpObj->interFrameProcToken++;

    Semaphore_post(dpObj->frameStart_semHandle);

}


/**
 *  @b Description
 *  @n
 *      System Initialization Task which initializes the various
 *      components in the system.
 *
 *  @retval
 *      Not Applicable.
 */
void VitalSignsDemo_initTask(UArg arg0, UArg arg1) {
    int32_t errCode;
    MMWave_InitCfg initCfg;
    UART_Params uartParams;
    Task_Params taskParams;

    /* Debug Message: */
    System_printf("Debug: Launched the Initialization Task\n");

    /*****************************************************************************
     * Initialize the mmWave SDK components:
     *****************************************************************************/

    /* Initialize the UART */
    UART_init();

    /* Initialize the Mailbox */
    Mailbox_init (MAILBOX_TYPE_MSS);

    /* Initialize the GPIO */
    GPIO_init ();

    /* Initialize the Data Path: */
    VitalSignsDemo_dataPathInit(&gMmwMCB.dataPathObj);

    /*****************************************************************************
     * Open & configure the drivers:
     *****************************************************************************/

    /* Setup the PINMUX to bring out the UART-1 */
    Pinmux_Set_OverrideCtrl(SOC_XWR14XX_PINN6_PADBE, PINMUX_OUTEN_RETAIN_HW_CTRL, PINMUX_INPEN_RETAIN_HW_CTRL);
    Pinmux_Set_FuncSel(SOC_XWR14XX_PINN6_PADBE, SOC_XWR14XX_PINN6_PADBE_MSS_UARTA_TX);
    Pinmux_Set_OverrideCtrl(SOC_XWR14XX_PINN5_PADBD, PINMUX_OUTEN_RETAIN_HW_CTRL, PINMUX_INPEN_RETAIN_HW_CTRL);
    Pinmux_Set_FuncSel(SOC_XWR14XX_PINN5_PADBD, SOC_XWR14XX_PINN5_PADBD_MSS_UARTA_RX);

    /* Setup the default UART Parameters */
    UART_Params_init(&uartParams);
    uartParams.clockFrequency   = gMmwMCB.cfg.sysClockFrequency;
    uartParams.baudRate         = gMmwMCB.cfg.commandBaudRate;
    uartParams.isPinMuxDone     = 1;

    /* Open the UART Instance */
    gMmwMCB.commandUartHandle = UART_open(0, &uartParams);
    if (gMmwMCB.commandUartHandle == NULL) {
        System_printf("Error: Unable to open the Command UART Instance\n");
        return;
    }
    System_printf("Debug: UART Instance %p has been opened successfully\n",
            gMmwMCB.commandUartHandle);

    /* Setup the default UART Parameters */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode  = UART_DATA_BINARY;
    uartParams.readDataMode   = UART_DATA_BINARY;
    uartParams.clockFrequency = gMmwMCB.cfg.sysClockFrequency;
    uartParams.baudRate       = gMmwMCB.cfg.loggingBaudRate;
    uartParams.isPinMuxDone   = 0;

    /* Open the Logging UART Instance: */
    gMmwMCB.loggingUartHandle = UART_open(1, &uartParams);
    if (gMmwMCB.loggingUartHandle == NULL) {
        System_printf("Error: Unable to open the Logging UART Instance\n");
        return;
    }
    System_printf("Debug: UART Instance %p has been opened successfully\n",
            gMmwMCB.loggingUartHandle);

    /*****************************************************************************
     * mmWave: Initialization of the high level module
     *****************************************************************************/

    /* Initialize the mmWave control init configuration */
    memset((void*) &initCfg, 0, sizeof(MMWave_InitCfg));

    /* Populate the init configuration: */
    initCfg.domain = MMWave_Domain_MSS;
    initCfg.socHandle = gMmwMCB.socHandle;
    initCfg.eventFxn                = MmwDemo_eventCallbackFxn;
    initCfg.linkCRCCfg.useCRCDriver = 1U;
    initCfg.linkCRCCfg.crcChannel = CRC_Channel_CH1;
    initCfg.cfgMode               = MMWave_ConfigurationMode_FULL;

    /* Initialize and setup the mmWave Control module */
    gMmwMCB.ctrlHandle = MMWave_init(&initCfg, &errCode);
    if (gMmwMCB.ctrlHandle == NULL) {
        /* Error: Unable to initialize the mmWave control module */
        System_printf(
                "Error: mmWave Control Initialization failed [Error code %d]\n",
                errCode);
        return;
    }
    System_printf("Debug: mmWave Control Initialization was successful\n");

    /* Synchronization: This will synchronize the execution of the control module
     * between the domains. This is a prerequiste and always needs to be invoked. */
    if (MMWave_sync(gMmwMCB.ctrlHandle, &errCode) < 0) {
        /* Error: Unable to synchronize the mmWave control module */
        System_printf(
                "Error: mmWave Control Synchronization failed [Error code %d]\n",
                errCode);
        return;
    }
    System_printf("Debug: mmWave Control Synchronization was successful\n");

    VitalSignsDemo_dataPathOpen(&gMmwMCB.dataPathObj);

    /* Configure banchmark counter */
    Pmu_configureCounter(0, 0x11, FALSE);
    Pmu_startCounter(0);

    /*****************************************************************************
     * Launch the mmWave control execution task
     * - This should have a higher priroity than any other task which uses the
     *   mmWave control API
     *****************************************************************************/
    Task_Params_init(&taskParams);
    taskParams.priority = 5;
    taskParams.stackSize = 3*1024;
    Task_create(VitalSignsDemo_mmWaveCtrlTask, &taskParams, NULL);

    /*****************************************************************************
     * Initialize the CLI Module:
     *****************************************************************************/
    VitalSignsDemo_CLIInit();

    /*****************************************************************************
     * Initialize the Sensor Management Module:
     *****************************************************************************/
    if (MmwDemo_sensorMgmtInit() < 0)
        return;

    /* Register Frame start interrupt handler */
    {
        SOC_SysIntListenerCfg  socIntCfg;
        int32_t errCode;

        Semaphore_Params       semParams;

        /* Register frame start interrupt listener */
        socIntCfg.systemInterrupt  = SOC_XWR14XX_DSS_FRAME_START_IRQ;
        socIntCfg.listenerFxn      = MmwDemo_frameStartIntHandler;
        socIntCfg.arg              = (uintptr_t)NULL;
        if (SOC_registerSysIntListener(gMmwMCB.socHandle, &socIntCfg, &errCode) == NULL)
        {
            System_printf("Error: Unable to register frame start interrupt listener , error = %d\n", errCode);
            return;
        }

        Semaphore_Params_init(&semParams);
        semParams.mode = Semaphore_Mode_BINARY;
        gMmwMCB.dataPathObj.frameStart_semHandle = Semaphore_create(0, &semParams, NULL);
    }

    /*****************************************************************************
     * Launch the Main task
     * - The main demo task
     *****************************************************************************/
    Task_Params_init(&taskParams);
    taskParams.priority = 4;
    taskParams.stackSize = 3*1024;
    Task_create(VitalSignsDemo_dataPathTask, &taskParams, NULL);

    return;
}

/**
 *  @b Description
 *  @n
 *     Function to sleep the R4F using WFI (Wait For Interrupt) instruction.
 *     When R4F has no work left to do,
 *     the BIOS will be in Idle thread and will call this function. The R4F will
 *     wake-up on any interrupt (e.g chirp interrupt).
 *
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_sleep(void)
{
    /* issue WFI (Wait For Interrupt) instruction */
    asm(" WFI ");
}

/**
 *  @b Description
 *  @n
 *      Entry point into the Vital Signs Measurement Demo
 *
 *  @retval
 *      Not Applicable.
 */
int main(void) {
    Task_Params taskParams;
    int32_t errCode;
    SOC_Handle socHandle;
    SOC_Cfg    socCfg;

    /* Initialize the ESM: */
    ESM_init(0U); //dont clear errors as TI RTOS does it

    /* Initialize the SOC configuration: */
    memset ((void *)&socCfg, 0, sizeof(SOC_Cfg));

    /* Populate the SOC configuration: */
    socCfg.clockCfg = SOC_SysClock_INIT;

    /* Initialize the SOC Module: This is done as soon as the application is started
     * to ensure that the MPU is correctly configured. */
    socHandle = SOC_init( &socCfg,&errCode);
    if (socHandle == NULL) {
        System_printf(
                "Error: SOC Module Initialization failed [Error code %d]\n",
                errCode);
        return -1;
    }

    /* Initialize and populate the demo MCB */
    memset((void*) &gMmwMCB, 0, sizeof(VitalSignsDemo_MCB));

    gMmwMCB.socHandle = socHandle;

    /* Initialize the DEMO configuration: */
    gMmwMCB.cfg.sysClockFrequency = (200 * 1000000);
    gMmwMCB.cfg.loggingBaudRate = 921600;
    gMmwMCB.cfg.commandBaudRate = 115200;

    /* Debug Message: */
    System_printf("**********************************************\n");
    System_printf("Debug: Launching the Vital Signs Measurements Demo\n");
    System_printf("**********************************************\n");

    /* Initialize the Task Parameters. */
    Task_Params_init(&taskParams);
    Task_create(VitalSignsDemo_initTask, &taskParams, NULL);

    /* Start BIOS */
    BIOS_start();
    return 0;
}





