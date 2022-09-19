#include "elf.h"

#include <stdbool.h>
#include <stddef.h>

#include "st.h"
#include "printf.h"

struct ProgramHeaderIter program_header_iter(struct Elf64 *elf)
{
    struct ProgramHeaderIter ret = {
        .next = (struct ProgramHeader *)((uint8_t *)elf + elf->phoff),
        .stride = elf->phentsize,
        .headers_left = elf->phnum};
    return ret;
}

struct ProgramHeader* ph_next(struct ProgramHeaderIter *iter)
{
    if (iter->headers_left == 0) {
        return NULL;
    }
    struct ProgramHeader *next = iter->next;
    iter->next = (struct ProgramHeader*)((uint8_t*)iter->next + iter->stride);
    iter->headers_left--;
    return next;
}

struct SectionHeaderIter section_header_iter(struct Elf64 *elf)
{
    struct SectionHeaderIter ret = {
        .next = (struct SectionHeader *)((uint8_t *)elf + elf->shoff),
        .stride = elf->phentsize,
        .headers_left = elf->phnum};
    return ret;
}

struct SectionHeader* sh_next(struct SectionHeaderIter *iter)
{
    if (iter->headers_left == 0) {
        return NULL;
    }
    struct SectionHeader *next = iter->next;
    iter->next = iter->next + iter->stride;
    iter->headers_left--;
    return next;
}

#define ASSERT(a, b, msg...)                           \
    {                                                  \
        if ((a) != (b))                                \
        {                                              \
            printf(u"Validation of ELF failed: " msg); \
            return false;                              \
        }                                              \
    }
#define ASSERT_EXPECTED(a, b, msg, fmt_a, fmt_b) ASSERT(a, b, msg "; expected " fmt_b ", got " fmt_a, b, a)

bool verify_elf(struct Elf64 *elf)
{
    uint32_t expected_magic;
    {
        uint8_t magic[4] = "\x7f"
                           "ELF";
        expected_magic = *(uint32_t *)magic;
    }
    uint32_t magic = *(uint32_t *)elf->magic;
    ASSERT_EXPECTED(magic, expected_magic, "Invalid magic", "%.8x", "%.8x");
    ASSERT(elf->elf_class, Class64, "ELF must be 64-bit");
    ASSERT(elf->endianness, LittleEndian, "ELF must be little-endian");
    ASSERT(elf->version, 1, "ELF must be v1");
    ASSERT(elf->abi, 0x00, "ABI must be System V");
    ASSERT(elf->object_type, 0x02, "ELF must be Executable");
    ASSERT(elf->machine, 0x3E, "Machine must be AMD64");
    ASSERT(elf->version2, 1, "ELF must be v1");

    return true;
}

const uint16_t *class_name(uint8_t class)
{
    switch (class)
    {
    case 1:
        return u"32-bit";
    case 2:
        return u"64-bit";
    default:
        return u"Unknown";
    }
}

const uint16_t *endianness_name(uint8_t endianness)
{
    switch (endianness)
    {
    case 1:
        return u"Little endian";
    case 2:
        return u"Big endan";
    default:
        return u"Unknown";
    }
}

const uint16_t *abi_name(uint8_t abi)
{
    switch (abi)
    {
    case 0x00:
        return u"System V";
    case 0x01:
        return u"HP-UX";
    case 0x02:
        return u"NetBSD";
    case 0x03:
        return u"Linux";
    case 0x04:
        return u"GNU Hurd";
    case 0x06:
        return u"Solaris";
    case 0x07:
        return u"AIX (Monterey)";
    case 0x08:
        return u"IRIX";
    case 0x09:
        return u"FreeBSD";
    case 0x0A:
        return u"Tru64";
    case 0x0B:
        return u"Novell Modesto";
    case 0x0C:
        return u"OpenBSD";
    case 0x0D:
        return u"OpenVMS";
    case 0x0E:
        return u"NonStop Kernel";
    case 0x0F:
        return u"AROS";
    case 0x10:
        return u"FenixOS";
    case 0x11:
        return u"Nuxi CloudABI";
    case 0x12:
        return u"Stratus Technologies OpenVOS";
    default:
        return u"Unknown";
    }
}

