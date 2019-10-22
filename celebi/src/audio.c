#include "headers/audioglobal.h"

__hifram uint32_t dataRecorded[AUDIO_SAMPLES_LENGTH] = {0};
__hifram int vccRecorded[VCC_SAMPLES_LENGTH] = {0};
__hifram int tempRecorded[TEMP_SAMPLES_LENGTH] = {0};
__hifram int engRecorded[ENG_SAMPLES_LENGTH] = {0};

// Globals
DMA_initParam dma0Config;
Audio_configParams gAudioConfig;

void run_sampling_audio()
{
    uint8_t running = 0;
    // Initialize the microphone for recording
    gAudioConfig.bufferSize = AUDIO_SAMPLES_LENGTH;
    gAudioConfig.sampleRate = AUDIO_SAMPLING_FREQUENCY;
    gAudioConfig.type = 1;
    Audio_setupCollect(&gAudioConfig);

    // Start the recording by enabling the timer
    Audio_startCollect();
    running = 1;

    if(running)
    {
        __bis_SR_register(LPM3_bits + GIE);
        running= 0;
    }
}

void runSampling_ENG(void)
{
    gAudioConfig.bufferSize = ENG_SAMPLES_LENGTH;
    gAudioConfig.sampleRate = AUDIO_SAMPLING_FREQUENCY;
    gAudioConfig.type = 2;
    Audio_setupCollect(&gAudioConfig);

    // Start the recording by enabling the timer
    Audio_startCollect();
    // __bis_SR_register(LPM3_bits + GIE);
}

void runSampling_temp(void)
{
    // Initialize the microphone for recording
    gAudioConfig.bufferSize = TEMP_SAMPLES_LENGTH;
    gAudioConfig.sampleRate = AUDIO_SAMPLING_FREQUENCY;
    gAudioConfig.type = 3;
    Audio_setupCollect(&gAudioConfig);

    // Start the recording by enabling the timer
    Audio_startCollect();
    // __bis_SR_register(LPM3_bits + GIE);
}

void runSampling_VCC(void)
{
    gAudioConfig.bufferSize = VCC_SAMPLES_LENGTH;
    gAudioConfig.sampleRate = AUDIO_SAMPLING_FREQUENCY;
    gAudioConfig.type = 4;
    Audio_setupCollect(&gAudioConfig);

    // Start the recording by enabling the timer
    Audio_startCollect();
    // __bis_SR_register(LPM3_bits + GIE);
}

//******************************************************************************
// Functions
//******************************************************************************
/* Function that powers up the external microphone and starts sampling
 * the microphone output.
 * The ADC is triggered to sample using the Timer module
 * Then the data is moved via DMA. The device would only wake-up once
 * the DMA is done. */
