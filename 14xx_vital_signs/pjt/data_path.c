/**
 *   @file  data_path.c
 *
 *   @brief
 *      Implements Data path processing functionality.
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
#include <xdc/runtime/HeapMin.h>
#include <xdc/runtime/Memory.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/heaps/HeapBuf.h>
#include <ti/sysbios/heaps/HeapMem.h>
#include <ti/sysbios/knl/Event.h>
#define DebugP_ASSERT_ENABLED 1
#include <ti/drivers/osal/DebugP.h>
#include <assert.h>
#include <ti/common/sys_common.h>
#include <ti/drivers/osal/SemaphoreP.h>
#include <ti/drivers/edma/edma.h>
#include <ti/drivers/esm/esm.h>
#include <ti/drivers/soc/soc.h>

#include "config_edma_util.h"
#include "config_hwa_util.h"

#include "data_path.h"
#include "vitalSigns.h"



 /*! HWA memory buffer to produce the M0,M1,M2,M3 partition addresses
  and link to the HWA section in the linker command file. */
 vitalSignsHwaBuf_t gMmwHwaMemBuf[VITALSIGNS_HWA_NUM_MEM_BUFS];
 #pragma DATA_SECTION(gMmwHwaMemBuf, ".hwaBufs");

 /**
  *  @b Description
  *  @n
  *      Function to generate a single FFT window sample.
  *
  *  @param[out] win Pointer to output calculated window sample.
  *  @param[in]  winIndx Index of window to generate sample at.
  *  @param[in]  phi Pre-calculated constant by caller as (2*pi/(window length - 1)).
  *  @param[in]  winType Type of window, one of @ref VITALSIGNS_WIN_BLACKMAN, @ref VITALSIGNS_WIN_HANNING,
  *              or @ref VITALSIGNS_WIN_RECT.
  *  @retval none.
  */
 static inline VitalSignsDemo_genWindow(uint32_t *win, uint32_t winIndx, float phi, uint32_t winType)
 {
     if(winType == VITALSIGNS_WIN_BLACKMAN)
     {
         //Blackman window
         float a0 = 0.42;
         float a1 = 0.5;
         float a2 = 0.08;
         *win = (uint32_t) ((ONE_Q17 * (a0 - a1*cos(phi * winIndx) +    a2*cos(2 * phi * winIndx))) + 0.5);//in Q17
         if(*win >= ONE_Q17)
         {
             *win = ONE_Q17 - 1;
         }
     }
     else if(winType == VITALSIGNS_WIN_HANNING)
     {
         //Hanning window
         *win = (uint32_t) ((ONE_Q17 * 0.5* (1 - cos(phi * winIndx))) + 0.5);//in Q17
         if(*win >= ONE_Q17)
         {
             *win = ONE_Q17 - 1;
         }
     }
     else if(winType == VITALSIGNS_WIN_RECT)
     {
         //Rectangular window
         *win = (uint32_t) (ONE_Q17/16);//in Q17
     }
 }

/**
 *  @b Description
 *  @n
 *      EDMA transfer completion call back function as per EDMA API.
 *      Depending on the programmed transfer completion codes,
 *      posts the corresponding done/completion semaphore.
 */
void VitalSignsDemo_EDMA_transferCompletionCallbackFxn(uintptr_t arg, uint8_t transferCompletionCode)
{
    VitalSignsDemo_DataPathObj *obj = &gMmwMCB.dataPathObj;

    switch (transferCompletionCode) {
    case VITALSIGNSDEMO_EDMA_TRANSFER_COMPLETION_CODE_1D_DONE:
        Semaphore_post(obj->EDMA_1Ddone_semHandle);
        break;

    case VITALSIGNSDEMO_EDMA_TRANSFER_COMPLETION_CODE_PHASE_DONE:
        Semaphore_post(obj->EDMA_FFTphaseDone_semHandle);
        break;

    default:
        DebugP_assert(0);
        break;
    }
}

/**
 *  @b Description
 *  @n
 *      Open HWA instance.
 */
void MmwDemo_hwaOpen(VitalSignsDemo_DataPathObj *obj, SOC_Handle socHandle)
{
    int32_t             errCode;

    /* Open the HWA Instance */
    obj->hwaHandle = HWA_open(0, socHandle, &errCode);
    if (obj->hwaHandle == NULL)
    {
        //System_printf("Error: Unable to open the HWA Instance err:%d\n",errCode);
        MmwDemo_debugAssert (0);
        return;
    }
    //System_printf("Debug: HWA Instance %p has been opened successfully\n", obj->hwaHandle);
}

