#include <msp430.h>
#include <stdlib.h>
#include <string.h>

#include <libio/console.h>
#include <libmspbuiltins/builtins.h>
#include <libmsp/mem.h>
#include <libmsp/periph.h>
#include <libmsp/clock.h>
#include <libmsp/watchdog.h>
#include <libmsp/gpio.h>

#include <libalpaca/alpaca.h>
#include <libfixed/fixed.h>
#include <libmat/mat.h>

#include <libdnn/misc.h>
#include <libdnn/mem.h>
#include <libdnn/state.h>
#include <libdnn/buffer.h>
#include <libdnn/nn.h>
#include <libdnn/nonlinear.h>
#include <libdnn/linalg.h>
#include<libdnn/cleanup.h>

#include "headers/conv1.h"
#include "headers/conv2.h"
#include "headers/fc1.h"
#include "headers/fc2.h"

#include "headers/plaintext.h"

#include <libzygarde/timing.h>


#include <libmsp/sleep.h>
#include <libmsp/temp.h>
#include <math.h>

#include "headers/bitcounter.h"
#include "headers/audioglobal.h"
#include <liblof/lof.h>
#include <librsa/rsa.h>
#include <libmspdriver/gpio.h>

// #include <libharvest/charge.h>

////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////Alapaca Shim///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define MEM_SIZE 0x400
__hifram uint8_t *data_src[MEM_SIZE];
__hifram uint8_t *data_dest[MEM_SIZE];
__hifram unsigned int data_size[MEM_SIZE];



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Tasks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void init();
void task_init();
void task_compute1();
void task_compute2();
void task_compute3();
void task_compute4();
void task_compute5();
// void task_finish();
void task_exit();
void task_scheduler();
void task_tempurature();
void task_rsa();
void task_bc1();
void task_bc2();

TASK(1, task_init);
TASK(2, task_bc1);
TASK(3, task_bc2);
TASK(4, task_exit);
TASK(5, task_scheduler);
TASK(6, task_tempurature);
TASK(7, task_rsa);
TASK(8, task_compute1);
TASK(9, task_compute2);
TASK(10, task_compute3);
TASK(11, task_compute4);
TASK(12, task_compute5);
// TASK(13, task_finish);


ENTRY_TASK(task_init)
INIT_FUNC(init)
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Setup///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifndef CONFIG_CONSOLE
	#pragma message "no console"
	#define printf(fmt, ...) (void)0
#endif
 

void initGPIO()
{
	PADIR=0xffff; PAOUT=0x0000; // Ports 1 and 2
    PBDIR=0xffff; PBOUT=0x0000; // Ports 3 and 4
    PCDIR=0xffff; PCOUT=0x0000; // Ports 5 and 6
    PDDIR=0xffff; PDOUT=0x0000; // Ports 7 and 8
    PJDIR=0xff; PJOUT=0x00;     // Port J
	
	// I2C Pins
	P7SEL0 |= BIT0 | BIT1;
	P7SEL1 &= ~(BIT0 | BIT1);

	GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1);
	GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN2);
	GPIO_setAsInputPin(GPIO_PORT_P1, GPIO_PIN4|GPIO_PIN5);
	// P5DIR |= BIT0 | BIT1;

	GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
	GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN1);
	// P1DIR |= BIT2;
	// P3DIR= BIT1;
	// GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2); |
	
	// Disable the GPIO power-on default high-impedance mode to activate previously configured port setting
	PM5CTL0 &= ~LOCKLPM5;
	
}

void clear_isDirty() {
	msp_sleep(20);
	initGPIO();
	initI2C();
}

static void init_hw() {
	msp_watchdog_disable();
	msp_clock_setup();
	// initGPIO();
	
}