void Audio_setupCollect(Audio_configParams * audioConfig)
{
    
    Timer_A_initUpModeParam upConfig = {0};
        upConfig.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
        upConfig.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
        upConfig.timerPeriod = (__SYSTEM_FREQUENCY_MHZ__ / audioConfig->sampleRate) - 1;
        upConfig.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
        upConfig.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
        upConfig.timerClear = TIMER_A_DO_CLEAR;
        upConfig.startTimer = false;


    Timer_A_initCompareModeParam compareConfig = {0};
        compareConfig.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
        compareConfig.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
        compareConfig.compareOutputMode = TIMER_A_OUTPUTMODE_TOGGLE_RESET;
        compareConfig.compareValue = ((__SYSTEM_FREQUENCY_MHZ__ / audioConfig->sampleRate) / 2) - 1;


    // Initialize Timer_A channel 1 to be used as ADC12 trigger
    // Initialize TACCR0 (period register) __SYSTEM_FREQUENCY_MHZ__/sampleRate = NUM
    // Simple counter with no interrupt. 0...NUM = NUM counts/sample
    Timer_A_initUpMode(TIMER_A0_BASE, &upConfig);

    // Initialize TA0CCR1 to generate trigger clock output, reset/set mode
    Timer_A_initCompareMode(TIMER_A0_BASE, &compareConfig);


    // Turn on mic power full drive strength and enable mic input pin to ADC
    if(audioConfig->type==1)
    {
        MIC_POWER_PORT_OUT |= MIC_POWER_PIN;
        MIC_POWER_PORT_DIR |= MIC_POWER_PIN;

        AUDIO_PORT_SEL0 |= MIC_INPUT_PIN;   // Enable A/D channel inputs
        AUDIO_PORT_SEL1 |= MIC_INPUT_PIN;
    }
    else if(audioConfig->type==2)
    {
        AUDIO_PORT_SEL0 |= ENG_INPUT_PIN;   // Enable A/D channel inputs
        AUDIO_PORT_SEL1 |= ENG_INPUT_PIN;
    }
    // else if(audioConfig->type==4)
    // {
    //     AUDIO_PORT_SEL0 |= VCC_INPUT_PIN;   // Enable A/D channel inputs
    //     AUDIO_PORT_SEL1 |= VCC_INPUT_PIN;
    // }

    // For safety, protect RMW Cpu instructions
    DMA_disableTransferDuringReadModifyWrite();

    // Initialize the DMA. Using DMA channel 1.
    dma0Config.channelSelect = DMA_CHANNEL_1;
    dma0Config.transferModeSelect = DMA_TRANSFER_SINGLE;

    dma0Config.transferSize = audioConfig->bufferSize;   // how many times of sampling

    dma0Config.triggerSourceSelect = DMA_TRIGGERSOURCE_26;
    dma0Config.transferUnitSelect = DMA_SIZE_SRCWORD_DSTWORD;
    dma0Config.triggerTypeSelect = DMA_TRIGGER_RISINGEDGE;


    DMA_init(&dma0Config);

    DMA_setSrcAddress(DMA_CHANNEL_1,
                      (uint32_t) &ADC12MEM0,
                      DMA_DIRECTION_UNCHANGED);

    if(audioConfig->type==1)
    {
        DMA_setDstAddress(DMA_CHANNEL_1,
                      (uint32_t) (&dataRecorded),
                      DMA_DIRECTION_INCREMENT);
    }
    else if(audioConfig->type==2)
    {
        DMA_setDstAddress(DMA_CHANNEL_1,
                      (uint32_t) (&engRecorded),
                      DMA_DIRECTION_INCREMENT);
    }
    else if(audioConfig->type==3)
    {
        DMA_setDstAddress(DMA_CHANNEL_1,
                      (uint32_t) (&tempRecorded),
                      DMA_DIRECTION_INCREMENT);
    }
    else if(audioConfig->type==4)
    {
        DMA_setDstAddress(DMA_CHANNEL_1,
                      (uint32_t) (&vccRecorded),
                      DMA_DIRECTION_INCREMENT);
    }

    // Configure ADC
    ADC12CTL0 &= ~ADC12ENC;           // Disable conversions to configure ADC12
    
    
    // Turn on ADC, sample 32 clock cycles =~ 2us
    ADC12CTL0 = ADC12ON + ADC12SHT0_3;

    // Use sample timer, rpt single chan 0, use MODOSC, TA0 timer channel 1
    ADC12CTL1 = ADC12SHP + ADC12CONSEQ_2 + ADC12SHS_1;

    // set input to ADC, (AVCC/AVSS ref), sequence end bit set
    if(audioConfig->type == 1){
        // if type == 1, sample from audio
        ADC12MCTL0 = MIC_INPUT_CHAN | ADC12VRSEL_0 | ADC12EOS; // Channel 31 for Vcc, last conversion in sequence  | ADC12VRSEL_1
    }
    else if(audioConfig->type == 2){
        // if type == 2, sample from energy generator
        ADC12MCTL0 = ENG_INPUT_CHAN | ADC12VRSEL_0 | ADC12EOS;
        
        // Configure internal reference
        while(Ref_A_isRefGenBusy(REF_A_BASE));              // If ref generator busy, WAIT
        Ref_A_setReferenceVoltage(REF_A_BASE, REF_A_VREF2_0V);
        Ref_A_enableReferenceVoltage(REF_A_BASE);
    }
    else if(audioConfig->type == 3){
        // if type == 3, sample from temp
        ADC12MCTL0 = ADC12_B_INPUT_TCMAP | ADC12VRSEL_1 | ADC12EOS;

        // Configure internal reference
        while(Ref_A_isRefGenBusy(REF_A_BASE));              // If ref generator busy, WAIT
        Ref_A_setReferenceVoltage(REF_A_BASE, REF_A_VREF2_0V);
        Ref_A_enableReferenceVoltage(REF_A_BASE);
    }
    else if(audioConfig->type == 4){
        // if type == 4, sample from vcc
        ADC12MCTL0 = ADC12_B_INPUT_BATMAP | ADC12VRSEL_1 | ADC12EOS;

        // Configure internal reference
        while(Ref_A_isRefGenBusy(REF_A_BASE));              // If ref generator busy, WAIT
        Ref_A_setReferenceVoltage(REF_A_BASE, REF_A_VREF2_0V);
        Ref_A_enableReferenceVoltage(REF_A_BASE);
    }

    // PRINTF("\r\n before audio enable : %u\r\n", ADC12CTL0);
    // Enable ADC to convert when a TA0 edge is generated
    ADC12CTL0 |= ADC12ENC;
    // PRINTF("\r\n after audio : %u\r\n", ADC12CTL0);

    if(audioConfig->type == 1){
        // if type == 1, sample from audio
        // Delay for the microphone to settle
        // ASMP401: the small red audio IC needs 0.2s initialization time before its stable sensing
        __delay_cycles(1600000); // 0.2s
    }
}

