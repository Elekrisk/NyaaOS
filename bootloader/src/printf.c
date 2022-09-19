
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "st.h"
#include "builtins.h"
#include "printf.h"

struct SpecifierInfo
{
    bool left_justify;
    bool force_sign;
    bool space_if_no_sign;
    bool force_decorator;
    bool pad_with_zero;

    int width;
    bool precision_specified;
    int precision;
    int length;
    uint16_t specifier;
};

#define BUFFER_LEN 1024

void print_int64_t(struct Printer printer, int64_t val, struct SpecifierInfo *specifiers)
{
    uint16_t buffer[BUFFER_LEN] = {0};
    uint16_t *ptr = &buffer[BUFFER_LEN - 1];

    bool neg = val < 0;
    val = val < 0 ? -val : val;

    if (val < 0)
    {
        *--ptr = val % 10 + '0';
        val /= 10;
        val = -val;
    }
    if (val == 0)
    {
        *--ptr = '0';
    }
    else
    {
        for (; val > 0; val /= 10)
        {
            *--ptr = val % 10 + '0';
        }
    }

    int digits = &buffer[BUFFER_LEN - 1] - ptr;

    for (; digits < specifiers->precision; ++digits)
    {
        *--ptr = '0';
    }

    if (neg)
    {
        *--ptr = '-';
    }
    else if (specifiers->force_sign)
    {
        *--ptr = '+';
    }
    else if (specifiers->space_if_no_sign)
    {
        *--ptr = ' ';
    }
    digits = &buffer[BUFFER_LEN - 1] - ptr;

    if (specifiers->width > digits)
    {
        if (specifiers->pad_with_zero)
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = '0';
            }
        }
        else if (specifiers->left_justify)
        {
            int diff = specifiers->width - digits;
            memmove(&buffer[BUFFER_LEN - 1 - specifiers->width], &buffer[BUFFER_LEN - 1 - digits], digits);
            memset(&buffer[BUFFER_LEN - 1 - diff], ' ', diff);
        }
        else
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = ' ';
            }
        }
    }

    printer.printer(printer.data, ptr);
}

void print_uint64_t(struct Printer printer, uint64_t val, struct SpecifierInfo *specifiers)
{
    uint16_t buffer[BUFFER_LEN] = {0};
    uint16_t *ptr = &buffer[BUFFER_LEN - 1];

    if (val == 0)
    {
        *--ptr = '0';
    }
    else
    {
        for (; val > 0; val /= 10)
        {
            *--ptr = val % 10 + '0';
        }
    }

    int digits = &buffer[BUFFER_LEN - 1] - ptr;

    for (; digits < specifiers->precision; ++digits)
    {
        *--ptr = '0';
    }

    if (specifiers->force_sign)
    {
        *--ptr = '+';
    }
    else if (specifiers->space_if_no_sign)
    {
        *--ptr = ' ';
    }
    digits = &buffer[BUFFER_LEN - 1] - ptr;

    if (specifiers->width > digits)
    {
        if (specifiers->pad_with_zero)
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = '0';
            }
        }
        else if (specifiers->left_justify)
        {
            int diff = specifiers->width - digits;
            memmove(&buffer[BUFFER_LEN - 1 - specifiers->width], &buffer[BUFFER_LEN - 1 - digits], digits);
            memset(&buffer[BUFFER_LEN - 1 - diff], ' ', diff);
        }
        else
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = ' ';
            }
        }
    }

    printer.printer(printer.data, ptr);
}

void print_oct(struct Printer printer, uint64_t val, struct SpecifierInfo *specifiers)
{
    uint16_t buffer[BUFFER_LEN] = {0};
    uint16_t *ptr = &buffer[BUFFER_LEN - 1];

    if (val == 0)
    {
        *--ptr = '0';
    }
    else
    {
        for (; val > 0; val >>= 3)
        {
            *--ptr = val & 0x7 + '0';
        }
    }

    int digits = &buffer[BUFFER_LEN - 1] - ptr;

    for (; digits < specifiers->precision; ++digits)
    {
        *--ptr = '0';
    }

    if (specifiers->force_decorator)
    {
        *--ptr = '0';
    }

    if (specifiers->force_sign)
    {
        *--ptr = '+';
    }
    else if (specifiers->space_if_no_sign)
    {
        *--ptr = ' ';
    }
    digits = &buffer[BUFFER_LEN - 1] - ptr;

    if (specifiers->width > digits)
    {
        if (specifiers->pad_with_zero)
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = '0';
            }
        }
        else if (specifiers->left_justify)
        {
            int diff = specifiers->width - digits;
            memmove(&buffer[BUFFER_LEN - 1 - specifiers->width], &buffer[BUFFER_LEN - 1 - digits], digits);
            memset(&buffer[BUFFER_LEN - 1 - diff], ' ', diff);
        }
        else
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = ' ';
            }
        }
    }

    printer.printer(printer.data, ptr);
}

