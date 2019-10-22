#include <lof.h>
#include <stdio.h>
#include <stdlib.h>
#include <libfixed/fixed.h>


void quicksort(fixed *distances, __uint16_t first, __uint16_t last, __uint16_t *indices)
{
    __uint16_t i, j, pivot, ind_temp;
    fixed temp;

    if(first < last)
    {
        pivot = first;
        i = first;
        j = last;

        while(i<j)
        {
            while(F_LTE(distances[i], distances[pivot]) && i < last)
            {
                i++;
            }
            while(F_GT(distances[j], distances[pivot]))
            {
                j--;
            }
            if(i<j)
            {
                temp = distances[i];
                ind_temp = indices[i];
                indices[i] = indices[j];
                distances[i] = distances[j];
                indices[j] = indices[ind_temp];
                distances[j] = temp;
            }
        }
        temp = distances[pivot];
        ind_temp = indices[pivot];
        distances[pivot] = distances[j];
        indices[pivot] = indices[j];
        distances[j] = temp;
        indices[j] = ind_temp;
        quicksort(distances, first, j-1, indices);
        quicksort(distances, j+1, last, indices);
    }

}

fixed get_kth_distance(fixed *data, int current_element, fixed *distances, __uint16_t *indices)
{
    // fixed distances[ELEMENT_NUMBER] = {0};
    __uint16_t i = 0;
    for(i = 0; i < ELEMENT_NUMBER; i++)
    {
        fixed temp_diff = F_SUB(data[i], current_element);
        fixed temp_mul = F_MUL(temp_diff, temp_diff);
        distances[i] = F_SQRT(temp_mul);
    }
    quicksort(distances, 0, ELEMENT_NUMBER-1, indices);
    return distances[K-1];
}

void get_reachability_distance(fixed *distances, fixed k_distance, fixed *reachability_distance)
{
    __uint16_t i = 0;
    for(i = 0; i < K; i++)
    {
        if(distances[i]< k_distance)
        {
            reachability_distance[i] = k_distance;
        }
        else
        {
            reachability_distance[i] = distances[i];
        }
    }
}

fixed get_lrd(fixed *distances, fixed *reachability_distance)
{
    __uint16_t i = 0;
    fixed sum = 0;
    for(i = 0; i < K; i++)
    {
        fixed diff = F_SUB(reachability_distance[i], distances[i]);
        sum = F_ADD(sum, diff);
    }
    fixed temp = F_DIV(sum, K);
    fixed lrd = F_DIV(1, temp);
    return lrd;
}

__uint8_t get_lof(fixed *lrds, __uint16_t *indices, fixed lrd)
{

    fixed lof_sum = 0;
    __uint16_t i = 0;

    for (i = 0; i < K; i++)
    {
        __uint16_t ind = indices[i];
        fixed temp_lrd = lrds[ind];
        fixed ratio = F_DIV(lrd, temp_lrd);
        lof_sum = F_ADD(lof_sum, ratio);
    }
    fixed lof = F_DIV(lof_sum, K);
    if (lof > 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
    
}