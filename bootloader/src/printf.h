#ifndef PRINTF_H
#define PRINTF_H

#include <stdint.h>


extern struct Printer
{
    void (*printer)(void *, const uint16_t *);
    void *data;
} default_printer;



void printf(const uint16_t *fmt, ...);
void sprintf(uint16_t *str, const uint16_t *fmt, ...);
void wprintf(struct Printer, const uint16_t *fmt, ...);

#endif // PRINTF_H