#ifndef CR3_H
#define CR3_H

#include "stdint.h"

uint64_t read_cr3(void);
void write_cr3(uint64_t);

#endif