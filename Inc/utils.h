#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>


#define CLAMP(x,min,max)           (((x)<(min))?(min):(((x)>(max))?(max):(x)))


uint32_t arr_to_num(int n, char arr[n], int min, int max);

#endif
