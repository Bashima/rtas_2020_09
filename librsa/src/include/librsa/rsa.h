#ifndef RSA_H
#define RSA_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <libalpaca/alpaca.h>
#include <libio/console.h>

int16_t gcd(int16_t a, int16_t b);
void fast_exponentiation(int16_t bit, int16_t n, int16_t* y, int16_t* a);
int16_t find_T(int16_t a, int16_t m, int16_t n);
int16_t primary_test(int16_t a, int16_t i);
int16_t inverse(int16_t a, int16_t b);
// int16_t key_generation();
int16_t key_generation1();
int16_t key_generation2();
void key_generation3(int16_t p, int16_t q, int16_t *ed);
void key_generation4(int16_t *ed);

#endif