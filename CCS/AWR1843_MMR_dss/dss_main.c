
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

#include <ti/drivers/osal/SemaphoreP.h>

/* MMWSDK Include Files. */
#include <ti/drivers/soc/soc.h>
#include <ti/common/sys_common.h>
#include <ti/common/mmwave_sdk_version.h>
#include <ti/control/mmwavelink/mmwavelink.h>
#include <ti/control/mmwavelink/include/rl_sensor.h>
#include <ti/utils/cycleprofiler/cycle_profiler.h>

#include <ti/alg/gtrack/gtrack.h>

/* MMWAVE Demo Include Files */
#include "dss_mmw.h"
#include "dss_data_path.h"
#include "mmw_messages.h"
#include "osal_nonos/Event_nonos.h"
#include <ti/control/mmwave/include/mmwave_internal.h>

#include "communication1843.h"
#include "user_parameters.h"

/* C674x mathlib */
/* Suppress the mathlib.h warnings
 *  #48-D: incompatible redefinition of macro "TRUE"
 *  #48-D: incompatible redefinition of macro "FALSE" 
 */
#pragma diag_push
#pragma diag_suppress 48
#include <ti/mathlib/mathlib.h>
#pragma diag_pop

/* Related to linker copy table for copying from L3 to L1PSRAM for example */
#include <cpy_tbl.h>

/**************************************************************************
 *************************** MmwDemo External DSS Functions ******************
 **************************************************************************/
extern int32_t MmwaveLink_initLink (rlUInt8_t deviceType, rlUInt8_t platform, void* asyncEventHandler(uint8_t devIndex, uint16_t sbId, uint16_t sbLen, uint8_t *payload));
extern int32_t MmwaveLink_getRfBootupStatus (void);
extern int32_t MmwaveLink_getVersion (rlVersion_t *verArgs);
extern int32_t MmwaveLink_setChannelConfig (rlChanCfg_t *chCfg);
extern int32_t MmwaveLink_setAdcOutConfig (rlAdcOutCfg_t *adcOutCfgArgs);
extern int32_t MmwaveLink_setLowPowerModeConfig (rlLowPowerModeCfg_t *lowPowerModeCfg);
extern int32_t MmwaveLink_setHsiClk (uint16_t hsiClk);
extern int32_t MmwaveLink_rfCalibration (void);
extern int32_t MmwaveLink_setProfileConfig (rlProfileCfg_t *pProfileCfg, uint16_t cnt);
extern int32_t MmwaveLink_setChirpConfig (rlChirpCfg_t *pChirpCfg, uint16_t cnt);
extern int32_t MmwaveLink_setFrameConfig (rlFrameCfg_t *frameCfg);
extern int32_t MmwaveLink_sensorStart (void);
extern int32_t MmwaveLink_sensorStop (void);
extern int32_t MmwaveLink_setCalMonConfig (uint16_t freqLimitLow, uint16_t freqLimitHigh);
extern int32_t MmwaveLink_setInitTimeCalibConfig (void);
extern int32_t MmwaveLink_setRunTimeCalibConfig (uint32_t calibPeriodicity);
extern int32_t MmwaveLink_setAdvFrameConfig (rlAdvFrameCfg_t *advFrameCfg);
extern int32_t MmwaveLink_setBpmCommonConfig (void);
extern int32_t MmwaveLink_setBpmChirpConfig (rlBpmChirpCfg_t *pBpmChirpCfg);
extern int32_t MmwaveLink_setContModeConfig (rlContModeCfg_t *contModeCfg);
extern int32_t MmwaveLink_enableContMode (uint8_t bEnable);
extern int32_t MmwaveLink_setRfDevCfg (rlUInt32_t);
extern uint32_t MmwaveLink_getSpawnCount(void);
extern void MmwaveLink_mmwaveLinkMgmtTask(void);
extern void dssStatusInfo(uint16_t eventType, int32_t errVal);
void nonOsLoop(void);



/**************************************************************************
 *************************** Global Definitions ********************************
 **************************************************************************/
/**
 * @brief
 *  DSS stores demo output and DSS to MSS ISR information (for fast exception 
 *  signalling) in HSRAM.
 */
/*!   */
typedef struct MmwDemo_HSRAM_t_ {
#define MMW_DATAPATH_DET_PAYLOAD_SIZE (SOC_HSRAM_SIZE -  sizeof(uint8_t))
    /*! @brief data path processing/detection related message payloads, these
               messages are signalled through DSS to MSS mailbox */ 
    uint8_t  dataPathDetectionPayload[MMW_DATAPATH_DET_PAYLOAD_SIZE];

    /*! @brief Information relayed through DSS triggering software interrupt to
               MSS. It stores one of the exception IDs @ref DSS_TO_MSS_EXCEPTION_IDS */
    uint8_t  dss2MssIsrInfo;
} MmwDemo_HSRAM_t;

#pragma DATA_SECTION(gHSRAM, ".demoSharedMem");
#pragma DATA_ALIGN(gHSRAM, 4);
MmwDemo_HSRAM_t gHSRAM;

#define RADAR_START          0x55
#define RADAR_STOP            0xAA


/**
 * @brief
 *  Global Variable for tracking information required by the mmw Demo
 */
MmwDemo_DSS_MCB    gMmwDssMCB;

volatile cycleLog_t gCycleLog;
rlRfTempData_t tempdata;
/**************************************************************************
 ************************* MmwDemo Functions Prototype  **********************
 **************************************************************************/

/* Internal DataPath Functions */
int32_t dssDataPathInit(void);
static int32_t dssDataPathConfig(void);
static int32_t dssDataPathStart(bool doRFStart);
static int32_t dssDataPathStop(void);
static int32_t dssDataPathProcessEvents(unsigned int event);
static int32_t dssDataPathReconfig(MmwDemo_DSS_DataPathObj *obj);


/* Internal Interrupt handler */
static void dssChirpIntHandler(uintptr_t arg);
static void dssFrameStartIntHandler(uintptr_t arg);

/* Internal mmwDemo Tasks running on DSS */
static void dssInitTask();
static void dssDataPathTask(unsigned int event);


static int32_t MmwDemo_dssSendProcessOutputToMSS
(
    MmwDemo_DSS_DataPathObj   *obj
);
void chirpProcess(MmwDemo_DSS_DataPathObj *obj, uint16_t chirpIndx);
void interFrameProcessing(MmwDemo_DSS_DataPathObj * obj);

/**************************************************************************
 *************************** MmwDemo DSS Functions **************************
 **************************************************************************/
/**
 *  @b Description
 *  @n
 *      Radar Link Registered Callback function to handle asynchronous events
 *
 *  @retval
 *      Success - 0
 *  @retval
 *      Error   - <0
 */
 volatile uint32_t gFrameStartStatus = 0U;
 uint32_t gInitTimeCalibStatus = 0U;
 uint32_t gRunTimeCalibStatus = 0U;

static void asyncEventHandler(uint8_t devIndex, uint16_t sbId, uint16_t sbLen, uint8_t *payload)
{
    uint16_t asyncSB = RL_GET_SBID_FROM_UNIQ_SBID(sbId);
    uint16_t msgId   = RL_GET_MSGID_FROM_SBID(sbId);

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
                    printf ("Debug: CPU Fault has been detected\n");
                    dssStatusInfo(MMWDEMO_BSS_CPUFAULT_EVT, -1);
                    /* BSS fault */
                    MmwDemo_dssAssert(0);
                    break;
                }
                case RL_RF_AE_ESMFAULT_SB:
                {
                    printf ("Debug: ESM Fault has been detected\n");
                    dssStatusInfo(MMWDEMO_BSS_CPUFAULT_EVT, -1);
                    /* BSS fault */
                    MmwDemo_dssAssert(0);
                    break;
                }
                case RL_RF_AE_INITCALIBSTATUS_SB:
                {
                    gInitTimeCalibStatus = ((rlRfInitComplete_t*)payload)->calibStatus;
                    if(gInitTimeCalibStatus != 0U)
                    {
                        printf ("Debug: Init time calibration status [0x%x] \n", gInitTimeCalibStatus);
                    }
                    else
                    {
                        printf ("Error: All Init time calibrations Failed:\n");
                    }
                    break;
                }
                case RL_RF_AE_FRAME_TRIGGER_RDY_SB:
                {
                    gFrameStartStatus = 1U;
                    break;
                }
                case RL_RF_AE_MON_TIMING_FAIL_REPORT_SB:
                {
                    printf ("Debug: Monitoring FAIL Report received \n");
                    break;
                }
                case RL_RF_AE_RUN_TIME_CALIB_REPORT_SB:
                {
                    gRunTimeCalibStatus = ((rlRfRunTimeCalibReport_t*)payload)->calibErrorFlag;
                    if(gRunTimeCalibStatus == 0U)
                    {
                        printf ("Error: All Run time calibrations Failed:\n");
                    }
                    break;
                }
                case RL_RF_AE_MON_DIG_PERIODIC_REPORT_SB:
                {
                    break;
                }
                case RL_RF_AE_MON_TEMPERATURE_REPORT_SB:
                {

                    break;
                }
                case RL_RF_AE_MON_REPORT_HEADER_SB:
                {
                    break;
                }
                case RL_RF_AE_FRAME_END_SB:
                {
                    gFrameStartStatus = 0U;

                    Event_post(gMmwDssMCB.eventHandle, BSS_STOP_COMPLETE_EVT);
                    printf ("Debug:  Frame Stop Async Event \n");
                    break;
                }
                case RL_RF_AE_ANALOG_FAULT_SB:
                {
                    printf ("Debug:  Analog Fault Async Event \n");
                    break;
                }
                default:
                {
                    printf ("Error: Asynchronous Event SB Id %d not handled with msg ID [0x%x] \n", asyncSB,msgId);
                    break;
                }
            }
            break;
        }
        default:
        {
            printf ("Error: Asynchronous message %d is NOT handled\n", msgId);
            break;
        }
    }
    return;
}

