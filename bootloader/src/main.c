#include "../Include/Uefi.h"

EFI_STATUS EFIAPI efi_main(IN EFI_HANDLE image_handle, IN EFI_SYSTEM_TABLE* st)
{
    st->ConOut->ClearScreen(st->ConOut);

    st->ConOut->OutputString(st->ConOut, u"Hello, World!");

    while (1) {}

    return EFI_SUCCESS;
}