/*--------------------------------------------------------------------------*/
/* Start collecting audio samples in ping-pong buffers */
void Audio_startCollect(void)
{
    // Enable DMA channel 1 interrupt
    DMA_enableInterrupt(DMA_CHANNEL_1);

    // Enable the DMA0 to start receiving triggers when ADC sample available
    DMA_enableTransfers(DMA_CHANNEL_1);

    // Start TA0 timer to begin audio data collection
    Timer_A_clear(TIMER_A0_BASE);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
}

/*--------------------------------------------------------------------------*/
/* Shut down the audio collection peripherals*/
void Audio_shutdownCollect(void)
{
    /*
     * DMA finishes
     */
    // Disable the dma transfer
    DMA_disableTransfers(DMA_CHANNEL_1);
    // Disable DMA channel 1 interrupt
    DMA_disableInterrupt(DMA_CHANNEL_1);
    P1OUT &= ~BIT0;  // turn off LED

    /*
     * audio stop collect
     */
    Timer_A_stop(TIMER_A0_BASE);
    ADC12_B_disableConversions(ADC12_B_BASE, ADC12_B_COMPLETECONVERSION);
    // Disable DMA channel and interrupt
    DMA_disableTransfers(DMA_CHANNEL_1);
    DMA_disableInterrupt(DMA_CHANNEL_1);


    /*
     * audio shut down
     */
    // Turn off preamp power
    MIC_POWER_PORT_OUT &= ~MIC_POWER_PIN;
    // Disable the ADC
    ADC12_B_disable(ADC12_B_BASE);
}


//******************************************************************************
// DMA interrupt service routine
//******************************************************************************

// interrupt for the timer
//
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = DMA_VECTOR
__interrupt void dmaIsrHandler(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(DMA_VECTOR))) dmaIsrHandler (void)
#else
#error Compiler not supported!
#endif
{
    switch (__even_in_range(DMAIV, DMAIV_DMA2IFG))
    {
        case DMAIV_DMA0IFG: 
            // ADC12IFG = 0;
            break;
        case DMAIV_DMA1IFG:



//            //shutdown audio collect
//            Audio_stopCollect();
            Audio_shutdownCollect();

            // Start Cpu on exit
            __bic_SR_register_on_exit(LPM3_bits);
            break;
        default: break;
   }
}


