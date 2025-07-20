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

/* mmWave SDK Include Files: */
#include <ti/common/sys_common.h>
#include <ti/drivers/soc/soc.h>
#include <ti/drivers/esm/esm.h>
#include <ti/drivers/crc/crc.h>
#include <ti/drivers/uart/UART.h>
#include <ti/drivers/uart/include/reg_sci.h> // Sara
#include <ti/drivers/uart/include/uartsci.h> // Sara
#include <ti/drivers/gpio/gpio.h>
#include <ti/drivers/mailbox/mailbox.h>
#include <ti/control/mmwave/mmwave.h>
#include <ti/drivers/osal/DebugP.h>
#include <ti/drivers/osal/HwiP.h>

/* Demo Include Files */
#include "mss_mmw.h"
#include "mmw_messages.h"
#include "osal_nonos/Event_nonos.h"
#include "communication1843.h"

#include "mss_functions.h"
#include "sleep.h"

/**************************************************************************
 *************************** Global Definitions ***************************
 **************************************************************************/

/**
 * @brief
 *  Global Variable for tracking information required by the mmw Demo
 */
MmwDemo_MCB    gMmwMssMCB;
uint8_t flag_primo_giro=1;

rlVersion_t gDevVersion;
MmwDemo_message      message;


/**************************************************************************
 *************************** Extern Definitions *******************************
 **************************************************************************/
/* CLI Init function */
extern void CLI_getMMWaveExtensionOpenConfig(MMWave_OpenCfg* ptrOpenCfg);
extern void CLI_getMMWaveExtensionConfig(MMWave_CtrlCfg* ptrCtrlCfg);

extern void MmwDemo_CLIInit (void);
/**************************************************************************
 ************************* Millimeter Wave Demo Functions Prototype**************
 **************************************************************************/

extern void CLI_write (const char* format, ...);

/* Data path functions */
int32_t MmwDemo_mssDataPathConfig(void);
int32_t MmwDemo_mssDataPathStop(void);


/* external sleep function when in idle (used in .cfg file) */
void MmwDemo_sleep(void);

/* DSS to MSS exception signalling ISR */
static void MmwDemo_installDss2MssExceptionSignallingISR(void);

void sleep_ms(uint32_t n);
void sleep_s(uint32_t n);

/* MMW demo Task */
void mssInitTask(unsigned int arg0, unsigned int arg1);
void mssCtrlPathTask(unsigned int event);
void nonOsLoop(uint8_t count);
int32_t CLI_task_mod(void);


/**************************************************************************
 ************************* Millimeter Wave Demo Functions **********************
 **************************************************************************/
UART_Params     uartParams;
int32_t BytesRcvd=0;
unsigned int matchingEvents;

/**
 *  @b Description
 *  @n
 *      Function to send a message to peer through Mailbox virtural channel
 *
 *  @param[in]  message
 *      Pointer to the MMW demo message.
 *
 *  @retval
 *      Success    - 0
 *      Fail       < -1
 */

int32_t MmwDemo_mboxWrite(MmwDemo_message     * message)
{
    int32_t                  retVal = -1;

    retVal = Mailbox_write (gMmwMssMCB.peerMailbox, (uint8_t*)message, sizeof(MmwDemo_message));
    if (retVal == sizeof(MmwDemo_message))
    {
        retVal = 0;
    }
    return retVal;
}

/**
 *  @b Description
 *  @n
 *      The Task is used to handle  the mmw demo messages received from 
 *      Mailbox virtual channel.  
 *
 *  @param[in]  arg0
 *      arg0 of the Task. Not used
 *  @param[in]  arg1
 *      arg1 of the Task. Not used
 *
 *  @retval
 *      Not Applicable.
 */
uint32_t ledOn = 0;

