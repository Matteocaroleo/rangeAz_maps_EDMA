/**
 *   @file  dss_data_path.h
 *
 *   @brief
 *      This is the data path processing header.
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2016 Texas Instruments, Inc.
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
#ifndef DSS_DATA_PATH_H
#define DSS_DATA_PATH_H

#include <ti/common/sys_common.h>
#include <ti/common/mmwave_error.h>

#include <ti/drivers/adcbuf/ADCBuf.h>
#include <ti/drivers/edma/edma.h>

#include "mmw_config.h"
#include "mmw_output.h"
#include "user_parameters.h"

#if BASIC_PROCESSING_ENA == 1
    #include "dss_vitalSignsDemo_utilsFunc.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


#define BYTES_PER_SAMP_1D (2*sizeof(int16_t))  /*16 bit real, 16 bit imaginary => 4 bytes */
#define BYTES_PER_SAMP_2D (2*sizeof(int32_t))  /*32 bit real, 32 bit imaginary => 8 bytes */

/*! Finds the max Range-bin every RESET_LOCAL_COUNT_VAL frames */
#define RANGE_BIN_TRACKING      1
#define RESET_LOCAL_COUNT_VAL   128

/* Compute the phase differences between the successive phase values */
#define FLAG_COMPUTE_PHASE_DIFFERENCE          1

/* Impulse-like Noise Removal */
#define FLAG_REMOVE_IMPULSE_NOISE              1

#define PI_ 3.1415926535897
#define ONE_Q15 (1 << 15)
#define ONE_Q19 (1 << 19)
#define ONE_Q8 (1 << 8)


/*! @brief DSP cycle profiling structure to accumulate different
    processing times in chirp and frame processing periods */
typedef struct cycleLog_t_
{
    uint32_t interChirpProcessingTime; /*!< @brief total processing time during all chirps in a frame excluding EDMA waiting time*/
    uint32_t interChirpWaitTime; /*!< @brief total wait time for EDMA data transfer during all chirps in a frame*/
    uint32_t interFrameProcessingTime; /*!< @brief total processing time for 2D and 3D excluding EDMA waiting time*/
    uint32_t interFrameWaitTime; /*!< @brief total wait time for 2D and 3D EDMA data transfer */
    uint32_t dataTransferTime; /*!< @brief total time to transfer data to MSS */

} cycleLog_t;


/*!
 *  @brief Timing information
 */