/**
 *  @b Description
 *  @n
 *      Open EDMA.
 */
void MmwDemo_edmaOpen(VitalSignsDemo_DataPathObj *obj)
{
    int32_t             errCode;
    EDMA_instanceInfo_t edmaInstanceInfo;
    EDMA_errorConfig_t  errorConfig;

    obj->edmaHandle = EDMA_open(0, &errCode, &edmaInstanceInfo);

    if (obj->edmaHandle == NULL)
    {
        //System_printf("Error: Unable to open the EDMA Instance err:%d\n",errCode);
        MmwDemo_debugAssert (0);
        return;
    }
    //System_printf("Debug: EDMA Instance %p has been opened successfully\n", obj->edmaHandle);

    errorConfig.isConfigAllEventQueues = true;
    errorConfig.isConfigAllTransferControllers = true;
    errorConfig.isEventQueueThresholdingEnabled = true;
    errorConfig.eventQueueThreshold = EDMA_EVENT_QUEUE_THRESHOLD_MAX;
    errorConfig.isEnableAllTransferControllerErrors = true;
    errorConfig.callbackFxn = VitalSignsDemo_EDMA_errorCallbackFxn;
    errorConfig.transferControllerCallbackFxn = VitalSignsDemo_EDMA_transferControllerErrorCallbackFxn;
    if ((errCode = EDMA_configErrorMonitoring(obj->edmaHandle, &errorConfig)) != EDMA_NO_ERROR)
    {
        //System_printf("Error: EDMA_configErrorMonitoring() failed with errorCode = %d\n", errCode);
        MmwDemo_debugAssert (0);
        return;
    }

}


void edmaInit(VitalSignsDemo_DataPathObj *obj)
{
    uint8_t edmaNumInstances, inst;
    Semaphore_Params semParams;
    int32_t errorCode;

    edmaNumInstances = EDMA_getNumInstances();
    for (inst = 0; inst < edmaNumInstances; inst++) {
        errorCode = EDMA_init(inst);
        if (errorCode != EDMA_NO_ERROR) {
            System_printf(
                    "Debug: EDMA instance %d initialization returned error %d\n",
                    errorCode);
            return;
        }
        System_printf("Debug: EDMA instance %d has been initialized\n", inst);
    }

    memset(&obj->EDMA_errorInfo, 0, sizeof(obj->EDMA_errorInfo));
    memset(&obj->EDMA_transferControllerErrorInfo, 0, sizeof(obj->EDMA_transferControllerErrorInfo));

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    obj->EDMA_1Ddone_semHandle = Semaphore_create(0, &semParams, NULL);

    /* Enable Done Interrupt */
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    obj->HWA_done_semHandle = Semaphore_create(0, &semParams, NULL);

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    obj->EDMA_2Ddone_semHandle = Semaphore_create(0, &semParams, NULL);

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    obj->EDMA_FFTphaseDone_semHandle = Semaphore_create(0, &semParams, NULL);
}
/**
 *  @b Description
 *  @n
 *      Configures all 1D processing related EDMA configuration.
 *
 *  @param[in] obj Pointer to data path object
 *  @retval EDMA error code, see EDMA API.
 */