static void MmwDemo_mboxReadTask(unsigned int arg0, unsigned int arg1)
{

    MmwDemo_message      message;
    int32_t              retVal = 0;
    uint32_t totalPacketLen;
    uint32_t numPaddingBytes;
    uint32_t itemIdx;
    int32_t i;
    volatile int32_t startTime,waitingTime;


    //                  p_message[1] = 11; //(gMmwMssMCB.gpio_cnt & 0xFF000000)>>24;
    //                   *(p_message+1)=12;
    //                   *(((int *)&message) +1)=13;

    /* Read the message from the peer mailbox: We are not trying to protect the read
     * from the peer mailbox because this is only being invoked from a single thread */
    // Clear message to MSS

    retVal = Mailbox_read(gMmwMssMCB.peerMailbox, (uint8_t*)&message, sizeof(MmwDemo_message));
    if (retVal < 0)
    {
        /* Error: Unable to read the message. Setup the error code and return values */
        printf ("Error: mss Mailbox read failed [Error code %d]\n", retVal);
    }
    else if (retVal == 0)
    {
        /* We are done: There are no messages available from the peer execution domain. */
        return;
    }
    else
    {
        /* Flush out the contents of the mailbox to indicate that we are done with the message. This will
         * allow us to receive another message in the mailbox while we process the received message. */
        Mailbox_readFlush (gMmwMssMCB.peerMailbox);

        /* Process the received message: */
        switch (message.type)
        {
            case MMWDEMO_DSS2MSS_VERSION_INFO:
                memcpy(&gDevVersion, &message.body.devVerArgs, sizeof(rlVersion_t));
                break;

            case MMWDEMO_DSS2MSS_DATA_READY:

                // If a stop request has been received, notify it to the DSP (here the mailbox is free)
                if ( gMmwMssMCB.stopRequired == true){
                    stopProcedure();
                }else{

                if (gMmwMssMCB.realTimeError == false)
                {

                    totalPacketLen = sizeof(MmwDemo_output_message_header);
                    UART_writePolling(gMmwMssMCB.loggingUartHandle,
                                       (uint8_t*)&message.body.detObj.header,
                                       sizeof(MmwDemo_output_message_header));

                     /* Send TLVs */
                     for (itemIdx = 0;  itemIdx < message.body.detObj.header.numTLVs; itemIdx++)
                     {

                         UART_writePolling(gMmwMssMCB.loggingUartHandle,
                                            (uint8_t*)&message.body.detObj.tlv[itemIdx],
                                            sizeof(MmwDemo_output_message_tl));
                         UART_writePolling(gMmwMssMCB.loggingUartHandle,
                                            (uint8_t*)SOC_translateAddress(message.body.detObj.tlv[itemIdx].address,
                                                                           SOC_TranslateAddr_Dir_FROM_OTHER_CPU,NULL),
                                            message.body.detObj.tlv[itemIdx].length);
                         totalPacketLen += sizeof(MmwDemo_output_message_tl) + message.body.detObj.tlv[itemIdx].length;
                     }

                     /* Send padding to make total packet length multiple of MMWDEMO_OUTPUT_MSG_SEGMENT_LEN */
                     numPaddingBytes = MMWDEMO_OUTPUT_MSG_SEGMENT_LEN - (totalPacketLen & (MMWDEMO_OUTPUT_MSG_SEGMENT_LEN-1));
                     if (numPaddingBytes<MMWDEMO_OUTPUT_MSG_SEGMENT_LEN)
                     {
                         uint8_t padding[MMWDEMO_OUTPUT_MSG_SEGMENT_LEN];
                         // MATTEO CHANGE: FROM 0xf to 0xff
                         /*DEBUG:*/ memset(&padding, 0xff, MMWDEMO_OUTPUT_MSG_SEGMENT_LEN);
                         UART_writePolling (gMmwMssMCB.loggingUartHandle,
                                             padding,
                                             numPaddingBytes);
                     }

                }
                }

                break;

            case DSS2MSS_REALTIME_ERROR:

                gMmwMssMCB.realTimeError = true;
                printf("MSS has received DSS2MSS_REALTIME_ERROR \n");

                break;

            case MMWDEMO_DSS2MSS_STOPDONE:

                /* The system is now ready to receive a new command from cli */
                gMmwMssMCB.isReadyForCli = true;

                /* Post event that stop is done */
                Event_post(gMmwMssMCB.eventHandleNotify, MMWDEMO_DSS_STOP_COMPLETED_EVT);

                break;

            case MMWDEMO_DSS2MSS_ASSERT_INFO:
                /* Send the received DSS assert info through CLI */
                CLI_write ("DSS Exception: %s, line %d.\n", message.body.assertInfo.file, \
                    message.body.assertInfo.line);
            break;

            case MMWDEMO_DSS2MSS_STATUS_INFO:
                 if(message.body.dssStatusInfo.eventType == MMWDEMO_BSS_CALIBRATION_REP_EVT)
                 {
                     CLI_write("Rf-init status [0x%x]\n", message.body.dssStatusInfo.errVal);
                 }
                 else if(message.body.dssStatusInfo.eventType == MMWDEMO_DSS_MMWAVELINK_FAILURE_EVT)
                 {
                     CLI_write("Link API failed [%d]\n", message.body.dssStatusInfo.errVal);
                 }
                 else if(message.body.dssStatusInfo.eventType == MMWDEMO_DSS_INIT_CONFIG_DONE_EVT)
                 {
                     gMmwMssMCB.isReadyForCli = true;
                 }
                 else if(message.body.dssStatusInfo.eventType == MMWDEMO_DSS_START_COMPLETED_EVT)
                 {
                    /* Post event that Sensor Start successfully done by DSS */
                    Event_post(gMmwMssMCB.eventHandleNotify, MMWDEMO_DSS_START_COMPLETED_EVT);
                 }
                 else if(message.body.dssStatusInfo.eventType == MMWDEMO_DSS_START_FAILED_EVT)
                 {
                    /* Post event that Sensor Start failed by DSS */
                    Event_post(gMmwMssMCB.eventHandleNotify, MMWDEMO_DSS_START_FAILED_EVT);
                 }
                 else
                 {
                    /* do nothing */
                 }


                break;
            case MMWDEMO_DSS2MSS_ISR_INFO_ADDRESS:
                gMmwMssMCB.dss2mssIsrInfoAddress = 
                    SOC_translateAddress(message.body.dss2mssISRinfoAddress,
                                         SOC_TranslateAddr_Dir_FROM_OTHER_CPU, NULL);
                MmwDemo_installDss2MssExceptionSignallingISR();
            break;
            case MMWDEMO_DSS2MSS_MEASUREMENT_INFO:
                /* Send the received DSS calibration info through CLI */
                CLI_write ("compRangeBiasAndRxChanPhase");
                CLI_write (" %.7f", message.body.compRxChanCfg.rangeBias);
                for (i = 0; i < SYS_COMMON_NUM_TX_ANTENNAS*SYS_COMMON_NUM_RX_CHANNEL; i++)
                {
                    CLI_write (" %.5f", (float) message.body.compRxChanCfg.rxChPhaseComp[i].real/32768.);
                    CLI_write (" %.5f", (float) message.body.compRxChanCfg.rxChPhaseComp[i].imag/32768.);
                }
                CLI_write ("\n");
            break;
            default:
            {
                /* Message not support */
                printf ("Error: unsupport Mailbox message id=%d\n", message.type);
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
void MmwDemo_mboxCallback
(
    Mbox_Handle  handle,
    Mailbox_Type    peer
)
{
    /* Message has been received from the peer endpoint. Wakeup the mmWave thread to process
     * the received message. */
    SemaphoreP_post (gMmwMssMCB.mboxSemHandle);
}



/* This functions notify to DSS a "stop sensor" command via mailbox */
int32_t notifySensorStop(void)
{
    memset((void *)&message, 0, sizeof(MmwDemo_message));

    message.type = MMWDEMO_MSS2DSS_SENSOR_STOP_CMD;
    message.subFrameNum = -1;

    if (MmwDemo_mboxWrite(&message) == 0)
        return 0;
    else
        return -1;
}

            

/**
 *  @b Description
 *  @n
 *      This is a utility function which can be used by the CLI to
 *      pend for stop complete (after MmwDemo_notifySensorStart is called)
 *
 *  @retval
 *      0: sensor Stop done
 *    0xF: no event
 */
int32_t MmwDemo_waitSensorStopComplete(void)
{
    uint32_t      event;
    int32_t       retVal;
    /* Pend on the START NOTIFY event */
    event = Event_pend(gMmwMssMCB.eventHandleNotify, 
                          Event_Id_NONE, 
                          MMWDEMO_DSS_STOP_COMPLETED_EVT,
                          EventP_NO_WAIT);

    /************************************************************************
     * DSS event:: STOP notification
     ************************************************************************/
    if(event & MMWDEMO_DSS_STOP_COMPLETED_EVT)
    {
        /* Sensor has been stopped successfully */
        gMmwMssMCB.isSensorStarted = false;
              
        /* print for user */
        printf("Sensor has been stopped\n");
        retVal = 0;
    }
    else
    {
        /* If above event didn't occur then return with non-zero +ve value */
        retVal = 0xF;
    }
    return retVal;
}


/**
 *  @b Description
 *  @n
 *      The task is used to process data path events
 *      control task
 *
 *  @retval
 *      Not Applicable.
 */
void mssCtrlPathTask(unsigned int event)
{

    /************************************************************************
     * BSS event:: CPU fault
     ************************************************************************/
    if(event & MMWDEMO_BSS_CPUFAULT_EVT)
    {
        CLI_write ("BSS is in CPU fault.\n");
        MmwDemo_mssAssert(0);
    }

    /************************************************************************
     * BSS event:: ESM fault
     ************************************************************************/
    if(event & MMWDEMO_BSS_ESMFAULT_EVT)
    {
        CLI_write ("BSS is in ESM fault.\n");
        MmwDemo_mssAssert(0);
    }

    if(event & MMWDEMO_DSS_CHIRP_PROC_DEADLINE_MISS_EVT)
    {
        CLI_write ("DSS Chirp Processing Deadline Miss Exception.\n");
        DebugP_assert(0);
    }

    if(event & MMWDEMO_DSS_FRAME_PROC_DEADLINE_MISS_EVT)
    {
        CLI_write ("DSS Frame Processing Deadline Miss Exception.\n");
        DebugP_assert(0);
    }

    printf("Debug: MMWDemoDSS Data path exit\n");
}

int32_t sensorStart()
{

    int32_t retVal = 0x0;

    //printf("Sensor start cmd\n");
    /* Post sensorSTart event to notify configuration is done */
    notifySensorStart();


    gMmwMssMCB.isInStop = false;
    GPIO_write(SOC_XWR18XX_GPIO_2, 0);

    //sleep_ms(500);

    return retVal;
}


/* This function notifies "sensor stop" to dss and waits for its response */
int32_t sensorStop()
{
    int32_t retVal = 0xF;

    /* we need to wait for Sensor-stop response from DSS, so disable to read next CLI  */
    gMmwMssMCB.isReadyForCli = false;

    /* Post sensorSTOP event to notify sensor stop command */
    notifySensorStop();

    while(retVal > 0)
    {
        /* check for sensor-stop completion */
        retVal = MmwDemo_waitSensorStopComplete();
        if(retVal != 0)
        {
            /* call Non OS loop once to check DSS mailbox for sensor-start status event */
            nonOsLoop(1);
        }
    }
    gMmwMssMCB.isInStop = true;
    gMmwMssMCB.stopRequired = false;
    GPIO_write(SOC_XWR18XX_GPIO_2, 1);

    return retVal;
}


/**
 *  @b Description
 *  @n
 *      This is a utility function which can be invoked from the CLI or
 *      the Switch press to start the sensor. This sends an event to the
 *      sensor management task where the actual *start* procedure is
 *      implemented.
 *
 *  @retval
 *      Not applicable
 */
int32_t notifySensorStart()
{

    /* we need to wait for Sensor-start response from DSS, so disable to read next CLI  */
    gMmwMssMCB.isReadyForCli = false;

    /* Get the open configuration from the CLI mmWave Extension */
    CLI_getMMWaveExtensionOpenConfig (&gMmwMssMCB.cfg.openCfg);

    gMmwMssMCB.cfg.openCfg.freqLimitLow  = 760U;
    gMmwMssMCB.cfg.openCfg.freqLimitHigh = 810U;
    gMmwMssMCB.cfg.openCfg.disableFrameStartAsyncEvent = true;
    gMmwMssMCB.cfg.openCfg.disableFrameStopAsyncEvent  = false;


    /* Send configuration to DSS over Mailbox */
    memset((void *)&message, 0, sizeof(MmwDemo_message));
    message.type = MMWDEMO_MSS2DSS_OPEN_CFG;
    message.subFrameNum = -1;
    memcpy((void *)&message.body.openCfg, (void *)&gMmwMssMCB.cfg.openCfg, sizeof(MMWave_OpenCfg));
    if (MmwDemo_mboxWrite(&message) != 0)
        return -1;


    CLI_getMMWaveExtensionConfig (&gMmwMssMCB.cfg.ctrlCfg);


    /* Send configuration to DSS */
    memset((void *)&message, 0, sizeof(MmwDemo_message));
    message.type = MMWDEMO_MSS2DSS_CONTROL_CFG;
    message.subFrameNum = -1;
    memcpy((void *)&message.body.ctrlCfg, (void *)&gMmwMssMCB.cfg.ctrlCfg, sizeof(MMWave_CtrlCfg));
    if (MmwDemo_mboxWrite(&message) != 0)
        return -1;


    /* Send configuration to DSS */
    memset((void *)&message, 0, sizeof(MmwDemo_message));
    message.type = MMWDEMO_MSS2DSS_SENSOR_START_CMD;
    message.subFrameNum = -1;
    if (MmwDemo_mboxWrite(&message) == 0)
        return 0;
    else
        return -1;

}

int32_t stopProcedure(){

        sensorStop();
        sleep_ms(1500);

        printf("MSS has stopped DSS\n");

        UART_close(gMmwMssMCB.commandUartHandle);
//        cmdUartSetup(false);


        UART_Params_init(&uartParams);
        uartParams.clockFrequency  = gMmwMssMCB.cfg.sysClockFrequency;
        uartParams.baudRate        = gMmwMssMCB.cfg.commandBaudRate;
        uartParams.isPinMuxDone    = 1U;

        uartParams.readDataMode  = UART_DATA_BINARY;
        uartParams.writeDataMode  = UART_DATA_BINARY;
        uartParams.readReturnMode = UART_RETURN_FULL;
        uartParams.readEcho = UART_ECHO_OFF;

        //uartParams.readEcho = UART_ECHO_ON;// SaraDbg
		uartParams.readReturnMode = UART_RETURN_NEWLINE;

		gMmwMssMCB.commandUartHandle = UART_open(0, &uartParams);


        return(0);

}

int32_t startProcedure(){

    sleep_ms(1500);

    //cmdUartSetup(true);

    UART_close(gMmwMssMCB.commandUartHandle);

    UART_Params_init(&uartParams);
    uartParams.clockFrequency  = gMmwMssMCB.cfg.sysClockFrequency;
    uartParams.baudRate        = gMmwMssMCB.cfg.commandBaudRate;
    uartParams.isPinMuxDone    = 1U;

    uartParams.readDataMode  = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.readTimeout = 1000;

    gMmwMssMCB.commandUartHandle = UART_open(0, &uartParams);

    configureDSS();
    sensorStart();
    sleep_ms(1500);

    return(0);
}


void cmdUartSetup(bool timeoutFlag){

    UART_Params     uartParams;

    UART_Params_init(&uartParams);
    uartParams.clockFrequency  = gMmwMssMCB.cfg.sysClockFrequency;
    uartParams.baudRate        = gMmwMssMCB.cfg.commandBaudRate;
    uartParams.isPinMuxDone    = 1U;
    if (timeoutFlag == true)
        uartParams.readTimeout      = 1000;

    uartParams.readDataMode  = UART_DATA_BINARY;
    uartParams.writeDataMode  = UART_DATA_BINARY;

    //uartParams.readEcho = UART_ECHO_ON;
    uartParams.readReturnMode = UART_RETURN_NEWLINE;


    sleep_ms(1500);

    gMmwMssMCB.commandUartHandle = UART_open(0, &uartParams);
    if (gMmwMssMCB.commandUartHandle == NULL)
    {
        printf("Error: MSS Unable to open the Command UART Instance\n");
        return;
    }


}

void MmwDemo_Dss2MssISR(uintptr_t arg)
{  
    switch(*(uint8_t*)gMmwMssMCB.dss2mssIsrInfoAddress)
    {
        case MMWDEMO_DSS2MSS_CHIRP_PROC_DEADLINE_MISS_EXCEPTION:
            //Event_post(gMmwMssMCB.eventHandle, MMWDEMO_DSS_CHIRP_PROC_DEADLINE_MISS_EVT);
            stopProcedure();
        break;
        
        case MMWDEMO_DSS2MSS_FRAME_PROC_DEADLINE_MISS_EXCEPTION:
            //Event_post(gMmwMssMCB.eventHandle, MMWDEMO_DSS_FRAME_PROC_DEADLINE_MISS_EVT);
            stopProcedure();
        break;

        case MMWDEMO_DSS2MSS_FRAME_TRIGGERED_EVENT:
            Event_post(gMmwMssMCB.eventHandleNotify, MMWDEMO_DSS_START_COMPLETED_EVT);
        break;
        default:
            MmwDemo_mssAssert(0);
        break;


      }
}

/**
 *  @b Description
 *  @n
 *      Installs DSS to MSS Software Interrupt ISR for exception signalling.
 *
 *  @retval
 *      Not Applicable.
 */
static void MmwDemo_installDss2MssExceptionSignallingISR(void)
{
    HwiP_Params  hwiParams;
    volatile HwiP_Handle  hwiHandle;

    HwiP_Params_init(&hwiParams);
    hwiParams.name = "Dss2MssSwISR";
    hwiHandle = HwiP_create(MMWDEMO_DSS2MSS_EXCEPTION_SIGNALLING_SW_INT_MSS, 
                            MmwDemo_Dss2MssISR, &hwiParams);
}

/**
 *  @b Description
 *  @n
 *      Non OS loop function which checks for DSS Mailbox message, CLI command and any event
 *      from DSS for number of loop count provided in function argument.
 *
 *  @param[in]  number of loop count, 0: infinite, 1: single count
  *
 *  @retval NONE
 */





uint32_t cntComando = 0;
uint32_t cntBytes[100];

volatile int i,k;

void nonOsLoop(uint8_t count)
{

    do
    {

       int32_t cliRequest = CLI_task_mod();
       if (cliRequest == STOP_CMD){
           stopProcedure();
           //stopUart();
       }else if  (cliRequest == START_CMD){
          startProcedure();

       }

        if(gMmwMssMCB.isReadyForCli == true && flag_primo_giro == 1){

            sensorStop(); // Stop the sensor
            sleep_ms(500);
            send_configuration();
            configureDSS();// Send configuration to the DSS
            sleep_ms(1500);
            sensorStart();

            flag_primo_giro = 0;
        }

        if(SemaphoreP_pend(gMmwMssMCB.mboxSemHandle, SemaphoreP_NO_WAIT) == SemaphoreP_OK)
        {
            /* Create task to handle mailbox messges */
            MmwDemo_mboxReadTask(0,0);
        }

        /* Check if DSS/BSS is in fault or exception */
        matchingEvents = Event_pend(gMmwMssMCB.eventHandle, Event_Id_NONE, MMWDEMO_BSS_FAULT_EVENTS | MMWDEMO_DSS_EXCEPTION_EVENTS, EventP_NO_WAIT);

        if(matchingEvents != 0)
        {
            /*****************************************************************************
             * Create a data path management task to handle data Path events
             *****************************************************************************/
            mssCtrlPathTask(matchingEvents);
        }
    } while(!count);
}

void mssInitTask(unsigned int arg0, unsigned int arg1)
{
    int32_t             errCode;
    SemaphoreP_Params   semParams;
    Mailbox_Config      mboxCfg;

    /* Debug Message: */
    printf("Debug: VitalSignsDemoMSS Launched the Initialization Task\n");


    /*****************************************************************************
     * Initialize the mmWave SDK components:
     *****************************************************************************/
    /* Setup the PINMUX to bring out the UART-1 */
    Pinmux_Set_OverrideCtrl(SOC_XWR18XX_PINN5_PADBE, PINMUX_OUTEN_RETAIN_HW_CTRL, PINMUX_INPEN_RETAIN_HW_CTRL);
    Pinmux_Set_FuncSel(SOC_XWR18XX_PINN5_PADBE, SOC_XWR18XX_PINN5_PADBE_MSS_UARTA_TX);
    Pinmux_Set_OverrideCtrl(SOC_XWR18XX_PINN4_PADBD, PINMUX_OUTEN_RETAIN_HW_CTRL, PINMUX_INPEN_RETAIN_HW_CTRL);
    Pinmux_Set_FuncSel(SOC_XWR18XX_PINN4_PADBD, SOC_XWR18XX_PINN4_PADBD_MSS_UARTA_RX);

    /* Setup the PINMUX to bring out the UART-3 */
    Pinmux_Set_OverrideCtrl(SOC_XWR18XX_PINF14_PADAJ, PINMUX_OUTEN_RETAIN_HW_CTRL, PINMUX_INPEN_RETAIN_HW_CTRL);
    Pinmux_Set_FuncSel(SOC_XWR18XX_PINF14_PADAJ, SOC_XWR18XX_PINF14_PADAJ_MSS_UARTB_TX);


    /**********************************************************************
      * Setup the PINMUX:
      * - GPIO Output: Configure pin K13 as GPIO_2 output
      **********************************************************************/
    Pinmux_Set_OverrideCtrl(SOC_XWR18XX_PINK13_PADAZ, PINMUX_OUTEN_RETAIN_HW_CTRL, PINMUX_INPEN_RETAIN_HW_CTRL);
    Pinmux_Set_FuncSel(SOC_XWR18XX_PINK13_PADAZ, SOC_XWR18XX_PINK13_PADAZ_GPIO_2);


    /* Initialize the GPIO */
    GPIO_init();

    /**********************************************************************
       * Setup the DS3 LED on the EVM connected to GPIO_2
    **********************************************************************/
    GPIO_setConfig (SOC_XWR18XX_GPIO_2, GPIO_CFG_OUTPUT);
    
    /* Initialize the Mailbox */
    Mailbox_init(MAILBOX_TYPE_MSS);


    /*****************************************************************************
     * Initialize the UART *
     *****************************************************************************/
    UART_init();

    /*  UART for commands */


    // init to default
    UART_Params_init(&uartParams);
    // setup parameters
    uartParams.clockFrequency  = gMmwMssMCB.cfg.sysClockFrequency;
    uartParams.baudRate        = gMmwMssMCB.cfg.commandBaudRate;
    uartParams.isPinMuxDone    = 1U;
    uartParams.readTimeout     = 1000;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_NEWLINE;
    //uartParams.readEcho = UART_ECHO_ON;

    // open the Command UART instance
    gMmwMssMCB.commandUartHandle = UART_open(0, &uartParams);
    if (gMmwMssMCB.commandUartHandle == NULL)
    {

        printf("Error: MSS Unable to open the Command UART Instance\n");
        return;
    }

    /* UART for data logger */
    // init to default
    UART_Params_init(&uartParams);
    // setup parameters
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.clockFrequency = gMmwMssMCB.cfg.sysClockFrequency;
    uartParams.baudRate       = gMmwMssMCB.cfg.loggingBaudRate;
    uartParams.isPinMuxDone   = 1U;



    // open the Logging UART instance
    gMmwMssMCB.loggingUartHandle = UART_open(1, &uartParams);
    gMmwMssMCB.loggingUartDataSize = 50000;
    if (gMmwMssMCB.loggingUartHandle == NULL)
    {
        printf("MSS is unable to open the datalogger UART instance\n");

        return;
    }

    MmwDemo_CLIInit();

    /*****************************************************************************
     * Creating communication channel between MSS & DSS
     *****************************************************************************/
    /* Create a binary semaphore which is used to handle mailbox interrupt. */
    SemaphoreP_Params_init(&semParams);
    semParams.mode             = SemaphoreP_Mode_BINARY;
    gMmwMssMCB.mboxSemHandle = SemaphoreP_create(0, &semParams);

    /* Setup the default mailbox configuration */
    Mailbox_Config_init(&mboxCfg);

    /* Setup the configuration: */
    mboxCfg.chType       = MAILBOX_CHTYPE_MULTI;
    mboxCfg.chId         = MAILBOX_CH_ID_0;
    mboxCfg.writeMode    = MAILBOX_MODE_BLOCKING;
    mboxCfg.readMode     = MAILBOX_MODE_CALLBACK;
    mboxCfg.readCallback = &MmwDemo_mboxCallback;

    /* Initialization of Mailbox Virtual Channel  */
    gMmwMssMCB.peerMailbox = Mailbox_open(MAILBOX_TYPE_DSS, &mboxCfg, &errCode);
    if (gMmwMssMCB.peerMailbox == NULL)
    {
        /* Error: Unable to open the mailbox */
        printf("Error: Unable to open the Mailbox to the DSS [Error code %d]\n", errCode);
        return;
    }

    /* Remote Mailbox is operational: Set the DSS Link State */
    SOC_setMMWaveMSSLinkState (gMmwMssMCB.socHandle, 1U, &errCode);


    /*****************************************************************************
     * Create Event to handle mmwave callback and system datapath events 
     *****************************************************************************/
    /* Default instance configuration params */
    gMmwMssMCB.eventHandle = Event_create(NULL, 0);
    if (gMmwMssMCB.eventHandle == NULL) 
    {
        MmwDemo_mssAssert(0);
        return ;
    }
    
    gMmwMssMCB.eventHandleNotify = Event_create(NULL, 0);
    if (gMmwMssMCB.eventHandleNotify == NULL) 
    {
        MmwDemo_mssAssert(0);
        return ;
    }


    /*****************************************************************************
     * mmWave: Initialization of the high level module
     *****************************************************************************/

    /* Synchronization: This will synchronize the execution of the control module
     * between the MSS and DSS cores. This is a prerequisite and always needs to be invoked. */
    while (1)
    {

        volatile int32_t syncStatus;

        /* Get the synchronization status: */
        syncStatus = SOC_isMMWaveDSSOperational (gMmwMssMCB.socHandle, &errCode);
        if (syncStatus < 0)
        {
            /* Error: Unable to synchronize the mmWave control module */
            printf ("Error: MMWDemoMSS mmWave Control Synchronization failed [Error code %d]\n", errCode);
            return;
        }
        if (syncStatus == 1)
        {
            /* Synchronization achieved: */
            break;
        }
        /* Sleep and poll again: */
        MmwDemo_sleep();
    }  


    sleep_ms(500);

    /* Non OS loop for infinite count where it will check for DSS mailbox event, CLI commands  */
    nonOsLoop(0);
   
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
 *      Send MSS assert information through CLI.
 */
void _MmwDemo_mssAssert(int32_t expression, const char *file, int32_t line)
{
    if (!expression) {
        CLI_write ("MSS Exception: %s, line %d.\n",file,line);
    }
}

/**
 *  @b Description
 *  @n
 *      Entry point into the Millimeter Wave Demo
 *
 *  @retvala
 *      Not Applicable.
 */
int main (void)
{
    int32_t         errCode;
    SOC_Cfg         socCfg;

    // Initialize and populate the demo MCB
    memset ((void*)&gMmwMssMCB, 0, sizeof(MmwDemo_MCB));

    // Initialize the SOC configuration:
    memset ((void *)&socCfg, 0, sizeof(SOC_Cfg));

    // Initialize the ESM:
    gMmwMssMCB.esmHandle = ESM_init(0U); //dont clear errors as TI RTOS does it

    // Populate the SOC configuration:
    socCfg.clockCfg = SOC_SysClock_INIT;

    // Initialize the SOC Module: This is done as soon as the application is started
    // to ensure that the MPU is correctly configured.
    gMmwMssMCB.socHandle   = SOC_init (&socCfg, &errCode);
    if (gMmwMssMCB.socHandle  == NULL)
    {
        printf ("Error: SOC Module Initialization failed [Error code %d]\n", errCode);
        return -1;
    }

    // Initialize the DEMO configuration:
    gMmwMssMCB.cfg.sysClockFrequency = MSS_SYS_VCLK;
    gMmwMssMCB.cfg.loggingBaudRate   = 921600;
    gMmwMssMCB.cfg.commandBaudRate   = 115200;

    // Debug Message:
    printf ("**********************************************\n");
    printf ("Debug: Launching the Vital-Signs Monitoring Demo\n");
    printf ("**********************************************\n");


    mssInitTask(0,0);

    return 0;
}
