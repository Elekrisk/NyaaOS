#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PT_NULL 0x00000000
#define PT_LOAD 0x00000001
#define PT_DYNAMIC 0x00000002
#define PT_INTERP 0x00000003
#define PT_NOTE 0x00000004
#define PT_SHLIB 0x00000005
#define PT_PHDR 0x00000006

enum Class {
    Class32 = 1,
    Class64 = 2,
};

enum Endianness {
    LittleEndian = 1,
    BigEndian = 2,
};

struct Elf64 {
    uint8_t magic[4];
    uint8_t elf_class;
    uint8_t endianness;
    uint8_t version;
    uint8_t abi;
    uint8_t abi_version;
    uint8_t _padding[7];
    uint16_t object_type;
    uint16_t machine;
    uint32_t version2;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
};

struct ProgramHeader {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
};

struct SectionHeader {
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
    uint64_t link;
    uint64_t info;
    uint64_t addralign;
    uint64_t entsize;
};

struct ProgramHeaderIter {
    struct ProgramHeader* next;
    size_t stride;
    size_t headers_left;
};
struct SectionHeaderIter {
    struct SectionHeader* next;
    size_t stride;
    size_t headers_left;
};

void print_elf(struct Elf64 *elf, int lines_per_screen);
bool verify_elf(struct Elf64 *elf);
struct ProgramHeaderIter program_header_iter(struct Elf64 *elf);
struct ProgramHeader* ph_next(struct ProgramHeaderIter *iter);
struct SectionHeaderIter section_header_iter(struct Elf64 *elf);
struct SectionHeader* sh_next(struct SectionHeaderIter *iter);

#endif // ELF_H