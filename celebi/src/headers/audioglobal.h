#ifndef AUDIOGLOBAL_H_
#define AUDIOGLOBAL_H_

#include <stdint.h>
#include <libdnn/mem.h>
#include <libmspdriver/dma.h>
#include <libmspdriver/timer_a.h>
#include <libmspdriver/adc12_b.h>
#include <libmspdriver/ref_a.h>
#include <libio/console.h>

#define AUDIO_SAMPLING_FREQUENCY              8000
#define __SYSTEM_FREQUENCY_MHZ__ 8000000

// Define a fixed sample length
#define AUDIO_SAMPLES_LENGTH  2520  // 2520/8000=0.315s
#define VCC_SAMPLES_LENGTH  20
#define TEMP_SAMPLES_LENGTH  20
#define ENG_SAMPLES_LENGTH  20

// Define audio ports
#define MIC_POWER_PORT_DIR  P6DIR
#define MIC_POWER_PORT_OUT  P6OUT
#define MIC_POWER_PIN       BIT2

#define AUDIO_PORT_SEL0     P1SEL0
#define AUDIO_PORT_SEL1     P1SEL1

#define ENG_INPUT_PIN       BIT5
#define ENG_INPUT_CHAN      ADC12INCH_5

#define MIC_INPUT_PIN       BIT3
#define MIC_INPUT_CHAN      ADC12INCH_3

#define VCC_INPUT_PIN       BIT4
#define VCC_INPUT_CHAN      ADC12INCH_4         




/* The Audio object structure, containing the Audio instance information */
typedef struct Audio_configParams
{
    /* Size of both audio buffers */
    uint16_t bufferSize;

    /* audio sample rate in Hz */
    uint16_t sampleRate;

    /* sample type */
    uint8_t type;
    // 1: audio
    // 2: Generated Energy
    // 3: temp
    // 4: Cap Voltage VCC

} Audio_configParams;

/*----------------------------------------------------------------------------
 * Functions
 * --------------------------------------------------------------------------*/
void run_sampling_audio(void);
void runSampling_ENG(void);
void runSampling_temp(void);
void runSampling_VCC(void);
void Audio_setupCollect(Audio_configParams * audioConfig);

/* Start collecting audio samples in ping-pong buffers */
void Audio_startCollect(void);

/* Shut down the audio collection peripherals */
void Audio_shutdownCollect(void);

/*
 * external variables from audio.c
 */
extern uint32_t dataRecorded[];
extern int engRecorded[];
extern int tempRecorded[];
extern int vccRecorded[];

#endif