/**
 *   @file  dss_data_path.c
 *
 *   @brief
 *      Implements Data path processing functionality.
 *
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

#include <ti/drivers/osal/SemaphoreP.h>

#define DebugP_ASSERT_ENABLED 1
#include <ti/drivers/osal/DebugP.h>
#include <assert.h>
#include <ti/common/sys_common.h>
#include <ti/drivers/osal/SemaphoreP.h>
#include <ti/drivers/edma/edma.h>
#include <ti/drivers/esm/esm.h>
#include <ti/drivers/soc/soc.h>
#include <ti/utils/cycleprofiler/cycle_profiler.h>
#include <ti/alg/mmwavelib/mmwavelib.h>


#include "mmw_messages.h"

/* C64P dsplib (fixed point part for C674X) */
#include "DSP_fft32x32.h"
#include "DSP_fft16x16.h"


#include "communication1843.h"
#include "user_parameters.h"

#pragma diag_push
#pragma diag_suppress 48
#include <ti/mathlib/mathlib.h>
#pragma diag_pop
#include <ti/utils/mathutils/mathutils.h>
#include "dss_mmw.h"
#include "dss_data_path.h"
#include "dss_config_edma_util.h"
#include "dss_resources.h"

#if BASIC_PROCESSING_ENA == 1
	#include "dss_vitalSignsDemo_utilsFunc.h"
#endif

#define MMW_ADCBUF_SIZE     0x4000U

/*! @brief L2 heap used for allocating buffers in L2 SRAM,
    mostly scratch buffers */
#define MMW_L2_HEAP_SIZE    0xC000U

/*! @brief L1 heap used for allocating buffers in L1D SRAM,
    mostly scratch buffers */
#define MMW_L1_HEAP_SIZE    0x4000U

/*! L3 RAM buffer */
#pragma DATA_SECTION(gMmwL3, ".l3data");
#pragma DATA_ALIGN(gMmwL3, 8);
uint8_t gMmwL3[SOC_L3RAM_SIZE];

/*! L2 Heap */
#pragma DATA_SECTION(gMmwL2, ".l2data");
#pragma DATA_ALIGN(gMmwL2, 8);
uint8_t gMmwL2[MMW_L2_HEAP_SIZE];

/*! L1 Heap */
#pragma DATA_SECTION(gMmwL1, ".l1data");
#pragma DATA_ALIGN(gMmwL1, 8);
uint8_t gMmwL1[MMW_L1_HEAP_SIZE];

/*! Data type of FFT window */
#define FFT_WINDOW_INT16 0
#define FFT_WINDOW_INT32 1
#define FFT_WINDOW_SP    2

/* FFT Window */
#define MMW_WIN_HANNING  0
#define MMW_WIN_BLACKMAN 1
#define MMW_WIN_RECT     2
#define MMW_WIN_HAMMING  3

#define MMW_EDMA_TRIGGER_DISABLE 0
#define MMW_EDMA_TRIGGER_ENABLE  1
extern volatile cycleLog_t gCycleLog;

void genWindow(void *win,
                        uint32_t windowDatumType,
                        uint32_t winLen,
                        uint32_t winGenLen,
                        int32_t oneQformat,
                        uint32_t winType);