/*
 *  Interrupt handler callback for chirp available ISR. It runs in the ISR context.
 *  It check if previous processing is completed:
 *  if not, assert error and exit
 *  if so, posts chirp available event for further processing
 */
static void dssChirpIntHandler(uintptr_t arg)
{
    // Exit if sensor is stopped or if interframe processing is not
    // finished yet
    if((gMmwDssMCB.state == DSS_STATE_STOPPED) ||
       (gMmwDssMCB.dataPathContext.interFrameProcToken <= 0) || gMmwDssMCB.dataPathContext.realTimeError == true)
    {
        return;
    }

    // Check if previous chirp processing has completed: if not, send a message to MSS
    // Otherwise, post event to notify chirp available interrupt
        if (gMmwDssMCB.dataPathContext.chirpProcToken != 0  )
        {
            if (gMmwDssMCB.state == DSS_STATE_STARTED){
                gMmwDssMCB.dataPathContext.realTimeError = true;
                DebugP_assert(0);
            }
        }else{
            gMmwDssMCB.dataPathContext.chirpProcToken++;
            Event_post(gMmwDssMCB.eventHandle, CHIRP_EVT);
        }

}

/*
 *  Interrupt handler callback for frame start ISR. It runs in the ISR context.
 *  It check if previous processing is completed:
 *  if not, assert error and exit
 *  if so, posts chirp available event for further processing
 */

static void dssFrameStartIntHandler(uintptr_t arg)
{
    // Exit if sensor is stopped
    if(gMmwDssMCB.state == DSS_STATE_STOPPED )
        return;

    // Check if previous chirp processing has completed: if not, send a message to MSS
    // Otherwise, post event to notify frame start interrupt
        if (gMmwDssMCB.dataPathContext.interFrameProcToken != 0 )
        {
            if (gMmwDssMCB.state == DSS_STATE_STARTED ){
                if (gMmwDssMCB.dataPathContext.realTimeError == false)
                    DebugP_assert(0);

                gMmwDssMCB.dataPathContext.realTimeError = true;

            }
        }else{
            gMmwDssMCB.dataPathContext.interFrameProcToken++;
            Event_post(gMmwDssMCB.eventHandle, FRAMESTART_EVT);
        }

        /* Increment interrupt counter for debugging purpose */
        if (gMmwDssMCB.subFrameIndx == 0)
            gMmwDssMCB.stats.frameStartIntCounter++;

}

/**
 *  @b Description
 *  @n
 *      Function to send a message to peer through Mailbox virtural channel
 *
 *  @param[in]  message
 *      Pointer to the Captuere demo message.
 *
 *  @retval
 *      Success    - 0
 *      Fail       < -1
 */
static int32_t mboxWrite(MmwDemo_message    *message)
{
    int32_t                  retVal = -1;

    retVal = Mailbox_write (gMmwDssMCB.peerMailbox, (uint8_t*)message, sizeof(MmwDemo_message));
    if (retVal == sizeof(MmwDemo_message))
    {
        retVal = 0;
    }
    return retVal;
}


static int32_t MmwDemo_mboxWrite(MmwDemo_message    *message)
{
    int32_t                  retVal = -1;

    retVal = Mailbox_write (gMmwDssMCB.peerMailbox, (uint8_t*)message, sizeof(MmwDemo_message));
    if (retVal == sizeof(MmwDemo_message))
    {
        retVal = 0;
    }
    return retVal;
}

static void cfgUpdate(void *srcPtr, uint32_t offset, uint32_t size, int8_t subFrameNum)
{    
    /* if subFrameNum undefined, broadcast to all sub-frames */
    if(subFrameNum == MMWDEMO_SUBFRAME_NUM_FRAME_LEVEL_CONFIG)
    {
        uint8_t  indx;
        for(indx = 0; indx < RL_MAX_SUBFRAMES; indx++)
        {
            memcpy((void *)((uint32_t) &gMmwDssMCB.cliCfg[indx] + offset), srcPtr, size);
        }
        
    }
    else
    {
        /* Apply configuration to specific subframe (or to position zero for the legacy case
           where there is no advanced frame config) */
        memcpy((void *)((uint32_t) &gMmwDssMCB.cliCfg[subFrameNum] + offset), srcPtr, size);
    }
}

/**
 *  @b Description
 *  @n
 *      Function that acts upon receiving message that BSS has stopped
 *      successfully.
 *
 *  @retval
 *      Not applicable
 */
static void bssStopDone(void)
{    
    /*Change state to stop_pending*/
    gMmwDssMCB.state = DSS_STATE_STOP_PENDING;
    
    printf ("bssStopDone DSS_STATE_STOP_PENDING \n");

    gMmwDssMCB.dataPathContext.interFrameProcToken = 0;
         Event_post(gMmwDssMCB.eventHandle, DATAPATH_STOP_EVT);
}

/**
 *  @b Description
 *  @n
 *      The Task is used to handle  the mmw demo messages received from Mailbox virtual channel.
 *
 *  @param[in]  arg0
 *      arg0 of the Task. Not used
 *  @param[in]  arg1
 *      arg1 of the Task. Not used
 *
 *  @retval
 *      Not Applicable.
 */

static void mboxReadTask(unsigned int arg0, unsigned int arg1)
{
    MmwDemo_message      message;
    int32_t              retVal = 0;
    int8_t               subFrameNum;

    /* Read the message from the peer mailbox: We are not trying to protect the read
     * from the peer mailbox because this is only being invoked from a single thread */
    retVal = Mailbox_read(gMmwDssMCB.peerMailbox, (uint8_t*)&message, sizeof(MmwDemo_message));
    if (retVal < 0)
    {
        /* Error: Unable to read the message. Setup the error code and return values */
        printf ("dss Error: Mailbox read failed [Error code %d]\n", retVal);
    }
    else if (retVal == 0)
    {
        /* We are done: There are no messages available from the peer execution domain. */
    }
    else
    {
        /* Flush out the contents of the mailbox to indicate that we are done with the message. This will
         * allow us to receive another message in the mailbox while we process the received message. */
        Mailbox_readFlush (gMmwDssMCB.peerMailbox);

        /* Process the received message: */
        subFrameNum = message.subFrameNum;                     

        switch (message.type)
        {

            case MMWDEMO_MSS2DSS_GUIMON_CFG:
            {
                /* Save guimon configuration */
                cfgUpdate((void *)&message.body.vitalSigns_GuiMonSel,
                              offsetof(MmwDemo_CliCfg_t, vitalSignsGuiMonSel),
                              sizeof(VitalSignsDemo_GuiMonSel), subFrameNum);
                break;
            }

            case MMWDEMO_MSS2DSS_VITALSIGNS_MEASUREMENT_PARAMS:
            {
               cfgUpdate((void *)&message.body.vitalSignsParamsCfg,
                                   offsetof(MmwDemo_CliCfg_t, vitalSignsParamsCfg),
                                   sizeof(VitalSignsDemo_ParamsCfg), subFrameNum);
               break;
            }

            case MMWDEMO_MSS2DSS_ADCBUFCFG:
            {
                /* Save ADCBUF configuration */ 
                cfgUpdate((void *)&message.body.adcBufCfg,
                                     offsetof(MmwDemo_CliCfg_t, adcBufCfg),
                                     sizeof(MmwDemo_ADCBufCfg), subFrameNum);
                break;
            }
            case MMWDEMO_MSS2DSS_CHIRP_CFG:
            {
                memcpy((void *) &gMmwDssMCB.chirpCfg[gMmwDssMCB.chirpCfgRcvCnt], (void *)&message.body.chirpCfg, sizeof(rlChirpCfg_t));
                gMmwDssMCB.chirpCfgRcvCnt++;
                break;
            }
            case MMWDEMO_MSS2DSS_PROFILE_CFG:
            {
                memcpy((void *)&gMmwDssMCB.profileCfg[gMmwDssMCB.profileCfgRcvCnt], (void *)&message.body.profileCfg, sizeof(rlProfileCfg_t));
                gMmwDssMCB.profileCfgRcvCnt++;

                break;
            }

            case MMWDEMO_MSS2DSS_CONTROL_CFG:
            {
                memcpy((void*) &gMmwDssMCB.cfg.ctrlCfg, (void *)&message.body.ctrlCfg, sizeof(MMWave_CtrlCfg));
                /* Post event to notify configuration is done */
                Event_post(gMmwDssMCB.eventHandle, CONFIG_EVT);
                break;
            }
            case MMWDEMO_MSS2DSS_OPEN_CFG:
            {
                memcpy((void *) &gMmwDssMCB.cfg.openCfg, (void *)&message.body.openCfg, sizeof(MMWave_OpenCfg));
                break;
            }
            case MMWDEMO_MSS2DSS_SENSOR_START_CMD:
            {
                gMmwDssMCB.startStopCmd = RADAR_START;

                gMmwDssMCB.dataPathContext.realTimeError = false;
                gMmwDssMCB.dataPathContext.realTimeErrorNotify = false;
                gMmwDssMCB.bssIsStopped = false;

                break;
            }
            case MMWDEMO_MSS2DSS_SENSOR_STOP_CMD:
            {
                printf("DSS has received MMWDEMO_MSS2DSS_SENSOR_STOP_CMD command\n");
                gMmwDssMCB.startStopCmd = RADAR_STOP;

                gMmwDssMCB.chirpCfgRcvCnt = 0;
                gMmwDssMCB.profileCfgRcvCnt = 0;

                break;
            }

            default:
            {
                /* Message not support */
                printf ("DSS Error: unsupported Mailbox message id=%d\n", message.type);
                MmwDemo_dssAssert(0);
                break;
            }
        }
    }
}