int32_t VitalSignsDemo_config1D_EDMA(VitalSignsDemo_DataPathObj *obj) {
    int32_t errorCode = EDMA_NO_ERROR;
    EDMA_Handle handle = obj->edmaHandle;
    HWA_SrcDMAConfig dmaConfig;

    /* Ping configuration to transfer 1D FFT output from HWA to L3 RAM transposed */
    errorCode = EDMAutil_configHwaTranspose(handle,
            VITALSIGNS_EDMA_1D_PING_CH_ID,       //chId,
            VITALSIGNS_EDMA_1D_PING_SHADOW_LINK_CH_ID,  //linkChId,
            VITALSIGNS_EDMA_1D_PING_CHAIN_CH_ID, //chainChId,
            (uint32_t*) SOC_translateAddress((uint32_t) VITALSIGNS_HWA_1D_OUT_PING, SOC_TranslateAddr_Dir_TO_EDMA,NULL), //*pSrcAddress,
            (uint32_t*) SOC_translateAddress((uint32_t) obj->radarCube, SOC_TranslateAddr_Dir_TO_EDMA,NULL), //*pDestAddress,
            obj->numRxAntennas,       //numAnt,
            obj->numRangeBins,       //numRangeBins,
            obj->numChirpsPerFrame,  //numChirpsPerFrame,
            true,   //isIntermediateChainingEnabled,
            true,   //isFinalChainingEnabled,
            false,  //isTransferCompletionEnabled
            NULL,
            NULL);  //transferCompletionCallbackFxn

    if (errorCode != EDMA_NO_ERROR) {
        goto exit;
    }

    /* Pong configuration to transfer 1D FFT output from HWA to L3 RAM transposed */
    errorCode = EDMAutil_configHwaTranspose(handle,
            VITALSIGNS_EDMA_1D_PONG_CH_ID,      //chId,
            VITALSIGNS_EDMA_1D_PONG_SHADOW_LINK_CH_ID, //linkChId,
            VITALSIGNS_EDMA_1D_PONG_CHAIN_CH_ID, //SR_DBT_xxx //chainChId,
            (uint32_t*) SOC_translateAddress((uint32_t) VITALSIGNS_HWA_1D_OUT_PONG, SOC_TranslateAddr_Dir_TO_EDMA,NULL), //*pSrcAddress,
            (uint32_t*) SOC_translateAddress( (uint32_t) (obj->radarCube + obj->numRxAntennas) , SOC_TranslateAddr_Dir_TO_EDMA,NULL), //*pDestAddress,
            obj->numRxAntennas,       // numAnt,
            obj->numRangeBins,       // numRangeBins,
            obj->numChirpsPerFrame,  // numChirpsPerFrame,
            true, //isIntermediateChainingEnabled,
            true, //isFinalChainingEnabled,
            true, //isTransferCompletionEnabled
            VitalSignsDemo_EDMA_transferCompletionCallbackFxn, //transferCompletionCallbackFxn
            (uintptr_t)obj);
    if (errorCode != EDMA_NO_ERROR) {
        goto exit;
    }

    /**************************************************************************
     *  PROGRAM EDMA2_ACC_CHAN0 (resp. EDMA2_ACC_CHAN1) to communicate completion
     of DMA CHAN EDMA_TPCC0_REQ_HWACC_0 (resp. EDMA_TPCC0_REQ_HWACC_1)
     *************************************************************************/

    HWA_getDMAconfig(obj->hwaHandle, VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PING, &dmaConfig);

    errorCode = EDMAutil_configHwaOneHotSignature(handle,
    VITALSIGNS_EDMA_1D_PING_CHAIN_CH_ID, //chId,
    false, //isEventTriggered
    (uint32_t*)SOC_translateAddress(dmaConfig.srcAddr,SOC_TranslateAddr_Dir_TO_EDMA,NULL), //pSrcAddress
    (uint32_t*)SOC_translateAddress(dmaConfig.destAddr,SOC_TranslateAddr_Dir_TO_EDMA,NULL), //pDestAddress
            dmaConfig.aCnt,
            dmaConfig.bCnt,
            dmaConfig.cCnt,
            VITALSIGNS_EDMA_1D_PING_ONE_HOT_SHADOW_LINK_CH_ID); //linkChId

    if (errorCode != EDMA_NO_ERROR) {
        goto exit;
    }

    HWA_getDMAconfig(obj->hwaHandle, VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PONG, &dmaConfig);

    errorCode = EDMAutil_configHwaOneHotSignature(handle,
    VITALSIGNS_EDMA_1D_PONG_CHAIN_CH_ID, //chId,
    false, //isEventTriggered
    (uint32_t*)SOC_translateAddress(dmaConfig.srcAddr,SOC_TranslateAddr_Dir_TO_EDMA,NULL),  //pSrcAddress
    (uint32_t*)SOC_translateAddress(dmaConfig.destAddr,SOC_TranslateAddr_Dir_TO_EDMA,NULL), //pDestAddress
            dmaConfig.aCnt,
            dmaConfig.bCnt,
            dmaConfig.cCnt,
            VITALSIGNS_EDMA_1D_PONG_ONE_HOT_SHADOW_LINK_CH_ID); //linkChId

    if (errorCode != EDMA_NO_ERROR) {
        goto exit;
    }

    exit: return (errorCode);
}


