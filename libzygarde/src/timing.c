#include <timing.h>


__ro_fram uint16_t initial_time = 100;
__ro_fram uint16_t current_time;
__ro_fram uint16_t reboot_counter;

/* ----- Used to track the state of the software state machine -----*/
I2C_Mode MasterMode = IDLE_MODE;

// ----------The register address command to use -------------------
uint8_t TransmitRegAddr = 0;

uint16_t ReceiveBuffer[MAX_BUFFER_SIZE] = {0};
uint8_t RXByteCtr = 0;
uint8_t ReceiveIndex = 0;
uint8_t TransmitBuffer[MAX_BUFFER_SIZE]	= {0};

void I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count)
{
	// Initialize state machine
	MasterMode = TX_REG_ADDRESS_MODE;
	TransmitRegAddr = reg_addr;
	RXByteCtr = count;
	ReceiveIndex = 0;

	// Initialize slave adress and interrupts
	UCB2I2CSA = dev_addr;
	UCB2IFG &= ~(UCTXIFG + UCRXIFG); //clear any pending interrupts
	UCB2IE &= ~UCRXIE; //Disable RX interrupts
	UCB2IE |= UCTXIE; //Enable TX interrupts
	
	UCB2CTLW0 |= UCTR + UCTXSTT; //I2C TX, start consition
	// msp_sleep(1000);
	__bis_SR_register(LPM3_bits + GIE); // enter LPM0 w/ interrupts	
}



void initRTCGPIO()
{
	// I2C Pins
	P7SEL0 |= BIT0 | BIT1;
	P7SEL1 &= ~(BIT0 | BIT1);
	

	// Disable the GPIO power-on default high-impedance mode to activate previously configured port setting
	PM5CTL0 &= ~LOCKLPM5;
}

void initI2C()
{
	UCB2CTLW0 = UCSWRST; // Enable SW reset
	UCB2CTLW0 |= UCMODE_3 | UCMST | UCSSEL__ACLK | UCSYNC; //I2C master mode, ACLK
	UCB2BRW = 2; // fSCL = ACLK/ 9.4 KHz = 4.7 KHz
	UCB2I2CSA = SLAVE_ADDR; // Slave Address
	UCB2CTLW0 &= ~ UCSWRST; // Clear SW reset, resume operation 
	UCB2IE |= 2;
}



/*void initTimer()
{
	//Start Timer
	Timer_A_clearTimerInterrupt(TIMER_A1_BASE);
	Timer_A_initUpModeParam param = {0};
		param.clockSource = TIMER_A_CLOCKSOURCE_ACLK; //ACLK = VLOCLK = 9.4 KHz
		param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
		param.timerPeriod = 32768; // count upto this number (period), request time every second
		param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
		param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
		param.timerClear = TIMER_A_DO_CLEAR;
		param.startTimer = true;
	Timer_A_initUpMode(TIMER_A1_BASE, &param);	
}*/


uint16_t getSecond(uint16_t minute, uint16_t second)
{
	// PRINTF("Getting time: %u %u", minute, second);
	return (60*minute)+second;
} 

uint8_t decode_second_minute(uint8_t value)
{
	uint8_t decoded = value & 127;
	decoded = (decoded & 15) + 10 * ((decoded & (15 << 4)) >>4);
	return decoded;
}

uint16_t get_time(void)
{
	
	// I2C_Master_ReadReg(SLAVE_ADDR, DS3231_REG_SECONDS, 2);
	// PRINTF("in get time");
	// PRINTF("\r\n sec: %u ", decode_second_minute(ReceiveBuffer[0]));
	// PRINTF("min: %u\n\r", decode_second_minute(ReceiveBuffer[1]));

	return getSecond(decode_second_minute(ReceiveBuffer[1]), decode_second_minute(ReceiveBuffer[0]));
}