/**
 *  @b Description
 *  @n
 *      This function is a callback funciton that invoked when a message is received from the peer.
 *
 *  @param[in]  handle
 *      Handle to the Mailbox on which data was received
 *  @param[in]  peer
 *      Peer from which data was received

 *  @retval
 *      Not Applicable.
 */
void mboxCallback
(
    Mbox_Handle  handle,
    Mailbox_Type    peer
)
{
    /* Message has been received from the peer endpoint. Wakeup the mmWave thread to process
     * the received message. */
    SemaphoreP_post (gMmwDssMCB.mboxSemHandle);
}



/***
 * This function fills the last part of the buffer for the user interface (detected objects)
 *  and writes it into the mailbox
 ***/
int32_t MmwDemo_dssSendProcessOutputToMSS
(
        MmwDemo_DSS_DataPathObj   *obj
)
{
    uint8_t             *ptrCurrBuffer;
    uint32_t            totalHsmSize = 0;
    uint32_t            totalPacketLen = sizeof(MmwDemo_output_message_header);
    uint32_t            itemPayloadLen;
    int32_t             retVal = 0;
    MmwDemo_message     message;
    uint32_t            tlvIdx;

    uint8_t        *ptrHsmBuffer = &gHSRAM.dataPathDetectionPayload[0];
    uint32_t       outputBufSize =        (uint32_t)MMW_DATAPATH_DET_PAYLOAD_SIZE;

    itemPayloadLen = 0;
    tlvIdx = 0;

    /* Clear message to MSS */
    memset((void *)&message, 0, sizeof(MmwDemo_message));

    /* Set message type */
    message.type = MMWDEMO_DSS2MSS_DATA_READY;

    /* Header: */
    message.body.detObj.header.platform = 0xA1642;
    message.body.detObj.header.magicWord[0] = 0x0102;
    message.body.detObj.header.magicWord[1] = 0x0304;
    message.body.detObj.header.magicWord[2] = 0x0506;
    message.body.detObj.header.magicWord[3] = 0x0708;
    message.body.detObj.header.numDetectedObj = 99;
    message.body.detObj.header.version =    MMWAVE_SDK_VERSION_BUILD |   //DEBUG_VERSION
            (MMWAVE_SDK_VERSION_BUGFIX << 8) |
            (MMWAVE_SDK_VERSION_MINOR << 16) |
            (MMWAVE_SDK_VERSION_MAJOR << 24);


    /* Set pointer to HSM buffer */
    ptrCurrBuffer = ptrHsmBuffer;


    /* Filling System info data */

    if (obj->transmitSysInfo == 1){

        SystemInfo sysInfo = obj->systemInfo;
        itemPayloadLen = sizeof(SystemInfo);
        totalHsmSize += itemPayloadLen;
        if(totalHsmSize > outputBufSize)
        {
            printf("too many output data!");
            return -1;
        }
        memcpy(ptrCurrBuffer, (void *)&sysInfo, itemPayloadLen);

        message.body.detObj.tlv[tlvIdx].length = itemPayloadLen;
        message.body.detObj.tlv[tlvIdx].type = MMWDEMO_OUTPUT_MSG_SYSINFO;
        message.body.detObj.tlv[tlvIdx].address = (uint32_t) ptrCurrBuffer;

        tlvIdx++;
        totalPacketLen += sizeof(MmwDemo_output_message_tl) + itemPayloadLen;

        // Incrementing pointer to HSM buffer
        ptrCurrBuffer = (uint8_t *)((uint32_t)ptrHsmBuffer + totalHsmSize);
    }

#if BASIC_PROCESSING_ENA == 1
    if (obj->transmitVitalSigns == 1){

        /* Filling vital signs results */
        VitalSignsDemo_OutputStats vitalSignsOut = obj->VitalSigns_Output;
        itemPayloadLen = sizeof(VitalSignsDemo_OutputStats);
        totalHsmSize += itemPayloadLen;
        if(totalHsmSize > outputBufSize)
        {
            printf("too many output data!");
            return -1;
        }
        //      DESTINATION -          SOURCE -            LENGTH
        memcpy(ptrCurrBuffer, (void *)&vitalSignsOut, itemPayloadLen);

        message.body.detObj.tlv[tlvIdx].length = itemPayloadLen;
        message.body.detObj.tlv[tlvIdx].type = MMWDEMO_OUTPUT_MSG_STATS;
        message.body.detObj.tlv[tlvIdx].address = (uint32_t) ptrCurrBuffer;

        tlvIdx++;
        totalPacketLen += sizeof(MmwDemo_output_message_tl) + itemPayloadLen;
        ptrCurrBuffer = (uint8_t *)((uint32_t)ptrHsmBuffer + totalHsmSize);
    }
#endif

    if (obj->transmitRangeProfile == 1){

        /* Filling range profile */
        ptrCurrBuffer = (uint8_t *)((uint32_t)ptrHsmBuffer + totalHsmSize);
        itemPayloadLen = obj->numRangeBinsProc *sizeof(cmplx16ImRe_t);
        totalHsmSize += itemPayloadLen;
        if(totalHsmSize > outputBufSize)
        {
            printf("too many output data!");
            return -1;
        }
        memcpy(ptrCurrBuffer, (void *)obj->pRangeProfileCplx,
               itemPayloadLen);

        message.body.detObj.tlv[tlvIdx].length = itemPayloadLen;
        message.body.detObj.tlv[tlvIdx].type = MMWDEMO_OUTPUT_MSG_RANGE_PROFILE;
        message.body.detObj.tlv[tlvIdx].address = (uint32_t) ptrCurrBuffer;

        tlvIdx++;
        totalPacketLen += sizeof(MmwDemo_output_message_tl) + itemPayloadLen;
        ptrCurrBuffer = (uint8_t *)((uint32_t)ptrHsmBuffer + totalHsmSize);
    }

    // OUR HEATMAP (Range-Azimuth)
/*
    if (obj->transmitAdcData == 1){

        // Heatmap data
        itemPayloadLen = obj->numAzBinsCalc*sizeof(cmplx16ReIm_t);
        totalHsmSize += itemPayloadLen;
        if(totalHsmSize > outputBufSize)
        {
            printf("too many output data!");
            return -1;
        }
        memcpy(ptrCurrBuffer, (void *)obj->rangeAzMap, itemPayloadLen);

        message.body.detObj.tlv[tlvIdx].length = itemPayloadLen;
        message.body.detObj.tlv[tlvIdx].type = MMWDEMO_OUTPUT_MSG_ADC_DATA;
        message.body.detObj.tlv[tlvIdx].address = (uint32_t) ptrCurrBuffer;

        tlvIdx++;
        totalPacketLen += sizeof(MmwDemo_output_message_tl) + itemPayloadLen;
        ptrCurrBuffer = (uint8_t *)((uint32_t)ptrHsmBuffer + totalHsmSize);
    }
*/
 //OLD CODE

     if (obj->transmitAdcData == 1){

        /* Adc data */
        itemPayloadLen = obj->numAdcSamples*obj->numRxAntennas *
                obj->numChirpsProc*sizeof(cmplx16ReIm_t);
        totalHsmSize += itemPayloadLen;
        if(totalHsmSize > outputBufSize)
        {
            printf("too many output data!");
            return -1;
        }
        memcpy(ptrCurrBuffer, (void *)obj->adcDataCube, itemPayloadLen);

        message.body.detObj.tlv[tlvIdx].length = itemPayloadLen;
        message.body.detObj.tlv[tlvIdx].type = MMWDEMO_OUTPUT_MSG_ADC_DATA;
        message.body.detObj.tlv[tlvIdx].address = (uint32_t) ptrCurrBuffer;

        tlvIdx++;
        totalPacketLen += sizeof(MmwDemo_output_message_tl) + itemPayloadLen;
        ptrCurrBuffer = (uint8_t *)((uint32_t)ptrHsmBuffer + totalHsmSize);
    }

    // Filling other info
    message.body.detObj.header.numTLVs = tlvIdx;
    /* Round up packet length to multiple of MMWDEMO_OUTPUT_MSG_SEGMENT_LEN */
    message.body.detObj.header.totalPacketLen = MMWDEMO_OUTPUT_MSG_SEGMENT_LEN *
            ((totalPacketLen + (MMWDEMO_OUTPUT_MSG_SEGMENT_LEN-1))/MMWDEMO_OUTPUT_MSG_SEGMENT_LEN);
    message.body.detObj.header.timeCpuCycles =  (uint32_t)(Cycleprofiler_getTimeStamp()/(float)600000);
    message.body.detObj.header.frameNumber = gMmwDssMCB.stats.frameStartIntCounter;
    message.body.detObj.header.subFrameNumber = gMmwDssMCB.subFrameIndx;

    /* Sending data to mailbox */
    if (MmwDemo_mboxWrite(&message) != 0)
        retVal = -1;

    return retVal;

}




