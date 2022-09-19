#ifndef CONSOLE_H
#define CONSOLE_H

#include "../Include/Uefi.h"
#include "../Include/Protocol/GraphicsOutput.h"

void init_console(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop);
void clear(void);

#endif // CONSOLE_H