/**
 *  @b Description
 *  @n
 *      Configure HWA for 1D processing.
 *  @param[in]  obj Pointer to data path obj.
 *  @retval none.
 */
void VitalSignsDemo_config1D_HWA(VitalSignsDemo_DataPathObj *obj) {
    int32_t errCode;
    HWA_CommonConfig hwaCommonConfig;

    /* Disable the HWA */
    errCode = HWA_enable(obj->hwaHandle, 0); // set 1 to enable
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_enable(1) returned %d\n", errCode);
        return;
    }

    HWAutil_configRangeFFT(obj->hwaHandle,
    VITALSIGNS_HWA_START_POS_PARAMSETS_1D,
            obj->numAdcSamples,
            obj->numRangeBins,
            obj->numRxAntennas,
            VITALSIGNS_HWA_WINDOWRAM_1D_OFFSET,
            VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PING,
            VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PONG,
            VITALSIGNS_HWA_DMA_DEST_CHANNEL_1D_PING,
            VITALSIGNS_HWA_DMA_DEST_CHANNEL_1D_PONG,
            ADDR_TRANSLATE_CPU_TO_HWA(VITALSIGNS_HWA_1D_ADCBUF_INP),
            ADDR_TRANSLATE_CPU_TO_HWA(VITALSIGNS_HWA_1D_OUT_PING),
            ADDR_TRANSLATE_CPU_TO_HWA(VITALSIGNS_HWA_1D_OUT_PONG));

    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    /* Config Common Registers */
    hwaCommonConfig.configMask = HWA_COMMONCONFIG_MASK_NUMLOOPS
            | HWA_COMMONCONFIG_MASK_PARAMSTARTIDX
            | HWA_COMMONCONFIG_MASK_PARAMSTOPIDX
            | HWA_COMMONCONFIG_MASK_FFT1DENABLE
            | HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD;
    hwaCommonConfig.numLoops = obj->numChirpsPerFrame / 2;
    hwaCommonConfig.paramStartIdx = VITALSIGNS_HWA_START_POS_PARAMSETS_1D;
    hwaCommonConfig.paramStopIdx = VITALSIGNS_HWA_START_POS_PARAMSETS_1D + HWAUTIL_NUM_PARAM_SETS_1D - 1;
    hwaCommonConfig.fftConfig.fft1DEnable = HWA_FEATURE_BIT_ENABLE;
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;
    errCode = HWA_configCommon(obj->hwaHandle, &hwaCommonConfig);
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_configCommon returned %d\n", errCode);
        return;
    }
}

/**
 *  @b Description
 *  @n
 *      Trigger HWA for 1D processing.
 *  @param[in]  obj Pointer to data path obj.
 *  @retval none.
 */
void VitalSignsDemo_dataPathTrigger1D(VitalSignsDemo_DataPathObj *obj) {
    int32_t errCode;

    /* Enable the HWA */
    errCode = HWA_enable(obj->hwaHandle, 1); // set 1 to enable
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_enable(1) returned %d\n", errCode);
        return;
    }

    /* trigger the HWA since triggerMode is set to DMA */
    errCode = HWA_setDMA2ACCManualTrig(obj->hwaHandle,
    VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PING);
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_setDMA2ACCManualTrig(0) returned %d\n",
                errCode);
        return;
    }
    errCode = HWA_setDMA2ACCManualTrig(obj->hwaHandle,
    VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_1D_PONG);
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_setDMA2ACCManualTrig(1) returned %d\n",
                errCode);
        return;
    }
}


/**
 *  @b Description
 *  @n
 *      Waits for 1D processing to finish. This is a blocking function.
 *
 *  @param[in] obj  Pointer to data path object
 *
 *  @retval
 *      NONE
 */
void VitalSignsDemo_dataPathWait1D(VitalSignsDemo_DataPathObj *obj) {
    Bool status;
//    Semaphore_Handle       paramSetSem;

    /**********************************************/
    /* WAIT FOR HWA NUMLOOPS INTERRUPT            */
    /**********************************************/
    /* wait for the all paramSets done interrupt */
    status = Semaphore_pend(obj->HWA_done_semHandle, BIOS_WAIT_FOREVER);
    if (status != TRUE) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: Semaphore_pend returned %d\n", status);
    }

    /**********************************************/
    /* WAIT FOR EDMA NUMLOOPS INTERRUPT            */
    /**********************************************/
    status = Semaphore_pend(obj->EDMA_1Ddone_semHandle, BIOS_WAIT_FOREVER);
    if (status != TRUE) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: Semaphore_pend returned %d\n", status);
    }
}