/**
 *  @b Description
 *  @n
 *      Function to do Data Path Initialization on DSS.
 *
 *  @retval
 *      Not Applicable.
 */
static int32_t dataPathAdcBufInit(MmwDemo_DSS_dataPathContext_t *context)
{
    ADCBuf_Params       ADCBufparams;

    /*****************************************************************************
     * Initialize ADCBUF driver
     *****************************************************************************/
    ADCBuf_init();

    /* ADCBUF Params initialize */
    ADCBuf_Params_init(&ADCBufparams);
    ADCBufparams.chirpThresholdPing = 1;
    ADCBufparams.chirpThresholdPong = 1;
    ADCBufparams.continousMode  = 0;
    ADCBufparams.socHandle=gMmwDssMCB.socHandle; //for newer version of sdk;

    /* Open ADCBUF driver */
    context->adcbufHandle = ADCBuf_open(0, &ADCBufparams);
    if (context->adcbufHandle == NULL)
    {
        printf("Error: MMWDemoDSS Unable to open the ADCBUF driver\n");
        return -1;
    }
    printf("Debug: MMWDemoDSS ADCBUF Instance(0) %p has been opened successfully\n",
        context->adcbufHandle);

    return 0;
}

/**
 *  @b Description
 *  @n
 *      Function to do Data Path Initialization on DSS.
 *
 *  @retval
 *      Not Applicable.
 */
int32_t dssDataPathInit(void)
{
    int32_t retVal;
    SOC_SysIntListenerCfg  socIntCfg;
    int32_t errCode;
    MmwDemo_DSS_dataPathContext_t *context;
    uint32_t subFrameIndx;

    context = &gMmwDssMCB.dataPathContext;

    for(subFrameIndx = 0; subFrameIndx < RL_MAX_SUBFRAMES; subFrameIndx++)
    {
        MmwDemo_DSS_DataPathObj *obj;

        obj = &gMmwDssMCB.dataPathObj[subFrameIndx];
        MmwDemo_dataPathObjInit(obj,
                                context,
                                &gMmwDssMCB.cliCfg[subFrameIndx],
                                &gMmwDssMCB.cliCommonCfg);
        MmwDemo_dataPathInit1Dstate(obj);
    }


    retVal = MmwDemo_dataPathInitEdma(context);
    if (retVal < 0)
    {
        return -1;
    }

    retVal = dataPathAdcBufInit(context);
    if (retVal < 0)
    {
        return -1;
    }

    /* Register chirp interrupt listener */
    socIntCfg.systemInterrupt  = SOC_XWR18XX_DSS_INTC_EVENT_CHIRP_AVAIL;
    socIntCfg.listenerFxn      = dssChirpIntHandler;
    socIntCfg.arg              = (uintptr_t)NULL;
    if (SOC_registerSysIntListener(gMmwDssMCB.socHandle, &socIntCfg, &errCode) == NULL)
    {
        printf("Error: Unable to register chirp interrupt listener , error = %d\n", errCode);
        return -1;
    }

    /* Register frame start interrupt listener */
    socIntCfg.systemInterrupt  = SOC_XWR18XX_DSS_INTC_EVENT_FRAME_START;
    socIntCfg.listenerFxn      = dssFrameStartIntHandler;
    socIntCfg.arg              = (uintptr_t)NULL;
    if (SOC_registerSysIntListener(gMmwDssMCB.socHandle, &socIntCfg, &errCode) == NULL)
    {
        printf("Error: Unable to register frame start interrupt listener , error = %d\n", errCode);
        return -1;
    }

    return 0;
}

/**
 *  @b Description
 *  @n
 *      Function to configure ADCBUF driver based on CLI inputs.
 *  @param[in] ptrDataPathObj Pointer to data path object.
 *
 *  @retval
 *      0 if no error, else error (there will be system prints for these).
 */
static int32_t dssDataPathConfigAdcBuf(MmwDemo_DSS_DataPathObj *ptrDataPathObj)
{
    ADCBuf_dataFormat   dataFormat;
    ADCBuf_RxChanConf   rxChanConf;
    int32_t             retVal;
    uint8_t             channel;
    uint8_t             numBytePerSample = 0;
    MMWave_OpenCfg*     ptrOpenCfg;
    uint32_t            chirpThreshold;
    uint32_t            rxChanMask = 0xF;
    uint32_t            maxChirpThreshold;
    uint32_t            bytesPerChirp;
    MmwDemo_DSS_dataPathContext_t *context = ptrDataPathObj->context;
    MmwDemo_ADCBufCfg   *adcBufCfg = &ptrDataPathObj->cliCfg->adcBufCfg;

    /* Get data path object and control configuration */
    ptrOpenCfg = &gMmwDssMCB.cfg.openCfg;

    /*****************************************************************************
     * Data path :: ADCBUF driver Configuration
     *****************************************************************************/
    /* Validate the adcFmt */
    if(adcBufCfg->adcFmt == 1)
    {
        /* Real dataFormat has 2 bytes */
        numBytePerSample =  2;
    }
    else if(adcBufCfg->adcFmt == 0)
    {
        /* Complex dataFormat has 4 bytes */
        numBytePerSample =  4;
    }
    else
    {
        MmwDemo_dssAssert(0); /* Data format not supported */
    }

    /* On XWR16xx only channel non-interleaved mode is supported */
    if(adcBufCfg->chInterleave != 1)
    {
        MmwDemo_dssAssert(0); /* Not supported */
    }

    /* Populate data format from configuration */
    dataFormat.adcOutFormat       = adcBufCfg->adcFmt;
    dataFormat.channelInterleave  = adcBufCfg->chInterleave;
    dataFormat.sampleInterleave   = adcBufCfg->iqSwapSel;

    /* Disable all ADCBuf channels */
    if ((retVal = ADCBuf_control(context->adcbufHandle, ADCBufMMWave_CMD_CHANNEL_DISABLE, (void *)&rxChanMask)) < 0)
    {
       printf("Error: Disable ADCBuf channels failed with [Error=%d]\n", retVal);
       return retVal;
    }

    retVal = ADCBuf_control(context->adcbufHandle, ADCBufMMWave_CMD_CONF_DATA_FORMAT, (void *)&dataFormat);
    if (retVal < 0)
    {
        printf ("Error: MMWDemoDSS Unable to configure the data formats\n");
        return -1;
    }

    memset((void*)&rxChanConf, 0, sizeof(ADCBuf_RxChanConf));

    // number of bytes for a single chirp
    bytesPerChirp = ptrDataPathObj->numAdcSamples * ptrDataPathObj->numRxAntennas * numBytePerSample;
    
    // maximum number of full chirps that can fit in the ADCBUF memory (32 kB)
    maxChirpThreshold = SOC_ADCBUF_SIZE / bytesPerChirp;
    if (maxChirpThreshold >= ptrDataPathObj->numChirpsPerFrame)
    {
        maxChirpThreshold = ptrDataPathObj->numChirpsPerFrame;
    }
    else
    {
        /* find largest divisor of numChirpsPerFrame no bigger than maxChirpThreshold */
        while (ptrDataPathObj->numChirpsPerFrame % maxChirpThreshold)
        {
            maxChirpThreshold--;
        }
    }

    /* ADCBuf control function requires argument alignment at 4 bytes boundary */
    chirpThreshold = adcBufCfg->chirpThreshold;
    
    /* if automatic, set to the calculated max */
    if (chirpThreshold == 0)
    {
        chirpThreshold = maxChirpThreshold;
    }
    else
    {
        if (chirpThreshold > maxChirpThreshold)
        {
            printf("Desired chirpThreshold %d higher than max possible of %d, setting to max\n",
                chirpThreshold, maxChirpThreshold);
            chirpThreshold = maxChirpThreshold;
        }
        else
        {
            /* check for divisibility of the user provided threshold */
            MmwDemo_dssAssert((ptrDataPathObj->numChirpsPerFrame % chirpThreshold) == 0);
        }
    }

    ptrDataPathObj->numChirpsPerChirpEvent = chirpThreshold;

    /* Enable Rx Channels */
    for (channel = 0; channel < SYS_COMMON_NUM_RX_CHANNEL; channel++)
    {
        if(ptrOpenCfg->chCfg.rxChannelEn & (0x1U << channel))
        {
            /* Populate the receive channel configuration: */
            rxChanConf.channel = channel;
            retVal = ADCBuf_control(context->adcbufHandle, ADCBufMMWave_CMD_CHANNEL_ENABLE, (void *)&rxChanConf);
            if (retVal < 0)
            {
                printf("Error: MMWDemoDSS ADCBuf Control for Channel %d Failed with error[%d]\n", channel, retVal);
                return -1;
            }
            rxChanConf.offset  += ptrDataPathObj->numAdcSamples * numBytePerSample * chirpThreshold;
        }
    }

    /* Set ping/pong chirp threshold: */
    retVal = ADCBuf_control(context->adcbufHandle, ADCBufMMWave_CMD_SET_PING_CHIRP_THRESHHOLD,
                            (void *)&chirpThreshold);
    if(retVal < 0)
    {
        printf("Error: ADCbuf Ping Chirp Threshold Failed with Error[%d]\n", retVal);
        return -1;
    }
    retVal = ADCBuf_control(context->adcbufHandle, ADCBufMMWave_CMD_SET_PONG_CHIRP_THRESHHOLD,
                            (void *)&chirpThreshold);
    if(retVal < 0)
    {
        printf("Error: ADCbuf Pong Chirp Threshold Failed with Error[%d]\n", retVal);
        return -1;
    }

    return 0;
}