typedef struct MmwDemo_timingInfo
{
    /*! @brief number of processor cycles between frames excluding
           processing time to transmit output on UART and excluding
           sub-frame switching time in case of advanced frame */
    uint32_t interFrameProcCycles;

    /*! @brief sub-frame switching cycles in case of advanced frame */
    uint32_t subFrameSwitchingCycles;

    /*! @brief time to transmit out detection information (in DSP cycles)
           */
    uint32_t transmitOutputCycles;

    /*! @brief Chirp processing end time */
    uint32_t chirpProcessingEndTime;

    /*! @brief Chirp processing end margin in number of cycles before due time
     * to start processing next chirp, minimum value*/
    uint32_t chirpProcessingEndMarginMin;

    /*! @brief Chirp processing end margin in number of cycles before due time
     * to start processing next chirp, maximum value*/
    uint32_t chirpProcessingEndMarginMax;

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
 *  Millimeter Wave Demo Data Path Context.
 *
 * @details
 *  The structure is used to hold common context among data path objects.
 */
typedef struct MmwDemo_DSS_dataPathContext_t_
{
    /*! @brief   ADCBUF handle. */
    ADCBuf_Handle adcbufHandle;

    /*! @brief   Handle of the EDMA driver. */
    EDMA_Handle edmaHandle[EDMA_NUM_CC];

    /*! @brief  Used for checking that chirp processing finshed on time */
    int8_t chirpProcToken;

    /*! @brief  Used for checking that inter frame processing finshed on time */
    int8_t interFrameProcToken;

    bool realTimeError;
    bool realTimeErrorNotify;

} MmwDemo_DSS_dataPathContext_t;


typedef struct MmwDemo_DSS_DataPathObj_t
{
    /*! @brief Pointer to common context across data path objects */
    MmwDemo_DSS_dataPathContext_t *context;

    /*! @brief Pointer to cli config */
    MmwDemo_CliCfg_t *cliCfg;

    /*! @brief Pointer to cli config common to all subframes*/
    MmwDemo_CliCommonCfg_t *cliCommonCfg;

    /*! @brief pointer to ADC buffer */
    cmplx16ReIm_t *ADCdataBuf;

    /*! @brief ADCBUF input samples */
    cmplx16ReIm_t *adcDataIn;

    ////////////////////// AGGIUNTO ///////////////////

    /* AZIMUTH FFT */
    /*! @brief twiddle table for 2D FFT */
    cmplx16ReIm_t *twiddle16x16_2D;
    /*! @brief input for azimuth calculation ---> L1*/
    cmplx16ReIm_t *dataAzIn;

    /*! @brief array of azimuth window--> L2*/
    cmplx16ReIm_t *window2D;

    /*! @brief output of one azimuth fft --> L2*/
    cmplx16ReIm_t *fftOut2D;

    /*! @brief output Range Azimuth map (all fft az) --> L3*/
    cmplx16ReIm_t *rangeAzMap;
    ////////////////////////////////////////////////

    /*! @brief Pointer to Radar Cube memory */
    cmplx16ReIm_t *radarCube;

    /* Range FFT */
    /*! @brief twiddle table for 1D FFT */
    cmplx16ReIm_t *twiddle16x16_1D;
    /*! @brief window coefficients for 1D FFT */
    int16_t *window1D;
    /*! @brief output of range fft  */
    cmplx16ReIm_t   *fftOut1D;          // fft output
    // Range-FFT params
    cmplx16ReIm_t *pRangeProfileCplx; // The Complex Range Profile extracted from the Radar Cube


    /*! @brief   Number of rx channels */
    uint8_t numRxAntennas;
    /*! @brief   Number of tx channels */
    uint8_t numTxAntennas;
    /*! @brief   Number of virtual channels */
    uint8_t numVirtualAntennas;
    /*! @brief number of ADC samples */
    uint16_t numAdcSamples;
    /*! @brief number of chirps per frame */
    uint16_t numChirpsPerFrame;
    /*! @brief number of chirps per frame for single tx */
    uint16_t numChirpsPerFramePerTx;

    uint8_t numChirpsProc;

    uint8_t rxAntennaProcess;

    /*! @brief number of calculated range bins */
    uint16_t numRangeBinsCalc;
    /*! @brief number of processed range bins */
    uint16_t numRangeBinsProc;

    /*! @brief number of calculated azimuth bins */
    uint16_t numAzBinsCalc;

    /*! @brief range resolution in meters */
    float rangeResolution;
    float rangeAccuracy;
    uint8_t rangeZeropad;

    /*! @brief Timing information */
    MmwDemo_timingInfo_t timingInfo;

    /*! @brief chirp counter modulo number of chirps per frame */
    uint16_t chirpCount;

    /*! @brief chirp counter modulo number of tx antennas */
    uint8_t txAntennaCount;
    /*! @brief chirp counter modulo number of Doppler bins */
    uint16_t chirpPerTxCount;

    /*! @brief  DSP cycles for chirp and interframe processing and pending
     *          on EDMA data transferes*/
    cycleLog_t cycleLog;


    /*! @brief ADCBUF will generate chirp interrupt event every this many chirps */
    uint16_t   numChirpsPerChirpEvent;

    /*! @brief Rx channel gain/phase offset compensation coefficients */
    MmwDemo_compRxChannelBiasCfg_t compRxChanCfg;

    /*! @brief subframe index for this obj */
    uint8_t subFrameIndx;

    uint32_t framePeriodicity_ms;     // Frame Periodicity in ms

    uint16_t rangeBinStartIndex;      // Range-bin Start Index
    uint16_t rangeBinEndIndex;        // Range-bin End Index

    SystemInfo systemInfo;

    cmplx16ReIm_t *adcDataCube;

#if BASIC_PROCESSING_ENA == 1
    float unwrapPhasePeak;            // Unwrapped phase value corresponding to the object range-bin

    // IIR Filtering
    float *pFilterCoefsBreath;             // IIR-Filter Coefficients for Breathing
    float *pScaleValsBreath;               // Scale values for IIR-Cascade Filter
    float *pFilterCoefsHeart_4Hz;          // IIR-Filter Coefficients for Cardiac Waveform
    float *pScaleValsHeart_4Hz;            // Scale values for IIR-Cascade Filter
    float pDelayHeart[HEART_WFM_IIR_FILTER_TAPS_LENGTH];          // IIR-Filter delay lines
    float pDelayBreath[BREATHING_WFM_IIR_FILTER_TAPS_LENGTH];     // IIR-Filter delay lines

    float    noiseImpulse_Thresh;
    VitalSignsDemo_OutputStats  VitalSigns_Output;
#endif

    uint8_t transmitSysInfo;
    uint8_t transmitVitalSigns;
    uint8_t transmitRangeProfile;
    uint8_t transmitAdcData;

} MmwDemo_DSS_DataPathObj;

#define MMWDEMO_MEMORY_ALLOC_WORD_ALIGN 4
#define MMWDEMO_MEMORY_ALLOC_BYTE_ALIGN 2

/**
 *  @b Description
 *  @n
 *   Initializes data path object with supplied context and cli Config.
 *   The context and cli config point to permanent storage outside of data path object 
 *   that data path object can refer to anytime during the lifetime of data path object.
 *   Data path default values that are not required to come through CLI commands are
 *   set in this function.
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_dataPathObjInit(MmwDemo_DSS_DataPathObj *obj,
                             MmwDemo_DSS_dataPathContext_t *context,
                             MmwDemo_CliCfg_t *cliCfg,
                             MmwDemo_CliCommonCfg_t *cliCommonCfg);


/**
 *  @b Description
 *  @n
 *   Initializes data path state variables for 1D processing.
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_dataPathInit1Dstate(MmwDemo_DSS_DataPathObj *obj);

/**
 *  @b Description
 *  @n
 *   Delete Semaphores which are created in MmwDemo_dataPathInitEdma().
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_dataPathDeleteSemaphore(MmwDemo_DSS_dataPathContext_t *context);

/**
 *  @b Description
 *  @n
 *   Initializes EDMA driver.
 *  @retval
 *      Not Applicable.
 */
int32_t MmwDemo_dataPathInitEdma(MmwDemo_DSS_dataPathContext_t *context);

/**
 *  @b Description
 *  @n
 *   Configures EDMA driver for all of the data path processing.
 *  @retval
 *      Not Applicable.
 */
int32_t MmwDemo_dataPathConfigEdma(MmwDemo_DSS_DataPathObj *obj);


/**
 *  @b Description
 *  @n
 *   Creates heap in L2 and L3 and allocates data path buffers,
 *   The heap is destroyed at the end of the function.
 *
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_dataPathInitVitalSignsAndMonitor(MmwDemo_DSS_DataPathObj *obj);
void MmwDemo_dataPathConfigBuffers(MmwDemo_DSS_DataPathObj *obj, uint32_t adcBufAddress);
void my_Initializations(MmwDemo_DSS_DataPathObj *obj);
/**
 *  @b Description
 *  @n
 *   Configures FFTs (twiddle tables etc) involved in 1D, 2D and Azimuth processing.
 *
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_dataPathConfigFFTs(MmwDemo_DSS_DataPathObj *obj);

/**
 *  @b Description
 *  @n
 *  Wait for transfer of data corresponding to the last 2 chirps (ping/pong)
 *  to the radarCube matrix before starting interframe processing.
 *  @retval
 *      Not Applicable.
 */
void MmwDemo_waitEndOfChirps(MmwDemo_DSS_DataPathObj *obj);

/**
 *  @b Description
 *  @n
 *    Chirp processing. It is called from MmwDemo_dssDataPathProcessEvents. It
 *    is executed per chirp
 *
 *  @retval
 *      Not Applicable.
 */
void chirpProcess(MmwDemo_DSS_DataPathObj *obj, uint16_t chirpIndxInMultiChirp);

/**
 *  @b Description
 *  @n
 *    Interframe processing. It is called from MmwDemo_dssDataPathProcessEvents
 *    after all chirps of the frame have been received and 1D FFT processing on them
 *    has been completed.
 *
 *  @retval
 *      Not Applicable.
 */
void interFrameProcessing(MmwDemo_DSS_DataPathObj *obj);

/**
 *  @b Description
 *  @n
 *      Power of 2 round up function.
 */
uint32_t MmwDemo_pow2roundup (uint32_t x);

///////////////////////// AGGIUNTO ////////////////////
void AzimuthProcess(MmwDemo_DSS_DataPathObj *obj, uint16_t azimuthIndx);
void interAzimuthProcessing(MmwDemo_DSS_DataPathObj *obj, uint8_t chirpPingPongId)
void MmwDemo_dataPathWait2DOutputData(MmwDemo_DSS_DataPathObj *obj, uint32_t pingPongId);




#ifdef __cplusplus
}
#endif

#endif /* DSS_DATA_PATH_H */