void print_hex(struct Printer printer, uint64_t val, struct SpecifierInfo *specifiers, bool small)
{
    uint16_t buffer[BUFFER_LEN] = {0};
    uint16_t *ptr = &buffer[BUFFER_LEN - 1];

    uint16_t base = (small ? 'a' : 'A') - 10;

    if (val == 0)
    {
        *--ptr = '0';
    }
    else
    {
        for (; val > 0; val >>= 4)
        {
            uint16_t dig = val & 0xF;
            *--ptr = dig + (dig > 9 ? base : '0');
        }
    }

    int digits = &buffer[BUFFER_LEN - 1] - ptr;

    for (; digits < specifiers->precision; ++digits)
    {
        *--ptr = '0';
    }

    if (specifiers->force_decorator)
    {
        *--ptr = small ? 'x' : 'X';
        *--ptr = '0';
    }

    if (specifiers->force_sign)
    {
        *--ptr = '+';
    }
    else if (specifiers->space_if_no_sign)
    {
        *--ptr = ' ';
    }
    digits = &buffer[BUFFER_LEN - 1] - ptr;

    if (specifiers->width > digits)
    {
        if (specifiers->pad_with_zero)
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = '0';
            }
        }
        else if (specifiers->left_justify)
        {
            int diff = specifiers->width - digits;
            memmove(&buffer[BUFFER_LEN - 1 - specifiers->width], &buffer[BUFFER_LEN - 1 - digits], digits);
            memset(&buffer[BUFFER_LEN - 1 - diff], ' ', diff);
        }
        else
        {
            for (; digits < specifiers->width; ++digits)
            {
                *--ptr = ' ';
            }
        }
    }

    printer.printer(printer.data, ptr);
}

void print_str(struct Printer printer, const uint16_t *str, struct SpecifierInfo *info)
{
    uint16_t buffer[BUFFER_LEN] = {};
    size_t len = strlen(str);

    if (info->precision_specified)
    {
        size_t n = MAX(len, info->precision);
        memmove(buffer, str, n * 2);
        len = n;
    }
    else
    {
        strcpy(buffer, str);
    }

    uint16_t *p = buffer + len;

    if (len < info->width)
    {
        if (info->left_justify)
        {
            for (; p - buffer < info->width;)
                *p++ = ' ';
        }
        else
        {
            size_t shift = info->width - len;
            memmove(buffer + shift, buffer, len * 2);
            for (int i = 0; i < shift; ++i)
                buffer[i] = ' ';
        }
    }

    printer.printer(printer.data, buffer);
}