void getRTC(void)
{
	// GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
	// GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN1);
	// PRINTF("B");
	// __delay_cycles(5000000);
	// PRINTF("A");
	initial_time = get_time();
	current_time = initial_time;
	reboot_counter++;
	// // P5DIR = 0x00;
	// PRINTF("b");
	// __delay_cycles(5000000);
	// PRINTF("a");
	// // initial_time =
	// GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
	// GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN1);
	// PRINTF("b");
	// __delay_cycles(5000000);
	// PRINTF("a");
	// GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
	// GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN1);
	// PRINTF("b");
	// __delay_cycles(5000000);
	// PRINTF("a");
	// __delay_cycles(1600000);
}

/*
// interrupt for the timer
//
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMERA0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER1_A0_VECTOR))) TIMERA0_ISR (void)
#else
#error Compiler not supported!
#endif
{
	//stop the timer, clear MC to 00
	Timer_A_stop(TIMER_A1_BASE); // stop the counting of the timer
	//clear the timer counter, so that when it starts next time, the counter starts from 0 instead of starting from where it left
	Timer_A_clear(TIMER_A1_BASE);

	// get the cirrent time
	//get_time();
	current_time++;
	PRINTF("\r\n %u %u\n\r", current_time, reboot_counter);
	
	//start Timer
	Timer_A_startCounter(TIMER_A1_BASE,TIMER_A_UP_MODE);
}*/

//******************************************************************************
// I2C Interrupt ***************************************************************
//******************************************************************************

// #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
// #pragma vector = USCI_B2_VECTOR
// __interrupt void USCI_B2_ISR(void)
// #elif defined(__GNUC__)
// void __attribute__ ((interrupt(USCI_B2_VECTOR))) USCI_B2_ISR (void)
// #else
// #error Compiler not supported!
// #endif
// {
// 	PRINTF("\r\nin the getrtc\r\n");
//   //Must read from UCB2RXBUF
//   uint8_t rx_val = 0;
//   switch(__even_in_range(UCB2IV, USCI_I2C_UCBIT9IFG))
//   {
//     case USCI_NONE:          break;         // Vector 0: No interrupts
//     case USCI_I2C_UCALIFG:   break;         // Vector 2: ALIFG
//     case USCI_I2C_UCNACKIFG:                // Vector 4: NACKIFG
//       break;
//     case USCI_I2C_UCSTTIFG:  break;         // Vector 6: STTIFG
//     case USCI_I2C_UCSTPIFG:  break;         // Vector 8: STPIFG
//     case USCI_I2C_UCRXIFG3:  break;         // Vector 10: RXIFG3
//     case USCI_I2C_UCTXIFG3:  break;         // Vector 12: TXIFG3
//     case USCI_I2C_UCRXIFG2:  break;         // Vector 14: RXIFG2
//     case USCI_I2C_UCTXIFG2:  break;         // Vector 16: TXIFG2
//     case USCI_I2C_UCRXIFG1:  break;         // Vector 18: RXIFG1
//     case USCI_I2C_UCTXIFG1:  break;         // Vector 20: TXIFG1
//     case USCI_I2C_UCRXIFG0:                 // Vector 22: RXIFG0
//         rx_val = UCB2RXBUF;
//         if (RXByteCtr)
//         {
//           ReceiveBuffer[ReceiveIndex++] = rx_val;
//           RXByteCtr--;
//         }

//         if (RXByteCtr == 1)
//         {
//           UCB2CTLW0 |= UCTXSTP;
//         }
//         else if (RXByteCtr == 0)
//         {
//           UCB2IE &= ~UCRXIE;
//           MasterMode = IDLE_MODE;
//           __bic_SR_register_on_exit(LPM3_bits);      // Exit LPM
//         }
//         break;
//     case USCI_I2C_UCTXIFG0:                 // Vector 24: TXIFG0
//         switch (MasterMode)
//         {
//           case TX_REG_ADDRESS_MODE:        // no matter in Rx or Tx mode, the first step is to transmit the
//               UCB2TXBUF = TransmitRegAddr;
//               MasterMode = SWITCH_TO_RX_MODE;   // Need to start receiving now
//               break;