/**
 *  @b Description
 *  @n
 *      HWA processing completion call back function as per HWA API.
 *      Depending on the programmed transfer completion codes,
 *      posts HWA done semaphore.
 */
void VitalSignsDemo_dataPathHwaDoneIsrCallback(void * arg) {
    Semaphore_Handle semHandle;
//    gMmwMCB.dataPathObj.hwaDoneIsrCounter++;
    if (arg != NULL) {
        semHandle = (Semaphore_Handle) arg;
        Semaphore_post(semHandle);
    }
}

/**
 *  @b Description
 *  @n
 *      Call back function for EDMA transfer controller error as per EDMA API.
 *      Declare fatal error if happens, the output errorInfo can be examined if code
 *      gets trapped here.
 */
void VitalSignsDemo_EDMA_transferControllerErrorCallbackFxn(EDMA_Handle handle,
                                                           EDMA_transferControllerErrorInfo_t *errorInfo
                                                           )
{
    DebugP_assert(0);
}

/**
 *  @b Description
 *  @n
 *      Common configuration of data path processing, required to be done only
 *      once. Configures all of EDMA and window RAM of HWA.
 *  @param[in]  obj Pointer to data path obj.
 *  @retval none.
 */
void VitalSignsDemo_dataPathConfigCommon(VitalSignsDemo_DataPathObj *obj) {
    int32_t errCode;
    uint32_t fftWindow;
    uint32_t winLen, winIndx, hwaRamOffset;
    HWA_CommonConfig hwaCommonConfig;
    float phi;

    VitalSignsDemo_config1D_EDMA(obj);
    VitalSignsDemo_configPhase_EDMA(obj);

    /**********************************************/
    /* Disable HWA and reset to known state       */
    /**********************************************/

    /* Disable the HWA */
    errCode = HWA_enable(obj->hwaHandle,0);
    if (errCode != 0)
    {
        System_printf("Error: MmwDemo_dataPathConfigCommon:HWA_enable(0) returned %d\n",errCode);
        return;
    }

    /* Reset the internal state of the HWA */
    errCode = HWA_reset(obj->hwaHandle);
    if (errCode != 0)
    {
        System_printf("Error: MmwDemo_dataPathConfigCommon:HWA_reset returned %d\n",errCode);
        return;
    }

    /***********************/
    /* CONFIG WINDOW RAM   */
    /***********************/
    /* if windowing is enabled, load the window coefficients in RAM */
    //1D-FFT window
    hwaRamOffset = 0;
    winLen = obj->numAdcSamples;
    phi = 2 * PI / ((float) winLen - 1);
    for (winIndx = 0; winIndx < winLen; winIndx++) {
        VitalSignsDemo_genWindow(&fftWindow, winIndx, phi, VITALSIGNS_WIN_BLACKMAN);
        errCode = HWA_configRam(obj->hwaHandle, HWA_RAM_TYPE_WINDOW_RAM,
                (uint8_t *) &fftWindow, sizeof(uint32_t),   //size in bytes
                hwaRamOffset); //offset in bytes
        if (errCode != 0) {
            System_printf("Error: HWA_configRam returned %d\n", errCode);
            return;
        }
        hwaRamOffset += sizeof(uint32_t);
    }

    winLen = obj->numDopplerBins;
    phi = 2 * PI / ((float) winLen - 1);
    for (winIndx = 0; winIndx < winLen; winIndx++) {
        VitalSignsDemo_genWindow(&fftWindow, winIndx, phi, VITALSIGNS_WIN_HANNING);
        errCode = HWA_configRam(obj->hwaHandle, HWA_RAM_TYPE_WINDOW_RAM,
                (uint8_t *) &fftWindow, sizeof(uint32_t), //size in bytes
                hwaRamOffset); //offset in bytes
        if (errCode != 0) {
            System_printf("Error: HWA_configRam returned %d\n", errCode);
            return;
        }
        hwaRamOffset += sizeof(uint32_t);
    }

    /**********************************************/
    /* ENABLE NUMLOOPS DONE INTERRUPT FROM HWA */
    /**********************************************/
    errCode = HWA_enableDoneInterrupt(obj->hwaHandle,
            VitalSignsDemo_dataPathHwaDoneIsrCallback, obj->HWA_done_semHandle);
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_enableDoneInterrupt returned %d\n", errCode);
        return;
    }

    /***********************************************/
    /* ENABLE FFT Twiddle coefficient dithering    */
    /***********************************************/
    /* Config Common Registers */
    hwaCommonConfig.configMask = HWA_COMMONCONFIG_MASK_TWIDDITHERENABLE |
                                 HWA_COMMONCONFIG_MASK_LFSRSEED;
    hwaCommonConfig.fftConfig.twidDitherEnable = HWA_FEATURE_BIT_ENABLE;
    hwaCommonConfig.fftConfig.lfsrSeed = 0x1234567; /*Some non-zero value*/

    errCode = HWA_configCommon(obj->hwaHandle,&hwaCommonConfig);
    if (errCode != 0)
    {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_configCommon returned %d\n",errCode);
        return;
    }
}