void init() {
	init_hw();

#ifdef CONFIG_CONSOLE
	#pragma message "init console"
	INIT_CONSOLE();
#endif

    __enable_interrupt();
	// harvest_charge();
	initGPIO();
    PRINTF("\r\n staring with task pointer .%u.\n\r", curctx->task->idx);
	
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////Stacks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
__fram stack_t st;
__fram stack_t *mat_stack = &st;

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////Weights Matrices/////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
__ro_fram mat_t mat_conv1_wd = {
	.dims = {CONV1_WMD_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_wmd,
	.sparse = {
		.dims = {16, 1, 1, 1},
		.len_dims = 4,
		.sizes = conv1_wmd_sizes,
		.offsets = conv1_wmd_offsets
	},
};

__ro_fram mat_t mat_conv1_wv = {
	.dims = {CONV1_WMV_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_wmv,
	.sparse = {
		.dims = {16, 1, 5, 1},
		.len_dims = 4,
		.sizes = conv1_wmv_sizes,
		.offsets = conv1_wmv_offsets
	},
};

__ro_fram mat_t mat_conv1_wh = {
	.dims = {CONV1_WMH_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_wmh,
	.sparse = {
		.dims = {16, 1, 1, 5},
		.len_dims = 4,
		.sizes = conv1_wmh_sizes,
		.offsets = conv1_wmh_offsets
	},
};

__ro_fram mat_t mat_conv1_b = {
	.dims = {16},
	.len_dims = 1,
	.strides = {1},
	.data = conv1_b,
};

__ro_fram mat_t  mat_conv2_w = {
	.dims = {CONV2_WM_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_wm,
	.sparse = {
		.dims = {32, 16, 5, 5},
		.len_dims = 4,
		.sizes = conv2_wm_sizes,
		.offsets = conv2_wm_offsets,
	}
};

__ro_fram mat_t mat_conv2_b = {
	.dims = {32},
	.strides = {1},
	.len_dims = 1,
	.data = conv2_b,
};

__ro_fram mat_t  mat_conv3_w = {
	.dims = {CONV2_WM_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = conv2_wm,
	.sparse = {
		.dims = {64, 32, 5, 5},
		.len_dims = 4,
		.sizes = conv2_wm_sizes,
		.offsets = conv2_wm_offsets,
	}
};

__ro_fram mat_t mat_conv3_b = {
	.dims = {64},
	.strides = {1},
	.len_dims = 1,
	.data = conv2_b,
};


__ro_fram mat_t mat_fc1_wh = {
	.dims = {FC1_WMH_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = fc1_wmh,
	.sparse = {
		.dims = {128, 256},
		.len_dims = 2,
		.offsets = fc1_wmh_offsets,
		.sizes = fc1_wmh_sizes,
	},
};

__ro_fram mat_t mat_fc1_wv = {
	.dims = {FC1_WMV_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = fc1_wmv,
	.sparse = {
		.dims = {96, 128},
		.len_dims = 2,
		.offsets = fc1_wmv_offsets,
		.sizes = fc1_wmv_sizes,
	},
};

__ro_fram mat_t mat_fc1_b = {
	.dims = {96, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc1_b,
};

__ro_fram mat_t mat_fc2_w = {
	.dims = {FC1_WMV_LEN},
	.len_dims = 1,
	.strides = {1},
	.data = fc1_wmv,
	.sparse = {
		.dims = {10, 96},
		.len_dims = 2,
		.offsets = fc1_wmv_offsets,
		.sizes = fc1_wmv_sizes,
	},
};

__ro_fram mat_t mat_fc2_b = {
	.dims = {10, 1},
	.strides = {1, 1},
	.len_dims = 2,
	.data = fc1_b,
};

__ro_fram mat_t mat_input_1 = {
	.dims = {1, 40, 63},
	.strides = {2520, 40, 1},
	.len_dims = 3,
	.data = dataRecorded,
};

__fram mat_t buf1 = {.data = LAYER_BUFFER(1)};
__fram mat_t buf2 = {.data = LAYER_BUFFER(2)};

__fram mat_t *b1 = &buf1;
__fram mat_t *b2 = &buf2;


/////////////////variables for the scheduler/////////////////////
// 1 = temperature lof 
// 2 = bit counter 1
// 3 = bit counter 2
// 4 = audio dnn 1
// 5 = audio dnn 2
// 6 = audio dnn 3
// 7 = audio dnn 4
// 8 = audio dnn 5
// 9 = rsa encryption 

unsigned int sleep_cycle = 50;
#define TEMP_PERIOD 13
#define BIT_PERIOD 10
#define AUDIO_PERIOD 12
#define RSA_PERIOD 5
__fram uint8_t schedule[18] = {2, 3, 9, 1, 2, 3, 4, 5, 6, 9, 2, 3, 1, 9, 2, 3, 7, 8}
// __fram uint8_t schedule[180] = {2, 3, 9, 1, 2, 3, 4, 5, 9, 2, 1, 9, 2, 3, 6, 7, 8}
// __fram uint8_t schedule[180] = {1, 4, 5, 6, 7, 8, 2, 3, 9, 2, 3, 9, 1, 4, 5, 6, 7, 8}

// __fram uint8_t schedule[10] = {1, 4, 2, 5, 9, 6 , 7, 3, 8, 1};
// __fram unsigned int schedule_ind = 0;
__fram uint8_t ifVCC = 0;
__fram uint8_t ifENG = 0;

#define TEMP_PERIOD 13
#define BIT_PERIOD 10
#define AUDIO_PERIOD 12
#define RSA_PERIOD 5
__fram uint16_t deadlines[10][10] = {0};
__fram uint8_t current_instance_count[9] = {0};
__fram uint8_t checkDeadline[9] = {1, 0, 1, 0, 0, 0, 0, 1, 1};
__fram uint8_t deadline_miss = 0;
__fram uint16_t arrivals[10][10] = {0};
void get_periods()
{
	getRTC();
	arrivals[0][0] = initial_time;
	arrivals[0][1] = initial_time + TEMP_PERIOD;
	arrivals[2][0] = initial_time;
	arrivals[7][0] = initial_time;
	arrivals[8][0] = initial_time;
	
	deadlines[0][0] = initial_time + TEMP_PERIOD;
	deadlines[0][1] = initial_time + (2*TEMP_PERIOD);
	deadlines[2][0] = initial_time + BIT_PERIOD;
	deadlines[7][0] = initial_time + AUDIO_PERIOD;
	deadlines[8][0] = initial_time + RSA_PERIOD;
	PRINTF("\r\ninitial: %u periods: %u, %u, %u, %u, %u", initial_time, deadlines[0][0], deadlines[0][1], deadlines[2][0], deadlines[7][0], deadlines[8][0]);
}

/////// variables for temperature anomaly detection///////////////////////////
__hifram uint16_t data[ELEMENT_NUMBER] = {1,100,71,67,88,
90,86,82,6,97,
96,36,93,33,86,
56,47,91,54,18,
47,17,3,5,26,
66,60,92,44,73,
85,98,58,68,30,
93,95,73,92,46,
41,34,29,89,52,
14,32,12,6,89,
71,33,7,96,77,
97,37,79,3,31,
24,23,16,66,73,
89,35,90,12,56,
88,25,35,55,33,
15,20,66,48,12,
30,100,32,20,42,
15,17,72,55,66,
91,100,35,68,1,
20,81,65,31,5};
__ro_fram uint16_t temperature;
__hifram fixed distances[ELEMENT_NUMBER] = {0};
__hifram uint16_t indices[ELEMENT_NUMBER] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 
20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 
40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 
50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 
60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 
70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 
90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 
};
__ro_fram fixed k_distance;
__hifram fixed reachability_distance[ELEMENT_NUMBER] = {0};
__hifram fixed lrds[ELEMENT_NUMBER] = {0};
__ro_fram fixed current_lrd = 0;

/////////////////////variables for RSA///////////////////////////
__fram int16_t p_main, q_main;
__ro_fram int16_t ed[4] = {0,0,0, 0}; // e, d, phi_n, n
__ro_fram uint16_t char_index = 0;
__hifram int16_t outputRSA[TEXT_LENGTH];

//////////////////////variables for bit counter///////////////
__ro_fram uint16_t n_0;
__ro_fram uint16_t n_1;
__ro_fram uint16_t n_2;
__ro_fram uint16_t n_3;
__ro_fram uint16_t n_4;
__ro_fram uint16_t n_5;
__ro_fram uint16_t n_6;

__ro_fram uint16_t bc_func;
__ro_fram uint32_t bc_seed;
__ro_fram uint16_t bc_iter;
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////Tasks///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void task_init() {
	// initI2C();
	// GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);
	// GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN1);
	msp_sleep(10);
	PRINTF("\r\n========================");

	params.same_padding = false;
	params.size[0] = 1;
	params.size[1] = 2;
	params.size[2] = 2;
	params.stride[0] = 1;
	params.stride[1] = 1;
	params.stride[2] = 1;

	if(reboot_counter == 0)
	{
		get_periods();
	}
	TRANSITION_TO(task_scheduler);
	// TRANSITION_TO(task_compute);
}

int AnalogRead(uint8_t channel)
{
	int temp = 0;
    /* Configure ADC Channel */
    ADC12CTL0 &= ~ADC12ENC; //make sure ENC is 0 so we can configure the read
    ADC12CTL0 = ADC12SHT0_3 + ADC12ON; //64 clk ticks, ADC on, enable interrupt
    ADC12CTL1 = ADC12SSEL_0 | ADC12SHP_1; // ADC12OSC
    ADC12MCTL0 = channel; // channel
    ADC12CTL0 |= ADC12ENC + ADC12SC; //Enable and start conversion
    while ((ADC12BUSY & ADC12CTL1) == 0x01); //Wait for conversion to end
	temp = ADC12MEM0;
	ADC12CTL0  = 0;
	ADC12CTL0 = ADC12SHT0_3 + ADC12ON;
    return temp;
}

void task_scheduler()
{ 
	uint16_t schedule_ind;
	schedule_ind = CUR_SCRATCH[0];
	GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2);
	PRINTF("\r\nscheduler %u", initial_time);
	// init();
	getRTC();
	
	if(schedule_ind > 0)
	{
		uint8_t current_ind = schedule[schedule_ind-1] - 1;

		if(checkDeadline[current_ind] == 1)
		{
			if(deadlines[current_ind][current_instance_count[current_ind]]>initial_time)
			{
				PRINTF("\r\nmissed: %u", current_ind+1);
				deadline_miss++;
			}
			current_instance_count[schedule_ind-1]++;
		}

	}
	else
	{
		PRINTF("\r\nDeadline Miss: ", deadline_miss);
	}
	
	if(ifVCC == 0)
	{
		ifVCC = 1;
		runSampling_VCC();
	}

	ifVCC = 0;
	// if(ifENG == 0)
	// {
	// 	ifENG = 1;
	// 	runSampling_ENG();
	// }

	// ifENG = 0;
	int a = AnalogRead(ADC12INCH_5);
	PRINTF("\r\nvcc: %u eng: %u \r\n", vccRecorded[VCC_SAMPLES_LENGTH-1], a);
	while(a <4059)
	{
		a = AnalogRead(ADC12INCH_5);
		PRINTF("In SLeep");
		GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2);
		msp_sleep(1000);
		GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
	}
	
	if(schedule_ind > 10)
	{
		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_exit)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_exit);
	}
	else 
	{
		schedule_ind++;7
	}
	
	scratch_bak[0] = schedule_ind;
	write_to_gbuf((uint8_t *)(scratch_bak),
		(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
	if(schedule[schedule_ind-1] == 1)
	{
		TASK_REF(task_tempurature)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_tempurature);
	}
	else if(schedule[schedule_ind-1] == 2)
	{
		TASK_REF(task_bc1)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_bc1);
	}
	else if(schedule[schedule_ind-1]==3)
	{
		TASK_REF(task_bc2)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_bc2);
	}
	else if(schedule[schedule_ind-1] == 4)
	{
		TASK_REF(task_compute1)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_compute1);
	}
	else if(schedule[schedule_ind-1] == 5)
	{
		TASK_REF(task_compute2)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_compute2);
	}
	else if(schedule[schedule_ind-1] == 6)
	{
		TASK_REF(task_compute3)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_compute3);
	}
	else if(schedule[schedule_ind-1] == 7)
	{
		TASK_REF(task_compute4)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_compute4);
	}
	else if(schedule[schedule_ind-1] == 8)
	{
		TASK_REF(task_compute5)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_compute5);
	}
	else if(schedule[schedule_ind-1] == 9)
	{
		TASK_REF(task_rsa)->info.return_task = CUR_TASK;
		TRANSITION_TO(task_rsa);
	}
	TRANSITION_TO(task_scheduler);
}