#pragma DATA_ALIGN(twiddleTableCommon, 8)
const int32_t twiddleTableCommon[2*256] = {
    0x00000000, 0x7fffffff, 0x00c90f88, 0x7fff6216, 0x01921d20, 0x7ffd885a, 0x025b26d7, 0x7ffa72d1,
    0x03242abf, 0x7ff62182, 0x03ed26e6, 0x7ff09477, 0x04b6195d, 0x7fe9cbbf, 0x057f0035, 0x7fe1c76b,
    0x0647d97c, 0x7fd8878d, 0x0710a345, 0x7fce0c3e, 0x07d95b9e, 0x7fc25596, 0x08a2009a, 0x7fb563b2,
    0x096a9049, 0x7fa736b4, 0x0a3308bd, 0x7f97cebc, 0x0afb6805, 0x7f872bf2, 0x0bc3ac35, 0x7f754e7f,
    0x0c8bd35e, 0x7f62368f, 0x0d53db92, 0x7f4de450, 0x0e1bc2e4, 0x7f3857f5, 0x0ee38766, 0x7f2191b3,
    0x0fab272b, 0x7f0991c3, 0x1072a048, 0x7ef0585f, 0x1139f0cf, 0x7ed5e5c6, 0x120116d5, 0x7eba3a39,
    0x12c8106e, 0x7e9d55fc, 0x138edbb1, 0x7e7f3956, 0x145576b1, 0x7e5fe493, 0x151bdf86, 0x7e3f57fe,
    0x15e21444, 0x7e1d93e9, 0x16a81305, 0x7dfa98a7, 0x176dd9de, 0x7dd6668e, 0x183366e9, 0x7db0fdf7,
    0x18f8b83c, 0x7d8a5f3f, 0x19bdcbf3, 0x7d628ac5, 0x1a82a026, 0x7d3980ec, 0x1b4732ef, 0x7d0f4218,
    0x1c0b826a, 0x7ce3ceb1, 0x1ccf8cb3, 0x7cb72724, 0x1d934fe5, 0x7c894bdd, 0x1e56ca1e, 0x7c5a3d4f,
    0x1f19f97b, 0x7c29fbee, 0x1fdcdc1b, 0x7bf88830, 0x209f701c, 0x7bc5e28f, 0x2161b3a0, 0x7b920b89,
    0x2223a4c5, 0x7b5d039d, 0x22e541af, 0x7b26cb4f, 0x23a6887e, 0x7aef6323, 0x24677757, 0x7ab6cba3,
    0x25280c5e, 0x7a7d055b, 0x25e845b6, 0x7a4210d8, 0x26a82186, 0x7a05eead, 0x27679df4, 0x79c89f6d,
    0x2826b928, 0x798a23b1, 0x28e5714b, 0x794a7c11, 0x29a3c485, 0x7909a92c, 0x2a61b101, 0x78c7aba1,
    0x2b1f34eb, 0x78848413, 0x2bdc4e6f, 0x78403328, 0x2c98fbba, 0x77fab988, 0x2d553afb, 0x77b417df,
    0x2e110a62, 0x776c4edb, 0x2ecc681e, 0x77235f2d, 0x2f875262, 0x76d94988, 0x3041c760, 0x768e0ea5,
    0x30fbc54d, 0x7641af3c, 0x31b54a5d, 0x75f42c0a, 0x326e54c7, 0x75a585cf, 0x3326e2c2, 0x7555bd4b,
    0x33def287, 0x7504d345, 0x34968250, 0x74b2c883, 0x354d9057, 0x745f9dd1, 0x36041ad9, 0x740b53fa,
    0x36ba2014, 0x73b5ebd0, 0x376f9e46, 0x735f6626, 0x382493b0, 0x7307c3d0, 0x38d8fe93, 0x72af05a6,
    0x398cdd32, 0x72552c84, 0x3a402dd2, 0x71fa3948, 0x3af2eeb7, 0x719e2cd2, 0x3ba51e29, 0x71410804,
    0x3c56ba70, 0x70e2cbc6, 0x3d07c1d6, 0x708378fe, 0x3db832a6, 0x70231099, 0x3e680b2c, 0x6fc19385,
    0x3f1749b8, 0x6f5f02b1, 0x3fc5ec98, 0x6efb5f12, 0x4073f21d, 0x6e96a99c, 0x4121589a, 0x6e30e349,
    0x41ce1e64, 0x6dca0d14, 0x427a41d0, 0x6d6227fa, 0x4325c135, 0x6cf934fb, 0x43d09aec, 0x6c8f351c,
    0x447acd50, 0x6c242960, 0x452456bd, 0x6bb812d1, 0x45cd358f, 0x6b4af278, 0x46756828, 0x6adcc964,
    0x471cece6, 0x6a6d98a4, 0x47c3c22f, 0x69fd614a, 0x4869e665, 0x698c246c, 0x490f57ee, 0x6919e320,
    0x49b41533, 0x68a69e81, 0x4a581c9d, 0x683257ab, 0x4afb6c98, 0x67bd0fbc, 0x4b9e038f, 0x6746c7d7,
    0x4c3fdff3, 0x66cf811f, 0x4ce10034, 0x66573cbb, 0x4d8162c4, 0x65ddfbd3, 0x4e210617, 0x6563bf92,
    0x4ebfe8a4, 0x64e88926, 0x4f5e08e3, 0x646c59bf, 0x4ffb654d, 0x63ef328f, 0x5097fc5e, 0x637114cc,
    0x5133cc94, 0x62f201ac, 0x51ced46e, 0x6271fa69, 0x5269126e, 0x61f1003f, 0x53028517, 0x616f146b,
    0x539b2aef, 0x60ec3830, 0x5433027d, 0x60686cce, 0x54ca0a4a, 0x5fe3b38d, 0x556040e2, 0x5f5e0db3,
    0x55f5a4d2, 0x5ed77c89, 0x568a34a9, 0x5e50015d, 0x571deef9, 0x5dc79d7c, 0x57b0d256, 0x5d3e5236,
    0x5842dd54, 0x5cb420df, 0x58d40e8c, 0x5c290acc, 0x59646497, 0x5b9d1153, 0x59f3de12, 0x5b1035cf,
    0x5a82799a, 0x5a82799a, 0x5b1035cf, 0x59f3de12, 0x5b9d1153, 0x59646497, 0x5c290acc, 0x58d40e8c,
    0x5cb420df, 0x5842dd54, 0x5d3e5236, 0x57b0d256, 0x5dc79d7c, 0x571deef9, 0x5e50015d, 0x568a34a9,
    0x5ed77c89, 0x55f5a4d2, 0x5f5e0db3, 0x556040e2, 0x5fe3b38d, 0x54ca0a4a, 0x60686cce, 0x5433027d,
    0x60ec3830, 0x539b2aef, 0x616f146b, 0x53028517, 0x61f1003f, 0x5269126e, 0x6271fa69, 0x51ced46e,
    0x62f201ac, 0x5133cc94, 0x637114cc, 0x5097fc5e, 0x63ef328f, 0x4ffb654d, 0x646c59bf, 0x4f5e08e3,
    0x64e88926, 0x4ebfe8a4, 0x6563bf92, 0x4e210617, 0x65ddfbd3, 0x4d8162c4, 0x66573cbb, 0x4ce10034,
    0x66cf811f, 0x4c3fdff3, 0x6746c7d7, 0x4b9e038f, 0x67bd0fbc, 0x4afb6c98, 0x683257ab, 0x4a581c9d,
    0x68a69e81, 0x49b41533, 0x6919e320, 0x490f57ee, 0x698c246c, 0x4869e665, 0x69fd614a, 0x47c3c22f,
    0x6a6d98a4, 0x471cece6, 0x6adcc964, 0x46756828, 0x6b4af278, 0x45cd358f, 0x6bb812d1, 0x452456bd,
    0x6c242960, 0x447acd50, 0x6c8f351c, 0x43d09aec, 0x6cf934fb, 0x4325c135, 0x6d6227fa, 0x427a41d0,
    0x6dca0d14, 0x41ce1e64, 0x6e30e349, 0x4121589a, 0x6e96a99c, 0x4073f21d, 0x6efb5f12, 0x3fc5ec98,
    0x6f5f02b1, 0x3f1749b8, 0x6fc19385, 0x3e680b2c, 0x70231099, 0x3db832a6, 0x708378fe, 0x3d07c1d6,
    0x70e2cbc6, 0x3c56ba70, 0x71410804, 0x3ba51e29, 0x719e2cd2, 0x3af2eeb7, 0x71fa3948, 0x3a402dd2,
    0x72552c84, 0x398cdd32, 0x72af05a6, 0x38d8fe93, 0x7307c3d0, 0x382493b0, 0x735f6626, 0x376f9e46,
    0x73b5ebd0, 0x36ba2014, 0x740b53fa, 0x36041ad9, 0x745f9dd1, 0x354d9057, 0x74b2c883, 0x34968250,
    0x7504d345, 0x33def287, 0x7555bd4b, 0x3326e2c2, 0x75a585cf, 0x326e54c7, 0x75f42c0a, 0x31b54a5d,
    0x7641af3c, 0x30fbc54d, 0x768e0ea5, 0x3041c760, 0x76d94988, 0x2f875262, 0x77235f2d, 0x2ecc681e,
    0x776c4edb, 0x2e110a62, 0x77b417df, 0x2d553afb, 0x77fab988, 0x2c98fbba, 0x78403328, 0x2bdc4e6f,
    0x78848413, 0x2b1f34eb, 0x78c7aba1, 0x2a61b101, 0x7909a92c, 0x29a3c485, 0x794a7c11, 0x28e5714b,
    0x798a23b1, 0x2826b928, 0x79c89f6d, 0x27679df4, 0x7a05eead, 0x26a82186, 0x7a4210d8, 0x25e845b6,
    0x7a7d055b, 0x25280c5e, 0x7ab6cba3, 0x24677757, 0x7aef6323, 0x23a6887e, 0x7b26cb4f, 0x22e541af,
    0x7b5d039d, 0x2223a4c5, 0x7b920b89, 0x2161b3a0, 0x7bc5e28f, 0x209f701c, 0x7bf88830, 0x1fdcdc1b,
    0x7c29fbee, 0x1f19f97b, 0x7c5a3d4f, 0x1e56ca1e, 0x7c894bdd, 0x1d934fe5, 0x7cb72724, 0x1ccf8cb3,
    0x7ce3ceb1, 0x1c0b826a, 0x7d0f4218, 0x1b4732ef, 0x7d3980ec, 0x1a82a026, 0x7d628ac5, 0x19bdcbf3,
    0x7d8a5f3f, 0x18f8b83c, 0x7db0fdf7, 0x183366e9, 0x7dd6668e, 0x176dd9de, 0x7dfa98a7, 0x16a81305,
    0x7e1d93e9, 0x15e21444, 0x7e3f57fe, 0x151bdf86, 0x7e5fe493, 0x145576b1, 0x7e7f3956, 0x138edbb1,
    0x7e9d55fc, 0x12c8106e, 0x7eba3a39, 0x120116d5, 0x7ed5e5c6, 0x1139f0cf, 0x7ef0585f, 0x1072a048,
    0x7f0991c3, 0x0fab272b, 0x7f2191b3, 0x0ee38766, 0x7f3857f5, 0x0e1bc2e4, 0x7f4de450, 0x0d53db92,
    0x7f62368f, 0x0c8bd35e, 0x7f754e7f, 0x0bc3ac35, 0x7f872bf2, 0x0afb6805, 0x7f97cebc, 0x0a3308bd,
    0x7fa736b4, 0x096a9049, 0x7fb563b2, 0x08a2009a, 0x7fc25596, 0x07d95b9e, 0x7fce0c3e, 0x0710a345,
    0x7fd8878d, 0x0647d97c, 0x7fe1c76b, 0x057f0035, 0x7fe9cbbf, 0x04b6195d, 0x7ff09477, 0x03ed26e6,
    0x7ff62182, 0x03242abf, 0x7ffa72d1, 0x025b26d7, 0x7ffd885a, 0x01921d20, 0x7fff6216, 0x00c90f88
};


int32_t MmwDemo_gen_twiddle_fft16x16_fast(short *w, int32_t n);
void windowing16x16_evenlen(int16_t inp[restrict],
                            const int16_t win[restrict],
                            uint32_t len);




/**
 *  @b Description
 *  @n
 *      Power of 2 round up function.
 */
uint32_t MmwDemo_pow2roundup (uint32_t x)
{
    uint32_t result = 1;
    while(x > result)
    {
        result <<= 1;
    }
    return result;
}


/**
 *  @b Description
 *  @n
 *      Waits for 1D FFT data to be transferrd to input buffer.
 *      This is a blocking function.
 *
 *  @param[in] obj  Pointer to data path object
 *  @param[in] pingPongId ping-pong id (ping is 0 and pong is 1)
 *
 *  @retval
 *      NONE
 */
void MmwDemo_dataPathWait1DInputData(MmwDemo_DSS_DataPathObj *obj, uint32_t pingPongId)
{
    MmwDemo_DSS_dataPathContext_t *context = obj->context;

#ifdef EDMA_1D_INPUT_BLOCKING
    Bool       status;

    status = Semaphore_pend(context->EDMA_1D_InputDone_semHandle[pingPongId], BIOS_WAIT_FOREVER);
    if (status != TRUE)
    {
        printf("Error: Semaphore_pend returned %d\n",status);
    }
#else
    /* wait until transfer done */
    volatile bool isTransferDone;
    uint8_t chId;
    if(pingPongId == 0)
    {
        chId = MMW_EDMA_CH_1D_IN_PING;
    }
    else
    {
        chId = MMW_EDMA_CH_1D_IN_PONG;
    }
    do {
        if (EDMA_isTransferComplete(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                                    chId,
                                    (bool *)&isTransferDone) != EDMA_NO_ERROR)
        {
            MmwDemo_dssAssert(0);
        }
    } while (isTransferDone == false);
#endif
}

/**
 *  @b Description
 *  @n
 *      Waits for 1D FFT data to be transferred to output buffer.
 *      This is a blocking function.
 *
 *  @param[in] obj  Pointer to data path object
 *  @param[in] pingPongId ping-pong id (ping is 0 and pong is 1)
 *
 *  @retval
 *      NONE
 */