const uint16_t *type_name(uint16_t type)
{
    switch (type)
    {
    case 0x00:
        return u"Unknown.";
    case 0x01:
        return u"Relocatable file.";
    case 0x02:
        return u"Executable file.";
    case 0x03:
        return u"Shared object.";
    case 0x04:
        return u"Core file.";
    default:
        if (type >= 0xFE00 && type < 0xFF00)
            return u"Reserved inclusive range. Operating system specific.";
        else if (type >= 0xFF00)
            return u"Reserved inclusive range. Processor specific.";
        else
            return u"Unknown";
    }
}

const uint16_t *machine_name(uint16_t machine)
{
    switch (machine)
    {
    case 0x00:
        return u"No specific instruction set";
    case 0x01:
        return u"AT&T WE 32100";
    case 0x02:
        return u"SPARC";
    case 0x03:
        return u"x86";
    case 0x04:
        return u"Motorola 68000 (M68k)";
    case 0x05:
        return u"Motorola 88000 (M88k)";
    case 0x06:
        return u"Intel MCU";
    case 0x07:
        return u"Intel 80860";
    case 0x08:
        return u"MIPS";
    case 0x09:
        return u"IBM System/370";
    case 0x0A:
        return u"MIPS RS3000 Little-endian";
    case 0x0E:
        return u"Hewlett-Packard PA-RISC";
    case 0x0F:
        return u"Reserved for future use";
    case 0x13:
        return u"Intel 80960";
    case 0x14:
        return u"PowerPC";
    case 0x15:
        return u"PowerPC (64-bit)";
    case 0x16:
        return u"S390, including S390x";
    case 0x17:
        return u"IBM SPU/SPC";
    case 0x24:
        return u"NEC V800";
    case 0x25:
        return u"Fujitsu FR20";
    case 0x26:
        return u"TRW RH-32";
    case 0x27:
        return u"Motorola RCE";
    case 0x28:
        return u"ARM (up to ARMv7/Aarch32)";
    case 0x29:
        return u"Digital Alpha";
    case 0x2A:
        return u"SuperH";
    case 0x2B:
        return u"SPARC Version 9";
    case 0x2C:
        return u"Siemens TriCore embedded processor";
    case 0x2D:
        return u"Argonaut RISC Core";
    case 0x2E:
        return u"Hitachi H8/300";
    case 0x2F:
        return u"Hitachi H8/300H";
    case 0x30:
        return u"Hitachi H8S";
    case 0x31:
        return u"Hitachi H8/500";
    case 0x32:
        return u"IA-64";
    case 0x33:
        return u"Stanford MIPS-X";
    case 0x34:
        return u"Motorola ColdFire";
    case 0x35:
        return u"Motorola M68HC12";
    case 0x36:
        return u"Fujitsu MMA Multimedia Accelerator";
    case 0x37:
        return u"Siemens PCP";
    case 0x38:
        return u"Sony nCPU embedded RISC processor";
    case 0x39:
        return u"Denso NDR1 microprocessor";
    case 0x3A:
        return u"Motorola Star*Core processor";
    case 0x3B:
        return u"Toyota ME16 processor";
    case 0x3C:
        return u"STMicroelectronics ST100 processor";
    case 0x3D:
        return u"Advanced Logic Corp. TinyJ embedded processor family";
    case 0x3E:
        return u"AMD x86-64";
    case 0x3F:
        return u"Sony DSP Processor";
    case 0x40:
        return u"Digital Equipment Corp. PDP-10";
    case 0x41:
        return u"Digital Equipment Corp. PDP-11";
    case 0x42:
        return u"Siemens FX66 microcontroller";
    case 0x43:
        return u"STMicroelectronics ST9+ 8/16 bit microcontroller";
    case 0x44:
        return u"STMicroelectronics ST7 8-bit microcontroller";
    case 0x45:
        return u"Motorola MC68HC16 Microcontroller";
    case 0x46:
        return u"Motorola MC68HC11 Microcontroller";
    case 0x47:
        return u"Motorola MC68HC08 Microcontroller";
    case 0x48:
        return u"Motorola MC68HC05 Microcontroller";
    case 0x49:
        return u"Silicon Graphics SVx";
    case 0x4A:
        return u"STMicroelectronics ST19 8-bit microcontroller";
    case 0x4B:
        return u"Digital VAX";
    case 0x4C:
        return u"Axis Communications 32-bit embedded processor";
    case 0x4D:
        return u"Infineon Technologies 32-bit embedded processor";
    case 0x4E:
        return u"Element 14 64-bit DSP Processor";
    case 0x4F:
        return u"LSI Logic 16-bit DSP Processor";
    case 0x8C:
        return u"TMS320C6000 Family";
    case 0xAF:
        return u"MCST Elbrus e2k";
    case 0xB7:
        return u"ARM 64-bits (ARMv8/Aarch64)";
    case 0xDC:
        return u"Zilog Z80";
    case 0xF3:
        return u"RISC-V";
    case 0xF7:
        return u"Berkeley Packet Filter";
    case 0x101:
        return u"WDC 65C816";
    default:
        return u"Unknown";
    }
}