/**
 *  @b Description
 *  @n
 *      Configures all Vital Signs Processing related EDMA configuration for transfering the Phase data from the
 *      Shared memory to the HWA memory.
 *
 *  @param[in] obj Pointer to data path object
 *  @retval EDMA error code, see EDMA API.
 */
int32_t VitalSignsDemo_configPhase_EDMA(VitalSignsDemo_DataPathObj *obj) {
    int32_t errorCode = EDMA_NO_ERROR;
    EDMA_Handle handle = obj->edmaHandle;
    HWA_SrcDMAConfig dmaConfig;

//program DMA to MOVE DATA FROM L3 TO ACCEL, THIS SHOULD BE LINKED TO DMA2ACC
    errorCode = EDMAutil_configHwaContiguous(handle,
            VITALSIGNS_EDMA_PHASE_INP_CH_ID,              //chId,
            false, //isEventTriggered
            VITALSIGNS_EDMA_PHASE_INP_SHADOW_LINK_CH_ID1, //linkChId,
            VITALSIGNS_EDMA_PHASE_INP_CHAIN_CH_ID,       //chainChId,
            (uint32_t*) SOC_translateAddress((uint32_t) obj->pVitalSignsCircularBuff, SOC_TranslateAddr_Dir_TO_EDMA, NULL), //*pSrcAddress,
            (uint32_t*) SOC_translateAddress((uint32_t) VITALSIGNS_HWA_PHASE_INP, SOC_TranslateAddr_Dir_TO_EDMA, NULL ), //*pDestAddress,
            ( obj->circularBufferSizeHeart + obj->circularBufferSizeBreath)* sizeof(uint32_t), //numBytes, (the factor of 2 because of transfering both the breathing and Heart waveform)
            1, //numBlocks,
            0, //srcIncrBytes,
            0, //dstIncrBytes,
            false,  //isIntermediateChainingEnabled,
            true,   //isFinalChainingEnabled,
            false, //isTransferCompletionEnabled;
            NULL,
            NULL); //transferCompletionCallbackFxn

    if (errorCode != EDMA_NO_ERROR) {
        goto exit;
    }

    HWA_getDMAconfig(obj->hwaHandle, VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_PHASE,
            &dmaConfig);

    errorCode = EDMAutil_configHwaOneHotSignature(handle,
             VITALSIGNS_EDMA_PHASE_INP_CHAIN_CH_ID, //chId,
             false, //isEventTriggered
             (uint32_t*)SOC_translateAddress(dmaConfig.srcAddr,SOC_TranslateAddr_Dir_TO_EDMA,NULL),  //pSrcAddress
             (uint32_t*)SOC_translateAddress(dmaConfig.destAddr,SOC_TranslateAddr_Dir_TO_EDMA,NULL), //pDestAddress
             dmaConfig.aCnt,
             dmaConfig.bCnt,
             dmaConfig.cCnt,
             VITALSIGNS_EDMA_PHASE_INP_SHADOW_LINK_CH_ID2); //linkChId

    if (errorCode != EDMA_NO_ERROR) {
        goto exit;
    }

//another DMA to move data from ACC to L3
    errorCode = EDMAutil_configHwaContiguous(handle,
            VITALSIGNS_EDMA_PHASE_OUT_CH_ID,              // chId,
            true, //isEventTriggered
            VITALSIGNS_EDMA_PHASE_OUT_SHADOW_LINK_CH_ID1, // linkChId,
            VITALSIGNS_EDMA_PHASE_OUT_CHAIN_CH_ID,        // chainChId,
            (uint32_t*) SOC_translateAddress((uint32_t) VITALSIGNS_HWA_PHASE_OUT, SOC_TranslateAddr_Dir_TO_EDMA,NULL), //*pSrcAddress,  VITALSIGNS_HWA_PHASE_OUT
            (uint32_t*) SOC_translateAddress((uint32_t) obj->pVitalSignsSpectrum, SOC_TranslateAddr_Dir_TO_EDMA,NULL), //*pDestAddress,
            PHASE_FFT_SIZE * sizeof(uint32_t), //numBytes   PHASE_FFT_SIZE
            1,     //numBlocks,
            0, //srcIncrBytes,
            0, //dstIncrBytes,
            false, //isIntermediateChainingEnabled,
            false, //isFinalChainingEnabled,
            true, //isTransferCompletionEnabled;
            VitalSignsDemo_EDMA_transferCompletionCallbackFxn, //transferCompletionCallbackFxn)
            (uintptr_t)obj);

    exit: return (errorCode);
}


