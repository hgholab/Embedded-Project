#include <ctype.h>
#include <string.h>

#include "utils.h"

uint32_t str_to_uint32(const char *str)
{
    const char *p = str;

    while (*str && *str != '.')
    {
        str++;
    }
    str--;

    uint32_t result = 0;
    uint32_t place = 1U;

    while (str >= p)
    {
        result += (uint32_t)(*str - '0') * place;
        place *= 10U;
        str--;
    }

    return result;
}

int32_t str_to_int32(const char *str)
{
    if (*str == '-')
    {
        return (-1) * (int32_t)str_to_uint32(str + 1);
    }
    else if (*str == '+')
    {
        return (int32_t)str_to_uint32(str + 1);
    }

    return (int32_t)(str_to_uint32(str));
}

void str_to_lower(char *str)
{
    while (*str)
    {
        *str = (char)tolower((unsigned char)*str);
        str++;
    }
}

/**
 * This function is used for converting the numeric values
 * entered by the user in the configuration mode from string
 * to float values.
 */
float str_to_float(const char *str)
{
    float result = 0.0f;
    const char *p = str;

    // First, suppose there is no decimal point in the number (index -1).
    int decimal_point_index = -1;

    // Then, find the position of the decimal point in the number.
    int i = 0;
    while (*str)
    {
        if (*str != '.')
        {
            i++;
            str++;
        }
        else
        {
            decimal_point_index = i;
            break;
        }
    }

    // No decimal point => integer conversion is enough
    if (decimal_point_index == -1)
    {
        return (float)str_to_int32(p);
    }
    else
    {
        // Integer part (str_to_int32 handles + / -)
        int32_t integer_part = str_to_int32(p);

        // Fractional part starts after '.'
        const char *frac_str = p + decimal_point_index + 1;

        // Number of digits after the decimal point
        int frac_len = (int)strlen(frac_str);

        uint32_t frac_digits = 0U;
        if (frac_len > 0)
        {
            frac_digits = str_to_uint32(frac_str);
        }

        // Scale fractional digits by 10^(-frac_len)
        float scale = 1.0f;
        while (frac_len > 0)
        {
            scale *= 10.0f;
            frac_len--;
        }

        float frac_part = (float)frac_digits / scale;

        // Combine integer and fractional parts
        if (integer_part < 0)
        {
            result = (float)integer_part - frac_part;
        }
        else
        {
            result = (float)integer_part + frac_part;
        }

        return result;
    }
}