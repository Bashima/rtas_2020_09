
#ifndef TIMING_H_
#define TIMING_H_

#include <libdnn/mem.h>
#include <stdlib.h>
#include <libmspdriver/timer_a.h>
#include <libmspdriver/gpio.h>
#include <libio/console.h>

//--------------------Defining Pins---------------
#define DS3231_I2C_ADDRESS 0x68
#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_MINUTES    0x01

#define SLAVE_ADDR 0x68
#define MAX_BUFFER_SIZE 20



// -----------------General I2C State Machine -------------------
// General I2C State Machine ***************************************************
typedef enum I2C_ModeEnum{
    IDLE_MODE,
    TX_REG_ADDRESS_MODE,
    RX_DATA_MODE,
    SWITCH_TO_RX_MODE,
} I2C_Mode;





void I2C_Master_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t count);
void initRTCGPIO();
void initI2C();
//void initTimer();
void getRTC();
// get time in seconds
uint16_t getSecond(uint16_t minute, uint16_t second);
uint8_t decode_second_minute(uint8_t value);
uint16_t get_time(void);

// void __attribute__ ((interrupt(USCI_B2_VECTOR))) USCI_B2_ISR (void);
//void __attribute__ ((interrupt(TIMER1_A0_VECTOR))) TIMERA0_ISR (void);

extern uint16_t initial_time;
extern uint16_t current_time;
extern uint16_t reboot_counter;

#endif