/**
 *  @b Description
 *  @n
 *      Configures HWA for Vital Signs processing.
 *
 *  @param[in] obj  Pointer to data path object
 *
 *  @retval
 *      NONE
 */
void VitalSignsDemo_configPhase_HWA(VitalSignsDemo_DataPathObj *obj) {
    int32_t errCode;
    HWA_CommonConfig hwaCommonConfig;

    /* Disable the HWA */
    errCode = HWA_enable(obj->hwaHandle, 0); // set 1 to enable
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_enable(1) returned %d\n", errCode);
        return;
    }

    HWAutil_configVitalSignsFFT(obj->hwaHandle,
            HWAUTIL_NUM_PARAM_SETS_1D, // paramSetStartIdx,
            obj->circularBufferSizeBreath,    // numInputSamples,
            obj->circularBufferSizeHeart,
            PHASE_FFT_SIZE,     //  FFT Size
            PHASE_FFT_SIZE / 2, // numOutputSamples , only want the half the samples as the Input is Real
            1,                  // numVirtualAnt,
            VITALSIGNS_NUM_RANGE_BINS_PER_TRANSFER,                  // number of Buffers per Iterations,
            obj->numAdcSamples + VITALSIGNS_HWA_WINDOWRAM_1D_OFFSET, // windowOffsetBytes
            VITALSIGNS_HWA_DMA_TRIGGER_SOURCE_PHASE, // dmaTriggerSource (this trigger comes from the DMA and is set in the EDMA configuration)
            VITALSIGNS_HWA_DMA_DEST_CHANNEL_PHASE, // dmaDestChannel (The DMA Channel to trigger once the HWA has done its job)
            ADDR_TRANSLATE_CPU_TO_HWA(VITALSIGNS_HWA_PHASE_INP), // Input  Memory location (Needs to be the HWA Memory i.e. M0, M1, M2, M3)
            ADDR_TRANSLATE_CPU_TO_HWA(VITALSIGNS_HWA_PHASE_OUT) // Output Memory location (Needs to be the HWA Memory i.e. M0, M1, M2, M3)
            );

    /***********************/
    /* HWA COMMON CONFIG   */
    /***********************/
    /* Config Common Registers */
    hwaCommonConfig.configMask = HWA_COMMONCONFIG_MASK_NUMLOOPS
            | HWA_COMMONCONFIG_MASK_PARAMSTARTIDX
            | HWA_COMMONCONFIG_MASK_PARAMSTOPIDX
            | HWA_COMMONCONFIG_MASK_FFT1DENABLE
            | HWA_COMMONCONFIG_MASK_INTERFERENCETHRESHOLD;

    hwaCommonConfig.numLoops = 1;
    hwaCommonConfig.paramStartIdx = HWAUTIL_NUM_PARAM_SETS_1D;
    hwaCommonConfig.paramStopIdx = HWAUTIL_NUM_PARAM_SETS_1D + VITALSIGNS_NUM_RANGE_BINS_PER_TRANSFER;
    hwaCommonConfig.fftConfig.fft1DEnable = HWA_FEATURE_BIT_DISABLE; // If Disable has access to all M0, M1, M2, M3
    hwaCommonConfig.fftConfig.interferenceThreshold = 0xFFFFFF;
    errCode = HWA_configCommon(obj->hwaHandle, &hwaCommonConfig);
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_configCommon returned %d\n", errCode);
        return;
    }
}