void MmwDemo_dataPathWait1DOutputData(MmwDemo_DSS_DataPathObj *obj, uint32_t pingPongId)
{
    MmwDemo_DSS_dataPathContext_t *context = obj->context;

    /* wait until transfer done */
    volatile bool isTransferDone;
    uint8_t chId;
    if(pingPongId == 0)
    {
        chId = MMW_EDMA_CH_1D_OUT_PING;
    }
    else
    {
        chId = MMW_EDMA_CH_1D_OUT_PONG;
    }
    do {
        if (EDMA_isTransferComplete(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                                    chId,
                                    (bool *)&isTransferDone) != EDMA_NO_ERROR)
        {
            MmwDemo_dssAssert(0);
        }
    } while (isTransferDone == false);

}

/*****************************************************
 * This function configures all the EDMA channels
 *****************************************************/
int32_t MmwDemo_dataPathConfigEdma(MmwDemo_DSS_DataPathObj *obj)
{
    uint32_t eventQueue;
    int32_t retVal = 0;
    uint16_t numPingOrPongSamples, aCount;
    int16_t oneD_destinationBindex;
    MmwDemo_DSS_dataPathContext_t *context = obj->context;
    uint8_t *oneD_destinationPongAddress;

    /*****************************************************
     * EDMA configuration for getting ADC data prior to 1D FFT
     * ADCdataBuf --> adcDataIn
     * Ping copies chirp samples from even antenna numbers (e.g. RxAnt0 and RxAnt2)
     * Pong copies chirp samples from odd antenna numbers (e.g. RxAnt1 and RxAnt3)
    *****************************************************/
    eventQueue = 0U;
    retVal = EDMAutil_configType1(
        context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
        (uint8_t *)(&obj->ADCdataBuf[0]),
        (uint8_t *)(SOC_translateAddress((uint32_t)&obj->adcDataIn[0],SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
        MMW_EDMA_CH_1D_IN_PING,
        false,
        MMW_EDMA_CH_1D_IN_PING_SHADOW,
        obj->numAdcSamples * BYTES_PER_SAMP_1D,
        MAX(obj->numRxAntennas / 2, 1) * obj->numChirpsPerChirpEvent,
        (obj->numAdcSamples * BYTES_PER_SAMP_1D * 2) * obj->numChirpsPerChirpEvent,
        0,
        eventQueue,
        NULL,
        (uintptr_t) obj
    );
    if (retVal < 0)
        return -1;


    retVal = EDMAutil_configType1(
        context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
        (uint8_t *)(&obj->ADCdataBuf[obj->numAdcSamples * obj->numChirpsPerChirpEvent]),
        (uint8_t *)(SOC_translateAddress((uint32_t)(&obj->adcDataIn[obj->numRangeBinsCalc]),SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
        MMW_EDMA_CH_1D_IN_PONG,
        false,
        MMW_EDMA_CH_1D_IN_PONG_SHADOW,
        obj->numAdcSamples * BYTES_PER_SAMP_1D,
        MAX(obj->numRxAntennas / 2, 1) * obj->numChirpsPerChirpEvent,
        (obj->numAdcSamples * BYTES_PER_SAMP_1D * 2) * obj->numChirpsPerChirpEvent,
        0,
        eventQueue,
        NULL,
        (uintptr_t) obj
     );
    if (retVal < 0)
        return -1;

    /*****************************************************
     * EDMA configuration for storing 1d fft output to radarCube.
     * It copies all Rx antennas of the chirp per trigger event.
     * Ping copies 1d fft of even chirp numbers (e.g. Chirp0, Chirp2 ...)
     * Pong copies 1d fft of odd chirp numbers (e.g. Chirp1, Chirp3 ...)
     *****************************************************/
    numPingOrPongSamples = obj->numRangeBinsCalc * obj->numRxAntennas;
    aCount = numPingOrPongSamples * BYTES_PER_SAMP_1D;

    /* If TDM-MIMO (BPM or otherwise), store odd chirps consecutively and even
       chirps consecutively. This is done because for the case of 1024 range bins
       and 4 rx antennas, the source jump required for 2D processing will be 32768
       which is negative jump for the EDMA (16-bit signed jump). Storing in this way
       reduces the jump to be positive which makes 2D processing feasible */
    oneD_destinationBindex = (int16_t)aCount;
    oneD_destinationPongAddress = (uint8_t *)(&obj->radarCube[numPingOrPongSamples /** obj->numDopplerBins*/]); // SaraDbg: mettere in funzione del numero di TX

    /* using different event queue between input and output to parallelize better */
    eventQueue = 1U;
    /* Ping */
    retVal =
    EDMAutil_configType1(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
        (uint8_t *)(SOC_translateAddress((uint32_t)(&obj->fftOut1D[0]),SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
        (uint8_t *)(&obj->radarCube[0]),
        MMW_EDMA_CH_1D_OUT_PING,
        false,      // flag is triggered
        MMW_EDMA_CH_1D_OUT_PING_SHADOW,
        aCount,                     // aCount
        obj->numChirpsPerFrame / 2, //bCount
        0, //srcBidx
        oneD_destinationBindex, //dstBidx
        eventQueue,
#ifdef EDMA_1D_OUTPUT_BLOCKING
        MmwDemo_EDMA_transferCompletionCallbackFxn,
#else
        NULL,
#endif
        (uintptr_t) obj);


    if (retVal < 0)
        return -1;

    /* Pong - Copies from pong FFT output (odd chirp indices)  to L3 */
    retVal =
    EDMAutil_configType1(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
        (uint8_t *)(SOC_translateAddress((uint32_t)(&obj->fftOut1D[numPingOrPongSamples]),
                                         SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
        oneD_destinationPongAddress,
        MMW_EDMA_CH_1D_OUT_PONG,
        false,
        MMW_EDMA_CH_1D_OUT_PONG_SHADOW,
        aCount,
        obj->numChirpsPerFrame / 2, //bCount
        0, //srcBidx
        oneD_destinationBindex, //dstBidx
        eventQueue,
        NULL,
        (uintptr_t) obj);

    if (retVal < 0)
        return -1;


    //////////////////////// AGGIUNTO ////////////////////////////
    // edma per passare i dati da radarCube (L3) a dataAzIn (L1) per processarli con ping pong
    // FA LA TRASPOSTA per preparare i dati alla fft
    // radarCube ---> dataAzIn
    // Ping : chirp pari
    // Pong : chirp dispari
    eventQueue = 2U;
    retVal = EDMAutil_configType1(
            context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
            (uint8_t *)(&obj->radarCube[0]),
            (uint8_t *)(SOC_translateAddress((uint32_t)&obj->dataAzIn[0],SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
            MMW_EDMA_CH_2D_IN_PING,
            true,                // PER TRIGGERARE bCount TRASFERIMENTI OGNI CHIAMATA
            MMW_EDMA_CH_2D_IN_PING_SHADOW,
            BYTES_PER_SAMP_1D,   // aCount
            (obj->numRxAntennas * obj->numTxAntennas) / 2, //bCount
            obj->numRangeBinsCalc * BYTES_PER_SAMP_1D, //srcBIdx
            0, //dstBIdx, non serve avere offset in destination
            eventQueue,
            NULL,
            (uintptr_t) obj
        );
    retVal = EDMAutil_configType1(
                context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                (uint8_t *)(&obj->radarCube[obj->numRangeBinsCalc * obj->numRxAntennas * obj->numTxAntennas]),
                (uint8_t *)(SOC_translateAddress((uint32_t)&obj->dataAzIn[0],SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
                MMW_EDMA_CH_2D_IN_PONG,
                true,   // PER TRIGGERARE bCount TRASFERIMENTI OGNI CHIAMATA
                MMW_EDMA_CH_2D_IN_PONG_SHADOW,
                BYTES_PER_SAMP_1D,   // aCount
                (obj->numRxAntennas * obj->numTxAntennas)/ 2, //bCount (1Tx => 4 )
                obj->numRangeBinsCalc * BYTES_PER_SAMP_1D, //srcBIdx
                0, //dstBIdx, non serve avere offset in destination
                eventQueue,
                NULL,
                (uintptr_t) obj
            );
    //////////////////////// AGGIUNTO ////////////////////////////
        // edma per passare i dati da fftOut2d (L2) a rangeAzMap (L3)
        // fftOut2D ---> rangeAzMap
        // Ping : chirp pari
        // Pong : chirp dispari
        eventQueue = 3U;
        retVal = EDMAutil_configType1(
                context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                (uint8_t *)(&obj->fftOut2D[0]),
                (uint8_t *)(SOC_translateAddress((uint32_t)&obj->rangeAzMap[0],SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
                MMW_EDMA_CH_2D_OUT_PING,
                false,
                MMW_EDMA_CH_2D_OUT_PING_SHADOW,
                obj->numRxAntennas * obj->numRangeBinsCalc * obj->numTxAntennas,   // aCount
                obj->numRxAntennas * obj->numTxAntennas,    //bCount (1Tx => 4 )
                obj->numRangeBinsCalc * BYTES_PER_SAMP_1D, //srcBIdx
                0, //dstBIdx, non serve avere offset in destination
                eventQueue,
                NULL,
                (uintptr_t) obj
            );
        retVal = EDMAutil_configType1(
                    context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                    (uint8_t *)(&obj->fftOut2D [obj->numAzBinsCalc]),
                    (uint8_t *)(SOC_translateAddress((uint32_t)&obj->rangeAzMap[obj->numAzBinsCalc],SOC_TranslateAddr_Dir_TO_EDMA,NULL)),
                    MMW_EDMA_CH_2D_OUT_PONG,
                    false,
                    MMW_EDMA_CH_2D_OUT_PONG_SHADOW,
                    BYTES_PER_SAMP_1D,   // aCount
                    obj->numRxAntennas * obj->numTxAntennas, //bCount (1Tx => 4 )
                    obj->numRangeBinsCalc * BYTES_PER_SAMP_1D, //srcBIdx
                    0, //dstBIdx, non serve avere offset in destination
                    eventQueue,
                    NULL,
                    (uintptr_t) obj
                );


    return(0);
}


#define pingPongId(x) ((x) & 0x1U)
#define isPong(x) (pingPongId(x))



/*
 *  Interchirp processing. It is executed per chirp event, after ADC
 *  buffer is filled with chirp samples.
 *  This function executes: windowing and fft
 */

void interChirpProcessing(MmwDemo_DSS_DataPathObj *obj, uint8_t chirpPingPongId)
{
    uint32_t antIndx, waitingTime;
    volatile uint32_t startTime;
    volatile uint32_t startTime1;
    MmwDemo_DSS_dataPathContext_t *context = obj->context;

    uint32_t adcDataOffset;

    waitingTime = 0;
    startTime = Cycleprofiler_getTimeStamp();

    // Start DMA transfer to fetch data from RX1
    EDMA_startDmaTransfer(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                       MMW_EDMA_CH_1D_IN_PING);

    // 1d fft for first antenna, followed by kicking off the DMA of fft output
    for (antIndx = 0; antIndx < obj->numRxAntennas; antIndx++)
    {
        adcDataOffset = pingPongId(antIndx)*obj->numRangeBinsCalc;

        // Start DMA transfer to move data from the next antenna (while processing the actual)
        // When processing data from RX1, move data from RX2. When processing data from RX4,
        // no further DMA transfer is needed
        if (antIndx < (obj->numRxAntennas - 1))
        {
            if (isPong(antIndx))
            {
                EDMA_startDmaTransfer(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                        MMW_EDMA_CH_1D_IN_PING);
            }
            else
            {
                EDMA_startDmaTransfer(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                        MMW_EDMA_CH_1D_IN_PONG);
            }
        }

        // verify if DMA has completed for current antenna
        startTime1 = Cycleprofiler_getTimeStamp();
        MmwDemo_dataPathWait1DInputData (obj, pingPongId(antIndx));
        waitingTime += Cycleprofiler_getTimeStamp() - startTime1;


        // copy adc data into the adcDataCube
        memcpy(&obj->adcDataCube[antIndx*obj->numAdcSamples],   // DESTINATION
               &obj->adcDataIn[adcDataOffset],                  // SOURCE
               obj->numAdcSamples*sizeof(cmplx16ReIm_t) );      // LENGTH


        /* Range FFT */
        // windowing
        mmwavelib_windowing16x16_evenlen(
                (int16_t *) &obj->adcDataIn[adcDataOffset],
                (int16_t *) obj->window1D,
                obj->numAdcSamples);

        // reset zero-padded data (fft function overwrites input)
        memset((void *)&obj->adcDataIn[adcDataOffset + obj->numAdcSamples],
            0 , (obj->numRangeBinsCalc - obj->numAdcSamples) * sizeof(cmplx16ReIm_t));

        // fft
        DSP_fft16x16(
                (int16_t *) obj->twiddle16x16_1D,
                obj->numRangeBinsCalc,
                (int16_t *) &obj->adcDataIn[adcDataOffset],
                (int16_t *) &obj->fftOut1D[chirpPingPongId * (obj->numRxAntennas * obj->numRangeBinsCalc) +
                                               (obj->numRangeBinsCalc * antIndx)]);
    }

    gCycleLog.interChirpProcessingTime += Cycleprofiler_getTimeStamp() - startTime - waitingTime;
    gCycleLog.interChirpWaitTime += waitingTime;

}


/*
 *    Interframe processing. It is called from MmwDemo_dssDataPathProcessEvents
 *    after all chirps of the frame have been received and 1D FFT processing on them
 *    has been completed.
 */
static uint32_t gFrameCount;



void interFrameProcessing(MmwDemo_DSS_DataPathObj *obj)
{

    uint16_t rangeBinIndex;
    int16_t temp_real, temp_imag;

#if BASIC_PROCESSING_ENA == 1

    uint16_t frameCountLocal;    // Local circular count

    float rangeBinPhase;                // Phase of the Range-bin selected for processing
    static uint16_t rangeBinIndexPhase; // Index of the Range Bin for which the phase is computed
    uint16_t rangeBinMax;               // Index of the Strongest Range-Bin

    /* Variables for tracking the range bin */
    float absVal;
    float maxVal;

    /* Variables for Impulse Noise Removal */
    static float dataCurr = 0;
    static float dataPrev2 = 0;
    static float dataPrev1 = 0;

    /* Variables for Phase Unwrapping */
    static float phasePrevFrame = 0;              // Phase value of Previous frame (For phase unwrapping)
    static float diffPhaseCorrectionCum = 0;      // Phase correction cumulative (For phase unwrapping)
    static float phaseUsedComputationPrev = 0;    // Phase values used for the Previous frame
    float phaseUsedComputation;            // Unwrapped Phase value used for computation

    /* Vital Signs Waveform */
    float outputFilterBreathOut;              // Breathing waveform after the IIR-Filter
    float outputFilterHeartOut;               // Cardiac waveform after the IIR-Filter

    if (RANGE_BIN_TRACKING == 1)
        frameCountLocal = gFrameCount % RESET_LOCAL_COUNT_VAL;
    else
        frameCountLocal = gFrameCount;

    rangeBinMax = 0;
    maxVal = 0;

#endif

    gFrameCount++;
    // tempPtr points towards the RX-Channel to process
    cmplx16ReIm_t *tempPtr = obj->fftOut1D + (obj->rxAntennaProcess - 1)*obj->numRangeBinsCalc;
    // tempPtr points towards rangeBinStartIndex
    tempPtr += (obj->rangeBinStartIndex) ;

    for (rangeBinIndex = obj->rangeBinStartIndex; rangeBinIndex <= obj->rangeBinEndIndex; rangeBinIndex++)
    {

        // Points towards the real part of the current range-bin i.e. rangeBinIndex
        temp_real = (int16_t) tempPtr->real;
        // Points towards the imaginary part of the current range-bin i.e. rangeBinIndex
        temp_imag = (int16_t) tempPtr->imag;

        // Copy the data into range profile buffer
        obj->pRangeProfileCplx[rangeBinIndex - obj->rangeBinStartIndex].real = temp_real;
        obj->pRangeProfileCplx[rangeBinIndex - obj->rangeBinStartIndex].imag = temp_imag;

        // Move the pointer towards the next range-bin
        tempPtr ++;


#if BASIC_PROCESSING_ENA == 1

        // Magnitude of the current range-bin
        absVal = (float) temp_real * (float) temp_real + (float) temp_imag * (float) temp_imag;
        // Maximum value range-bin of the current range-profile
        if (absVal > maxVal)
        {
            maxVal = absVal;
            rangeBinMax = rangeBinIndex;
        }

        // Update the range bin to be processed if needed
        if (frameCountLocal == 1)
           rangeBinIndexPhase = rangeBinMax;

        if (rangeBinIndex == rangeBinIndexPhase)
           rangeBinPhase = atan2(temp_imag, temp_real);

#endif


    } // For Loop ends


#if BASIC_PROCESSING_ENA == 1

    // Phase-Unwrapping
    obj->unwrapPhasePeak = unwrap(rangeBinPhase, phasePrevFrame, &diffPhaseCorrectionCum);
    phasePrevFrame = rangeBinPhase;

    // Computes the phase differences between successive phase samples
    if(FLAG_COMPUTE_PHASE_DIFFERENCE == 1)
    {
        phaseUsedComputation = obj->unwrapPhasePeak - phaseUsedComputationPrev;
        phaseUsedComputationPrev = obj->unwrapPhasePeak;
    }
    else
    {
        phaseUsedComputation = obj->unwrapPhasePeak;
    }

    outputFilterBreathOut = phaseUsedComputation;
    outputFilterHeartOut = phaseUsedComputation;

    // Removes impulse like noise from the waveforms
    if (FLAG_REMOVE_IMPULSE_NOISE == 1)
    {
        dataPrev2 = dataPrev1;
        dataPrev1 = dataCurr;
        dataCurr = phaseUsedComputation;
        phaseUsedComputation = filter_RemoveImpulseNoise(dataPrev2, dataPrev1, dataCurr, obj->noiseImpulse_Thresh);
    }

    // IIR Filtering
    outputFilterBreathOut = filter_IIR_BiquadCascade(phaseUsedComputation, obj->pFilterCoefsBreath, obj->pScaleValsBreath, obj->pDelayBreath, IIR_FILTER_BREATH_NUM_STAGES);
    outputFilterHeartOut  = filter_IIR_BiquadCascade(phaseUsedComputation, obj->pFilterCoefsHeart_4Hz, obj->pScaleValsHeart_4Hz, obj->pDelayHeart, IIR_FILTER_HEART_NUM_STAGES);

  ///////////////////////////////////////////////////////////////////////
 /////// CALCOLO DELL'AZIMUTH //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

    /* WORKFLOW*/

    //- trasferimento ping pong CON TRASPOSTA



    //- fft in azimuth SU L2

    // heatmap?

    // ping pong su L3 per concludere





    // Output Values
    obj->VitalSigns_Output.rangeBinIndexMax      = rangeBinMax;
    obj->VitalSigns_Output.rangeBinIndexPhase    = rangeBinIndexPhase;
    obj->VitalSigns_Output.unwrapPhasePeak_mm    = obj->unwrapPhasePeak;
    obj->VitalSigns_Output.outputFilterBreathOut = outputFilterBreathOut;
    obj->VitalSigns_Output.outputFilterHeartOut  = outputFilterHeartOut;
    obj->VitalSigns_Output.maxVal                = maxVal;
    obj->VitalSigns_Output.frameCounter          = gFrameCount; // For debug purposes
//
//  obj->VitalSigns_Output.heatmap... OPPURE nuovoOggetto->out
#endif

}


/*
 *    Chirp processing. In case of CHIRP_EVT it is executed
 *    for every chirp of the event ( 1 chirp -> all RX)
 */
void chirpProcess(MmwDemo_DSS_DataPathObj *obj,uint16_t chirpIndx)
{
     uint32_t channelId;
     uint32_t radarCubeOffset;

     MmwDemo_DSS_dataPathContext_t *context = obj->context;

     // Set src address for ping/poing edma channels
     EDMA_setSourceAddress(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE], MMW_EDMA_CH_1D_IN_PING,
        (uint32_t) &obj->ADCdataBuf[chirpIndx * obj->numAdcSamples]);

     EDMA_setSourceAddress(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE], MMW_EDMA_CH_1D_IN_PONG,
         (uint32_t) &obj->ADCdataBuf[(chirpIndx + obj->numChirpsPerChirpEvent) * obj->numAdcSamples]);

     // If the first 2 chirps has been transferred, wait for ping(or pong) transfer to finish
     if(obj->chirpCount > 1)
         MmwDemo_dataPathWait1DOutputData (obj, pingPongId(obj->chirpCount));

     // Process the chirp (all rx antennas)
     interChirpProcessing(obj, pingPongId(obj->chirpCount));

     // Set and trigger dma transfer to move data to the radar cube
     if(isPong(obj->chirpCount))
         channelId = MMW_EDMA_CH_1D_OUT_PONG;
     else
         channelId = MMW_EDMA_CH_1D_OUT_PING;

     radarCubeOffset = obj->numRangeBinsCalc * obj->numRxAntennas * obj->chirpCount;
     EDMAutil_setAndTrigger(
             context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
             (uint8_t *)NULL,
             (uint8_t *)(&obj->radarCube[radarCubeOffset]),
             (uint8_t)channelId,
             (uint8_t)MMW_EDMA_TRIGGER_ENABLE);

     // Update counters
     obj->chirpCount++;
     obj->txAntennaCount++;
     if(obj->txAntennaCount == obj->numTxAntennas)
     {
         obj->txAntennaCount = 0;
         obj->chirpPerTxCount++;
         if (obj->chirpPerTxCount == obj->numChirpsPerFramePerTx)
         {
             obj->chirpPerTxCount = 0;
             obj->chirpCount = 0;
         }
     }

 }

 /**
  *  @b Description
  *  @n
  *  Wait for transfer of data corresponding to the last 2 chirps (ping/pong)
  *  to the radarCube matrix before starting interframe processing.
  *  @retval
  *      Not Applicable.
  */
void    MmwDemo_waitEndOfChirps(MmwDemo_DSS_DataPathObj *obj)
{

    /* Wait for transfer of data corresponding to last 2 chirps (ping/pong) */
    MmwDemo_dataPathWait1DOutputData (obj, 0);
    MmwDemo_dataPathWait1DOutputData (obj, 1);

}


void MmwDemo_edmaErrorCallbackFxn(EDMA_Handle handle, EDMA_errorInfo_t *errorInfo)
{
    MmwDemo_dssAssert(0);
}


void MmwDemo_edmaTransferControllerErrorCallbackFxn(EDMA_Handle handle,
                EDMA_transferControllerErrorInfo_t *errorInfo)
{
    MmwDemo_dssAssert(0);
}

void MmwDemo_dataPathObjInit(MmwDemo_DSS_DataPathObj *obj,
                             MmwDemo_DSS_dataPathContext_t *context,
                             MmwDemo_CliCfg_t *cliCfg,
                             MmwDemo_CliCommonCfg_t *cliCommonCfg)
{
    memset((void *)obj, 0, sizeof(MmwDemo_DSS_DataPathObj));
    obj->context = context;
    obj->cliCfg = cliCfg;
    obj->cliCommonCfg = cliCommonCfg;
}


void MmwDemo_dataPathInit1Dstate(MmwDemo_DSS_DataPathObj *obj)
{
    obj->chirpCount = 0;
    obj->txAntennaCount = 0;
    obj->chirpPerTxCount = 0;

    memset((void *) &gCycleLog, 0, sizeof(cycleLog_t));

}




void MmwDemo_dataPathInitVitalSignsAndMonitor(MmwDemo_DSS_DataPathObj *obj)
{

    VitalSignsDemo_ParamsCfg  vitalSignsParamCLI;
    VitalSignsDemo_GuiMonSel  monitorSelCLI;

    vitalSignsParamCLI = obj->cliCfg->vitalSignsParamsCfg;
    monitorSelCLI = obj->cliCfg->vitalSignsGuiMonSel;

    obj->rangeBinStartIndex = floor(vitalSignsParamCLI.startRange_m/obj->rangeAccuracy);    // Range-bin index corresponding to the Starting range in meters
    obj->rangeBinEndIndex   = floor(vitalSignsParamCLI.endRange_m/obj->rangeAccuracy);      // Range-bin index corresponding to the Ending range in meters
    if (obj->rangeBinEndIndex >= (obj->numRangeBinsCalc-1))
        obj->rangeBinEndIndex = obj->numRangeBinsCalc-1;
    obj->numRangeBinsProc = obj->rangeBinEndIndex - obj->rangeBinStartIndex + 1;          // Number of range bins that can be processed


    obj->rxAntennaProcess = vitalSignsParamCLI.rxAntennaProcess;
    // Make sure that the RX-channel to process is valid
    obj->rxAntennaProcess = (obj->rxAntennaProcess < 1) ? 1 : obj->rxAntennaProcess;
    obj->rxAntennaProcess = (obj->rxAntennaProcess > obj->numRxAntennas) ? obj->numRxAntennas : obj->rxAntennaProcess;

#if BASIC_PROCESSING_ENA == 1

    obj->noiseImpulse_Thresh   = 1.5;
    
    memset(obj->pDelayHeart, 0, sizeof(obj->pDelayHeart));
    memset(obj->pDelayBreath,0, sizeof(obj->pDelayBreath));

#endif

    obj->systemInfo.rangeBinStartIndex = obj->rangeBinStartIndex;
    obj->systemInfo.rangeBinEndIndex = obj->rangeBinEndIndex;
    obj->systemInfo.rangeBinEndIndex = obj->rangeBinEndIndex;
    obj->systemInfo.rxAntennaProcess = obj->rxAntennaProcess;

    obj->transmitSysInfo = monitorSelCLI.sysInfo;
    obj->transmitVitalSigns = monitorSelCLI.vitalSignsStats;
    obj->transmitAdcData = monitorSelCLI.adcData;
    obj->transmitRangeProfile = monitorSelCLI.rangeProfile;


}

int32_t MmwDemo_dataPathInitEdma(MmwDemo_DSS_dataPathContext_t *context)
{
    SemaphoreP_Params       semParams;
    uint8_t numInstances;
    int32_t errorCode;
    EDMA_Handle handle;
    EDMA_errorConfig_t errorConfig;
    uint32_t instanceId;
    EDMA_instanceInfo_t instanceInfo;

    SemaphoreP_Params_init(&semParams);
    semParams.mode = SemaphoreP_Mode_BINARY;


    numInstances = EDMA_getNumInstances();

    /* Initialize the edma instance to be tested */
    for(instanceId = 0; instanceId < numInstances; instanceId++) {
        EDMA_init(instanceId);

        handle = EDMA_open(instanceId, &errorCode, &instanceInfo);
        if (handle == NULL)
        {
            printf("Error: Unable to open the edma Instance, erorCode = %d\n", errorCode);
            return -1;
        }
        context->edmaHandle[instanceId] = handle;

        errorConfig.isConfigAllEventQueues = true;
        errorConfig.isConfigAllTransferControllers = true;
        errorConfig.isEventQueueThresholdingEnabled = true;
        errorConfig.eventQueueThreshold = EDMA_EVENT_QUEUE_THRESHOLD_MAX;
        errorConfig.isEnableAllTransferControllerErrors = true;
        errorConfig.callbackFxn = MmwDemo_edmaErrorCallbackFxn;
        errorConfig.transferControllerCallbackFxn = MmwDemo_edmaTransferControllerErrorCallbackFxn;
        if ((errorCode = EDMA_configErrorMonitoring(handle, &errorConfig)) != EDMA_NO_ERROR)
        {
            printf("Debug: EDMA_configErrorMonitoring() failed with errorCode = %d\n", errorCode);
            return -1;
        }
    }
    return 0;
}

void MmwDemo_printHeapStats(char *name, uint32_t heapUsed, uint32_t heapSize)
{
    printf("Heap %s : size %d (0x%x), free %d (0x%x)\n", name, heapSize, heapSize,
        heapSize - heapUsed, heapSize - heapUsed);
}


#define SOC_MAX_NUM_RX_ANTENNAS SYS_COMMON_NUM_RX_CHANNEL
#define SOC_MAX_NUM_TX_ANTENNAS SYS_COMMON_NUM_TX_ANTENNAS


void MmwDemo_dataPathConfigBuffers(MmwDemo_DSS_DataPathObj *obj, uint32_t adcBufAddress)
{

//Allinea il valore x al multiplo più vicino di a che è maggiore o uguale a x
#define ALIGN(x,a)  (((x)+((a)-1))&~((a)-1))

#define MMW_ALLOC_BUF(name, nameType, startAddr, alignment, size) \
        obj->name = (nameType *) ALIGN(prev_end, alignment); \
        prev_end = (uint32_t)obj->name + (size) * sizeof(nameType);

    uint32_t heapUsed;
    uint32_t heapL1start = (uint32_t) &gMmwL1[0];
    uint32_t heapL2start = (uint32_t) &gMmwL2[0];
    uint32_t heapL3start = (uint32_t) &gMmwL3[0];

    /* +-> parallelized allocation
     *
     * {adcDataIN}
     * +
     * {dstPingPong+window2D+windowout+twidlle32x32+fftout}
     */
    uint32_t prev_end = heapL1start;
    MMW_ALLOC_BUF(adcDataIn, cmplx16ReIm_t,
                  heapL1start, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  2 * obj->numRangeBinsCalc);
    heapUsed = prev_end  - heapL1start;

    ////////////////////////////// AGGIUNTO ////////////////////////////////
    MMW_ALLOC_BUF (dataAzIn, cmplx16ReIm_t,
                    adcDataIn_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP, // giusto alignment?
                    2 * obj->numAzBinsCalc);
    ////////////////////////////////////////////////////////////////////////


    MmwDemo_dssAssert(heapUsed <= MMW_L1_HEAP_SIZE);
    MmwDemo_printHeapStats("L1", heapUsed, MMW_L1_HEAP_SIZE);
	prev_end = heapL2start;

    MMW_ALLOC_BUF(fftOut1D, cmplx16ReIm_t,
                  heapL2start, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  2 * obj->numRxAntennas * obj->numRangeBinsCalc);

    MMW_ALLOC_BUF(twiddle16x16_1D, cmplx16ReIm_t,
                  MAX(fftOut1D_end, fftOut1D_end),
                  SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  obj->numRangeBinsCalc);

    MMW_ALLOC_BUF(window1D, int16_t,
                  twiddle16x16_1D_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  obj->numAdcSamples / 2);

    ////////////////////////////////// AGGIUNTO //////////////////////

    // AL MOMENTO NON IMPLEMENTATA IN QUANTO CON SOLO 1 TX FARLA SU 4 CAMPIONI potrebbe essere troppo aggressiva
    //MMW_ALLOC_BUF(window2D, int16_t,
     //             window1D_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
      //            obj->numRxAntennas / 2);
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////// AGGIUNTO //////////////////////
    MMW_ALLOC_BUF(fftOut2D, int16_t,
                  window1D_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  2 * obj->numAzBinsCalc);
    ////////////////////////////////////////////////////////////////////



	heapUsed = prev_end - heapL2start;

	MmwDemo_dssAssert(heapUsed <= MMW_L2_HEAP_SIZE);
    MmwDemo_printHeapStats("L2", heapUsed, MMW_L2_HEAP_SIZE);

    prev_end = heapL3start;
    
    MMW_ALLOC_BUF(ADCdataBuf, cmplx16ReIm_t,
                  heapL3start, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  obj->numAdcSamples * obj->numRxAntennas * obj->numTxAntennas);

    if (adcBufAddress != NULL)
    {
        obj->ADCdataBuf = (cmplx16ReIm_t *)adcBufAddress;
    }

    MMW_ALLOC_BUF(radarCube, cmplx16ReIm_t,
                  ADCdataBuf_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  obj->numRangeBinsCalc * obj->numChirpsPerFrame * obj->numRxAntennas *
                  obj->numTxAntennas);

    // rangeProfile
    MMW_ALLOC_BUF(pRangeProfileCplx, cmplx16ReIm_t,
                  radarCube_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  obj->numRangeBinsCalc);

    // adc data
    MMW_ALLOC_BUF(adcDataCube, cmplx16ReIm_t,
                  pRangeProfileCplx_end, sizeof(cmplx16ReIm_t),
                  obj->numAdcSamples * obj->numRxAntennas * obj->numChirpsProc);

    ////////////////////////////////// AGGIUNTO //////////////////////////////////
    MMW_ALLOC_BUF(rangeAzMap, cmplx16ReIm_t,
                  adcDataCube_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  obj->numRangeBinsCalc * obj->numChirpsPerFrame * obj->numRxAntennas *
                  obj->numTxAntennas);
    ////////////////////////////////// //////////////////////////////////

#if BASIC_PROCESSING_ENA == 1

    MMW_ALLOC_BUF(pFilterCoefsBreath, float,
                  adcDataCube_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  IIR_FILTER_BREATH_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER);

    MMW_ALLOC_BUF(pScaleValsBreath, float,
                  pFilterCoefsBreath_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  IIR_FILTER_BREATH_NUM_STAGES + 1);


    MMW_ALLOC_BUF(pFilterCoefsHeart_4Hz, float,
                  pScaleValsBreath_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  IIR_FILTER_HEART_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER);

    MMW_ALLOC_BUF(pScaleValsHeart_4Hz, float,
                  pFilterCoefsHeart_4Hz_end, SYS_MEMORY_ALLOC_DOUBLE_WORD_ALIGN_DSP,
                  IIR_FILTER_HEART_NUM_STAGES + 1);

    //////////////  Parameters :  IIR-Cascade Bandpass Filter  //////////////////////////////////////

     // 0.1 Hz to 0.5 Hz Bandpass Filter coefficients
     float pFilterCoefsBreath[IIR_FILTER_BREATH_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER] = {
                                            1.0000, 0, -1.0000, 1.0000, -1.9632, 0.9644,
                                            1.0000, 0, -1.0000, 1.0000, -1.8501, 0.8681 };
     float pScaleValsBreath[IIR_FILTER_BREATH_NUM_STAGES + 1] = {0.0602, 0.0602, 1.0000};
     memcpy(obj->pFilterCoefsBreath, pFilterCoefsBreath, sizeof(pFilterCoefsBreath));
     memcpy(obj->pScaleValsBreath, pScaleValsBreath, sizeof(pScaleValsBreath));

   // Heart Beat Rate    0.8 - 4.0 Hz
    float pFilterCoefsHeart_4Hz[IIR_FILTER_HEART_NUM_STAGES * IIR_FILTER_COEFS_SECOND_ORDER] = {
                                               1.0000, 0, -1.0000, 1.0000, -0.5306, 0.5888,
                                               1.0000, 0, -1.0000, 1.0000, -1.8069, 0.8689,
                                               1.0000, 0, -1.0000, 1.0000, -1.4991, 0.5887,
                                               1.0000, 0, -1.0000, 1.0000, -0.6654, 0.2099 };
    float pScaleValsHeart_4Hz[IIR_FILTER_HEART_NUM_STAGES + 1] = {0.4188, 0.4188, 0.3611, 0.3611, 1.0000};
    memcpy(obj->pFilterCoefsHeart_4Hz, pFilterCoefsHeart_4Hz, sizeof(pFilterCoefsHeart_4Hz));
    memcpy(obj->pScaleValsHeart_4Hz, pScaleValsHeart_4Hz, sizeof(pScaleValsHeart_4Hz));

#endif


    heapUsed = prev_end - heapL3start;
    MmwDemo_dssAssert(heapUsed <= sizeof(gMmwL3));
    MmwDemo_printHeapStats("L3", heapUsed, sizeof(gMmwL3));


}


void MmwDemo_dataPathConfigFFTs(MmwDemo_DSS_DataPathObj *obj)
{

    // uint8_t numAzBinsCalc = 8;
    genWindow((void *)obj->window1D,
                        FFT_WINDOW_INT16,
                        obj->numAdcSamples,
                        obj->numAdcSamples/2,
                        ONE_Q15,
                        MMW_WIN_BLACKMAN);


    // GENERAZIONE WINDOW PER AZIMUTH: non serve forse (pochi campioni)?



    /* Generate twiddle factors for 1D FFT. This is one time */
    MmwDemo_gen_twiddle_fft16x16_fast((int16_t *)obj->twiddle16x16_1D, obj->numRangeBinsCalc);

    // GENERAZIONE TWIDDLE FACTOR PER AZIMUTH
    MmwDemo_gen_twiddle_fft16x16_fast((int16_t *)obj->twiddle16x16_1D, obj->numAzBinsCalc);
}


void genWindow(void *win,
                        uint32_t windowDatumType,
                        uint32_t winLen,
                        uint32_t winGenLen,
                        int32_t oneQformat,
                        uint32_t winType)
{
    uint32_t winIndx;
    int32_t winVal;
    int16_t * win16 = (int16_t *) win;
    int32_t * win32 = (int32_t *) win;
    float   * winfl = (float *) win;

    float eR = 1.;
    float eI = 0.;
    float e2R = 1.;
    float e2I = 0.;
    float ephyR, ephyI;
    float e2phyR, e2phyI;
    float tmpR;

    float phi = 2 * PI_ / ((float) winLen - 1);


    ephyR  = cossp(phi);
    ephyI  = sinsp(phi);

    e2phyR  = ephyR * ephyR - ephyI * ephyI;
    e2phyI  = 2 * ephyR * ephyI;


    if(winType == MMW_WIN_BLACKMAN)
    {
        //Blackman window
        float a0 = 0.42;
        float a1 = 0.5;
        float a2 = 0.08;
        for(winIndx = 0; winIndx < winGenLen; winIndx++)
        {
            if (windowDatumType == FFT_WINDOW_SP){

                winfl[winIndx] =  a0 - a1*eR + a2*e2R;

            }else{

            winVal = (int32_t) ((oneQformat * (a0 - a1*eR + a2*e2R)) + 0.5);
            if(winVal >= oneQformat)
            {
                winVal = oneQformat - 1;
            }
            if (windowDatumType == FFT_WINDOW_INT16)
            {
                win16[winIndx] = (int16_t) winVal;
             }
            if (windowDatumType == FFT_WINDOW_INT32)
            {
                win32[winIndx] = (int32_t) winVal;
             }

            }
            tmpR = eR;
            eR = eR * ephyR - eI * ephyI;
            eI = tmpR * ephyI + eI * ephyR;

            tmpR = e2R;
            e2R = e2R * e2phyR - e2I * e2phyI;
            e2I = tmpR * e2phyI + e2I * e2phyR;
        }
    }
    else if (winType == MMW_WIN_HANNING)
    {
        //Hanning window
        for(winIndx = 0; winIndx < winGenLen; winIndx++)
        {
            if (windowDatumType == FFT_WINDOW_SP){

                winfl[winIndx] = (float)0.5* (1 - eR);

            }else{

            winVal = (int32_t) ((oneQformat * 0.5* (1 - eR)) + 0.5);
            if(winVal >= oneQformat)
            {
                winVal = oneQformat - 1;
            }
            if (windowDatumType == FFT_WINDOW_INT16)
            {
                win16[winIndx] = (int16_t) winVal;
            }
            if (windowDatumType == FFT_WINDOW_INT32)
            {
                win32[winIndx] = (int32_t) winVal;
            }
            }

            tmpR = eR;
            eR = eR*ephyR - eI*ephyI;
            eI = tmpR*ephyI + eI*ephyR;
        }
    }
    else if (winType == MMW_WIN_HAMMING)
    {for (winIndx = 0; winIndx < winGenLen; winIndx++)
         {
             winVal = (int32_t)((oneQformat *  (0.53836 - (0.46164*cos(phi * winIndx)))) + 0.5);

             if (winVal >= oneQformat)
             {
                 winVal = oneQformat - 1;
             }

             switch (windowDatumType)
             {
             case FFT_WINDOW_INT16:
                 win16[winIndx] = (int16_t)winVal;
                 break;
             case FFT_WINDOW_INT32:
                 win32[winIndx] = (int32_t)winVal;
                 break;
             }
         }
     }
    else if (winType == MMW_WIN_RECT)
    {
        //Rectangular window
        for(winIndx = 0; winIndx < winGenLen; winIndx++)
        {
            if (windowDatumType == FFT_WINDOW_SP){
                winfl[winIndx] =  1;
            }

            if (windowDatumType == FFT_WINDOW_INT16)
            {
                win16[winIndx] =  (int16_t)  (oneQformat-1);
            }
            if (windowDatumType == FFT_WINDOW_INT32)
            {
                win32[winIndx] = (int32_t) (oneQformat-1);
            }
        }
    }
}


int32_t MmwDemo_gen_twiddle_fft16x16_fast(short *w, int32_t n)
{
     int32_t i, j, k;
     int32_t log2n = 30 - _norm(n); //Note n is always power of 2
     int32_t step = 1024 >> log2n;
     int32_t step6 = 3 * step;
     int32_t step4 = 2 * step;
     int32_t step2 = 1 * step;
     int32_t ind, indLsb, indMsb;
     int64_t * restrict table = (int64_t *) twiddleTableCommon;
     uint32_t * restrict wd = (uint32_t *) w;
     int32_t xRe;
     int32_t xIm;

     for (j = 1, k = 0; j < n >> 2; j = j << 2) {
         for (i = 0; i < n >> 2; i += j << 1) {
            ind = step2 * (i + j);
            indLsb = ind & 0xFF;
            indMsb = (ind >> 8) & 0x3;
            xRe =  ((int32_t)_sadd(_hill(table[indLsb]), 0x00008000)) >> 16;
            xIm =  ((int32_t)_sadd(_loll(table[indLsb]), 0x00008000)) >> 16;
            if (indMsb == 0)
            {
             wd[k + 1] =  _pack2(xRe, xIm);
            }
            if (indMsb == 1)
            {
             wd[k + 1] =  _pack2(-xIm, xRe);
            }
            if (indMsb == 2)
            {
             wd[k + 1] =  _pack2(-xRe, -xIm);
            }

            ind = step2 * (i);
            indLsb = ind & 0xFF;
            indMsb = (ind >> 8) & 0x3;
            xRe =  ((int32_t)_sadd(_hill(table[indLsb]), 0x00008000)) >> 16;
            xIm =  ((int32_t)_sadd(_loll(table[indLsb]), 0x00008000)) >> 16;
            if (indMsb == 0)
            {
             wd[k + 0] =  _pack2(xRe, xIm);
            }
            if (indMsb == 1)
            {
             wd[k + 0] =  _pack2(-xIm, xRe);
            }
            if (indMsb == 2)
            {
             wd[k + 0] =  _pack2(-xRe, -xIm);
            }

            ind = step4 * (i + j);
            indLsb = ind & 0xFF;
            indMsb = (ind >> 8) & 0x3;
            xRe =  ((int32_t)_sadd(_hill(table[indLsb]), 0x00008000)) >> 16;
            xIm =  ((int32_t)_sadd(_loll(table[indLsb]), 0x00008000)) >> 16;
            if (indMsb == 0)
            {
              wd[k + 3] =  _pack2(-xRe, -xIm);
            }
            if (indMsb == 1)
            {
              wd[k + 3] =  _pack2(xIm, -xRe);
            }
            if (indMsb == 2)
            {
              wd[k + 3] =  _pack2(xRe, xIm);
            }

            ind = step4 * (i);
            indLsb = ind & 0xFF;
            indMsb = (ind >> 8) & 0x3;
            xRe =  ((int32_t)_sadd(_hill(table[indLsb]), 0x00008000)) >> 16;
            xIm =  ((int32_t)_sadd(_loll(table[indLsb]), 0x00008000)) >> 16;
            if (indMsb == 0)
            {
              wd[k + 2] =  _pack2(-xRe, -xIm);
            }
            if (indMsb == 1)
            {
              wd[k + 2] =  _pack2(xIm, -xRe);
            }
            if (indMsb == 2)
            {
              wd[k + 2] =  _pack2(xRe, xIm);
            }

            ind = step6 * (i + j);
            indLsb = ind & 0xFF;
            indMsb = (ind >> 8) & 0x3;
            xRe =  ((int32_t)_sadd(_hill(table[indLsb]), 0x00008000)) >> 16;
            xIm =  ((int32_t)_sadd(_loll(table[indLsb]), 0x00008000)) >> 16;
            if (indMsb == 0)
            {
               wd[k + 5] =  _pack2(xRe, xIm);
            }
            if (indMsb == 1)
            {
               wd[k + 5] =  _pack2(-xIm, xRe);
            }
            if (indMsb == 2)
            {
               wd[k + 5] =  _pack2(-xRe, -xIm);
            }

            ind = step6 * (i);
            indLsb = ind & 0xFF;
            indMsb = (ind >> 8) & 0x3;
            xRe =  ((int32_t)_sadd(_hill(table[indLsb]), 0x00008000)) >> 16;
            xIm =  ((int32_t)_sadd(_loll(table[indLsb]), 0x00008000)) >> 16;
            if (indMsb == 0)
            {
               wd[k + 4] =  _pack2(xRe, xIm);
            }
            if (indMsb == 1)
            {
               wd[k + 4] =  _pack2(-xIm, xRe);
            }
            if (indMsb == 2)
            {
               wd[k + 4] =  _pack2(-xRe, -xIm);
            }

            k += 6;
         }
     }
     return 2*k;
}


//////////////// AGGIUNTO /////////////////////////
/*
 * Azimuth process: processa una fft range passando
 * le righe tramite ping pong
 *
 *  azimuthIndx: index della fft range per capire se ping o pong
 */

void AzimuthProcess(MmwDemo_DSS_DataPathObj *obj, uint16_t azimuthIndx)
{
     uint32_t channelId;
     uint32_t rangeAzMapOffset;

     MmwDemo_DSS_dataPathContext_t *context = obj->context;

     // Set src address for ping/pong edma channels
     EDMA_setSourceAddress(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE], MMW_EDMA_CH_2D_IN_PING,
        (uint32_t) &obj->radarCube[azimuthIndx * obj->numRxAntennas * obj->numRangeBinsCalc]); //* obj->numAdcSamples]);

     EDMA_setSourceAddress(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE], MMW_EDMA_CH_2D_IN_PONG,
         (uint32_t) &obj->radarCube[(azimuthIndx*  obj->numRxAntennas * obj->numRangeBinsCalc) + 1]);

     // If the first 2 chirps has been transferred, wait for ping(or pong) transfer to finish
     if(obj->chirpCount > 1)
         MmwDemo_dataPathWait2DOutputData (obj, pingPongId(obj->chirpCount));

     // Process the chirp (all rx antennas)
     interAzimuthProcessing(obj, pingPongId(obj->chirpCount));

     // Set and trigger dma transfer to move data to the radar cube
     if(isPong(obj->chirpCount))
         channelId = MMW_EDMA_CH_1D_OUT_PONG;
     else
         channelId = MMW_EDMA_CH_1D_OUT_PING;

     rangeAzMapOffset = obj->numRangeBinsCalc * obj->numRxAntennas * obj->chirpCount;
     EDMAutil_setAndTrigger(
             context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
             (uint8_t *)NULL,
             (uint8_t *)(&obj->rangeAzMap[rangeAzMapOffset]),
             (uint8_t)channelId,
             (uint8_t)MMW_EDMA_TRIGGER_ENABLE);

     // Update counters
     obj->chirpCount++;
     obj->txAntennaCount++;
     if(obj->txAntennaCount == obj->numTxAntennas)
     {
         obj->txAntennaCount = 0;
         obj->chirpPerTxCount++;
         if (obj->chirpPerTxCount == obj->numChirpsPerFramePerTx)
         {
             obj->chirpPerTxCount = 0;
             obj->chirpCount = 0;
         }
     }

 }


/*
 *  Interchirp processing. It is executed per chirp event, after ADC
 *  buffer is filled with chirp samples.
 *  This function executes: windowing and fft
 */

void interAzimuthProcessing(MmwDemo_DSS_DataPathObj *obj, uint8_t chirpPingPongId)
{
    uint32_t antIndx, waitingTime;
    volatile uint32_t startTime;
    volatile uint32_t startTime1;
    MmwDemo_DSS_dataPathContext_t *context = obj->context;

    uint32_t adcDataOffset;

    waitingTime = 0;
    startTime = Cycleprofiler_getTimeStamp();

    // Start DMA transfer to fetch data from RX1
    EDMA_startDmaTransfer(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                       MMW_EDMA_CH_1D_IN_PING);

    // 1d fft for first antenna, followed by kicking off the DMA of fft output
    for (antIndx = 0; antIndx < obj->numRxAntennas; antIndx++)
    {
        adcDataOffset = pingPongId(antIndx)*obj->numRangeBinsCalc;

        // Start DMA transfer to move data from the next antenna (while processing the actual)
        // When processing data from RX1, move data from RX2. When processing data from RX4,
        // no further DMA transfer is needed
        if (antIndx < (obj->numRxAntennas - 1))
        {
            if (isPong(antIndx))
            {
                EDMA_startDmaTransfer(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                        MMW_EDMA_CH_1D_IN_PING);
            }
            else
            {
                EDMA_startDmaTransfer(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                        MMW_EDMA_CH_1D_IN_PONG);
            }
        }

        // verify if DMA has completed for current antenna
        startTime1 = Cycleprofiler_getTimeStamp();
        MmwDemo_dataPathWait1DInputData (obj, pingPongId(antIndx));
        waitingTime += Cycleprofiler_getTimeStamp() - startTime1;


        // copy adc data into the adcDataCube
        memcpy(&obj->adcDataCube[antIndx*obj->numAdcSamples],   // DESTINATION
               &obj->adcDataIn[adcDataOffset],                  // SOURCE
               obj->numAdcSamples*sizeof(cmplx16ReIm_t) );      // LENGTH


        /* Range FFT */
        // windowing
        mmwavelib_windowing16x16_evenlen(
                (int16_t *) &obj->adcDataIn[adcDataOffset],
                (int16_t *) obj->window1D,
                obj->numAdcSamples);

        // reset zero-padded data (fft function overwrites input)
        memset((void *)&obj->adcDataIn[adcDataOffset + obj->numAdcSamples],
            0 , (obj->numRangeBinsCalc - obj->numAdcSamples) * sizeof(cmplx16ReIm_t));

        // fft
        DSP_fft16x16(
                (int16_t *) obj->twiddle16x16_1D,
                obj->numRangeBinsCalc,
                (int16_t *) &obj->adcDataIn[adcDataOffset],
                (int16_t *) &obj->fftOut1D[chirpPingPongId * (obj->numRxAntennas * obj->numRangeBinsCalc) +
                                               (obj->numRangeBinsCalc * antIndx)]);
    }

    gCycleLog.interChirpProcessingTime += Cycleprofiler_getTimeStamp() - startTime - waitingTime;
    gCycleLog.interChirpWaitTime += waitingTime;

}


void MmwDemo_dataPathWait2DOutputData(MmwDemo_DSS_DataPathObj *obj, uint32_t pingPongId)
{
    MmwDemo_DSS_dataPathContext_t *context = obj->context;

    /* wait until transfer done */
    volatile bool isTransferDone;
    uint8_t chId;
    if(pingPongId == 0)
    {
        chId = MMW_EDMA_CH_2D_OUT_PING;
    }
    else
    {
        chId = MMW_EDMA_CH_2D_OUT_PONG;
    }
    do {
        if (EDMA_isTransferComplete(context->edmaHandle[MMW_DATA_PATH_EDMA_INSTANCE],
                                    chId,
                                    (bool *)&isTransferDone) != EDMA_NO_ERROR)
        {
            MmwDemo_dssAssert(0);
        }
    } while (isTransferDone == false);

}