#define PRINT(fmt, msg...)                                                   \
    {                                                                        \
        printf(fmt "\r\n", msg);                                             \
        if (++printed_lines >= lines_per_screen)                             \
        {                                                                    \
            printed_lines = 0;                                               \
            printf(u": Press Enter for next page\r\n");                      \
            st->ConIn->Reset(st->ConIn, false);                              \
            st->BootServices->WaitForEvent(1, &st->ConIn->WaitForKey, NULL); \
        }                                                                    \
    }

#define ELF_PRINT(label, spc, fmt, value) PRINT(u"%-22s %6s " fmt, u"" label ":", spc, value)
#define ELF_PRINT_FMT(label, fmt, value) ELF_PRINT(label, "", fmt, value)
#define ELF_PRINT_STR(label, value) ELF_PRINT_FMT(label, "%s", value)
#define ELF_PRINT_FMT_DESC(label, fmt, value, desc_func) \
    {                                                    \
        uint16_t buffer[8] = {};                         \
        sprintf(buffer, u"[" fmt "]", value);            \
        ELF_PRINT(label, buffer, "%s", desc_func(value)) \
    }

void print_elf(struct Elf64 *elf, int lines_per_screen)
{
    if (lines_per_screen == 0)
    {
        lines_per_screen = INT32_MAX;
    }

    int printed_lines = 0;

    // clang-format off
    ELF_PRINT_FMT       ("Magic",                   "%.8x", *(uint32_t *)elf->magic             );
    ELF_PRINT_FMT_DESC  ("Class",                   "%.2x", elf->elf_class,     class_name      );
    ELF_PRINT_FMT_DESC  ("Endianness",              "%.2x", elf->endianness,    endianness_name );
    ELF_PRINT_FMT       ("Version",                 "%llu", elf->version                        );
    ELF_PRINT_FMT_DESC  ("ABI",                     "%.2x", elf->abi,           abi_name        );
    ELF_PRINT_FMT       ("ABI Version",             "%.2x", elf->abi_version                    );
    ELF_PRINT_FMT_DESC  ("Type",                    "%.4x", elf->object_type,   type_name       );
    ELF_PRINT_FMT_DESC  ("Machine",                 "%.4x", elf->machine,       machine_name    );
    ELF_PRINT_FMT       ("Entry point",             "%16x", elf->entry                          );
    ELF_PRINT_FMT       ("Program header offset",   "%16x", elf->phoff                          );
    ELF_PRINT_FMT       ("Section header offset",   "%16x", elf->shoff                          );
    ELF_PRINT_FMT       ("Flags",                   "%8x",  elf->flags                          );
    ELF_PRINT_FMT       ("Header size",             "%d",   elf->ehsize                         );
    ELF_PRINT_FMT       ("Program entry size",      "%d",   elf->phentsize                      );
    ELF_PRINT_FMT       ("Program entry count",     "%d",   elf->phnum                          );
    ELF_PRINT_FMT       ("Section entry size",      "%d",   elf->shentsize                      );
    ELF_PRINT_FMT       ("Section entry count",     "%d",   elf->shnum                          );
    ELF_PRINT_FMT       ("String section index",    "%d",   elf->shstrndx                       );

    // clang-format on
}