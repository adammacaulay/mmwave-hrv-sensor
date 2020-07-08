/*
 *   @file  cli.c
 *
 *   @brief
 *      (Vital Signs Measurement) DEMO CLI Implementation
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

#ifdef PROC_R4
#include <ti/sysbios/family/arm/v7r/vim/Hwi.h>
#endif

/* mmWave SDK Include Files: */
#include <ti/common/sys_common.h>
#include <ti/common/mmwave_sdk_version.h>
#include <ti/drivers/uart/UART.h>
#include <ti/control/mmwavelink/mmwavelink.h>
#include <ti/utils/cli/cli.h>

/* Demo Include Files */
#include "vitalSigns.h"

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/**
 * @brief
 *  This is the MAXIMUM number of CLI arguments which can be passed to a
 *  command.
 */
#define MMW_CLI_MAX_ARGS           20

/**
 * @brief
 *  This is the CLI handler which is registered to handle CLI commands.
 *
 *  @param[in] argc
 *      Number of arguments
 *  @param[in] argv
 *      Arguments.
 *
 *  @retval
 *      Success - 0
 *  @retval
 *      Error   - <0
 */
typedef int32_t (*MmwDemo_CLIFxn)(int32_t argc, char* argv[]);

/**
 * @brief
 *  Mmw Demo CLI Command Entry
 *
 * @details
 *  The structure maps a CLI command to a corresponding function
 *  which is to be executed
 */
typedef struct MmwDemo_CLICmdEntry {
	/**
	 * @brief   Command string
	 */
	char* cli;

	/**
	 * @brief   Optional command description string
	 */
	char* description;

	/**
	 * @brief   Function to be executed for the CLI command
	 */
	MmwDemo_CLIFxn cliFxn;
} MmwDemo_CLICmdEntry;

/* CLI Command Functions */
static int32_t VitalSignsDemo_CLISensorStart(int32_t argc, char* argv[]);
static int32_t VitalSignsDemo_CLISensorStop(int32_t argc, char* argv[]);
static int32_t VitalSignsDemo_CLIvitalSignsParamsCfg(int32_t argc, char* argv[]);
static int32_t VitalSignsDemo_CLIMotionDetection (int32_t argc, char* argv[]);
static int32_t VitalSignsDemo_CLIGuiMonSel(int32_t argc, char* argv[]);
static int32_t MmwDemo_CLIADCBufCfg (int32_t argc, char* argv[]);
void VitalSignsDemo_CLIInit (void);

/**************************************************************************
 *************************** Extern Definitions ***************************
 **************************************************************************/

extern VitalSignsDemo_MCB gMmwMCB;

/**************************************************************************
 **************************** MMW CLI Functions ***************************
 **************************************************************************/

void VitalSignsDemo_CLIInit (void)
{
    CLI_Cfg     cliCfg;
       uint32_t    cnt = 0;
       char        demoBanner[256];

       /* Create Demo Banner to be printed out by CLI */
       sprintf(&demoBanner[0],
                          "******************************************\n" \
                          "xWR14xx Vital Signs Demo %02d.%02d.%02d.%02d\n"  \
                          "******************************************\n",
                          MMWAVE_SDK_VERSION_MAJOR,
                          MMWAVE_SDK_VERSION_MINOR,
                          MMWAVE_SDK_VERSION_BUGFIX,
                          MMWAVE_SDK_VERSION_BUILD
               );


       /* Initialize the CLI configuration: */
       memset ((void *)&cliCfg, 0, sizeof(CLI_Cfg));

       /* Populate the CLI configuration: */
       cliCfg.cliPrompt                    = "vitalSignsDemo:/>";
       cliCfg.cliBanner                    = demoBanner;
       cliCfg.cliUartHandle                = gMmwMCB.commandUartHandle;
       cliCfg.taskPriority                 = 3;
       cliCfg.enableMMWaveExtension        = 1U;
       cliCfg.usePolledMode                = true;
       cliCfg.mmWaveHandle                 = gMmwMCB.ctrlHandle;
       cliCfg.tableEntry[cnt].cmd            = "sensorStart";
       cliCfg.tableEntry[cnt].helpString     = "[doReconfig(optional, default:enabled)]";
       cliCfg.tableEntry[cnt].cmdHandlerFxn  = VitalSignsDemo_CLISensorStart;
       cnt++;

       cliCfg.tableEntry[cnt].cmd            = "adcbufCfg";
   #if 0
       cliCfg.tableEntry[cnt].helpString     = "<adcOutputFmt> <SampleSwap> <ChanInterleave> <ChirpThreshold>";
   #else
       cliCfg.tableEntry[cnt].helpString     = NULL;
   #endif
       cliCfg.tableEntry[cnt].cmdHandlerFxn  = MmwDemo_CLIADCBufCfg;
       cnt++;

       cliCfg.tableEntry[cnt].cmd            = "sensorStop";
       cliCfg.tableEntry[cnt].helpString     = "No arguments";
       cliCfg.tableEntry[cnt].cmdHandlerFxn  = VitalSignsDemo_CLISensorStop;
       cnt++;

       cliCfg.tableEntry[cnt].cmd            = "guiMonitor";
       cliCfg.tableEntry[cnt].helpString     = "<guiFlag_param1> <guiFlag_param2> <guiFlag_spectralEst_Method> <guiFlag_Reset> <guiFlag_ClutterRemoval> <guiFlag_MotionDetection> <guiFlag_ReceiverBeamForming >";
       cliCfg.tableEntry[cnt].cmdHandlerFxn  = VitalSignsDemo_CLIGuiMonSel;
       cnt++;

       cliCfg.tableEntry[cnt].cmd            = "vitalSignsCfg";
       cliCfg.tableEntry[cnt].helpString     = "<rangeStart_meters> <rangeEnd_meters> <winLenBreath> <winLenHeart> <RX_recieverProcess> <alpha_breathingWfm> <alpha_heartWfm> <scale_breathingWfm> <scale_heartWfm>";
       cliCfg.tableEntry[cnt].cmdHandlerFxn  = VitalSignsDemo_CLIvitalSignsParamsCfg;
       cnt++;

       cliCfg.tableEntry[cnt].cmd            = "motionDetection";
       cliCfg.tableEntry[cnt].helpString     = "<enabled> <threshold> <blockSize> <gainControl>";
       cliCfg.tableEntry[cnt].cmdHandlerFxn  = VitalSignsDemo_CLIMotionDetection;

       /* Open the CLI: */
       if (CLI_open (&cliCfg) < 0)
       {
           System_printf ("Error: Unable to open the CLI\n");
           return;
       }
       System_printf ("Debug: CLI is operational\n");
       return;
}

