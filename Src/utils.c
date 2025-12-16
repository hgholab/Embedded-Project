#include "utils.h"

uint32_t arr_to_num(int n, char arr[n], int min, int max)
{
    char *p = arr + n - 1;
    uint32_t result = 0;
    uint32_t place = 1;

    while (p >= arr)
    {
        result += (*p - '0') * place;
        place *= 10;
        p--;
    }

    result = CLAMP(result, min, max);

    return result;
}