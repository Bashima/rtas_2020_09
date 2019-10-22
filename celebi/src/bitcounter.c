#include "headers/bitcounter.h"

uint16_t bit_count(uint32_t bc_seed, uint16_t n_0) {
	uint32_t tmp_seed = bc_seed;
	// bc_seed = bc_seed + 13;
	unsigned temp = 0;
	if(tmp_seed) do 
		temp++;
	while (0 != (tmp_seed = tmp_seed&(tmp_seed-1)));
	n_0 += temp;
    return n_0;
	// bc_iter++;
}

uint16_t bitcount(uint32_t bc_seed, uint16_t n_1)
{
	uint32_t tmp_seed = bc_seed;
	// bc_seed = bc_seed + 13;
	tmp_seed = ((tmp_seed & 0xAAAAAAAAL) >>  1) + (tmp_seed & 0x55555555L);
	tmp_seed = ((tmp_seed & 0xCCCCCCCCL) >>  2) + (tmp_seed & 0x33333333L);
	tmp_seed = ((tmp_seed & 0xF0F0F0F0L) >>  4) + (tmp_seed & 0x0F0F0F0FL);
	tmp_seed = ((tmp_seed & 0xFF00FF00L) >>  8) + (tmp_seed & 0x00FF00FFL);
	tmp_seed = ((tmp_seed & 0xFFFF0000L) >> 16) + (tmp_seed & 0x0000FFFFL);
	n_1 += (int)tmp_seed;
	// bc_iter++;
    return n_1;
}

int recursive_cnt(uint32_t x){
	int cnt = bc_bits[(int)(x & 0x0000000FL)];

	if (0L != (x >>= 4))
		cnt += recursive_cnt(x);

	return cnt;
}

uint16_t task_ntbl_bitcnt(uint32_t bc_seed, uint16_t n_2) {
	n_2 += recursive_cnt(bc_seed);	
	// bc_seed = bc_seed + 13;
	// bc_iter++;
    return n_2;
}

uint16_t task_ntbl_bitcount(uint32_t bc_seed, uint16_t n_3) {
	n_3 += bc_bits[ (int) (bc_seed & 0x0000000FUL)       ] +
		bc_bits[ (int)((bc_seed & 0x000000F0UL) >> 4) ] +
		bc_bits[ (int)((bc_seed & 0x00000F00UL) >> 8) ] +
		bc_bits[ (int)((bc_seed & 0x0000F000UL) >> 12)] +
		bc_bits[ (int)((bc_seed & 0x000F0000UL) >> 16)] +
		bc_bits[ (int)((bc_seed & 0x00F00000UL) >> 20)] +
		bc_bits[ (int)((bc_seed & 0x0F000000UL) >> 24)] +
		bc_bits[ (int)((bc_seed & 0xF0000000UL) >> 28)];
	// bc_seed = bc_seed + 13;
	// bc_iter++;
    return n_3;
}

uint16_t task_BW_btbl_bitcount(uint32_t bc_seed, uint16_t n_4) {
	union 
	{ 
		unsigned char ch[4]; 
		long y; 
	} U; 

	U.y = bc_seed; 

	n_4 += bc_bits[ U.ch[0] ] + bc_bits[ U.ch[1] ] + 
		bc_bits[ U.ch[3] ] + bc_bits[ U.ch[2] ]; 
	// bc_seed = bc_seed + 13;
	// bc_iter++;
    return n_4;
}

uint16_t task_AR_btbl_bitcount(uint32_t bc_seed, uint16_t n_5) {
	unsigned char * Ptr = (unsigned char *) &bc_seed ;
	int Accu ;

	Accu  = bc_bits[ *Ptr++ ];
	Accu += bc_bits[ *Ptr++ ];
	Accu += bc_bits[ *Ptr++ ];
	Accu += bc_bits[ *Ptr ];
	n_5+= Accu;
	// bc_seed = bc_seed + 13;
	// bc_iter++;
    return n_5;
}

uint16_t task_bit_shifter(uint32_t bc_seed, uint16_t n_6) {
	int i, nn;
	uint32_t tmp_seed = bc_seed;
	for (i = nn = 0; tmp_seed && (i < (sizeof(long) * CHAR_BIT)); ++i, tmp_seed >>= 1)
		nn += (int)(tmp_seed & 1L);
	n_6 += nn;
	// bc_seed = bc_seed + 13;

	// bc_iter++;
    return n_6;
}