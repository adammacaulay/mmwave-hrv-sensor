/**
 *   @file  config_hwa_util.h
 *
 *   @brief
 *      Hardware Accelerator Configuration Utility APIs.
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


#ifndef CONFIG_HWA_PARAMS_H
#define CONFIG_HWA_PARAMS_H

#include <ti/drivers/hwa/hwa.h>

#define HWAUTIL_NUM_PARAM_SETS_1D   4
#define HWAUTIL_NUM_PARAM_SETS_2D   8
#define ENABLE_PHASE_WINDOW         0


#ifdef __cplusplus
extern "C" {
#endif

/** @brief Configures ParameterSet for Range FFT on ADC samples from DFE
 *
 *   @param[in] handle                  HWA driver handle
 *
 *   @param[in] paramSetStartIdx        HWA parameter set start index
 *
 *   @param[in] numAdcSamples           Number of ADC samples
 *
 *   @param[in] numRangeBins            Number of range bins (1st D FFT size)
 *
 *   @param[in] numRxAnt                Number of Rx antennas
 *
 *   @param[in] windowOffsetBytes       Window offset in bytes (for pre-FFT windowing)
 *
 *   @param[in] dmaTriggerSourcePing    DMA trigger source channel for Ping param sets
 *
 *   @param[in] dmaTriggerSourcePong    DMA trigger source channel for Pong param sets
 *
 *   @param[in] dmaDestChannelPing      DMA destination channel for Ping param sets
 *
 *   @param[in] dmaDestChannelPong      DMA destination channel for Pong param sets
 *
 *   @param[in] hwaMemAdcBufOffset      HWA memory offset ADC buffer
 *
 *   @param[in] hwaMemDestPingOffset    HWA memory offset for Ping destination
 *
 *   @param[in] hwaMemDestPongOffset    HWA memory offset for Pong destination
 *
 */
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
                            uint16_t hwaMemDestPongOffset);

/** @brief Configures ParameterSet for Phase FFT on the samples in the L3 memory
 *
 *   @param[in] handle                  HWA driver handle
 *
 *   @param[in] paramSetStartIdx        HWA parameter set start index
 *
 *   @param[in] numInputSamplesBreath   Number of Input samples for the Breathing Waveform
 *
 *   @param[in] numInputSamplesHeart    Number of Input sample for the Heart Rate Waveform
 *   
 *   @param[in] fftsize                 FFT size (should be equal to or greater than the numInputSamples)
 *   
 *   @param[in] numOutputSamples        Number of Output samples (should be equal to or less than the FFT size) 
 *   
 *   @param[in] numVirtualAnt           Number of Rx virtual antennas
 *
 *   @param[in] numRangeBinsPerIter     Number of range bins per iteration (currently has to be 2)
 *
 *   @param[in] windowOffsetBytes       Window offset in bytes (for pre-FFT windowing)
 *
 *   @param[in] dmaTriggerSourcePing    DMA trigger source channel for Ping param
 *                                      set (Ping param set calculates 2nd D FFT
 *                                      for two rows, (even pair of rows))
 *
 *
 *   @param[in] dmaDestChannelPing      DMA destination channel for Ping param sets
 *
 *
 *   @param[in] hwaMemSourcePingOffset  HWA memory offset with Ping input data,
 *                                      two rows (range bins) of 1st D FFT symbols
 *
 *
 *   @param[in] hwaMemDestPingOffset    HWA memory offset with Ping output data,
 *                                      two rows (range bins) with 2nd D FFT complex symbols,
 *                                      followed by two rows (range bins) with summed log
 *                                      magnitude 2nd D FFT symbols across all antennas
 *
 *
 */
void HWAutil_configVitalSignsFFT(HWA_Handle handle,
                                uint32_t paramSetStartIdx,
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
                                uint16_t hwaMemDestPingOffset );

/**@}*/
#ifdef __cplusplus
}
#endif

#endif




