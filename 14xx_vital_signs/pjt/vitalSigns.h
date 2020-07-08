/**
 *   @file  vitalSigns.h
 *
 *   @brief
 *      This is the main header file for the Vital Signs Measurement Demo
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


#ifndef VITALSIGNS_DEMO_H
#define VITALSIGNS_DEMO_H

#include <ti/common/mmwave_error.h>
#include <ti/drivers/soc/soc.h>
#include <ti/drivers/crc/crc.h>
#include <ti/drivers/uart/UART.h>
#include <ti/drivers/pinmux/pinmux.h>
#include <ti/drivers/esm/esm.h>
#include <ti/drivers/soc/soc.h>
#include <ti/drivers/mailbox/mailbox.h>
#include <ti/drivers/adcbuf/ADCBuf.h>
#include <ti/drivers/hwa/hwa.h>
#include <ti/drivers/edma/edma.h>
#include <ti/control/mmwave/mmwave.h>
#include <ti/drivers/osal/DebugP.h>
#include <ti/sysbios/knl/Semaphore.h>

#include "vitalSignsDemo_utilsFunc.h"
#include "data_path.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 *  Millimeter Wave Demo statistics
 *
 * @details
 *  The structure is used to hold the statistics information for the
 *  Vital Signs Measurement demo
 */
typedef struct MmwDemo_Stats_t
{
    /*! @brief   Counter which tracks the number of frame trigger events from BSS */
    uint64_t     frameTriggerReady;

    /*! @brief   Counter which tracks the number of failed calibration reports
     *           The event is triggered by an asynchronous event from the BSS */
    uint32_t     failedTimingReports;

    /*! @brief   Counter which tracks the number of calibration reports received
     *           The event is triggered by an asynchronous event from the BSS */
    uint32_t     calibrationReports;
}MmwDemo_Stats;

/**
 * @brief
 *  Vital Signs Measurement Demo MCB
 *
 * @details
 *  The structure is used to hold all the relevant information for the
 *  Vital Signs Measurement demo
 */
typedef struct VitalSignsDemo_MCB_t
{
#if LEGACY
    /*! @brief   Configuration which is used to execute the demo */
	VitalSignsDemo_Cfg                 cfg;

#else
    /*! @brief   Configuration which is used to execute the demo */
    MmwDemo_Cfg                 cfg;

    /*! @brief CLI related configuration */
    MmwDemo_CliCfg_t           cliCfg;

    /*! @brief   CLI related configuration */
    MmwDemo_CliCommonCfg_t      cliCommonCfg;
#endif

    /*! * @brief   Handle to the SOC Module */
    SOC_Handle                  socHandle;

    /*! @brief   UART Logging Handle */
    UART_Handle                 loggingUartHandle;

    /*! @brief   UART Command Rx/Tx Handle */
    UART_Handle                 commandUartHandle;

    /*! @brief   This is the mmWave control handle which is used
     * to configure the BSS. */
    MMWave_Handle               ctrlHandle;

    /*! @brief   mmWave Control Configuration. */
    MMWave_CtrlCfg              ctrlCfg;

    /*! @brief   mmWave Chirp Configuration Cnt. */
    uint16_t                    numChirpCfg;

    /*! @brief   Data Path object */
    VitalSignsDemo_DataPathObj        dataPathObj;

    /*! @brief   Demo Stats */
    MmwDemo_Stats               stats;


} VitalSignsDemo_MCB;


/**************************************************************************
 *************************** Extern Definitions ***************************
 **************************************************************************/
extern VitalSignsDemo_MCB  gMmwMCB;
extern int32_t VitalSignsDemo_dataPathStart (void);
extern void VitalSignsDemo_dataPathConfig(void);

/* Sensor Management Module Exported API */
extern int32_t MmwDemo_sensorMgmtInit (void);
extern int32_t MmwDemo_sensorMgmtDeinit (void);
extern void MmwDemo_notifySensorStart(bool doReconfig);
extern void MmwDemo_notifySensorStop(void);
extern void MmwDemo_waitSensorStopComplete(void);
extern int32_t MmwDemo_waitSensorStartComplete(void);

extern void _MmwDemo_debugAssert(int32_t expression, const char *file, int32_t line);
#define MmwDemo_debugAssert(expression) {                                      \
                                         _MmwDemo_debugAssert(expression,      \
                                                  __FILE__, __LINE__);         \
                                         DebugP_assert(expression);             \
                                        }

#ifdef __cplusplus
}
#endif

#endif /* VITALSIGNS_DEMO_H */

