/**
 *   @file  config_hwa_util.c
 *
 *   @brief
 *      Hardware accelerator Configuration Utility API implementation.
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

/* Standard Include Files. */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* BIOS/XDC Include Files. */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/drivers/hwa/hwa.h>

#include "config_hwa_util.h"

uint32_t log2Approx(uint32_t x)
{
    uint32_t idx, detectFlag = 0;

    idx = 32U;
    while((detectFlag==0U) || (idx==0U))
    {
        if(x & 0x80000000U)
        {
            detectFlag = 1;
        }
        x <<= 1U;
        idx--;
    }

    if(x != 0)
    {
        idx = idx + 1;
    }

    return(idx);
}

void HWAutil_configRangeFFT(HWA_Handle handle,
                            uint32_t  paramSetStartIdx,
                            uint32_t numAdcSamples,
                            uint32_t numRangeBins,
                            uint8_t numRxAnt,
                            uint32_t windowOffsetBytes,
                            uint8_t dmaTriggerSourcePing,
                            uint8_t dmaTriggerSourcePong,
                            uint8_t dmaDestChannelPing,
                            uint8_t dmaDestChannelPong,
                            uint16_t hwaMemAdcBufOffset,
                            uint16_t hwaMemDestPingOffset,
                            uint16_t hwaMemDestPongOffset)
{
    HWA_InterruptConfig     paramISRConfig;
    int32_t errCode = 0;
    uint32_t paramsetIdx = paramSetStartIdx;
    uint32_t pingParamSetIdx = 0;
    HWA_ParamConfig hwaParamCfg[HWAUTIL_NUM_PARAM_SETS_1D];
    memset(hwaParamCfg,0,sizeof(hwaParamCfg));

    /***********************/
    /* PING DUMMY PARAMSET */
    /***********************/
    hwaParamCfg[paramsetIdx].triggerMode = HWA_TRIG_MODE_DMA; //Software triggered  - in demo this will be HWA_TRIG_MODE_DMA
    hwaParamCfg[paramsetIdx].dmaTriggerSrc = dmaTriggerSourcePing; //in demo this will be first EDMA Src channel id
    hwaParamCfg[paramsetIdx].accelMode = HWA_ACCELMODE_NONE; //dummy
    errCode = HWA_configParamSet(handle,paramsetIdx,&hwaParamCfg[paramsetIdx],NULL);
    if (errCode != 0)
    {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_configParamSet(%d) returned %d  in function HWAutil_configRangeFFT \n", paramsetIdx, errCode);
        return;
    }

    /***********************/
    /* PING PROCESS PARAMSET */
    /***********************/
    paramsetIdx++;
    pingParamSetIdx = paramsetIdx;
    hwaParamCfg[paramsetIdx].triggerMode = HWA_TRIG_MODE_DFE;
    hwaParamCfg[paramsetIdx].accelMode = HWA_ACCELMODE_FFT; //do FFT

    hwaParamCfg[paramsetIdx].source.srcAddr = hwaMemAdcBufOffset; // address is relative to start of MEM0
    hwaParamCfg[paramsetIdx].source.srcAcnt = numAdcSamples - 1; //this is samples - 1
    hwaParamCfg[paramsetIdx].source.srcAIdx = numRxAnt * sizeof(uint32_t); // 16 bytes
    hwaParamCfg[paramsetIdx].source.srcBcnt = numRxAnt-1; //no iterations here
    hwaParamCfg[paramsetIdx].source.srcBIdx = sizeof(uint32_t); //should be dont care
    hwaParamCfg[paramsetIdx].source.srcShift = 0; //no shift
    hwaParamCfg[paramsetIdx].source.srcCircShiftWrap = 0; //no shift
    hwaParamCfg[paramsetIdx].source.srcRealComplex = HWA_SAMPLES_FORMAT_COMPLEX; //complex data
    hwaParamCfg[paramsetIdx].source.srcWidth = HWA_SAMPLES_WIDTH_16BIT; //16-bit
    hwaParamCfg[paramsetIdx].source.srcSign = HWA_SAMPLES_SIGNED; //signed
    hwaParamCfg[paramsetIdx].source.srcConjugate = 0; //no conjugate
    hwaParamCfg[paramsetIdx].source.srcScale = 8;
    hwaParamCfg[paramsetIdx].source.bpmEnable = 0; //bpm removal not enabled
    hwaParamCfg[paramsetIdx].source.bpmPhase = 0; //dont care

    hwaParamCfg[paramsetIdx].dest.dstAddr = hwaMemDestPingOffset; // address is relative to start of MEM0
    hwaParamCfg[paramsetIdx].dest.dstAcnt = numRangeBins-1; //this is samples - 1
    hwaParamCfg[paramsetIdx].dest.dstAIdx = numRxAnt * sizeof(uint32_t); //
    hwaParamCfg[paramsetIdx].dest.dstBIdx = sizeof(uint32_t); //should be dont care
    hwaParamCfg[paramsetIdx].dest.dstRealComplex = HWA_SAMPLES_FORMAT_COMPLEX; //same as input - complex
    hwaParamCfg[paramsetIdx].dest.dstWidth = HWA_SAMPLES_WIDTH_16BIT; //same as input - 16 bit
    hwaParamCfg[paramsetIdx].dest.dstSign = HWA_SAMPLES_SIGNED; //same as input - signed
    hwaParamCfg[paramsetIdx].dest.dstConjugate = 0; //no conjugate
    hwaParamCfg[paramsetIdx].dest.dstScale = 0;
    hwaParamCfg[paramsetIdx].dest.dstSkipInit = 0; // no skipping

    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftEn = 1;
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftSize = log2Approx(numRangeBins);
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.butterflyScaling = 0x3; //LSB fftSize bits are relevant - revisit this for all FFT size and data size
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.interfZeroOutEn = 0; //disabled
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.windowEn = 1; //enabled
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.windowStart = windowOffsetBytes; //start of window RAM
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.winSymm = 0; //non-symmetric - in demo do we make this symmetric
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.winInterpolateMode = 0; //fftsize is less than 1K
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_LOG2_DISABLED; //disabled
    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT; // output FFT samples

    hwaParamCfg[paramsetIdx].complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
    errCode = HWA_configParamSet(handle,paramsetIdx,&hwaParamCfg[paramsetIdx],NULL);
    if (errCode != 0)
    {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_configParamSet(%d) returned %d  in function HWAutil_configRangeFFT\n",errCode,paramsetIdx);
        return;
    }
    /* enable the DMA hookup to this paramset so that data gets copied out */
    paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
    paramISRConfig.dma.dstChannel = dmaDestChannelPing;  //TODO sync this define EDMA channel to trigger to copy the data out
    //paramISRConfig.cpu.callbackArg = paramSetSem;//TODO check if NULL is required
    errCode = HWA_enableParamSetInterrupt(handle,paramsetIdx,&paramISRConfig);
    if (errCode != 0)
    {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_enableParamSetInterrupt(PING DMA) returned %d\n",errCode);
        return;
    }

    /***********************/
    /* PONG DUMMY PARAMSET */
    /***********************/
    paramsetIdx++;
    hwaParamCfg[paramsetIdx].triggerMode = HWA_TRIG_MODE_DMA;
    hwaParamCfg[paramsetIdx].dmaTriggerSrc = dmaTriggerSourcePong; //in demo this will be second EDMA Src channel id
    hwaParamCfg[paramsetIdx].accelMode = HWA_ACCELMODE_NONE; //dummy
    errCode = HWA_configParamSet(handle,paramsetIdx,&hwaParamCfg[paramsetIdx],NULL);
    if (errCode != 0)
    {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_configParamSet(%d) returned %d\n",errCode,paramsetIdx);
        return;
    }

    /***********************/
    /* PONG PROCESS PARAMSET */
    /***********************/
    paramsetIdx++;
    hwaParamCfg[paramsetIdx] = hwaParamCfg[pingParamSetIdx];
    hwaParamCfg[paramsetIdx].dest.dstAddr = hwaMemDestPongOffset;
    errCode = HWA_configParamSet(handle,paramsetIdx,&hwaParamCfg[paramsetIdx],NULL);
    if (errCode != 0)
    {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_configParamSet(%d) returned %d  in function HWAutil_configRangeFFT \n",errCode,paramsetIdx);
        return;
    }
    /* enable the DMA hookup to this paramset so that data gets copied out */
    paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
    paramISRConfig.dma.dstChannel = dmaDestChannelPong;
    //paramISRConfig.cpu.callbackArg = paramSetSem;//TODO check if NULL is required
    errCode = HWA_enableParamSetInterrupt(handle,paramsetIdx,&paramISRConfig);
    if (errCode != 0)
    {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_enableParamSetInterrupt(PING DMA) returned %d\n",errCode);
        return;
    }

}

