#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define CLAMP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

uint32_t str_to_uint32(const char *str);
int32_t str_to_int32(const char *str);
float str_to_float(const char *str);

void str_to_lower(char *str);

#endif
