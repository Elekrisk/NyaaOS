#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "stdint.h"
#include "stddef.h"

#define VERSION 1

struct Framebuffer {
    uint32_t *base;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
};

enum PhysMemoryType {
    PhysFree,
    PhysReserved,
};

struct PhysMemoryMap {
    struct PhysMemoryMap* entries;
    size_t entry_count;
};

struct PhysMemoryMapEntry {
    uint64_t start_frame;
    uint64_t frame_count;
    enum PhysMemoryType type;
};

struct BootInfo {
    uint32_t version;
    struct Framebuffer framebuffer;
    struct PhysMemoryMap phys_memory_map;
};

#endif