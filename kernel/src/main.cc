#include "../../common/bootinfo.h"

template <typename T>
void consume(T){}

extern "C" int kmain(BootInfo* boot_info) {
    consume(boot_info);
    return 0;
}
