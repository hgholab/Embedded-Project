#include "uart.h"


// This function retargets printf() to UART.
int _write(int file, char *ptr, int len)
{
    (void)file;
    int DataIdx;

    for (DataIdx = 0; DataIdx < len; DataIdx++)
    {
        uart2_write_char_blocking(ptr[DataIdx]);
    }
    return len;
}