//           case SWITCH_TO_RX_MODE:
//               UCB2IE |= UCRXIE;              // Enable RX interrupt
//               UCB2IE &= ~UCTXIE;             // Disable TX interrupt
//               UCB2CTLW0 &= ~UCTR;            // Switch to receiver
//               MasterMode = RX_DATA_MODE;    // State state is to receive data
//               UCB2CTLW0 |= UCTXSTT;          // Send the TransmitRegAddr
//               break;


//           default:
//               __no_operation();
//               break;
//         }
//         break;
//     default: break;
//   }
// }




#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = EUSCI_B2_VECTOR
__interrupt void USCI_B2_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_B2_VECTOR))) USCI_B2_ISR (void)
#else
#error Compiler not supported!
#endif
{
	 uint8_t rx_val = 0;
    switch(__even_in_range(UCB2IV, USCI_I2C_UCBIT9IFG))
    {
        case USCI_NONE:          break;     // Vector 0: No interrupts
        case USCI_I2C_UCALIFG:   break;     // Vector 2: ALIFG
        case USCI_I2C_UCNACKIFG:            // Vector 4: NACKIFG
            UCB2CTLW0 |= UCTXSTT;           // resend start if NACK
            break;
        case USCI_I2C_UCSTTIFG:  break;     // Vector 6: STTIFG
        case USCI_I2C_UCSTPIFG:  break;     // Vector 8: STPIFG
        case USCI_I2C_UCRXIFG3:  break;     // Vector 10: RXIFG3
        case USCI_I2C_UCTXIFG3:  break;     // Vector 12: TXIFG3
        case USCI_I2C_UCRXIFG2:  break;     // Vector 14: RXIFG2
        case USCI_I2C_UCTXIFG2:  break;     // Vector 16: TXIFG2
        case USCI_I2C_UCRXIFG1:  break;     // Vector 18: RXIFG1
        case USCI_I2C_UCTXIFG1:  break;     // Vector 20: TXIFG1
        case USCI_I2C_UCRXIFG0:  
			PRINTF("here");
			rx_val = UCB2RXBUF;
			if (RXByteCtr)
			{
			ReceiveBuffer[ReceiveIndex++] = rx_val;
			RXByteCtr--;
			}

			if (RXByteCtr == 1)
			{
			UCB2CTLW0 |= UCTXSTP;
			}
			else if (RXByteCtr == 0)
			{
			UCB2IE &= ~UCRXIE;
			MasterMode = IDLE_MODE;
			__bic_SR_register_on_exit(LPM3_bits);      // Exit LPM
			}
		
			break;     // Vector 22: RXIFG0
        case USCI_I2C_UCTXIFG0:             // Vector 24: TXIFG0
		
			switch (MasterMode)
			{
			case TX_REG_ADDRESS_MODE:        // no matter in Rx or Tx mode, the first step is to transmit the
				UCB2TXBUF = TransmitRegAddr;
				MasterMode = SWITCH_TO_RX_MODE;   // Need to start receiving now
				break;

			case SWITCH_TO_RX_MODE:
				UCB2IE |= UCRXIE;              // Enable RX interrupt
				UCB2IE &= ~UCTXIE;             // Disable TX interrupt
				UCB2CTLW0 &= ~UCTR;            // Switch to receiver
				MasterMode = RX_DATA_MODE;    // State state is to receive data
				UCB2CTLW0 |= UCTXSTT;          // Send the TransmitRegAddr
				break;


			default:
				__no_operation();
				break;
			}
			// Bashima added this
			// __bic_SR_register_on_exit(LPM3_bits);      // Exit LPM
			break;
				
        case USCI_I2C_UCBCNTIFG: break;     // Vector 26: BCNTIFG
        case USCI_I2C_UCCLTOIFG: break;     // Vector 28: clock low timeout
        case USCI_I2C_UCBIT9IFG: break;     // Vector 30: 9th bit
        default: break;
    }
}