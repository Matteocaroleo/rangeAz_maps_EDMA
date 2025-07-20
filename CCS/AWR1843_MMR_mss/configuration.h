/*
 * configuration.h
 *
 *  Created on: 23 ago 2019
 *      Author: X86
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_


/* Options for DATA_OUTPUT_MODE */
#define FRAME_BASED_CHIRP   1
#define CONTINUOUS_CHIRP    2 // not supported
#define ADVANCED            3

/* Options for number of bit of the ADC */
#define ADC_12BITS   0 // not supported
#define ADC_14BITS   1 // not supported
#define ADC_16BITS   2
/* Options for output format of adc signals */
#define ADC_OUT_FORMAT_REAL     0
#define ADC_OUT_FORMAT_CMPLX1x  1
#define ADC_OUT_FORMAT_CMPLX2x  2
/* Options for ADC_buffer out format */
#define ADCBUF_OUT_CMPLX 0
#define ADCBUF_OUT_REAL  1  // not supported
/* Options for ADC_buffer sample swap */
#define ADCBUF_SWAP_ILSBQMSB    0
#define ADCBUF_SWAP_QLSBIMSB    1
/* Options for ADC buffer interleaved data */
#define ADCBUF_CH_INTERLEAVED    0  // not supported
#define ADCBUF_CH_NOTINTERLEAVED 1
/* Options for high pass filter 1 corner frequency */
#define HPF1_CORNER_FREQ_175KHZ 0
#define HPF1_CORNER_FREQ_235KHZ 1
#define HPF1_CORNER_FREQ_350KHZ 2
#define HPF1_CORNER_FREQ_700KHZ 3
/* Options for high pass filter 2 corner frequency */
#define HPF2_CORNER_FREQ_350KHZ  0
#define HPF2_CORNER_FREQ_700KHZ  1
#define HPF2_CORNER_FREQ_1400KHZ 2
#define HPF2_CORNER_FREQ_2800KHZ 3
/* Options for frame trigger origin */
#define TRIGGER_SELECT_SW 1
#define TRIGGER_SELECT_HW 2 // not supported
/* Options for adc power mode */
#define ADC_MODE_REGULAR   0
#define ADC_MODE_LOWPOWER  1


// dfeDataOutputMode
#define DFE_DATA_OUTPUT_MODE    FRAME_BASED_CHIRP

// channel configuration
// #define RX_ANTENNA_MASK         0X3
#define RX_ANTENNA_MASK         0XF // MODIFICA
#define TX_ANTENNA_MASK         1


// adcConfiguration
#define NUM_ADC_BITS            ADC_16BITS
#define ADC_OUT_FORMAT          ADC_OUT_FORMAT_CMPLX1x

#define ADCBUF_OUT_FORMAT       ADCBUF_OUT_CMPLX            // complex data (I,Q)
#define ADCBUF_SAMPLE_SWAP      ADCBUF_SWAP_ILSBQMSB        // I in LSB, Q in MSB
#define ADCBUF_CH_INTERLEAVE    ADCBUF_CH_NOTINTERLEAVED    // not interleaved
#define ADCBUF_CHIRP_THRESH     0                           // 0-8 with DSP FFT, 1 with HWA


// Profile configuration
#define PROFILE_ID              0
#define START_FREQ_GHZ          77.0f
#define IDLE_TIME_US            7.0f
#define ADC_START_TIME_US       6.0f
#define RAMP_END_TIME_US        57.0f
#define TX_OUT_POWER_BKOFF      0
#define TX_PHASE_SHIFTER        0
#define FREQ_SLOPE_MHZ_US       70.0f  // frequency slope, MHz/us
#define TX_START_TIME_US        1.0f
#define NUM_ADC_SAMPLES         200
#define ADC_SAMP_FREQ           4000
#define HPF1_CORNER_FREQ        HPF1_CORNER_FREQ_175KHZ // 0
#define HPF2_CORNER_FREQ        HPF2_CORNER_FREQ_350KHZ // 0
#define RX_GAIN                 40


// Chirp configuration
#define CHIRP_START_IDX         0 // [0-511]
#define CHIRP_END_IDX           0 // [0-511]
#define CHIRP_PROFILE_ID        0
#define START_FREQ_VAR          0.0f
#define FREQ_SLOPE_VAR          0.0f
#define IDLE_TIME_VAR_US        0.0f
#define ADC_START_TIME_VAR_US   0.0f
#define TX_ENABLE               1

// frame configuration
#define FRAME_CHIRP_START_IDX   0
#define FRAME_CHIRP_END_IDX     0
#define FRAME_NUM_LOOPS         2
#define NUMBER_OF_FRAMES        0 // [0 - 65535]: 0 = infinite
#define FRAME_RATE_MS           50
#define FRAME_TRIGGER_SELECT    TRIGGER_SELECT_SW
#define FRAME_TRIGGER_DELAY_MS  0.0f

#define ADC_POWER_MODE          ADC_MODE_LOWPOWER

#define NEAR_FIELD_CORRECTION_ENA        0
#define NEAR_FIELD_CORRECTION_MIN_RANGE  0
#define NEAR_FIELD_CORRECTION_MAX_RANGE  65535




#endif /* CONFIGURATION_H_ */