//      Configuruation for Vital Signs FFT
//      Paramset for both the Breathing and HeartRate waveforms  
//      Input  : 32 bits, signed,  Real Samples
//      Output : 32 bits, Magnitude enabled
//      Don't need a Ping Pong Buffer

void HWAutil_configVitalSignsFFT(HWA_Handle handle,
                                uint32_t  paramSetStartIdx,
                                uint32_t numInputSamplesBreath,
                                uint32_t numInputSamplesHeart,
                                uint32_t fftsize,
                                uint32_t numOutputSamples,
                                uint8_t numVirtualAnt,
                                uint32_t numRangeBinsPerIter,
                                uint32_t windowOffsetBytes,
                                uint8_t dmaTriggerSourcePing,
                                uint8_t dmaDestChannelPing,
                                uint16_t hwaMemSourcePingOffset,
                                uint16_t hwaMemDestPingOffset
                                )
{
	
	  HWA_ParamConfig hwaParamCfg[3];    // For both the Breathing and the HeartRate waveforms
	  uint32_t paramsetIdx = 0;
	  int32_t errCode = 0;
	  HWA_InterruptConfig     paramISRConfig;

	  memset( (void*) &hwaParamCfg[paramsetIdx], 0, sizeof(HWA_ParamConfig));
  
	  hwaParamCfg[paramsetIdx].triggerMode = HWA_TRIG_MODE_DMA;           // The DMA triggers the HWA 
	  hwaParamCfg[paramsetIdx].dmaTriggerSrc = dmaTriggerSourcePing;      // The Source of the trigger
	  hwaParamCfg[paramsetIdx].accelMode = HWA_ACCELMODE_FFT;             //do FFT

	  hwaParamCfg[paramsetIdx].source.srcAddr = hwaMemSourcePingOffset; // address is relative to start of MEM0
	  hwaParamCfg[paramsetIdx].source.srcAcnt = numInputSamplesBreath - 1; //size in samples - 1

	  hwaParamCfg[paramsetIdx].source.srcAIdx = sizeof(uint32_t); //
	  hwaParamCfg[paramsetIdx].source.srcBcnt = 0; //no iterations here
	  hwaParamCfg[paramsetIdx].source.srcBIdx = sizeof(uint32_t); //should be dont care
	  hwaParamCfg[paramsetIdx].source.srcShift = 0; //no shift
	  hwaParamCfg[paramsetIdx].source.srcCircShiftWrap = 0; //no shift
	  hwaParamCfg[paramsetIdx].source.srcRealComplex = 1; //Real data
	  hwaParamCfg[paramsetIdx].source.srcWidth = HWA_SAMPLES_WIDTH_32BIT; //32-bit
	  hwaParamCfg[paramsetIdx].source.srcSign = HWA_SAMPLES_SIGNED; //signed
	  hwaParamCfg[paramsetIdx].source.srcConjugate = 0; //no conjugate
	  hwaParamCfg[paramsetIdx].source.srcScale = 0;
	  hwaParamCfg[paramsetIdx].source.bpmEnable = 0; //bpm removal not enabled
	  hwaParamCfg[paramsetIdx].source.bpmPhase = 0; //dont care

	  hwaParamCfg[paramsetIdx].dest.dstAddr = hwaMemDestPingOffset; // address is relative to start of MEM0
	  hwaParamCfg[paramsetIdx].dest.dstAcnt = numOutputSamples - 1; //this is samples - 1
	    hwaParamCfg[paramsetIdx].dest.dstAIdx = sizeof(uint32_t); //
	    hwaParamCfg[paramsetIdx].dest.dstBIdx = sizeof(uint32_t); //should be dont care
	    hwaParamCfg[paramsetIdx].dest.dstRealComplex = 1; // Real Data
	    hwaParamCfg[paramsetIdx].dest.dstWidth = HWA_SAMPLES_WIDTH_32BIT; //same as input - 32 bit
	    hwaParamCfg[paramsetIdx].dest.dstSign = HWA_SAMPLES_UNSIGNED;//HWA_SAMPLES_SIGNED; //same as input - signed
	    hwaParamCfg[paramsetIdx].dest.dstConjugate = 0; //no conjugate
	    hwaParamCfg[paramsetIdx].dest.dstScale = 8;
	    hwaParamCfg[paramsetIdx].dest.dstSkipInit = 0; // no skipping

	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftEn = 1;
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftSize = log2Approx(fftsize);
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.butterflyScaling = 0;//0x3FF; //LSB fftSize bits are relevant - revisit this for all FFT size and data size
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.interfZeroOutEn = 0; //disabled
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.windowEn = ENABLE_PHASE_WINDOW; //enabled
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.windowStart = windowOffsetBytes; //start of window RAM
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.winSymm = 1; //non-symmetric - in demo do we make this symmetric
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.winInterpolateMode = 0; //fftsize is less than 1K
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.magLogEn = HWA_FFT_MODE_MAGNITUDE_ONLY_ENABLED ; //
	    hwaParamCfg[paramsetIdx].accelModeArgs.fftMode.fftOutMode = HWA_FFT_MODE_OUTPUT_DEFAULT; // output FFT samples

	    hwaParamCfg[paramsetIdx].complexMultiply.mode = HWA_COMPLEX_MULTIPLY_MODE_DISABLE;
	    errCode = HWA_configParamSet(handle, paramSetStartIdx+paramsetIdx, &hwaParamCfg[paramsetIdx], NULL);
	    if (errCode != 0)
	    {
	        //retCode = HWA_TEST_ERROR;
	        System_printf("Error: HWA_configParamSet(%d) returned %d   in function HWAutil_configVitalSignsFFT \n", paramSetStartIdx+paramsetIdx, errCode);
	        return;
	    }
	
	    
	    // FFT
	    for (paramsetIdx = 1; paramsetIdx < numRangeBinsPerIter; paramsetIdx++)
	    {
	        hwaParamCfg[paramsetIdx] = hwaParamCfg[paramsetIdx-1];
	        hwaParamCfg[paramsetIdx].source.srcAddr += sizeof(int32_t) * numVirtualAnt * numInputSamplesBreath;
	  	    hwaParamCfg[paramsetIdx].source.srcAcnt = numInputSamplesHeart - 1; //size in samples - 1
	        hwaParamCfg[paramsetIdx].dest.dstAddr   += sizeof(uint32_t) * numVirtualAnt * numOutputSamples;
	        hwaParamCfg[paramsetIdx].triggerMode = HWA_TRIG_MODE_IMMEDIATE;

	        errCode = HWA_configParamSet(handle, paramSetStartIdx+paramsetIdx, &hwaParamCfg[paramsetIdx], NULL);
	        if (errCode != 0)
	        {
	            //retCode = HWA_TEST_ERROR;
	            System_printf("Error: HWA_configParamSet(%d) returned %d   in function HWAutil_configVitalSignsFFT \n", paramSetStartIdx+paramsetIdx, errCode);
	            return;
	        }
	    }

	    /* Enable the DMA hookup to the last paramset */
	    paramISRConfig.interruptTypeFlag = HWA_PARAMDONE_INTERRUPT_TYPE_DMA;
	    paramISRConfig.dma.dstChannel = dmaDestChannelPing;  //EDMA channel to trigger to copy the data out
	    paramISRConfig.cpu.callbackArg = NULL;
	    errCode = HWA_enableParamSetInterrupt(handle, paramSetStartIdx + paramsetIdx, &paramISRConfig);  
	    if (errCode != 0)
	    {
	        //retCode = HWA_TEST_ERROR;
	        System_printf("Error: HWA_enableParamSetInterrupt(PONG DMA) returned %d\n",errCode);
	        return;
	    }
	
}