void wvprintf(struct Printer printer, const uint16_t *fmt, va_list args)
{
    uint16_t buffer[1024] = {0};

    for (uint16_t *ptr = buffer; *fmt != 0;)
    {
        if (ptr >= &buffer[1023])
        {

            printer.printer(printer.data, buffer);
            memset(buffer, 0, 2048);
            ptr = buffer;
        }

        if (*fmt == '%')
        {
            fmt++;

            if (*fmt == '%')
            {
                *ptr = '%';
                ptr++;
                continue;
            }

            struct SpecifierInfo info = {0};

            for (;; fmt++)
            {
                switch (*fmt)
                {
                case '-':
                    info.left_justify = true;
                    break;
                case '+':
                    info.force_sign = true;
                    break;
                case ' ':
                    info.space_if_no_sign = true;
                    break;
                case '#':
                    info.force_decorator = true;
                    break;
                case '0':
                    info.pad_with_zero = true;
                    break;
                default:
                    goto exit_loop;
                }
            }
        exit_loop:;

            // Width

            if (*fmt == '*')
            {
                info.width = va_arg(args, int);
                fmt++;
            }
            else
            {
                for (; *fmt >= '0' && *fmt <= '9'; fmt++)
                {
                    info.width = info.width * 10 + *fmt - '0';
                }
            }

            // Precision

            if (*fmt == '.')
            {
                fmt++;
                info.precision_specified = true;

                if (*fmt == '*')
                {
                    info.precision = va_arg(args, int);
                    fmt++;
                }
                else
                {
                    for (; *fmt >= '0' && *fmt <= '9'; fmt++)
                    {
                        info.precision = info.precision * 10 + *fmt - '0';
                    }
                }
            }

            // Length

#define LEN_NONE 0
#define LEN_HH 1
#define LEN_H 2
#define LEN_L 3
#define LEN_LL 4
#define LEN_J 5
#define LEN_Z 6
#define LEN_T 7
#define LEN_CAPITAL_L 8

            switch (*fmt)
            {
            case 'h':
                fmt++;
                if (*fmt == 'h')
                {
                    info.length = LEN_HH;
                }
                else
                {
                    info.length = LEN_H;
                    fmt--;
                }
                break;
            case 'l':
                fmt++;
                if (*fmt == 'l')
                {
                    info.length = LEN_LL;
                }
                else
                {
                    info.length = LEN_L;
                    fmt--;
                }
                break;
            case 'j':
                info.length = LEN_J;
                break;
            case 'z':
                info.length = LEN_Z;
                break;
            case 't':
                info.length = LEN_T;
                break;
            case 'L':
                info.length = LEN_CAPITAL_L;
                break;
            default:
                fmt--;
                break;
            }
            fmt++;

            // Specifier

            info.specifier = *fmt++;

            switch (info.specifier)
            {
            case 'd':
            case 'i':;
                int64_t val;
                switch (info.length)
                {
                case LEN_NONE:
                    val = va_arg(args, int);
                    break;
                case LEN_HH:
                    val = (signed char)va_arg(args, int);
                    break;
                case LEN_H:
                    val = (short int)va_arg(args, int);
                    break;
                case LEN_L:
                    val = va_arg(args, long int);
                    break;
                case LEN_LL:
                    val = va_arg(args, long long int);
                    break;
                case LEN_J:
                    val = va_arg(args, intmax_t);
                    break;
                case LEN_Z:
                    val = va_arg(args, size_t);
                    break;
                case LEN_T:
                    val = va_arg(args, ptrdiff_t);
                    break;
                default:
                    continue;
                }

                printer.printer(printer.data, buffer);
                memset(buffer, 0, 2048);
                ptr = buffer;
                print_int64_t(printer, val, &info);
                break;
            case 'u':
            case 'o':
            case 'x':
            case 'X':;
                uint64_t uval;
                switch (info.length)
                {
                case LEN_NONE:
                    uval = va_arg(args, unsigned int);
                    break;
                case LEN_HH:
                    uval = (unsigned char)va_arg(args, unsigned int);
                    break;
                case LEN_H:
                    uval = (unsigned short int)va_arg(args, unsigned int);
                    break;
                case LEN_L:
                    uval = va_arg(args, unsigned long int);
                    break;
                case LEN_LL:
                    uval = va_arg(args, unsigned long long int);
                    break;
                case LEN_J:
                    uval = va_arg(args, uintmax_t);
                    break;
                case LEN_Z:
                    uval = va_arg(args, size_t);
                    break;
                case LEN_T:
                    uval = va_arg(args, ptrdiff_t);
                    break;
                default:
                    continue;
                }

                printer.printer(printer.data, buffer);
                memset(buffer, 0, 2048);
                ptr = buffer;
                switch (info.specifier)
                {
                case 'u':
                    print_uint64_t(printer, uval, &info);
                    break;
                case 'o':
                    print_oct(printer, uval, &info);
                    break;
                case 'x':
                    print_hex(printer, uval, &info, true);
                    break;
                case 'X':
                    print_hex(printer, uval, &info, false);
                    break;
                }
                break;
            case 'c':
                *ptr++ = va_arg(args, int);
                break;
            case 's':
                printer.printer(printer.data, buffer);
                memset(buffer, 0, 2048);
                ptr = buffer;
                print_str(printer, va_arg(args, uint16_t*), &info);
                break;
            default:
                *ptr++ = '?';
                break;
            }
        }
        else
        {
            *ptr++ = *fmt++;
        }
    }

    printer.printer(printer.data, buffer);
}

void wprintf(struct Printer printer, const uint16_t* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    wvprintf(printer, fmt, args);
    va_end(args);
}


void st_print(void *data, const uint16_t *str)
{
    st->ConOut->OutputString(st->ConOut, (uint16_t *)str);
}

struct Printer default_printer = {
    .printer = st_print,
    .data = NULL
};

void printf(const uint16_t *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    wvprintf(default_printer, fmt, args);

    va_end(args);
}

void str_print(void *data, const uint16_t *str)
{
    uint16_t **ptr = (uint16_t**)data;
    uint16_t *dst = *ptr;
    size_t len = strlen(str);
    strcpy(dst, str);
    *ptr = dst + len;
}

void sprintf(uint16_t *str, const uint16_t *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    uint16_t *ptr = str;

    struct Printer printer = {
        .printer = str_print,
        .data = (void*)&ptr
    };

    wvprintf(printer, fmt, args);

    va_end(args);
}