void task_bc1()
{
	uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
	if(state == 0)
	{
		n_0 = 0;
		n_1 = 0;
		n_2 = 0;
		n_3 = 0;
		n_4 = 0;
		n_5 = 0;
		n_6 = 0;

		bc_seed = (uint32_t)SEED;
		CUR_SCRATCH[2] = 0;
		CUR_SCRATCH[0] = 0;
		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	else if(state == 1)
	{
		n_0 = bit_count(bc_seed, n_0);
		bc_seed = bc_seed + 13;
		bc_iter++;
		
		if(bc_iter < ITER)
		{
			transition_to(CUR_TASK);
		}
		else
		{
			// PRINTF("task bitcount %u\n\r", bc_iter);
			bc_iter = 0;
			bc_seed = (uint32_t)SEED;
			scratch_bak[0] = 2;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
			transition_to(CUR_TASK);
		}
	}
	else if(state == 2)
	{
		n_1 = bitcount(bc_seed, n_1);
		bc_seed = bc_seed + 13;
		bc_iter++;
		
		if(bc_iter < ITER)
		{
			transition_to(CUR_TASK);
		}
		else
		{
			// PRINTF("task bitcount %u\n\r", bc_iter);
			bc_iter = 0;
			bc_seed = (uint32_t)SEED;
			scratch_bak[0] = 3;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
			transition_to(CUR_TASK);
		}
	}
	else if(state == 3)
	{
		task_ntbl_bitcnt(bc_seed, n_2);
		bc_seed = bc_seed + 13;
		bc_iter++;
		
		if(bc_iter < ITER)
		{
			transition_to(CUR_TASK);
		}
		else
		{
			// PRINTF("task ntbl_bitcnt %u\n\r", bc_iter);
			bc_iter = 0;
			bc_seed = (uint32_t)SEED;
			scratch_bak[0] = 4;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
			transition_to(CUR_TASK);
		}
	}
	else if(state == 4)
	{
		task_ntbl_bitcount(bc_seed, n_3);
		bc_seed = bc_seed + 13;
		bc_iter++;
		
		if(bc_iter < ITER)
		{
			transition_to(CUR_TASK);
		}
		else
		{
			// PRINTF("task ntbl_bitcount %u\n\r", bc_iter);
			bc_iter = 0;
			bc_seed = (uint32_t)SEED;
			scratch_bak[0] = 5;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
			transition_to(CUR_TASK);
		}
	}
	PRINTF("\r\nBit 1\r\n");
	scratch_bak[0] = 0;
	write_to_gbuf((uint8_t *)(scratch_bak),
	(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
	char_index = 0;
	state = 0;
	// TRANSITION_TO(task_scheduler);
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
}
void task_bc2()
{
	uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
	if(state == 0)
	{
		task_BW_btbl_bitcount(bc_seed, n_4);
		bc_seed = bc_seed + 13;
		bc_iter++;
		
		if(bc_iter < ITER)
		{
			transition_to(CUR_TASK);
		}
		else
		{
			bc_iter = 0;
			bc_seed = (uint32_t)SEED;
			scratch_bak[0] = 1;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
			transition_to(CUR_TASK);
		}
	}
	else if(state == 1)
	{
		task_AR_btbl_bitcount(bc_seed, n_5);
		bc_seed = bc_seed + 13;
		bc_iter++;
		
		if(bc_iter < ITER)
		{
			transition_to(CUR_TASK);
		}
		else
		{
			bc_iter = 0;
			bc_seed = (uint32_t)SEED;
			scratch_bak[0] = 2;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
			transition_to(CUR_TASK);
		}
	}
	else if(state == 2)
	{
		task_bit_shifter(bc_seed, n_6);
		bc_seed = bc_seed + 13;
		bc_iter++;
		
		if(bc_iter < ITER)
		{
			transition_to(CUR_TASK);
		}
		else
		{
			bc_iter = 0;
			bc_seed = (uint32_t)SEED;
			scratch_bak[0] = 3;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
			transition_to(CUR_TASK);
		}
	}
	PRINTF("\r\nBit 2\r\n");
	scratch_bak[0] = 0;
	write_to_gbuf((uint8_t *)(scratch_bak),
	(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
	char_index = 0;
	state = 0;
	// TRANSITION_TO(task_scheduler);
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
}


void task_rsa()
{
	uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
	if(state == 0)
	{
		p_main = key_generation1();
		CUR_SCRATCH[2] = 0;
		CUR_SCRATCH[0] = 0;

		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	if(state == 1)
	{
		// PRINTF("getting q");
		q_main = key_generation2();

		scratch_bak[0] = 2;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	if(state == 2)
	{
		// PRINTF("getting e %i", p_main);
		// PRINTF("%i", q_main);
		key_generation3(p_main, q_main, ed);

		scratch_bak[0] = 3;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	if(state == 3)
	{
		// PRINTF("getting d");
		key_generation4(ed);

		scratch_bak[0] = 4;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	if(state>3 && state<4+TEXT_LENGTH-1)
	{
		while(1)
		{
			// PRINTF("processing char %u", char_index);
			if(char_index>TEXT_LENGTH-2)
			{
				break;
			}
			outputRSA[char_index] = find_T(text[char_index], ed[0], ed[3]);
			// outputRSA1[char_index] = encryption(text[char_index], ed[0], ed[3]);
			//  for decryption find_T(text[char_index], ed[1], ed[3]);
			
			char_index++;
			scratch_bak[0] = 4 + char_index;
			write_to_gbuf((uint8_t *)(scratch_bak),
				(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		}
		transition_to(CUR_TASK);
		
	}
	if(state == 4+TEXT_LENGTH-1)
	{
		// uint8_t i = 0;
		// for(i = 0; i < 6; i++)
		// {
		// 	PRINTF("encrypted %i  ", outputRSA[i]);
		// 	// PRINTF(",  %i  \n", outputRSA1[i]);
		// }
		PRINTF("\r\nEncryption DONE\r\n");
		scratch_bak[0] = 0;
		write_to_gbuf((uint8_t *)(scratch_bak),
		(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		char_index = 0;
		state = 0;
	}
	// TRANSITION_TO(task_scheduler);
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
}

void task_tempurature()
{
	uint16_t state;
	state = CUR_SCRATCH[0];
	P1OUT |= BIT2;

	if(state == 0)
	{
		// temperature = msp_sample_temperature();
		// PRINTF("\r\ntemperature: %u\r\n", temperature);

		CUR_SCRATCH[0] = 1;

		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));

		runSampling_temp();
		
		transition_to(CUR_TASK);
	}
	else if(state == 1)
	{
		k_distance = get_kth_distance(data, tempRecorded[3], distances, indices);

		scratch_bak[0] = 2;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	else if(state == 2)
	{
		get_reachability_distance(distances, k_distance, reachability_distance);
		scratch_bak[0] = 3;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	else if(state == 3)
	{
		current_lrd = get_lrd(distances, reachability_distance);
		get_reachability_distance(distances, k_distance, reachability_distance);
		
		scratch_bak[0] = 4;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
	else if(state == 4)
	{
		uint8_t lof_val = get_lof(lrds, indices, current_lrd);
		if(lof_val == 1)
		{
			PRINTF("ANOMALY");
		}
		else
		{
			PRINTF("NOT ANOMALY");
		}

		scratch_bak[0] = 5;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
		
	}
	else if(state == 5)
	{
		scratch_bak[0] = 0;
		write_to_gbuf((uint8_t *)(scratch_bak),
		(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		state = 0;
	}
	msp_sleep(50);
	// temperature = msp_sample_temperature();
	// PRINTF("temperature: %d", temperature);
	// unsigned int i = 0;
	// for (i = 0; i < 0x6000; i++);
	// TRANSITION_TO(task_scheduler);
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);

}

__ro_fram uint8_t flag_task = 0; // This flag is used for relooping the tasks.

void task_compute1() {
    uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
	if(state == 0)
    {
		state = 1;
		CUR_SCRATCH[0] = 1;
		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		run_sampling_audio();
		// transition_to(CUR_TASK);
		
	}
    if(state == 1)
    {
        MAT_RESHAPE(b2, 1, 40, 63);
		mat_t *mat_input_ptr = &mat_input_1;
		for(uint16_t i = CUR_SCRATCH[1]; i < 40; i = ++CUR_SCRATCH[1]) {
			for(uint16_t j = CUR_SCRATCH[2]; j < 63; j = ++CUR_SCRATCH[2]) {
				fixed w = MAT_GET(mat_input_ptr, 0, 1, 1);
				MAT_SET(b2, w, 0, i, j);
			}
			CUR_SCRATCH[2] = 0;
			CUR_SCRATCH[0] = 0;
		}

		scratch_bak[0] = 2;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		transition_to(CUR_TASK);
	}
    else if(state == 2)
    {
		// PRINTF("Here in the state 1");
        MAT_RESHAPE(b1, 20, 40, 63);
		mat_t *w_ptr = &mat_conv1_wd;
		mat_t *b_ptr = NULL;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b1, b2);

		scratch_bak[0] = 3;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_conv)->info.return_task = TASK_REF(task_compute1); // get the address of the global function task_s_conv
		TRANSITION_TO(task_s_conv); // perform convolution
    }
	PRINTF("\r\nCompute 1\r\n");
	scratch_bak[0] = 0;
	write_to_gbuf((uint8_t *)(scratch_bak),
	(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
	state = 0;
	// TRANSITION_TO(task_scheduler);
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
}
void task_compute2() {
    uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
    if(state == 0)
    {
        MAT_RESHAPE(b2, 20, 36, 58);
		mat_t *w_ptr = &mat_conv1_wh;
		mat_t *b_ptr = NULL;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b2, b1);

		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_depthconv)->info.return_task = TASK_REF(task_compute2);
		TRANSITION_TO(task_s_depthconv); // CUR_SCRATCH is updated in this section
    }
	PRINTF("\r\nCompute 2\r\n");
	scratch_bak[0] = 0;
	write_to_gbuf((uint8_t *)(scratch_bak),
	(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
	state = 0;
	// TRANSITION_TO(task_scheduler);
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
}
void task_compute3() {
    uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
    if(state == 0)
    {
        MAT_RESHAPE(b1, 16, 36, 58);
		mat_t *w_ptr = &mat_conv1_wv;
		mat_t *b_ptr = &mat_conv1_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b1, b2);

		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_depthconv)->info.return_task = TASK_REF(task_compute3);
		TRANSITION_TO(task_s_depthconv);
    }
	PRINTF("\r\nCompute 3\r\n");
	scratch_bak[0] = 0;
	write_to_gbuf((uint8_t *)(scratch_bak),
	(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
	state = 0;
	// TRANSITION_TO(task_scheduler);
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
}
void task_compute4() {
    uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
    if(state == 0)
    {
		MAT_RESHAPE(b2, 16, 36, 58);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b2, b1);

		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute4);
		TRANSITION_TO(task_relu);
	}
    else if(state == 1)
    {
        MAT_RESHAPE(b1, 16, 18, 29);
		params.stride[1] = 2;
		params.stride[2] = 2;
		// Assumes src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 2;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_pool)->info.return_task = TASK_REF(task_compute4);
		TRANSITION_TO(task_pool);
    }
    else if(state == 2)
    {
        MAT_RESHAPE(b2, 32, 14, 24);
		params.stride[1] = 1;
		params.stride[2] = 1;
		mat_t *w_ptr = &mat_conv2_w;
		mat_t *b_ptr = &mat_conv2_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b2, b1); // b_ptr was NULL in the original code. Bashima changes it.

		scratch_bak[0] = 3;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_conv)->info.return_task = TASK_REF(task_compute4);
		TRANSITION_TO(task_s_conv);
    }
    else if(state == 4)
    {
        MAT_RESHAPE(b1, 32, 14, 24);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 5;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute4);
		TRANSITION_TO(task_relu);
    }
    else if(state == 5)
    {
        MAT_RESHAPE(b2, 32, 7, 12);
		params.stride[1] = 2;
		params.stride[2] = 2;
		// Assumes src in that order
		PUSH_STACK(mat_stack, b2, b1);

		scratch_bak[0] = 6;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_pool)->info.return_task = TASK_REF(task_compute4);
		TRANSITION_TO(task_pool);
    }
	else
    {
        PRINTF("\r\nCompute 4\r\n");
		scratch_bak[0] = 0;
		write_to_gbuf((uint8_t *)(scratch_bak),
		(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		state = 0;
		setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
		// TRANSITION_TO(task_scheduler);
    }

}

__fram fixed max = 0;
__fram uint16_t predict = 0;
void task_compute5() {
    uint16_t state;
	state = CUR_SCRATCH[0];
	GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN2);
    if(state == 0)
    {
        MAT_RESHAPE(b1, 64, 3, 8);
		params.stride[1] = 1;
		params.stride[2] = 1;
		mat_t *w_ptr = &mat_conv3_w;
		mat_t *b_ptr = &mat_conv3_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b1, b2); // b_ptr was NULL in the original code. Bashima changes it.

		scratch_bak[0] = 1;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_conv)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_s_conv);
    }
    else if(state == 1)
    {
        MAT_RESHAPE(b2, 64, 3, 8);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b2, b1);

		scratch_bak[0] = 2;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_relu);
    }
    else if(state == 2)
    {
        MAT_RESHAPE(b1, 64, 1, 4);
		params.stride[1] = 2;
		params.stride[2] = 2;
		// Assumes src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 3;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_pool)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_pool);
    }
    else if(state == 3)
    {
        MAT_RESHAPE(b2, 256, 1);
		MAT_RESHAPE(b1, 128, 1);
		mat_t *w_ptr = &mat_fc1_wh;
		mat_t *b_ptr = NULL;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b1, b2);

		scratch_bak[0] = 4;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_fc)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_s_fc);
    }
	else if(state == 4)
    {
        MAT_RESHAPE(b2, 96, 1);
		mat_t *w_ptr = &mat_fc1_wv;
		mat_t *b_ptr = &mat_fc1_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b2, b1);

		scratch_bak[0] = 5;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_fc)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_s_fc);
    }
	else if(state == 5) {
		MAT_RESHAPE(b1, 96, 1);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 6;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_relu);
	} else if(state == 6) {

		MAT_RESHAPE(b1, 96, 1);
		// Assumes dest, src in that order
		PUSH_STACK(mat_stack, b1, b2);

		scratch_bak[0] = 7;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_relu)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_relu);
	}
	else if(state == 7) {

		MAT_RESHAPE(b2, 10, 1);
		mat_t *w_ptr = &mat_fc2_w;
		mat_t *b_ptr = &mat_fc2_b;
		// Assumes b, w, dest, src in that order
		PUSH_STACK(mat_stack, b_ptr, w_ptr, b2, b1);

		scratch_bak[0] = 8;
		write_to_gbuf((uint8_t *)(scratch_bak),
			(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		TASK_REF(task_s_fc)->info.return_task = TASK_REF(task_compute5);
		TRANSITION_TO(task_s_fc);
	}
	else if(state == 8)
	{
		fixed max = 0;
		for(uint16_t i = CUR_SCRATCH[0]; i < 10; i = i+1) {
			fixed v = MAT_GET(b2, i, 0);
			if(v > max) {
				predict = i;
				max = v;
			}
			// PRINTF("\r\n%u => %i", i, v);
		}
		PRINTF("\r\nCompute 5\r\n");
		scratch_bak[0] = 0;
		write_to_gbuf((uint8_t *)(scratch_bak),
		(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
		state = 0;
		setup_cleanup(CUR_TASK);
		TRANSITION_TO(task_cleanup);
	}

    // TRANSITION_TO(task_finish);
}



// void task_finish()
// {
//     fixed max = 0;
//     for(uint16_t i = CUR_SCRATCH[0]; i < 10; i = ++CUR_SCRATCH[0]) {
// 		fixed v = MAT_GET(b2, i, 0);
// 		if(v > max) {
// 			predict = i;
// 			max = v;
// 		}
// 		// PRINTF("\r\n%u => %i", i, v);
// 	}
// 	PRINTF("\r\nCompute 5\r\n");
//     scratch_bak[0] = 0;
// 	CUR_SCRATCH[0] = 0;
// 	write_to_gbuf((uint8_t *)(scratch_bak),
// 		(uint8_t *)(CUR_SCRATCH), sizeof(uint16_t));
// 	flag_task = 1;
//     // TRANSITION_TO(task_scheduler);
// 	// TRANSITION_TO(task_exit);
// 	setup_cleanup(CUR_TASK);
// 	TRANSITION_TO(task_cleanup);
// }

void task_exit() {
	PRINTF("\r\n=========Exiting============ %u", deadline_miss);

	GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN2);
	deadline_miss = 0;
	// exit(0);
	msp_sleep(1000);
	get_periods();
	setup_cleanup(CUR_TASK);
	TRANSITION_TO(task_cleanup);
	// TRANSITION_TO(task_scheduler);
}