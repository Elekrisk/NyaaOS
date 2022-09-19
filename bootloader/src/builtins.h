#ifndef BUILTINS_H
#define BUILTINS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void* memset(void* ptr, int value, size_t num);
void* memmove(void* dest, const void* src, size_t num);
size_t strlen(const uint16_t* str);
uint16_t* strcpy(uint16_t* dst, const uint16_t* src);
bool memcmp(void* a, void*b, size_t num);

#endif // BUILTINS_H