static uint16_t getChirpStartIdx(MMWave_CtrlCfg *cfg, uint8_t subFrameIndx)
{
    if (cfg->dfeDataOutputMode == MMWave_DFEDataOutputMode_ADVANCED_FRAME)
    {
        return(cfg->u.advancedFrameCfg.frameCfg.frameSeq.subFrameCfg[subFrameIndx].chirpStartIdx);
    }
    else 
    {
        return(cfg->u.frameCfg.frameCfg.chirpStartIdx);
    }
}

static uint16_t getChirpEndId(MMWave_CtrlCfg *cfg, uint8_t subFrameIndx)
{
    if (cfg->dfeDataOutputMode == MMWave_DFEDataOutputMode_ADVANCED_FRAME)
    {
        return(cfg->u.advancedFrameCfg.frameCfg.frameSeq.subFrameCfg[subFrameIndx].chirpStartIdx +
              (cfg->u.advancedFrameCfg.frameCfg.frameSeq.subFrameCfg[subFrameIndx].numOfChirps - 1));
    }
    else 
    {
        return(cfg->u.frameCfg.frameCfg.chirpEndIdx);
    }
}

static uint16_t getNumLoops(MMWave_CtrlCfg *cfg, uint8_t subFrameIndx)
{
    if (cfg->dfeDataOutputMode == MMWave_DFEDataOutputMode_ADVANCED_FRAME)
    {
        return(cfg->u.advancedFrameCfg.frameCfg.frameSeq.subFrameCfg[subFrameIndx].numLoops);
    }
    else 
    {
        return(cfg->u.frameCfg.frameCfg.numLoops);
    }
}

uint32_t getNumChirps(void)
{
    return (gMmwDssMCB.cfg.ctrlCfg.u.frameCfg.frameCfg.chirpEndIdx - \
            gMmwDssMCB.cfg.ctrlCfg.u.frameCfg.frameCfg.chirpStartIdx + 1);
}


rlChirpCfg_t MmwDemo_getChirpCfg(uint16_t chirpIdx)
{
    return (gMmwDssMCB.chirpCfg[chirpIdx]);
}

rlProfileCfg_t MmwDemo_getProfileCfg()
{
    return (gMmwDssMCB.profileCfg[0]);
}


/**
 *  @b Description
 *  @n
 *      parses Profile, Chirp and Frame config and extracts parameters
 *      needed for processing chain configuration
 */
