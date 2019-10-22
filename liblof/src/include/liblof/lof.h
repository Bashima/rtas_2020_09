#ifndef LOF_H
#define LOF_H


#include <stdio.h>
#include <stdlib.h>
#include <libfixed/fixed.h>


#define ELEMENT_NUMBER 100
#define K 80

void quicksort(fixed *distances, __uint16_t first, __uint16_t last, __uint16_t *indices);
fixed get_kth_distance(fixed *data, int current_element, fixed *distances, __uint16_t *indices);
void get_reachability_distance(fixed *distances, fixed k_distance, fixed *reachability_distance);
fixed get_lrd(fixed *distances, fixed *reachability_distance);
__uint8_t get_lof(fixed *lrds, __uint16_t *indices, fixed lrd);

#endif