/**
 *  @b Description
 *  @n
 *      This is the CLI Handler for vital Signs parameters configuration
 *
 *  @param[in] argc
 *      Number of arguments
 *  @param[in] argv
 *      Arguments
 *
 *  @retval
 *      Success -   0
 *  @retval
 *      Error   -   <0
 */
static int32_t VitalSignsDemo_CLIvitalSignsParamsCfg (int32_t argc, char* argv[])
{

	VitalSignsDemo_ParamsCfg   vitalSignsParamsCfg;

    /* Sanity Check: Minimum argument check */
    if (argc < 8)
    {
        CLI_write ("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Initialize the ADC Output configuration: */
    memset ((void *)&vitalSignsParamsCfg, 0, sizeof(VitalSignsDemo_ParamsCfg));

    /* Populate configuration: */
    vitalSignsParamsCfg.startRange_m         = (float) atof (argv[1]);
    vitalSignsParamsCfg.endRange_m           = (float) atof (argv[2]);
    vitalSignsParamsCfg.winLen_breathing     = (uint16_t) atoi (argv[3]);
    vitalSignsParamsCfg.winLen_heartRate     = (uint16_t) atoi (argv[4]);
    vitalSignsParamsCfg.rxAntennaProcess     = (float)   atof (argv[5]);
    vitalSignsParamsCfg.alpha_breathingWfm   = (float)   atof (argv[6]);
    vitalSignsParamsCfg.alpha_heartWfm       = (float)   atof (argv[7]);
    vitalSignsParamsCfg.scale_breathingWfm   = (float)   atof (argv[8]);
    vitalSignsParamsCfg.scale_heartWfm       = (float)   atof (argv[9]);

    /* Save Configuration to use later */
//    memcpy((void *)&gMmwMCB.dataPathObj.cliCfg->vitalSignsParamsCfg, (void *)&vitalSignsParamsCfg, sizeof(VitalSignsDemo_ParamsCfg));
    memcpy((void *)&gMmwMCB.cliCfg.vitalSignsParamsCfg, (void *)&vitalSignsParamsCfg, sizeof(VitalSignsDemo_ParamsCfg));

    return 0;
}

static int32_t VitalSignsDemo_CLIMotionDetection (int32_t argc, char* argv[])
{

    VitalSignsDemo_MotionDetection   motionDetectionParamsCfg;

    /* Sanity Check: Minimum argument check */
    if (argc < 4)
    {
        CLI_write ("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Initialize the ADC Output configuration: */
    memset ((void *)&motionDetectionParamsCfg, 0, sizeof(motionDetectionParamsCfg));

    /* Populate configuration: */

    motionDetectionParamsCfg.enabled     = (uint16_t) atoi (argv[1]);
    motionDetectionParamsCfg.blockSize   = (uint16_t) atoi (argv[2]);
    motionDetectionParamsCfg.threshold    = (float) atof (argv[3]);
    motionDetectionParamsCfg.gainControl  = (uint16_t) atof (argv[4]);

    memcpy((void *)&gMmwMCB.cliCfg.motionDetectionParamsCfg, (void *)&motionDetectionParamsCfg, sizeof(VitalSignsDemo_MotionDetection));
    return 0;
}

/**
 *  @b Description
 *  @n
 *      Logging function which can log the messages to the CLI console
 *
 *  @retval
 *      Not Applicable.
 */
static void VitalSignsDemo_cliWrite(const char* format, ...) {
	va_list arg;
	char logMessage[256];
	int32_t sizeMessage;

	/* Format the message: */
	va_start(arg, format);
	sizeMessage = vsnprintf(&logMessage[0], sizeof(logMessage), format, arg);
	va_end(arg);

	/* Log the message on the UART CLI console: */
	UART_writePolling(gMmwMCB.commandUartHandle, (uint8_t*) &logMessage[0],	sizeMessage);
}


/**
 *  @b Description
 *  @n
 *      This is the CLI Handler for gui monitoring configuration
 *
 *  @param[in] argc
 *      Number of arguments
 *  @param[in] argv
 *      Arguments
 *
 *  @retval
 *      Success -   0
 *  @retval
 *      Error   -   <0
 */
static int32_t VitalSignsDemo_CLIGuiMonSel(int32_t argc, char* argv[]) {
	VitalSignsDemo_GuiMonSel guiMonSel;

	/* Sanity Check: Minimum argument check */
	if (argc < 5) {
		VitalSignsDemo_cliWrite("Error: Invalid usage of the CLI command\n");
		return -1;
	}

	/* Initialize the ADC Output configuration: */
	memset((void *) &guiMonSel, 0, sizeof(VitalSignsDemo_GuiMonSel));

	/* Populate configuration: */
    guiMonSel.guiFlag_Param1         = atoi(argv[1]);
    guiMonSel.guiFlag_Param2         = atoi(argv[2]);
    guiMonSel.guiFlag_ClutterRemoval = atoi(argv[3]);
    guiMonSel.guiFlag_Reset          = atoi(argv[4]);
    guiMonSel.statsInfo              = atoi(argv[5]);


	/* Save Configuration to use later */
//	memcpy((void *) &gMmwMCB.dataPathObj.cliCfg->guiMonSel, (void *) &guiMonSel, sizeof(VitalSignsDemo_GuiMonSel));
    memcpy((void *) &gMmwMCB.cliCfg.vitalSigns_GuiMonSel, (void *) &guiMonSel, sizeof(VitalSignsDemo_GuiMonSel));


	return 0;
}


/**
 *  @b Description
 *  @n
 *      This is the CLI Handler for the ADCBUF configuration
 *
 *  @param[in] argc
 *      Number of arguments
 *  @param[in] argv
 *      Arguments
 *
 *  @retval
 *      Success -   0
 *  @retval
 *      Error   -   <0
 */
static int32_t MmwDemo_CLIADCBufCfg (int32_t argc, char* argv[])
{
    MmwDemo_ADCBufCfg    adcBufCfg;

    /* Sanity Check: Minimum argument check */
    if (argc != 5)
    {
        CLI_write ("Error: Invalid usage of the CLI command\n");
        return -1;
    }

    /* Initialize the ADC Output configuration: */
    memset ((void *)&adcBufCfg, 0, sizeof(MmwDemo_ADCBufCfg));

    /* Populate configuration: */
    adcBufCfg.adcFmt          = (uint8_t) atoi (argv[1]);
    adcBufCfg.iqSwapSel       = (uint8_t) atoi (argv[2]);
    adcBufCfg.chInterleave    = (uint8_t) atoi (argv[3]);
    adcBufCfg.chirpThreshold  = (uint8_t) atoi (argv[4]);

    /* Save Configuration to use later */
    memcpy((void *)&gMmwMCB.dataPathObj.cliCfg->adcBufCfg, (void *)&adcBufCfg,
        sizeof(MmwDemo_ADCBufCfg));
    return 0;
}


/**
 *  @b Description
 *  @n
 *      This is the CLI Handler for the sensor start command
 *
 *  @param[in] argc
 *      Number of arguments
 *  @param[in] argv
 *      Arguments
 *
 *  @retval
 *      Success -   0
 *  @retval
 *      Error   -   <0
 */
static int32_t VitalSignsDemo_CLISensorStart (int32_t argc, char* argv[])
{

    bool doReconfig = true;
    if (argc==2)
    {
        doReconfig = (bool) atoi (argv[1]);
    }
    /* Notify the sensor management module to start the sensor */
    MmwDemo_notifySensorStart (doReconfig);
    /* Pend for completion */
    MmwDemo_waitSensorStartComplete();

    return 0;
}

/**
 *  @b Description
 *  @n
 *      This is the CLI Handler for the sensor stop command
 *
 *  @param[in] argc
 *      Number of arguments
 *  @param[in] argv
 *      Arguments
 *
 *  @retval
 *      Success -   0
 *  @retval
 *      Error   -   <0
 */
static int32_t VitalSignsDemo_CLISensorStop (int32_t argc, char* argv[])
{
    /* Notify the sensor management module to stop the sensor */
    MmwDemo_notifySensorStop ();
    /* Pend for completion */
    MmwDemo_waitSensorStopComplete();

    return 0;
}