bool parseProfileAndChirpConfig(MmwDemo_DSS_DataPathObj *dataPathObj,
    MMWave_CtrlCfg* ptrCtrlCfg, uint8_t subFrameIndx)
{
    uint16_t        frameChirpStartIdx;
    uint16_t        frameChirpEndIdx;
    uint16_t        numLoops;
    int16_t         frameTotalChirps;
    uint32_t        profileLoopIdx, chirpLoopIdx;
    bool            foundValidProfile = false;
    uint16_t        channelTxEn;
    uint8_t         channel;
    uint8_t         numRxChannels = 0;
    MMWave_OpenCfg* ptrOpenCfg;
    uint8_t         rxAntOrder [SYS_COMMON_NUM_RX_CHANNEL];
    uint8_t         txAntOrder [SYS_COMMON_NUM_TX_ANTENNAS];
    int32_t         i;
    int32_t         txIdx, rxIdx;

    /* Get data path object and control configuration */
    ptrOpenCfg = &gMmwDssMCB.cfg.openCfg;

    /* Get the Transmit channel enable mask: */
    channelTxEn = ptrOpenCfg->chCfg.txChannelEn;

    /* Find total number of Rx Channels */
    for (channel = 0; channel < SYS_COMMON_NUM_RX_CHANNEL; channel++)
    {
        rxAntOrder[channel] = 0;
        if(ptrOpenCfg->chCfg.rxChannelEn & (0x1U << channel))
        {
            rxAntOrder[numRxChannels] = channel;
            /* Track the number of receive channels: */
            numRxChannels += 1;
        }
    }
    dataPathObj->numRxAntennas = numRxChannels;
    dataPathObj->framePeriodicity_ms = (5e-6)*(gMmwDssMCB.cfg.ctrlCfg.u.frameCfg.frameCfg.framePeriodicity);  // 1 LSB = 5 ns ; (1e3)(5e-9) = 5e-6 ; 1e3 to convert sec to ms

    /* read frameCfg chirp start/stop*/
    if (ptrCtrlCfg->dfeDataOutputMode == MMWave_DFEDataOutputMode_ADVANCED_FRAME)
    {
        MmwDemo_dssAssert(
            ptrCtrlCfg->u.advancedFrameCfg.frameCfg.frameSeq.subFrameCfg[subFrameIndx].numOfBurst == 1);
    }

    /* read frameCfg chirp start/stop*/
    frameChirpStartIdx = getChirpStartIdx(ptrCtrlCfg, subFrameIndx);
    frameChirpEndIdx   = getChirpEndId(ptrCtrlCfg, subFrameIndx);
    numLoops = getNumLoops(ptrCtrlCfg, subFrameIndx);

    frameTotalChirps   = frameChirpEndIdx - frameChirpStartIdx + 1;

    /* since validChirpTxEnBits is static array of 32 */
    MmwDemo_dssAssert(frameTotalChirps <= 32);

    /* loop for profiles and find if it has valid chirps */
    /* we support only one profile in this processing chain */
    for (profileLoopIdx = 0;
        ((profileLoopIdx < MMWAVE_MAX_PROFILE) && (foundValidProfile == false));
        profileLoopIdx++)
    {
        uint32_t    mmWaveNumChirps = 0;
        bool        validProfileHasOneTxPerChirp = false;
        uint16_t    validProfileTxEn = 0;
        uint16_t    validChirpTxEnBits[32] = {0};

        /* get numChirps for this profile; skip error checking */
        mmWaveNumChirps = getNumChirps();
        /* loop for chirps and find if it has valid chirps for the frame
           looping around for all chirps in a profile, in case
           there are duplicate chirps
         */
        for (chirpLoopIdx = 0; chirpLoopIdx < mmWaveNumChirps; chirpLoopIdx++)
        {
            rlChirpCfg_t chirpCfg = MmwDemo_getChirpCfg(chirpLoopIdx);

            uint16_t chirpTxEn = chirpCfg.txEnable;
            /* do chirps fall in range and has valid antenna enabled */
            if ((chirpCfg.chirpStartIdx >= frameChirpStartIdx) &&
                (chirpCfg.chirpEndIdx <= frameChirpEndIdx) &&
                ((chirpTxEn & channelTxEn) > 0))
            {
                uint16_t idx = 0;
                for (idx = (chirpCfg.chirpStartIdx - frameChirpStartIdx);
                     idx <= (chirpCfg.chirpEndIdx - frameChirpStartIdx); idx++)
                {
                    validChirpTxEnBits[idx] = chirpTxEn;
                    foundValidProfile = true;
                }

            }
        }

        /* now loop through unique chirps and check if we found all of the ones
           needed for the frame and then determine the azimuth antenna
           configuration
         */
        if (foundValidProfile) {
            for (chirpLoopIdx = 0; chirpLoopIdx < frameTotalChirps; chirpLoopIdx++)
            {
                bool validChirpHasOneTxPerChirp = false;
                uint16_t chirpTxEn = validChirpTxEnBits[chirpLoopIdx];
                if (chirpTxEn == 0) {
                    /* this profile doesnt have all the needed chirps */
                    foundValidProfile = false;
                    break;
                }
                validChirpHasOneTxPerChirp = ((chirpTxEn == 0x1) || (chirpTxEn == 0x2)||(chirpTxEn == 0x4));
                /* if this is the first chirp, record the chirp's
                   MIMO config as profile's MIMO config. We dont handle intermix
                   at this point */
                if (chirpLoopIdx==0) {
                    validProfileHasOneTxPerChirp = validChirpHasOneTxPerChirp;
                }
                /* check the chirp's MIMO config against Profile's MIMO config */
                if (validChirpHasOneTxPerChirp != validProfileHasOneTxPerChirp)
                {
                    /* this profile doesnt have all chirps with same MIMO config */
                    foundValidProfile = false;
                    break;
                }
                /* save the antennas actually enabled in this profile */
                validProfileTxEn |= chirpTxEn;
            }
        }

        /* found valid chirps for the frame; mark this profile valid */
        if (foundValidProfile == true) {
            rlProfileCfg_t  profileCfg;
            uint32_t        numTxAntAzim = 0;

            dataPathObj->numTxAntennas = 0;
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
                if (validProfileTxEn & 0x2)
                {
                    numTxAntAzim++;
                }
                if (validProfileTxEn & 0x4)
                                {
                                    numTxAntAzim++;
                                }
            }
            dataPathObj->numTxAntennas       = numTxAntAzim ;
            dataPathObj->numVirtualAntennas   =  dataPathObj->numTxAntennas * dataPathObj->numRxAntennas;

            /* Copy the Rx channel compensation coefficients from common area to data path structure */
            if (validProfileHasOneTxPerChirp)
            {
                for (i = 0; i < dataPathObj->numTxAntennas; i++)
                {
                        txAntOrder[i] = MmwDemo_floorLog2(validChirpTxEnBits[i]);
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
            profileCfg = MmwDemo_getProfileCfg();

            /* multiplicity of 4 due to windowing library function requirement */
            if ((profileCfg.numAdcSamples % 4) != 0)
            {
                printf("Number of ADC samples must be multiple of 4\n");
                MmwDemo_dssAssert(0);
            }

            dataPathObj->rangeZeropad   = RANGE_ZEROPAD;
            dataPathObj->numAdcSamples       = profileCfg.numAdcSamples;
            dataPathObj->numRangeBinsCalc    = MmwDemo_pow2roundup(dataPathObj->rangeZeropad*dataPathObj->numAdcSamples);
            ///////////// AGGIUNTO ////////////////////
            dataPathObj->numAzBinsCalc       = AZIMUTH_ZEROPAD;
                                                //MmwDemo_pow2roundup(dataPathObj->numRxAntennas*dataPathObj->numTxAntennas);
            /////////////////////////////////////////
            dataPathObj->numChirpsPerFrame   = (frameChirpEndIdx -frameChirpStartIdx + 1) * numLoops;
            dataPathObj->numChirpsPerFramePerTx = dataPathObj->numChirpsPerFrame/dataPathObj->numTxAntennas;

            dataPathObj->numChirpsProc = 1;


#ifndef MMW_ENABLE_NEGATIVE_FREQ_SLOPE
            /* Check frequency slope */
            if (profileCfg.freqSlopeConst < 0)
            {
                printf("Frequency slope must be positive\n");
                MmwDemo_dssAssert(0);
            }
#endif
            dataPathObj->rangeResolution = MMWDEMO_SPEED_OF_LIGHT_IN_METERS_PER_SEC * 
                profileCfg.digOutSampleRate * 1e3 / 
                (2 * profileCfg.freqSlopeConst * ((3.6*1e3*900) /
                 (1U << 26)) * 1e12 * dataPathObj->numAdcSamples);

            dataPathObj->rangeAccuracy = dataPathObj->rangeResolution / dataPathObj->rangeZeropad;

            dataPathObj->systemInfo.rangeAccuracy = dataPathObj->rangeAccuracy;
            dataPathObj->systemInfo.framePeriodicity_ms = dataPathObj->framePeriodicity_ms;
            dataPathObj->systemInfo.numChirpsPerFrame = dataPathObj->numChirpsPerFrame;

        }
    }
    return foundValidProfile;
}

/**
 *  @b Description
 *  @n
 *      Function to do Data Path Re-Configuration on DSS.
 *      This is used when switching between sub-frames.
 *
 *  @retval
 *      -1 if error, 0 otherwise.
 */
static int32_t dssDataPathReconfig(MmwDemo_DSS_DataPathObj *obj)
{
    int32_t retVal;

    retVal = dssDataPathConfigAdcBuf(obj);
    if (retVal < 0)
    {
        return -1;
    }

    // Generates window and twiddle factors for ffts
    MmwDemo_dataPathConfigFFTs(obj);

    /* must be after dssDataPathConfigAdcBuf above as it calculates
       numChirpsPerChirpEvent that is used in EDMA configuration */
    MmwDemo_dataPathConfigEdma(obj);

    return 0;
}

/**
 *  @b Description
 *  @n
 *      Function to do Data Path Configuration on DSS.
 *
 *  @retval
 *      Not Applicable.
 */
static int32_t dssDataPathConfig(void)
{
    int32_t             retVal = 0;
    MMWave_CtrlCfg      *ptrCtrlCfg;
    MmwDemo_DSS_DataPathObj *dataPathObj;
    uint8_t subFrameIndx;

    /* Get data path object and control configuration */
    ptrCtrlCfg   = &gMmwDssMCB.cfg.ctrlCfg;

    if (ptrCtrlCfg->dfeDataOutputMode == MMWave_DFEDataOutputMode_ADVANCED_FRAME)
    {
        gMmwDssMCB.numSubFrames = 
            ptrCtrlCfg->u.advancedFrameCfg.frameCfg.frameSeq.numOfSubFrames;
    }
    else
    {
        gMmwDssMCB.numSubFrames = 1;
    }


    for(subFrameIndx = 0; subFrameIndx < gMmwDssMCB.numSubFrames; subFrameIndx++)
    {
        dataPathObj  = &gMmwDssMCB.dataPathObj[subFrameIndx];

        /*****************************************************************************
         * Data path :: Algorithm Configuration
         *****************************************************************************/

        /* Parse the profile and chirp configs and get the valid number of TX Antennas */
        if (parseProfileAndChirpConfig(dataPathObj, ptrCtrlCfg, subFrameIndx) == true)
        {
            /* Data path configurations */
            MmwDemo_dataPathInitVitalSignsAndMonitor(dataPathObj);
            MmwDemo_dataPathConfigBuffers(dataPathObj, SOC_XWR18XX_DSS_ADCBUF_BASE_ADDRESS);

            /* Below configurations are to be reconfigured every sub-frame so do only for first one */
            if (subFrameIndx == 0)
            {
                retVal = dssDataPathReconfig(dataPathObj);
                if (retVal < 0)
                {
                    return -1;
                }
            }
        }
        else
        {
            /* no valid profile found - assert! */
            MmwDemo_dssAssert(0);
        }
    }


    return 0;
}

/**
 *  @b Description
 *  @n
 *      Function to start Data Path on DSS.
 *
 *  @retval
 *      Not Applicable.
 */
static int32_t dssDataPathStart(bool doRFStart)
{
    gMmwDssMCB.dataPathContext.chirpProcToken = 0;
    gMmwDssMCB.dataPathContext.interFrameProcToken = 0;

    return 0;
}

/**
 *  @b Description
 *  @n
 *      Function to process Data Path events at runtime.
 *
 *  @param[in]  event
 *      Data Path Event
 *
 *  @retval
 *      Not Applicable.
 */




static int32_t dssDataPathProcessEvents(unsigned int event)
{
    MmwDemo_DSS_DataPathObj *dataPathObj;

    dataPathObj = &gMmwDssMCB.dataPathObj[gMmwDssMCB.subFrameIndx];
    /* Handle dataPath events */
    switch(event)
    {volatile uint32_t startTime;
        case CHIRP_EVT:

            // Process all chirps
            {
                uint16_t chirpIndex;
                for (chirpIndex = 0; chirpIndex < dataPathObj->numChirpsPerChirpEvent; chirpIndex++)
                     chirpProcess(dataPathObj, chirpIndex);

            }
            gMmwDssMCB.dataPathContext.chirpProcToken--;

            // If the frame is over, wait for the transfer of the last chirp data,
            // process the frame and send output data to mss
            if (dataPathObj->chirpCount == 0)
            {
                MmwDemo_waitEndOfChirps(dataPathObj);
                dataPathObj->cycleLog.interChirpProcessingTime = gCycleLog.interChirpProcessingTime;
                dataPathObj->cycleLog.interChirpWaitTime = gCycleLog.interChirpWaitTime;
                gCycleLog.interChirpProcessingTime = 0;
                gCycleLog.interChirpWaitTime = 0;

                startTime = Cycleprofiler_getTimeStamp();
                interFrameProcessing (dataPathObj);
                dataPathObj->cycleLog.interFrameProcessingTime = Cycleprofiler_getTimeStamp() - startTime;


                startTime = Cycleprofiler_getTimeStamp();
                if (gMmwDssMCB.dataPathContext.realTimeError == false)
                    MmwDemo_dssSendProcessOutputToMSS(dataPathObj);

                dataPathObj->cycleLog.dataTransferTime = Cycleprofiler_getTimeStamp() - startTime;

                if (gMmwDssMCB.dataPathContext.interFrameProcToken>0)
                    gMmwDssMCB.dataPathContext.interFrameProcToken--;

            }
            break;

        case FRAMESTART_EVT:

            MmwDemo_dssAssert(dataPathObj->chirpCount == 0);
            break;

        case MMWDEMO_BSS_FRAME_TRIGGER_READY_EVT:

            break;

        default:
            break;
    }
    return 0;
}

/**
 *  @b Description
 *  @n
 *      Function to stop Data Path on DSS. Assume BSS has been stopped by mmWave already.
 *      This also sends the STOP done message back to MSS to signal the procssing
 *      chain has come to a stop.
 *
 *  @retval
 *      Not Applicable.
 */
static int32_t dssDataPathStop(void)
{
    MmwDemo_message     message;
    uint32_t numFrameEvtsHandled = 0;


    /* move to stop state */
    gMmwDssMCB.state = DSS_STATE_STOPPED;

    /* send message back to MSS */
    message.type = MMWDEMO_DSS2MSS_STOPDONE;
    mboxWrite(&message);

    printf ("Debug: MMWDemoDSS Data Path stop succeeded stop%d,frames:%d \n",
                    gMmwDssMCB.stats.stopEvt,numFrameEvtsHandled);

    return 0;
}

/**
 *  @b Description
 *  @n
 *      Data Path main task that handles events from remote and do dataPath processing.
 *  This task is created when MSS is responsible for the mmwave Link and DSS is responsible
 *  for data path processing.
 *
 *  @retval
 *      Not Applicable.
 */
MmwDemo_message     message;
static void dssDataPathTask(unsigned int event)
{
    int32_t       retVal = 0;

    /************************************************************************
     * Data Path :: Config
     ************************************************************************/
    if((event & BSS_STOP_COMPLETE_EVT) && gMmwDssMCB.bssIsStopped == false)
    {
        bssStopDone();
        gMmwDssMCB.bssIsStopped = true;


    }

    /************************************************************************
     * Data Path process frame start event
     ************************************************************************/
    if(event & FRAMESTART_EVT)
    {
        if((gMmwDssMCB.state == DSS_STATE_STARTED) || (gMmwDssMCB.state == DSS_STATE_STOP_PENDING))
        {
            if ((retVal = dssDataPathProcessEvents(FRAMESTART_EVT)) < 0 )
            {
                printf ("Error: MMWDemoDSS Data Path process frame start event failed with Error[%d]\n",
                              retVal);
            }
        }
    }

    /************************************************************************
     * Data Path process chirp event
     ************************************************************************/
    if(event & CHIRP_EVT)
    {
        if((gMmwDssMCB.state == DSS_STATE_STARTED) || (gMmwDssMCB.state == DSS_STATE_STOP_PENDING))
        {
            if ((retVal = dssDataPathProcessEvents(CHIRP_EVT)) < 0 )
            {
                printf ("Error: MMWDemoDSS Data Path process chirp event failed with Error[%d]\n",
                              retVal);
            }
        }
    }

    /************************************************************************
     * Data Path re-config, only supported reconfiguration in stop state
     ************************************************************************/
    if(event & CONFIG_EVT)
    {
        if(gMmwDssMCB.state == DSS_STATE_STOPPED)
        {
            if ((retVal = dssDataPathConfig()) < 0 )
            {
                printf ("Debug: MMWDemoDSS Data Path config failed with Error[%d]\n",retVal);
                goto exit;
            }

            /************************************************************************
             * Data Path :: Start, mmwaveLink start will be triggered from DSS!
             ************************************************************************/
            if ((retVal = dssDataPathStart(true)) < 0 )
            {
                printf ("Error: MMWDemoDSS Data Path start failed with Error[%d]\n",retVal);
                goto exit;
            }
            gMmwDssMCB.state = DSS_STATE_STARTED;
        }
        else
        {
            printf ("Error: MMWDemoDSS Data Path config event in wrong state[%d]\n", gMmwDssMCB.state);
            goto exit;
        }
    }

    /************************************************************************
     * Quick start after stop //I donìt
     ************************************************************************/
    if(event & START_EVT)
    {
        if(gMmwDssMCB.state == DSS_STATE_STOPPED)
        {
            /* RF start is done by MSS in this case; so just do DSS start */
            if ((retVal = dssDataPathStart(false)) < 0 )
            {
                printf ("Error: MMWDemoDSS Data Path start failed with Error[%d]\n",retVal);
                goto exit;
            }
            gMmwDssMCB.state = DSS_STATE_STARTED;
        }
        else
        {
            printf ("Error: MMWDemoDSS Data Path config event in wrong state[%d]\n", gMmwDssMCB.state);
            goto exit;
        }
    }

    /************************************************************************
     * Data Path process frame start event
     ************************************************************************/
    if(event & DATAPATH_STOP_EVT)
    {
        if (gMmwDssMCB.state == DSS_STATE_STOP_PENDING)
        {



            /************************************************************************
             * Local Data Path Stop
             ************************************************************************/
            if ((retVal = dssDataPathStop()) < 0 )
            {
                printf ("Debug: MMWDemoDSS Data Path stop failed with Error[%d]\n",retVal);

            }
        }
    }
exit:
    return;
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


void MmwDemo_dataPathInitVitalSignsToDefault(){

    VitalSignsDemo_ParamsCfg  vitalSignsParamCLI ;
    VitalSignsDemo_GuiMonSel vitalSignsGuiMonCLI;

    vitalSignsParamCLI.startRange_m = RANGE_START_METERS;
    vitalSignsParamCLI.endRange_m = RANGE_END_METERS;
    vitalSignsParamCLI.rxAntennaProcess = RX_ANT_PROCESS;

    vitalSignsGuiMonCLI.adcData = 0;
    vitalSignsGuiMonCLI.rangeProfile = 0;
    vitalSignsGuiMonCLI.sysInfo = 1;
    vitalSignsGuiMonCLI.vitalSignsStats = 1;

    gMmwDssMCB.cliCfg[0].vitalSignsParamsCfg = vitalSignsParamCLI;
    gMmwDssMCB.cliCfg[0].vitalSignsGuiMonSel = vitalSignsGuiMonCLI;
}


static void dssInitTask()
{
    int32_t             errCode, retVal;
    SemaphoreP_Params    semParams;
    Mailbox_Config      mboxCfg;
    MmwDemo_message     message;
    //set L1D caceh to 16 kB
    *( unsigned int* )0x01840040 = 0x3;
    //set L2 cache to 0 KB
    *( unsigned int* )0x01840000=0x00;

    /* Initialize the Mailbox */
    Mailbox_init(MAILBOX_TYPE_DSS);


    /*****************************************************************************
     * Create mailbox Semaphore:
     *****************************************************************************/
    /* Create a binary semaphore which is used to handle mailbox interrupt. */
    SemaphoreP_Params_init(&semParams);
    semParams.mode             = SemaphoreP_Mode_BINARY;
    gMmwDssMCB.mboxSemHandle = SemaphoreP_create(0, &semParams);

    /* Setup the default mailbox configuration */
    Mailbox_Config_init(&mboxCfg);

    /* Setup the configuration: */
    mboxCfg.chType       = MAILBOX_CHTYPE_MULTI;
    mboxCfg.chId         = MAILBOX_CH_ID_0      ;
    mboxCfg.writeMode    = MAILBOX_MODE_BLOCKING;
    mboxCfg.readMode     = MAILBOX_MODE_CALLBACK;
    mboxCfg.readCallback = &mboxCallback;

    gMmwDssMCB.peerMailbox = Mailbox_open(MAILBOX_TYPE_MSS, &mboxCfg, &errCode);
    if (gMmwDssMCB.peerMailbox == NULL)
    {
        /* Error: Unable to open the mailbox */
        printf("Error: Unable to open the Mailbox to the DSS [Error code %d]\n", errCode);
        return;
    }

    /* Remote Mailbox is operational: Set the DSS Link State */
    SOC_setMMWaveDSSLinkState (gMmwDssMCB.socHandle, 1U, &errCode);

    /*****************************************************************************
     * Create Event to handle mmwave callback and system datapath events
     *****************************************************************************/
    /* Default instance configuration params */
    gMmwDssMCB.eventHandle = Event_create(NULL, 0);
    if (gMmwDssMCB.eventHandle == NULL)
    {
        /* FATAL_TBA */
        printf("Error: MMWDemoDSS Unable to create an event handle\n");
        return ;
    }
    printf("DSS create event handle succeeded\n");


    MmwDemo_dataPathInitVitalSignsToDefault();


    /******************************************************************************
     * TEST: Synchronization
     * - The synchronization API always needs to be invoked.
     ******************************************************************************/
    while (1)
    {
        int32_t syncStatus;
        /* Get the synchronization status: */
        syncStatus = SOC_isMMWaveMSSOperational (gMmwDssMCB.socHandle, &errCode);
        if (syncStatus < 0)
        {
            /* Error: Unable to synchronize the mmWave control module */
            printf ("Error: MMWDemoDSS mmWave Control Synchronization failed [Error code %d]\n", errCode);
            return;
        }
        if (syncStatus == 1)
        {
            /* Synchronization achieved: */
            break;
        }
    }

     /* Debug Message: */
    printf("Debug: DSS Mailbox Handle %p\n", gMmwDssMCB.peerMailbox);

    message.type = MMWDEMO_DSS2MSS_ISR_INFO_ADDRESS;
    message.body.dss2mssISRinfoAddress = (uint32_t) &gHSRAM.dss2MssIsrInfo;
    mboxWrite(&message);

    if ((retVal = MmwaveLink_initLink (RL_AR_DEVICETYPE_18XX, RL_PLATFORM_DSS, &asyncEventHandler)) < 0)
    {
        printf("mmwavelink init failed!!");
        MmwDemo_dssAssert(0);
    }

    if ((retVal = MmwaveLink_getRfBootupStatus()) < 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
    }

    rlVersion_t devVerData;
    if ((retVal = MmwaveLink_getVersion(&devVerData)) < 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
    }
    else
    {
        /* Send version Info to MSS */
        memset((void *)&message, 0, sizeof(MmwDemo_message));
        /* send these configurations to DSS over Mailbox */
        message.type = MMWDEMO_DSS2MSS_VERSION_INFO;
        memcpy((void *)&message.body.devVerArgs, (void *)&devVerData, sizeof(rlVersion_t));

        mboxWrite(&message);
        /* After sending version info, notify MSS about DSS mmwavelink init done */
        dssStatusInfo(MMWDEMO_DSS_INIT_CONFIG_DONE_EVT, retVal);
    }

    /* Send the DSS to MSS signal ISR payload address to the DSS. Note this
       should be done after both sides mailboxes have been opened, and because
       MMWave_sync above is a good one to check for synchronization, this is a good place */

    /*****************************************************************************
     * Data path Startup
     *****************************************************************************/
    if ((errCode = dssDataPathInit()) < 0 )
    {
        printf ("Error: MMWDemoDSS Data Path init failed with Error[%d]\n",errCode);
        return;
    }
    printf ("Debug: MMWDemoDSS Data Path init succeeded\n");
    gMmwDssMCB.state = DSS_STATE_INIT;

    nonOsLoop();

    printf("Debug: MMWDemoDSS initTask exit\n");
    return;
}


void StopRadar()
{
    int32_t retVal = MmwaveLink_sensorStop();
    /* Async Event Enable and Direction configuration */
    extern rlRfDevCfg_t rfDevCfg;

    if(retVal < 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
    }

}


void ConfigRadarSS()
{
    int32_t     retVal;
    uint8_t idx;

    /* Channel Config */
    if((retVal = MmwaveLink_setChannelConfig(&gMmwDssMCB.cfg.openCfg.chCfg)) != 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }

    if((retVal = MmwaveLink_setAdcOutConfig(&gMmwDssMCB.cfg.openCfg.adcOutCfg)) != 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }

    if((retVal = MmwaveLink_setLowPowerModeConfig(&gMmwDssMCB.cfg.openCfg.lowPowerMode)) != 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }

    if((retVal = MmwaveLink_setRfDevCfg(0xA)) != 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }

    /* Setup the HSI Clock as per the Radar Interface Document:
         * - This is set to 600Mhz DDR Mode */
    if((retVal = MmwaveLink_setHsiClk(0x9)) != 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }

    if((retVal = MmwaveLink_setCalMonConfig(gMmwDssMCB.cfg.openCfg.freqLimitLow, gMmwDssMCB.cfg.openCfg.freqLimitHigh)) != 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }

    if((retVal = MmwaveLink_setInitTimeCalibConfig()) != 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }

    if((retVal = MmwaveLink_rfCalibration()) < 0)
    {
        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
        return;
    }
    else
    {
        while(gInitTimeCalibStatus == 0)
        {
            MmwaveLink_mmwaveLinkMgmtTask();
        }
        dssStatusInfo(MMWDEMO_BSS_CALIBRATION_REP_EVT, gInitTimeCalibStatus);
    }

    switch(gMmwDssMCB.cfg.ctrlCfg.dfeDataOutputMode)
    {
        case MMWave_DFEDataOutputMode_FRAME:
        {
            /**************************************************************************
             * Frame Mode:
             * Order of operations as specified by the mmWave Link are
             *  - Profile configuration
             *  - Chirp configuration
             *  - Frame configuration
             **************************************************************************/

            if((retVal = MmwaveLink_setProfileConfig(&gMmwDssMCB.profileCfg[0], gMmwDssMCB.profileCfgRcvCnt)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }

            if((retVal = MmwaveLink_setChirpConfig(&gMmwDssMCB.chirpCfg[0], gMmwDssMCB.chirpCfgRcvCnt)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }

            for(idx = 0; idx < gMmwDssMCB.bpmChirpCfgRcvCnt; idx++)
            {
                if((retVal = MmwaveLink_setBpmChirpConfig(&gMmwDssMCB.bpmChirpCfg[idx])) != 0)
                {
                    dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                    return;
                }

                if(idx==0)
                {
                    if((retVal = MmwaveLink_setBpmCommonConfig()) != 0)
                    {
                        dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                        return;
                    }
                }
            }

            if((retVal = MmwaveLink_setFrameConfig(&gMmwDssMCB.cfg.ctrlCfg.u.frameCfg.frameCfg)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }

#if 0
            if((retVal = MmwaveLink_setRunTimeCalibConfig(10)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
            }
#endif
            break;
        }
        case MMWave_DFEDataOutputMode_CONTINUOUS:
        {
            if((retVal = MmwaveLink_setContModeConfig(&gMmwDssMCB.cfg.ctrlCfg.u.continuousModeCfg.cfg)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }
            break;
        }
        case MMWave_DFEDataOutputMode_ADVANCED_FRAME:
        {
            if((retVal = MmwaveLink_setChirpConfig(&gMmwDssMCB.chirpCfg[0], gMmwDssMCB.chirpCfgRcvCnt)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }
            if((retVal = MmwaveLink_setProfileConfig(&gMmwDssMCB.profileCfg[0], gMmwDssMCB.profileCfgRcvCnt)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }
            if((retVal = MmwaveLink_setBpmCommonConfig()) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }
            for(idx = 0; idx < gMmwDssMCB.bpmChirpCfgRcvCnt; idx++)
            {
                if((retVal = MmwaveLink_setBpmChirpConfig(&gMmwDssMCB.bpmChirpCfg[idx])) != 0)
                {
                    dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                    return;
                }
            }

            if((retVal = MmwaveLink_setAdvFrameConfig(&gMmwDssMCB.cfg.ctrlCfg.u.advancedFrameCfg.frameCfg)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }
            if((retVal = MmwaveLink_setRunTimeCalibConfig(10)) != 0)
            {
                dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
                return;
            }

            break;
        }
    }

    if(gMmwDssMCB.cfg.ctrlCfg.dfeDataOutputMode != MMWave_DFEDataOutputMode_CONTINUOUS)
    {
        if((retVal = MmwaveLink_sensorStart()) != 0)
        {
            dssStatusInfo(MMWDEMO_DSS_START_FAILED_EVT, retVal);
            return;
        }
        else
        {
            /* Notifying MSS about Frame trigger over Mailbox may take time which will cause
             * Non OS based DSS to miss processing of chirp data and further DSS will go into
             * ASSERT due to missing processing counts.
             * So notify MSS over SW interrupt */
        }
    }
    else
    {
        if((retVal = MmwaveLink_enableContMode(1)) != 0)
        {
            dssStatusInfo(MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT, retVal);
            return;
        }
    }

}




void nonOsLoop(void)
{
    unsigned int matchingEvents;
    MmwDemo_message      message;

    while(1)
    {
        if(SemaphoreP_pend(gMmwDssMCB.mboxSemHandle, SemaphoreP_NO_WAIT) == SemaphoreP_OK)
        {
            /* Create task to handle mailbox messges */
            mboxReadTask(0,0);
        }

        /* Check if any command received from MSS, then DSS needs to configure BSS */
        matchingEvents = Event_checkEvents(gMmwDssMCB.eventHandle, Event_Id_NONE, FRAMESTART_EVT | CHIRP_EVT |
                          BSS_STOP_COMPLETE_EVT | CONFIG_EVT |
                          DATAPATH_STOP_EVT | START_EVT);
        if(matchingEvents != 0)
        {
           /* Start data path task */
            dssDataPathTask(matchingEvents);
        }
        if (gMmwDssMCB.dataPathContext.realTimeError == true && gMmwDssMCB.dataPathContext.realTimeErrorNotify == false){
            message.type = DSS2MSS_REALTIME_ERROR;
            gMmwDssMCB.dataPathContext.realTimeErrorNotify = true;
            mboxWrite(&message);
        }
        /*****************************************************************************
         * Launch the mmWave control execution task
         * - This should have a higher priority than any other task which uses the
         *   mmWave control API
         *****************************************************************************/
        /* check if new SPAWN from mmWaveLink then process that */
        if(MmwaveLink_getSpawnCount() > 0)
        {
            MmwaveLink_mmwaveLinkMgmtTask();
        }

        /* if sensor-start command received */
        if(gMmwDssMCB.startStopCmd == RADAR_START)
        {
            ConfigRadarSS();
            gMmwDssMCB.startStopCmd = 0;
        }

        /* If sensor-stop command received */
        if(gMmwDssMCB.startStopCmd == RADAR_STOP)
        {
            printf("DSS : RADAR_STOP\n");
            gMmwDssMCB.startStopCmd = 0;
            StopRadar();
            printf("DSS has executed StopRadar\n");
            Event_post(gMmwDssMCB.eventHandle, BSS_STOP_COMPLETE_EVT);

        }

    }
}



void _MmwDemo_dssAssert(int32_t expression,const char *file, int32_t line)
{
    MmwDemo_message  message;
    uint32_t         nameSize;

    if (!expression)
    {
        message.type = MMWDEMO_DSS2MSS_ASSERT_INFO;
        nameSize = strlen(file);
        if(nameSize > MMWDEMO_MAX_FILE_NAME_SIZE)
            nameSize = MMWDEMO_MAX_FILE_NAME_SIZE;

        memcpy((void *) &message.body.assertInfo.file[0], (void *)file, nameSize);
        message.body.assertInfo.line = (uint32_t)line;
        if (mboxWrite(&message) != 0)
        {
            printf ("Error: Failed to send exception information to MSS.\n");
        }

    }
}



void dssStatusInfo(uint16_t eventType, int32_t errVal)
{
    MmwDemo_message  message;

    message.type = MMWDEMO_DSS2MSS_STATUS_INFO;
    message.body.dssStatusInfo.eventType = eventType;
    message.body.dssStatusInfo.errVal = errVal;
    if (mboxWrite(&message) != 0)
    {
        printf ("Error: Failed to send DSS status information to MSS.\n");
    }
}


int main (void)
    {
    SOC_Cfg        socCfg;
    int32_t        errCode;

    // Initialize and populate the demo MCB
    memset ((void*)&gMmwDssMCB, 0, sizeof(MmwDemo_DSS_MCB));

    // Initialize the SOC confiugration:
    memset ((void *)&socCfg, 0, sizeof(SOC_Cfg));

    // Populate the SOC configuration:
    socCfg.clockCfg = SOC_SysClock_BYPASS_INIT;

    // Initialize the SOC Module: This is done as soon as the application is started
    // to ensure that the MPU is correctly configured.
    gMmwDssMCB.socHandle = SOC_init (&socCfg, &errCode);
    if (gMmwDssMCB.socHandle == NULL)
    {
        printf ("Error: SOC Module Initialization failed [Error code %d]\n", errCode);
        return -1;
    }

    // Initialize the DEMO configuration:
    gMmwDssMCB.cfg.sysClockFrequency = DSS_SYS_VCLK;

    Cycleprofiler_init();

    dssInitTask();


    return 0;
}