// interrupt for the timer
//
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(ADC12IV,76))
  {
    case ADC12IV_NONE: break;                // Vector  0:  No interrupt
    case ADC12IV_ADC12OVIFG: break;          // Vector  2:  ADC12MEMx Overflow
    case ADC12IV_ADC12TOVIFG: break;         // Vector  4:  Conversion time overflow
    case ADC12IV_ADC12HIIFG: break;          // Vector  6:  ADC12HI
    case ADC12IV_ADC12LOIFG: break;          // Vector  8:  ADC12LO
    case ADC12IV_ADC12INIFG: break;           // Vector 10:  ADC12IN
    case ADC12IV_ADC12IFG0:  break;
    case ADC12IV_ADC12IFG1: break;                   // Vector 14:  ADC12MEM1
    case ADC12IV_ADC12IFG2: break;            // Vector 16:  ADC12MEM2
    case ADC12IV_ADC12IFG3: break;            // Vector 18:  ADC12MEM3
    case ADC12IV_ADC12IFG4: break;            // Vector 20:  ADC12MEM4
    case ADC12IV_ADC12IFG5: break;            // Vector 22:  ADC12MEM5
    case ADC12IV_ADC12IFG6: break;            // Vector 24:  ADC12MEM6
    case ADC12IV_ADC12IFG7: break;            // Vector 26:  ADC12MEM7
    case ADC12IV_ADC12IFG8: break;            // Vector 28:  ADC12MEM8
    case ADC12IV_ADC12IFG9: break;            // Vector 30:  ADC12MEM9
    case ADC12IV_ADC12IFG10: break;           // Vector 32:  ADC12MEM10
    case ADC12IV_ADC12IFG11: break;           // Vector 34:  ADC12MEM11
    case ADC12IV_ADC12IFG12: break;           // Vector 36:  ADC12MEM12
    case ADC12IV_ADC12IFG13: break;           // Vector 38:  ADC12MEM13
    case ADC12IV_ADC12IFG14: break;           // Vector 40:  ADC12MEM14
    case ADC12IV_ADC12IFG15: break;           // Vector 42:  ADC12MEM15
    case ADC12IV_ADC12IFG16: break;           // Vector 44:  ADC12MEM16
    case ADC12IV_ADC12IFG17: break;           // Vector 46:  ADC12MEM17
    case ADC12IV_ADC12IFG18: break;           // Vector 48:  ADC12MEM18
    case ADC12IV_ADC12IFG19: break;           // Vector 50:  ADC12MEM19
    case ADC12IV_ADC12IFG20: break;           // Vector 52:  ADC12MEM20
    case ADC12IV_ADC12IFG21: break;           // Vector 54:  ADC12MEM21
    case ADC12IV_ADC12IFG22: break;           // Vector 56:  ADC12MEM22
    case ADC12IV_ADC12IFG23: break;           // Vector 58:  ADC12MEM23
    case ADC12IV_ADC12IFG24: break;           // Vector 60:  ADC12MEM24
    case ADC12IV_ADC12IFG25: break;           // Vector 62:  ADC12MEM25
    case ADC12IV_ADC12IFG26: break;           // Vector 64:  ADC12MEM26
    case ADC12IV_ADC12IFG27: break;           // Vector 66:  ADC12MEM27
    case ADC12IV_ADC12IFG28: break;           // Vector 68:  ADC12MEM28
    case ADC12IV_ADC12IFG29: break;           // Vector 70:  ADC12MEM29
    case ADC12IV_ADC12IFG30: break;           // Vector 72:  ADC12MEM30
    case ADC12IV_ADC12IFG31: break;           // Vector 74:  ADC12MEM31
    case ADC12IV_ADC12RDYIFG: break;          // Vector 76:  ADC12RDY
    default: break;
  }
}
