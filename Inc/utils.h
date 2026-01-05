#ifndef UTILS_H
#define UTILS_H

#define CLAMP(x, min, max) (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))
#define ARRAY_LEN(a)       (sizeof(a) / sizeof((a)[0]))
#define ABS_FLOAT(x)       (((x) > 0.0f) ? (x) : (-1.0f * (x)))

float str_to_float(const char *str);
void str_to_lower(char *str);

#endif