void VitalSignsDemo_dataPathTriggerPhase(VitalSignsDemo_DataPathObj *obj) {
    int32_t errCode;

    /* Enable the HWA */
    errCode = HWA_enable(obj->hwaHandle, 1); // set 1 to enable
    if (errCode != 0) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: HWA_enable(1) returned %d\n", errCode);
        return;
    }

    EDMA_startTransfer(obj->edmaHandle, VITALSIGNS_EDMA_PHASE_INP_CH_ID,
            EDMA3_CHANNEL_TYPE_DMA);
}

void VitalSignsDemo_dataPathWaitPhase(VitalSignsDemo_DataPathObj *obj) {
    Bool status;

    /* then wait for the all paramSets done interrupt */
    status = Semaphore_pend(obj->HWA_done_semHandle, BIOS_WAIT_FOREVER);
    if (status != TRUE) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: Semaphore_pend returned %d\n", status);
        return;
    }

    /* wait for EDMA done */
    status = Semaphore_pend(obj->EDMA_FFTphaseDone_semHandle, BIOS_WAIT_FOREVER);
    if (status != TRUE) {
        //retCode = HWA_TEST_ERROR;
        System_printf("Error: Semaphore_pend returned %d\n", status);
        return;
    }
}

/**
 *  @b Description
 *  @n
 *      Call back function for EDMA CC (Channel controller) error as per EDMA API.
 *      Declare fatal error if happens, the output errorInfo can be examined if code
 *      gets trapped here.
 */
void VitalSignsDemo_EDMA_errorCallbackFxn(EDMA_Handle handle,
                                          EDMA_errorInfo_t *errorInfo
                                         )
{
   DebugP_assert(0);
}


/**
 *  @b Description
 *  @n
 *      Utility function for reseting memory pool.
 *
 *  @param[in]  pool Handle to pool structure.
 *
 *  @retval
 *      none.
 */
void VitalSignsDemo_memPoolReset(VitalSignsDemoMemPool_t *pool) {
    pool->indx = 0;
}

/**
 *  @b Description
 *  @n
 *      Utility function for allocating from a static memory pool.
 *
 *  @pre It is assumed that no allocation from l3 will need better than 32-bit
 *      alignment (structures with 64-bit prohibited) and so for simplicity,
 *      size is assumed to be multiple of 4 bytes
 *
 *  @param[in]  pool Handle to pool structure.
 *  @param[in]  size Size in bytes to be allocated.
 *
 *  @retval
 *      pointer to beginning of allocated block. NULL indicates could notice
 *      allocate.
 */
uint8_t *VitalSignsDemo_memPoolAlloc(VitalSignsDemoMemPool_t *pool, uint32_t size) {
    uint8_t *ptr = NULL;

    if ((size % 4) != 0) {
        return (ptr);
    }

    if ((pool->indx + size) <= pool->size) {
        ptr = pool->base + pool->indx;
        pool->indx += size;
    }

    return (ptr);
}


/**
 *  @b Description
 *  @n
 *      Power of 2 round up function.
 */
uint32_t pow2roundup(uint32_t x) {
    uint32_t result = 1;
    while (x > result) {
        result <<= 1;
    }
    return result;
}


/**
 *  @b Description
 *  @n
 *      Delete semaphores.
 */
void MmwDemo_dataPathDeleteSemaphore(VitalSignsDemo_DataPathObj *obj)
{
    Semaphore_delete(&obj->chirpSemHandle);
    Semaphore_delete(&obj->EDMA_1Ddone_semHandle);
    Semaphore_delete(&obj->EDMA_2Ddone_semHandle);
    Semaphore_delete(&obj->EDMA_FFTphaseDone_semHandle);
    Semaphore_delete(&obj->HWA_done_semHandle);
}

