#include "../Include/Uefi.h"
#include "../Include/Protocol/LoadedImage.h"
#include "../Include/Protocol/SimpleFileSystem.h"
#include "../Include/Guid/FileInfo.h"
#include "../Include/Protocol/GraphicsOutput.h"
#include "../Include/Protocol/PartitionInfo.h"

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "st.h"
#include "builtins.h"
#include "printf.h"
#include "elf.h"
#include "console.h"
#include "cr3.h"
#include "paging.h"
#include "../../common/bootinfo.h"

#define FOREACH(type, next, obj)        \
    {                                   \
        CONCATENATE(type, Iter)         \
        __iter = obj;                   \
        while (1)                       \
        {                               \
            type *item = next(&__iter); \
            if (item == NULL)           \
                break;
#define ENDFOREACH \
    }              \
    }

void print(EFI_SYSTEM_TABLE *st, uint16_t *msg)
{
    st->ConOut->OutputString(st->ConOut, msg);
}

void print_hex_(EFI_SYSTEM_TABLE *st, uint64_t num, int pad_num)
{

    print(st, u"0x");
    if (num == 0)
    {
        print(st, u"0");
        return;
    }

    uint16_t buffer[17] = {0};

    uint16_t *ptr = &buffer[16];

    for (; num > 0; num >>= 4, pad_num--)
    {
        ptr--;
        uint8_t val = num & 0xF;
        if (val > 9)
        {
            *ptr = val - 10 + 'A';
        }
        else
        {
            *ptr = val + '0';
        }
    }

    for (; pad_num > 0; pad_num--)
    {
        ptr--;
        *ptr = '0';
    }

    print(st, ptr);
}

struct GI
{
    EFI_GUID guid;
    uint16_t *name;
} guids[] = {
    {.guid = {0xFEDF8E0C, 0xE147, 0x11E3, {0x99, 0x03, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA}}, .name = u"EFI_BOOT_MANAGER_POLICY_PROTOCOL"},
    {.guid = {0xCAB0E94C, 0xE15F, 0x11E3, {0x91, 0x8D, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA}}, .name = u"EFI_BOOT_MANAGER_POLICY_CONSOLE"},
    {.guid = {0xD04159DC, 0xE15F, 0x11E3, {0xB2, 0x61, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA}}, .name = u"EFI_BOOT_MANAGER_POLICY_NETWORK"},
    {.guid = {0x113B2126, 0xFC8A, 0x11E3, {0xBD, 0x6C, 0xB8, 0xE8, 0x56, 0x2C, 0xBA, 0xFA}}, .name = u"EFI_BOOT_MANAGER_POLICY_CONNECT_ALL"},
    {.guid = {0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}}, .name = u"EFI_ACPI_20_TABLE"},
    {.guid = {0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"ACPI_TABLE"},
    {.guid = {0xeb9d2d32, 0x2d88, 0x11d3, {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"SAL_SYSTEM_TABLE"},
    {.guid = {0xeb9d2d31, 0x2d88, 0x11d3, {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"SMBIOS_TABLE"},
    {.guid = {0xf2fd1544, 0x9794, 0x4a2c, {0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94}}, .name = u"SMBIOS3_TABLE"},
    {.guid = {0xeb9d2d2f, 0x2d88, 0x11d3, {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"MPS_TABLE"},
    {.guid = {0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}}, .name = u"EFI_ACPI_TABLE"},
    {.guid = {0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"ACPI_TABLE"},
    {.guid = {0x87367f87, 0x1119, 0x41ce, {0xaa, 0xec, 0x8b, 0xe0, 0x11, 0x1f, 0x55, 0x8a}}, .name = u"EFI_JSON_CONFIG_DATA_TABLE"},
    {.guid = {0x35e7a725, 0x8dd2, 0x4cac, {0x80, 0x11, 0x33, 0xcd, 0xa8, 0x10, 0x90, 0x56}}, .name = u"EFI_JSON_CAPSULE_DATA_TABLE"},
    {.guid = {0xdbc461c3, 0xb3de, 0x422a, {0xb9, 0xb4, 0x98, 0x86, 0xfd, 0x49, 0xa1, 0xe5}}, .name = u"EFI_JSON_CAPSULE_RESULT_TABLE"},
    {.guid = {0xb1b621d5, 0xf19c, 0x41a5, {0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0}}, .name = u"EFI_DTB_TABLE"},
    {.guid = {0xeb66918a, 0x7eef, 0x402a, {0x84, 0x2e, 0x93, 0x1d, 0x21, 0xc3, 0x8a, 0xe9}}, .name = u"EFI_RT_PROPERTIES_TABLE"},
    {.guid = {0xdcfa911d, 0x26eb, 0x469f, {0xa2, 0x20, 0x38, 0xb7, 0xdc, 0x46, 0x12, 0x20}}, .name = u"EFI_MEMORY_ATTRIBUTES_TABLE"},
    {.guid = {0x36122546, 0xf7e7, 0x4c8f, {0xbd, 0x9b, 0xeb, 0x85, 0x25, 0xb5, 0x0c, 0x0b}}, .name = u"EFI_CONFORMANCE_PROFILES_TABLE"},
    {.guid = {0x523c91af, 0xa195, 0x4382, {0x81, 0x8d, 0x29, 0x5f, 0xe4, 0x00, 0x64, 0x65}}, .name = u"EFI_CONFORMANCE_PROFILES_UEFI_SPEC"},
    {.guid = {0x18633bfc, 0x1735, 0x4217, {0x8a, 0xc9, 0x17, 0x23, 0x92, 0x82, 0xd3, 0xf8}}, .name = u"EFI_BTT_ABSTRACTION"},
    {.guid = {0x6a1ee763, 0xd47a, 0x43b4, {0xaa, 0xbe, 0xef, 0x1d, 0xe2, 0xab, 0x56, 0xfc}}, .name = u"EFI_HII_PACKAGE_LIST_PROTOCOL"},
    {.guid = {0x0de9f0ec, 0x88b6, 0x428f, {0x97, 0x7a, 0x25, 0x8f, 0x1d, 0x0e, 0x5e, 0x72}}, .name = u"EFI_MEMORY_RANGE_CAPSULE"},
    {.guid = {0x39b68c46, 0xf7fb, 0x441b, {0xb6, 0xec, 0x16, 0xb0, 0xf6, 0x98, 0x21, 0xf3}}, .name = u"EFI_CAPSULE_REPORT"},
    {.guid = {0x5B1B31A1, 0x9562, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}, .name = u"EFI_LOADED_IMAGE_PROTOCOL"},
    {.guid = {0xbc62157e, 0x3e33, 0x4fec, {0x99, 0x20, 0x2d, 0x3b, 0x36, 0xd7, 0x50, 0xdf}}, .name = u"EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL"},
    {.guid = {0x09576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}, .name = u"EFI_DEVICE_PATH_PROTOCOL"},
    {.guid = {0xe0c14753, 0xf9be, 0x11d2, {0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"EFI_PC_ANSI"},
    {.guid = {0xdfa66065, 0xb419, 0x11d3, {0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"EFI_VT_100"},
    {.guid = {0x7baec70b, 0x57e0, 0x4c76, {0x8e, 0x87, 0x2f, 0x9e, 0x28, 0x08, 0x83, 0x43}}, .name = u"EFI_VT_100_PLUS"},
    {.guid = {0xad15a0d6, 0x8bec, 0x4acf, {0xa0, 0x73, 0xd0, 0x1d, 0xe7, 0x7e, 0x2d, 0x88}}, .name = u"EFI_VT_UTF8"},
    {.guid = {0x77AB535A, 0x45FC, 0x624B, {0x55, 0x60, 0xF7, 0xB2, 0x81, 0xD1, 0xF9, 0x6E}}, .name = u"EFI_VIRTUAL_DISK"},
    {.guid = {0x3D5ABD30, 0x4175, 0x87CE, {0x6D, 0x64, 0xD2, 0xAD, 0xE5, 0x23, 0xC4, 0xBB}}, .name = u"EFI_VIRTUAL_CD"},
    {.guid = {0x5CEA02C9, 0x4D07, 0x69D3, {0x26, 0x9F, 0x44, 0x96, 0xFB, 0xE0, 0x96, 0xF9}}, .name = u"EFI_PERSISTENT_VIRTUAL_DISK"},
    {.guid = {0x08018188, 0x42CD, 0xBB48, {0x10, 0x0F, 0x53, 0x87, 0xD5, 0x3D, 0xED, 0x3D}}, .name = u"EFI_PERSISTENT_VIRTUAL_CD"},
    {.guid = {0x0379be4e, 0xd706, 0x437d, {0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4}}, .name = u"EFI_DEVICE_PATH_UTILITIES_PROTOCOL"},
    {.guid = {0x8b843e20, 0x8132, 0x4852, {0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c}}, .name = u"EFI_DEVICE_PATH_TO_TEXT_PROTOCOL"},
    {.guid = {0x05c99a21, 0xc70f, 0x4ad2, {0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e}}, .name = u"EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL"},
    {.guid = {0x18A031AB, 0xB443, 0x4D1A, {0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71}}, .name = u"EFI_DRIVER_BINDING_PROTOCOL"},
    {.guid = {0x6b30c738, 0xa391, 0x11d4, {0x9a, 0x3b, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL"},
    {.guid = {0x3bc1b285, 0x8a15, 0x4a82, {0xaa, 0xbf, 0x4d, 0x7d, 0x13, 0xfb, 0x32, 0x65}}, .name = u"EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL"},
    {.guid = {0x4d330321, 0x025f, 0x4aac, {0x90, 0xd8, 0x5e, 0xd9, 0x00, 0x17, 0x3b, 0x63}}, .name = u"EFI_DRIVER_DIAGNOSTICS_PROTOCOL"},
    {.guid = {0x6a7a5cff, 0xe8d9, 0x4f70, {0xba, 0xda, 0x75, 0xab, 0x30, 0x25, 0xce, 0x14}}, .name = u"EFI_COMPONENT_NAME2_PROTOCOL"},
    {.guid = {0x642cd590, 0x8059, 0x4c0a, {0xa9, 0x58, 0xc5, 0xec, 0x07, 0xd2, 0x3c, 0x4b}}, .name = u"EFI_PLATFORM_TO_DRIVER_CONFIGURATION_PROTOCOL"},
    {.guid = {0x345ecc0e, 0x0cb6, 0x4b75, {0xbb, 0x57, 0x1b, 0x12, 0x9c, 0x47, 0x33, 0x3e}}, .name = u"EFI_PLATFORM_TO_DRIVER_CONFIGURATION_CLP"},
    {.guid = {0x5c198761, 0x16a8, 0x4e69, {0x97, 0x2c, 0x89, 0xd6, 0x79, 0x54, 0xf8, 0x1d}}, .name = u"EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL"},
    {.guid = {0xb1ee129e, 0xda36, 0x4181, {0x91, 0xf8, 0x04, 0xa4, 0x92, 0x37, 0x66, 0xa7}}, .name = u"EFI_DRIVER_FAMILY_OVERRIDE_PROTOCOL"},
    {.guid = {0x2a534210, 0x9280, 0x41d8, {0xae, 0x79, 0xca, 0xda, 0x01, 0xa2, 0xb1, 0x27}}, .name = u"EFI_DRIVER_HEALTH_PROTOCOL"},
    {.guid = {0xE5DD1403, 0xD622, 0xC24E, {0x84, 0x88, 0xC7, 0x1B, 0x17, 0xF5, 0xE8, 0x02}}, .name = u"EFI_ADAPTER_INFORMATION_PROTOCOL"},
    {.guid = {0xD7C74207, 0xA831, 0x4A26, {0xB1, 0xF5, 0xD1, 0x93, 0x06, 0x5C, 0xE8, 0xB6}}, .name = u"EFI_ADAPTER_INFO_MEDIA_STATE"},
    {.guid = {0x1FBD2960, 0x4130, 0x41E5, {0x94, 0xAC, 0xD2, 0xCF, 0x03, 0x7F, 0xB3, 0x7C}}, .name = u"EFI_ADAPTER_INFO_NETWORK_BOOT"},
    {.guid = {0x114da5ef, 0x2cf1, 0x4e12, {0x9b, 0xbb, 0xc4, 0x70, 0xb5, 0x52, 0x05, 0xd9}}, .name = u"EFI_ADAPTER_INFO_SAN_MAC_ADDRESS"},
    {.guid = {0x4bd56be3, 0x4975, 0x4d8a, {0xa0, 0xad, 0xc4, 0x91, 0x20, 0x4b, 0x5d, 0x4d}}, .name = u"EFI_ADAPTER_INFO_UNDI_IPV6_SUPPORT"},
    {.guid = {0x8484472f, 0x71ec, 0x411a, {0xb3, 0x9c, 0x62, 0xcd, 0x94, 0xd9, 0x91, 0x6e}}, .name = u"EFI_ADAPTER_INFO_MEDIA_TYPE"},
    {.guid = {0x77af24d1, 0xb6f0, 0x42b9, {0x83, 0xf5, 0x8f, 0xe6, 0xe8, 0x3e, 0xb6, 0xf0}}, .name = u"EFI_ADAPTER_INFO_CDAT_TYPE"},
    {.guid = {0xdd9e7534, 0x7762, 0x4698, {0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa}}, .name = u"EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL"},
    {.guid = {0x387477c1, 0x69c7, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}, .name = u"EFI_SIMPLE_TEXT_INPUT_PROTOCOL"},
    {.guid = {0x387477c2, 0x69c7, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}, .name = u"EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL"},
    {.guid = {0x31878c87, 0x0b75, 0x11d5, {0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"EFI_SIMPLE_POINTER_PROTOCOL"},
    {.guid = {0x8D59D32B, 0xC655, 0x4AE9, {0x9B, 0x15, 0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43}}, .name = u"EFI_ABSOLUTE_POINTER_PROTOCOL"},
    {.guid = {0xBB25CF6F, 0xF1D4, 0x11D2, {0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0xfd}}, .name = u"EFI_SERIAL_IO_PROTOCOL"},
    {.guid = {0x6ad9a60f, 0x5815, 0x4c7c, {0x8a, 0x10, 0x50, 0x53, 0xd2, 0xbf, 0x7a, 0x1b}}, .name = u"EFI_SERIAL_TERMINAL_DEVICE_TYPE"},
    {.guid = {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}, .name = u"EFI_GRAPHICS_OUTPUT_PROTOCOL"},
    {.guid = {0x1c0c34f6, 0xd380, 0x41fa, {0xa0, 0x49, 0x8a, 0xd0, 0x6c, 0x1a, 0x66, 0xaa}}, .name = u"EFI_EDID_DISCOVERED_PROTOCOL"},
    {.guid = {0xbd8c1056, 0x9f36, 0x44ec, {0x92, 0xa8, 0xa6, 0x33, 0x7f, 0x81, 0x79, 0x86}}, .name = u"EFI_EDID_ACTIVE_PROTOCOL"},
    {.guid = {0x48ecb431, 0xfb72, 0x45c0, {0xa9, 0x22, 0xf4, 0x58, 0xfe, 0x04, 0x0b, 0xd5}}, .name = u"EFI_EDID_OVERRIDE_PROTOCOL"},
    {.guid = {0x56EC3091, 0x954C, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}, .name = u"EFI_LOAD_FILE_PROTOCOL"},
    {.guid = {0x4006c0c1, 0xfcb3, 0x403e, {0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d}}, .name = u"EFI_LOAD_FILE2_PROTOCOL"},
    {.guid = {0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}, .name = u"EFI_SIMPLE_FILE_SYSTEM_PROTOCOL"},
    {.guid = {0x1e93e633, 0xd65a, 0x459e, {0xab, 0x84, 0x93, 0xd9, 0xec, 0x26, 0x6d, 0x18}}, .name = u"EFI_TAPE_IO_PROTOCOL"},
    {.guid = {0xCE345171, 0xBA0B, 0x11d2, {0x8e, 0x4F, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}, .name = u"EFI_DISK_IO_PROTOCOL"},
    {.guid = {0x151c8eae, 0x7f2c, 0x472c, {0x9e, 0x54, 0x98, 0x28, 0x19, 0x4f, 0x6a, 0x88}}, .name = u"EFI_DISK_IO2_PROTOCOL"},
    {.guid = {0x964e5b21, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}, .name = u"EFI_BLOCK_IO_PROTOCOL"},
    {.guid = {0xa77b2472, 0xe282, 0x4e9f, {0xa2, 0x45, 0xc2, 0xc0, 0xe2, 0x7b, 0xbc, 0xc1}}, .name = u"EFI_BLOCK_IO2_PROTOCOL"},
    {.guid = {0xa00490ba, 0x3f1a, 0x4b4c, {0xab, 0x90, 0x4f, 0xa9, 0x97, 0x26, 0xa1, 0xe8}}, .name = u"EFI_BLOCK_IO_CRYPTO_PROTOCOL"},
    {.guid = {0x2f87ba6a, 0x5c04, 0x4385, {0xa7, 0x80, 0xf3, 0xbf, 0x78, 0xa9, 0x7b, 0xec}}, .name = u"EFI_BLOCK_IO_CRYPTO_ALGO_AES_XTS"},
    {.guid = {0x689e4c62, 0x70bf, 0x4cf3, {0x88, 0xbb, 0x33, 0xb3, 0x18, 0x26, 0x86, 0x70}}, .name = u"EFI_BLOCK_IO_CRYPTO_ALGO_AES_CBC_MICROSOFT_BITLOCKER"},
    {.guid = {0x95A9A93E, 0xA86E, 0x4926, {0xaa, 0xef, 0x99, 0x18, 0xe7, 0x72, 0xd9, 0x87}}, .name = u"EFI_ERASE_BLOCK_PROTOCOL"},
    {.guid = {0x1d3de7f0, 0x0807, 0x424f, {0xaa, 0x69, 0x11, 0xa5, 0x4e, 0x19, 0xa4, 0x6f}}, .name = u"EFI_ATA_PASS_THRU_PROTOCOL"},
    {.guid = {0xc88b0b6d, 0x0dfc, 0x49a7, {0x9c, 0xb4, 0x49, 0x07, 0x4b, 0x4c, 0x3a, 0x78}}, .name = u"EFI_STORAGE_SECURITY_COMMAND_PROTOCOL"},
    {.guid = {0x52c78312, 0x8edc, 0x4233, {0x98, 0xf2, 0x1a, 0x1a, 0xa5, 0xe3, 0x88, 0xa5}}, .name = u"EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL"},
    {.guid = {0x716ef0d9, 0xff83, 0x4f69, {0x81, 0xe9, 0x51, 0x8b, 0xd3, 0x9a, 0x8e, 0x70}}, .name = u"EFI_SD_MMC_PASS_THRU_PROTOCOL"},
    {.guid = {0xab38a0df, 0x6873, 0x44a9, {0x87, 0xe6, 0xd4, 0xeb, 0x56, 0x14, 0x84, 0x49}}, .name = u"EFI_RAM_DISK_PROTOCOL"},
    {.guid = {0x8cf2f62c, 0xbc9b, 0x4821, {0x80, 0x8d, 0xec, 0x9e, 0xc4, 0x21, 0xa1, 0xa0}}, .name = u"EFI_PARTITION_INFO_PROTOCOL"},
    {.guid = {0xd40b6b80, 0x97d5, 0x4282, {0xbb, 0x1d, 0x22, 0x3a, 0x16, 0x91, 0x80, 0x58}}, .name = u"EFI_NVDIMM_LABEL_PROTOCOL"},
    {.guid = {0xb81bfab0, 0x0eb3, 0x4cf9, {0x84, 0x65, 0x7f, 0xa9, 0x86, 0x36, 0x16, 0x64}}, .name = u"EFI_UFS_DEVICE_CONFIG"},
    {.guid = {0x2F707EBB, 0x4A1A, 0x11d4, {0x9A, 0x38, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}}, .name = u"EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL"},
    {.guid = {0x4cf5b200, 0x68b8, 0x4ca5, {0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x02, 0x9a}}, .name = u"EFI_PCI_IO_PROTOCOL"},
    {.guid = {0x932f47e6, 0x2362, 0x4002, {0x80, 0x3e, 0x3c, 0xd5, 0x4b, 0x13, 0x8f, 0x85}}, .name = u"EFI_SCSI_IO_PROTOCOL"},
    {.guid = {0x143b7632, 0xb81b, 0x4cb7, {0xab, 0xd3, 0xb6, 0x25, 0xa5, 0xb9, 0xbf, 0xfe}}, .name = u"EFI_EXT_SCSI_PASS_THRU_PROTOCOL"},
    {.guid = {0x59324945, 0xec44, 0x4c0d, {0xb1, 0xcd, 0x9d, 0xb1, 0x39, 0xdf, 0x07, 0x0c}}, .name = u"EFI_ISCSI_INITIATOR_NAME_PROTOCOL"},
    {.guid = {0x3e745226, 0x9818, 0x45b6, {0xa2, 0xac, 0xd7, 0xcd, 0x0e, 0x8b, 0xa2, 0xbc}}, .name = u"EFI_USB2_HC_PROTOCOL"},
    {.guid = {0x2B2F68D6, 0x0CD2, 0x44cf, {0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75}}, .name = u"EFI_USB_IO_PROTOCOL"},
    {.guid = {0x32d2963a, 0xfe5d, 0x4f30, {0xb6, 0x33, 0x6e, 0x5d, 0xc5, 0x58, 0x03, 0xcc}}, .name = u"EFI_USBFN_IO_PROTOCOL"},
    {.guid = {0x2755590C, 0x6F3C, 0x42FA, {0x9E, 0xA4, 0xA3, 0xBA, 0x54, 0x3C, 0xDA, 0x25}}, .name = u"EFI_DEBUG_SUPPORT_PROTOCOL"},
    {.guid = {0xEBA4E8D2, 0x3858, 0x41EC, {0xA2, 0x81, 0x26, 0x47, 0xBA, 0x96, 0x60, 0xD0}}, .name = u"EFI_DEBUGPORT_PROTOCOL"},
    {.guid = {0x49152E77, 0x1ADA, 0x4764, {0xB7, 0xA2, 0x7A, 0xFE, 0xFE, 0xD9, 0x5E, 0x8B}}, .name = u"EFI_DEBUG_IMAGE_INFO_TABLE"},
    {.guid = {0xd8117cfe, 0x94a6, 0x11d4, {0x9a, 0x3a, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"EFI_DECOMPRESS_PROTOCOL"},
    {.guid = {0xffe06bdd, 0x6107, 0x46a6, {0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c}}, .name = u"EFI_ACPI_TABLE_PROTOCOL"},
    {.guid = {0xa4c751fc, 0x23ae, 0x4c3e, {0x92, 0xe9, 0x49, 0x64, 0xcf, 0x63, 0xf3, 0x49}}, .name = u"EFI_UNICODE_COLLATION_PROTOCOL2"},
    {.guid = {0xB3F79D9A, 0x436C, 0xDC11, {0xB0, 0x52, 0xCD, 0x85, 0xDF, 0x52, 0x4C, 0xE6}}, .name = u"EFI_REGULAR_EXPRESSION_PROTOCOL"},
    {.guid = {0x5F05B20F, 0x4A56, 0xC231, {0xFA, 0x0B, 0xA7, 0xB1, 0xF1, 0x10, 0x04, 0x1D}}, .name = u"EFI_REGEX_SYNTAX_TYPE_POSIX_EXTENDED"},
    {.guid = {0x63E60A51, 0x497D, 0xD427, {0xC4, 0xA5, 0xB8, 0xAB, 0xDC, 0x3A, 0xAE, 0xB6}}, .name = u"EFI_REGEX_SYNTAX_TYPE_PERL"},
    {.guid = {0x9A473A4A, 0x4CEB, 0xB95A, {0x41, 0x5E, 0x5B, 0xA0, 0xBC, 0x63, 0x9B, 0x2E}}, .name = u"EFI_REGEX_SYNTAX_TYPE_ECMA_262"},
    {.guid = {0x3FD32128, 0x4BB1, 0xF632, {0xBE, 0x4F, 0xBA, 0xBF, 0x85, 0xC9, 0x36, 0x76}}, .name = u"EFI_REGEX_SYNTAX_TYPE_POSIX_EXTENDED_ASCII"},
    {.guid = {0x87DFB76D, 0x4B58, 0xEF3A, {0xF7, 0xC6, 0x16, 0xA4, 0x2A, 0x68, 0x28, 0x10}}, .name = u"EFI_REGEX_SYNTAX_TYPE_PERL_ASCII"},
    {.guid = {0xB2284A2F, 0x4491, 0x6D9D, {0xEA, 0xB7, 0x11, 0xB0, 0x67, 0xD4, 0x9B, 0x9A}}, .name = u"EFI_REGEX_SYNTAX_TYPE_ECMA_262_ASCII"},
    {.guid = {0x13ac6dd1, 0x73d0, 0x11d4, {0xb0, 0x6b, 0x00, 0xaa, 0x00, 0xbd, 0x6d, 0xe7}}, .name = u"EFI_EBC_PROTOCOL"},
    {.guid = {0x86c77a67, 0x0b97, 0x4633, {0xa1, 0x87, 0x49, 0x10, 0x4d, 0x06, 0x85, 0xc7}}, .name = u"EFI_FIRMWARE_MANAGEMENT_PROTOCOL"},
    {.guid = {0x6dcbd5ed, 0xe82d, 0x4c44, {0xbd, 0xa1, 0x71, 0x94, 0x19, 0x9a, 0xd9, 0x2a}}, .name = u"EFI_FIRMWARE_MANAGEMENT_CAPSULE_ID"},
    {.guid = {0xb122a263, 0x3661, 0x4f68, {0x99, 0x29, 0x78, 0xf8, 0xb0, 0xd6, 0x21, 0x80}}, .name = u"EFI_SYSTEM_RESOURCE_TABLE"},
    {.guid = {0x67d6f4cd, 0xd6b8, 0x4573, {0xbf, 0x4a, 0xde, 0x5e, 0x25, 0x2d, 0x61, 0xae}}, .name = u"EFI_JSON_CAPSULE_ID"},
    {.guid = {0xA19832B9, 0xAC25, 0x11D3, {0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}}, .name = u"EFI_SIMPLE_NETWORK_PROTOCOL"},
    {.guid = {0x1ACED566, 0x76ED, 0x4218, {0xBC, 0x81, 0x76, 0x7F, 0x1F, 0x97, 0x7A, 0x89}}, .name = u"EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_31"},
    {.guid = {0x03C4E603, 0xAC28, 0x11d3, {0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}}, .name = u"EFI_PXE_BASE_CODE_PROTOCOL"},
    {.guid = {0x245DCA21, 0xFB7B, 0x11d3, {0x8F, 0x01, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}, .name = u"EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL"},
    {.guid = {0x0b64aab0, 0x5429, 0x11d4, {0x98, 0x16, 0x00, 0xa0, 0xc9, 0x1f, 0xad, 0xcf}}, .name = u"EFI_BIS_PROTOCOL"},
    {.guid = {0xedd35e31, 0x07b9, 0x11d2, {0x83, 0xa3, 0x00, 0xa0, 0xc9, 0x1f, 0xad, 0xcf}}, .name = u"BOOT_OBJECT_AUTHORIZATION_PARMSET"},
    {.guid = {0xba23b311, 0x343d, 0x11e6, {0x91, 0x85, 0x58, 0x20, 0xb1, 0xd6, 0x52, 0x99}}, .name = u"EFI_HTTP_BOOT_CALLBACK_PROTOCOL"},
    {.guid = {0xf36ff770, 0xa7e1, 0x42cf, {0x9e, 0xd2, 0x56, 0xf0, 0xf2, 0x71, 0xf4, 0x4c}}, .name = u"EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x7ab33a91, 0xace5, 0x4326, {0xb5, 0x72, 0xe7, 0xee, 0x33, 0xd3, 0x9f, 0x16}}, .name = u"EFI_MANAGED_NETWORK_PROTOCOL"},
    {.guid = {0xb3930571, 0xbeba, 0x4fc5, {0x92, 0x03, 0x94, 0x27, 0x24, 0x2e, 0x6a, 0x43}}, .name = u"EFI_BLUETOOTH_HC_PROTOCOL"},
    {.guid = {0x388278d3, 0x7b85, 0x42f0, {0xab, 0xa9, 0xfb, 0x4b, 0xfd, 0x69, 0xf5, 0xab}}, .name = u"EFI_BLUETOOTH_IO_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x467313de, 0x4e30, 0x43f1, {0x94, 0x3e, 0x32, 0x3f, 0x89, 0x84, 0x5d, 0xb5}}, .name = u"EFI_BLUETOOTH_IO_PROTOCOL"},
    {.guid = {0x62960cf3, 0x40ff, 0x4263, {0xa7, 0x7c, 0xdf, 0xde, 0xbd, 0x19, 0x1b, 0x4b}}, .name = u"EFI_BLUETOOTH_CONFIG_PROTOCOL"},
    {.guid = {0x898890e9, 0x84b2, 0x4f3a, {0x8c, 0x58, 0xd8, 0x57, 0x78, 0x13, 0xe0, 0xac}}, .name = u"EFI_BLUETOOTH_ATTRIBUTE_PROTOCOL"},
    {.guid = {0x5639867a, 0x8c8e, 0x408d, {0xac, 0x2f, 0x4b, 0x61, 0xbd, 0xc0, 0xbb, 0xbb}}, .name = u"EFI_BLUETOOTH_ATTRIBUTE_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x8f76da58, 0x1f99, 0x4275, {0xa4, 0xec, 0x47, 0x56, 0x51, 0x5b, 0x1c, 0xe8}}, .name = u"EFI_BLUETOOTH_LE_CONFIG_PROTOCOL"},
    {.guid = {0x9e23d768, 0xd2f3, 0x4366, {0x9f, 0xc3, 0x3a, 0x7a, 0xba, 0x86, 0x43, 0x74}}, .name = u"EFI_VLAN_CONFIG_PROTOCOL"},
    {.guid = {0x5d9f96db, 0xe731, 0x4caa, {0xa0, 0x0d, 0x72, 0xe1, 0x87, 0xcd, 0x77, 0x62}}, .name = u"EFI_EAP_PROTOCOL"},
    {.guid = {0xbb62e663, 0x625d, 0x40b2, {0xa0, 0x88, 0xbb, 0xe8, 0x36, 0x23, 0xa2, 0x45}}, .name = u"EFI_EAP_MANAGEMENT_PROTOCOL"},
    {.guid = {0x5e93c847, 0x456d, 0x40b3, {0xa6, 0xb4, 0x78, 0xb0, 0xc9, 0xcf, 0x7f, 0x20}}, .name = u"EFI_EAP_MANAGEMENT2_PROTOCOL"},
    {.guid = {0xe5b58dbb, 0x7688, 0x44b4, {0x97, 0xbf, 0x5f, 0x1d, 0x4b, 0x7c, 0xc8, 0xdb}}, .name = u"EFI_EAP_CONFIGURATION_PROTOCOL"},
    {.guid = {0x0da55bc9, 0x45f8, 0x4bb4, {0x87, 0x19, 0x52, 0x24, 0xf1, 0x8a, 0x4d, 0x45}}, .name = u"EFI_WIRELESS_MAC_CONNECTION_PROTOCOL"},
    {.guid = {0x1b0fb9bf, 0x699d, 0x4fdd, {0xa7, 0xc3, 0x25, 0x46, 0x68, 0x1b, 0xf6, 0x3b}}, .name = u"EFI_WIRELESS_MAC_CONNECTION_II_PROTOCOL"},
    {.guid = {0x45bcd98e, 0x59ad, 0x4174, {0x95, 0x46, 0x34, 0x4a, 0x07, 0x48, 0x58, 0x98}}, .name = u"EFI_SUPPLICANT_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x54fcc43e, 0xaa89, 0x4333, {0x9a, 0x85, 0xcd, 0xea, 0x24, 0x05, 0x1e, 0x9e}}, .name = u"EFI_SUPPLICANT_PROTOCOL"},
    {.guid = {0x00720665, 0x67EB, 0x4a99, {0xBA, 0xF7, 0xD3, 0xC3, 0x3A, 0x1C, 0x7C, 0xC9}}, .name = u"EFI_TCP4_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x65530BC7, 0xA359, 0x410f, {0xB0, 0x10, 0x5A, 0xAD, 0xC7, 0xEC, 0x2B, 0x62}}, .name = u"EFI_TCP4_PROTOCOL"},
    {.guid = {0xec20eb79, 0x6c1a, 0x4664, {0x9a, 0x0d, 0xd2, 0xe4, 0xcc, 0x16, 0xd6, 0x64}}, .name = u"EFI_TCP6_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x46e44855, 0xbd60, 0x4ab7, {0xab, 0x0d, 0xa6, 0x79, 0xb9, 0x44, 0x7d, 0x77}}, .name = u"EFI_TCP6_PROTOCOL"},
    {.guid = {0xc51711e7, 0xb4bf, 0x404a, {0xbf, 0xb8, 0x0a, 0x04, 0x8e, 0xf1, 0xff, 0xe4}}, .name = u"EFI_IP4_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x41d94cd2, 0x35b6, 0x455a, {0x82, 0x58, 0xd4, 0xe5, 0x13, 0x34, 0xaa, 0xdd}}, .name = u"EFI_IP4_PROTOCOL"},
    {.guid = {0x3b95aa31, 0x3793, 0x434b, {0x86, 0x67, 0xc8, 0x07, 0x08, 0x92, 0xe0, 0x5e}}, .name = u"EFI_IP4_CONFIG_PROTOCOL"},
    {.guid = {0x5b446ed1, 0xe30b, 0x4faa, {0x87, 0x1a, 0x36, 0x54, 0xec, 0xa3, 0x60, 0x80}}, .name = u"EFI_IP4_CONFIG2_PROTOCOL"},
    {.guid = {0x2c8759d5, 0x5c2d, 0x66ef, {0x92, 0x5f, 0xb6, 0x6c, 0x10, 0x19, 0x57, 0xe2}}, .name = u"EFI_IP6_PROTOCOL"},
    {.guid = {0x937fe521, 0x95ae, 0x4d1a, {0x89, 0x29, 0x48, 0xbc, 0xd9, 0x0a, 0xd3, 0x1a}}, .name = u"EFI_IP6_CONFIG_PROTOCOL"},
    {.guid = {0xce5e5929, 0xc7a3, 0x4602, {0xad, 0x9e, 0xc9, 0xda, 0xf9, 0x4e, 0xbf, 0xcf}}, .name = u"EFI_IPSEC_CONFIG_PROTOCOL"},
    {.guid = {0xdfb386f7, 0xe100, 0x43ad, {0x9c, 0x9a, 0xed, 0x90, 0xd0, 0x8a, 0x5e, 0x12}}, .name = u"EFI_IPSEC_PROTOCOL"},
    {.guid = {0xa3979e64, 0xace8, 0x4ddc, {0xbc, 0x07, 0x4d, 0x66, 0xb8, 0xfd, 0x09, 0x77}}, .name = u"EFI_IPSEC2_PROTOCOL"},
    {.guid = {0x0faaecb1, 0x226e, 0x4782, {0xaa, 0xce, 0x7d, 0xb9, 0xbc, 0xbf, 0x4d, 0xaf}}, .name = u"EFI_FTP4_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0xeb338826, 0x681b, 0x4295, {0xb3, 0x56, 0x2b, 0x36, 0x4c, 0x75, 0x7b, 0x09}}, .name = u"EFI_FTP4_PROTOCOL"},
    {.guid = {0x952cb795, 0xff36, 0x48cf, {0xa2, 0x49, 0x4d, 0xf4, 0x86, 0xd6, 0xab, 0x8d}}, .name = u"EFI_TLS_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x00ca959f, 0x6cfa, 0x4db1, {0x95, 0xbc, 0xe4, 0x6c, 0x47, 0x51, 0x43, 0x90}}, .name = u"EFI_TLS_PROTOCOL"},
    {.guid = {0x1682fe44, 0xbd7a, 0x4407, {0xb7, 0xc7, 0xdc, 0xa3, 0x7c, 0xa3, 0x92, 0x2d}}, .name = u"EFI_TLS_CONFIGURATION_PROTOCOL"},
    {.guid = {0xf44c00ee, 0x1f2c, 0x4a00, {0xaa, 0x09, 0x1c, 0x9f, 0x3e, 0x08, 0x00, 0xa3}}, .name = u"EFI_ARP_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0xf4b427bb, 0xba21, 0x4f16, {0xbc, 0x4e, 0x43, 0xe4, 0x16, 0xab, 0x61, 0x9c}}, .name = u"EFI_ARP_PROTOCOL"},
    {.guid = {0x9d9a39d8, 0xbd42, 0x4a73, {0xa4, 0xd5, 0x8e, 0xe9, 0x4b, 0xe1, 0x13, 0x80}}, .name = u"EFI_DHCP4_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x8a219718, 0x4ef5, 0x4761, {0x91, 0xc8, 0xc0, 0xf0, 0x4b, 0xda, 0x9e, 0x56}}, .name = u"EFI_DHCP4_PROTOCOL"},
    {.guid = {0x87c8bad7, 0x0595, 0x4053, {0x82, 0x97, 0xde, 0xde, 0x39, 0x5f, 0x5d, 0x5b}}, .name = u"EFI_DHCP6_PROTOCOL"},
    {.guid = {0xb625b186, 0xe063, 0x44f7, {0x89, 0x05, 0x6a, 0x74, 0xdc, 0x6f, 0x52, 0xb4}}, .name = u"EFI_DNS4_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0xae3d28cc, 0xe05b, 0x4fa1, {0xa0, 0x11, 0x7e, 0xb5, 0x5a, 0x3f, 0x14, 0x01}}, .name = u"EFI_DNS4_PROTOCOL"},
    {.guid = {0x7f1647c8, 0xb76e, 0x44b2, {0xa5, 0x65, 0xf7, 0x0f, 0xf1, 0x9c, 0xd1, 0x9e}}, .name = u"EFI_DNS6_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0xca37bc1f, 0xa327, 0x4ae9, {0x82, 0x8a, 0x8c, 0x40, 0xd8, 0x50, 0x6a, 0x17}}, .name = u"EFI_DNS6_PROTOCOL"},
    {.guid = {0xbdc8e6af, 0xd9bc, 0x4379, {0xa7, 0x2a, 0xe0, 0xc4, 0xe7, 0x5d, 0xae, 0x1c}}, .name = u"EFI_HTTP_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x7A59B29B, 0x910B, 0x4171, {0x82, 0x42, 0xA8, 0x5A, 0x0D, 0xF2, 0x5B, 0x5B}}, .name = u"EFI_HTTP_PROTOCOL"},
    {.guid = {0x3E35C163, 0x4074, 0x45DD, {0x43, 0x1E, 0x23, 0x98, 0x9D, 0xD8, 0x6B, 0x32}}, .name = u"EFI_HTTP_UTILITIES_PROTOCOL"},
    {.guid = {0x0DB48A36, 0x4E54, 0xEA9C, {0x9B, 0x09, 0x1E, 0xA5, 0xBE, 0x3A, 0x66, 0x0B}}, .name = u"EFI_REST_PROTOCOL"},
    {.guid = {0x456bbe01, 0x99d0, 0x45ea, {0xbb, 0x5f, 0x16, 0xd8, 0x4b, 0xed, 0xc5, 0x59}}, .name = u"EFI_REST_EX_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x55648b91, 0x0e7d, 0x40a3, {0xa9, 0xb3, 0xa8, 0x15, 0xd7, 0xea, 0xdf, 0x97}}, .name = u"EFI_REST_EX_PROTOCOL"},
    {.guid = {0xa9a048f6, 0x48a0, 0x4714, {0xb7, 0xda, 0xa9, 0xad, 0x87, 0xd4, 0xda, 0xc9}}, .name = u"EFI_REST_JSON_STRUCTURE_PROTOCOL"},
    {.guid = {0x83f01464, 0x99bd, 0x45e5, {0xb3, 0x83, 0xaf, 0x63, 0x05, 0xd8, 0xe9, 0xe6}}, .name = u"EFI_UDP4_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x3ad9df29, 0x4501, 0x478d, {0xb1, 0xf8, 0x7f, 0x7f, 0xe7, 0x0e, 0x50, 0xf3}}, .name = u"EFI_UDP4_PROTOCOL"},
    {.guid = {0x66ed4721, 0x3c98, 0x4d3e, {0x81, 0xe3, 0xd0, 0x3d, 0xd3, 0x9a, 0x72, 0x54}}, .name = u"EFI_UDP6_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x4f948815, 0xb4b9, 0x43cb, {0x8a, 0x33, 0x90, 0xe0, 0x60, 0xb3, 0x49, 0x55}}, .name = u"EFI_UDP6_PROTOCOL"},
    {.guid = {0x02e800be, 0x8f01, 0x4aa6, {0x94, 0x6b, 0xd7, 0x13, 0x88, 0xe1, 0x83, 0x3f}}, .name = u"EFI_MTFTP4_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x78247c57, 0x63db, 0x4708, {0x99, 0xc2, 0xa8, 0xb4, 0xa9, 0xa6, 0x1f, 0x6b}}, .name = u"EFI_MTFTP4_PROTOCOL"},
    {.guid = {0xd9760ff3, 0x3cca, 0x4267, {0x80, 0xf9, 0x75, 0x27, 0xfa, 0xfa, 0x42, 0x23}}, .name = u"EFI_MTFTP6_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0xbf0a78ba, 0xec29, 0x49cf, {0xa1, 0xc9, 0x7a, 0xe5, 0x4e, 0xab, 0x6a, 0x51}}, .name = u"EFI_MTFTP6_PROTOCOL"},
    {.guid = {0x5db12509, 0x4550, 0x4347, {0x96, 0xb3, 0x73, 0xc0, 0xff, 0x6e, 0x86, 0x9f}}, .name = u"EFI_REDFISH_DISCOVER_PROTOCOL"},
    {.guid = {0x7671d9d0, 0x53db, 0x4173, {0xaa, 0x69, 0x23, 0x27, 0xf2, 0x1f, 0x0b, 0xc7}}, .name = u"EFI_AUTHENTICATION_INFO_PROTOCOL"},
    {.guid = {0xd6062b50, 0x15ca, 0x11da, {0x92, 0x19, 0x00, 0x10, 0x83, 0xff, 0xca, 0x4d}}, .name = u"EFI_AUTHENTICATION_CHAP_RADIUS"},
    {.guid = {0xc280c73e, 0x15ca, 0x11da, {0xb0, 0xca, 0x00, 0x10, 0x83, 0xff, 0xca, 0x4d}}, .name = u"EFI_AUTHENTICATION_CHAP_LOCAL"},
    {.guid = {0xa7717414, 0xc616, 0x4977, {0x94, 0x20, 0x84, 0x47, 0x12, 0xa7, 0x35, 0xbf}}, .name = u"EFI_CERT_TYPE_RSA2048_SHA256"},
    {.guid = {0x4aafd29d, 0x68df, 0x49ee, {0x8a, 0xa9, 0x34, 0x7d, 0x37, 0x56, 0x65, 0xa7}}, .name = u"EFI_CERT_TYPE_PKCS7"},
    {.guid = {0xc1c41626, 0x504c, 0x4092, {0xac, 0xa9, 0x41, 0xf9, 0x36, 0x93, 0x43, 0x28}}, .name = u"EFI_CERT_SHA256"},
    {.guid = {0x3c5766e8, 0x269c, 0x4e34, {0xaa, 0x14, 0xed, 0x77, 0x6e, 0x85, 0xb3, 0xb6}}, .name = u"EFI_CERT_RSA2048"},
    {.guid = {0xe2b36190, 0x879b, 0x4a3d, {0xad, 0x8d, 0xf2, 0xe7, 0xbb, 0xa3, 0x27, 0x84}}, .name = u"EFI_CERT_RSA2048_SHA256"},
    {.guid = {0x826ca512, 0xcf10, 0x4ac9, {0xb1, 0x87, 0xbe, 0x01, 0x49, 0x66, 0x31, 0xbd}}, .name = u"EFI_CERT_SHA1"},
    {.guid = {0x67f8444f, 0x8743, 0x48f1, {0xa3, 0x28, 0x1e, 0xaa, 0xb8, 0x73, 0x60, 0x80}}, .name = u"EFI_CERT_RSA2048_SHA1"},
    {.guid = {0x0b6e5233, 0xa65c, 0x44c9, {0x94, 0x07, 0xd9, 0xab, 0x83, 0xbf, 0xc8, 0xbd}}, .name = u"EFI_CERT_SHA224"},
    {.guid = {0xff3e5307, 0x9fd0, 0x48c9, {0x85, 0xf1, 0x8a, 0xd5, 0x6c, 0x70, 0x1e, 0x01}}, .name = u"EFI_CERT_SHA384"},
    {.guid = {0x093e0fae, 0xa6c4, 0x4f50, {0x9f, 0x1b, 0xd4, 0x1e, 0x2b, 0x89, 0xc1, 0x9a}}, .name = u"EFI_CERT_SHA512"},
    {.guid = {0x3bd2a492, 0x96c0, 0x4079, {0xb4, 0x20, 0xfc, 0xf9, 0x8e, 0xf1, 0x03, 0xed}}, .name = u"EFI_CERT_X509_SHA256"},
    {.guid = {0x7076876e, 0x80c2, 0x4ee6, {0xaa, 0xd2, 0x28, 0xb3, 0x49, 0xa6, 0x86, 0x5b}}, .name = u"EFI_CERT_X509_SHA384"},
    {.guid = {0x446dbf63, 0x2502, 0x4cda, {0xbc, 0xfa, 0x24, 0x65, 0xd2, 0xb0, 0xfe, 0x9d}}, .name = u"EFI_CERT_X509_SHA512"},
    {.guid = {0x452e8ced, 0xdfff, 0x4b8c, {0xae, 0x01, 0x51, 0x18, 0x86, 0x2e, 0x68, 0x2c}}, .name = u"EFI_CERT_EXTERNAL_MANAGEMENT"},
    {.guid = {0xd719b2cb, 0x3d3a, 0x4596, {0xa3, 0xbc, 0xda, 0xd0, 0x0e, 0x67, 0x65, 0x6f}}, .name = u"EFI_IMAGE_SECURITY_DATABASE"},
    {.guid = {0xb9c2b4f4, 0xbf5f, 0x462d, {0x8a, 0xdf, 0xc5, 0xc7, 0x0a, 0xc3, 0x5d, 0xad}}, .name = u"EFI_DEVICE_SECURITY_DATABASE"},
    {.guid = {0x3bd2f4ec, 0xe524, 0x46e4, {0xa9, 0xd8, 0x51, 0x01, 0x17, 0x42, 0x55, 0x62}}, .name = u"EFI_HII_STANDARD_FORM"},
    {.guid = {0xe9ca4775, 0x8657, 0x47fc, {0x97, 0xe7, 0x7e, 0xd6, 0x5a, 0x08, 0x43, 0x24}}, .name = u"EFI_HII_FONT_PROTOCOL"},
    {.guid = {0x849e6875, 0xdb35, 0x4df8, {0xb4, 0x1e, 0xc8, 0xf3, 0x37, 0x18, 0x07, 0x3f}}, .name = u"EFI_HII_FONT_EX_PROTOCOL"},
    {.guid = {0x0fd96974, 0x23aa, 0x4cdc, {0xb9, 0xcb, 0x98, 0xd1, 0x77, 0x50, 0x32, 0x2a}}, .name = u"EFI_HII_STRING_PROTOCOL"},
    {.guid = {0x31a6406a, 0x6bdf, 0x4e46, {0xb2, 0xa2, 0xeb, 0xaa, 0x89, 0xc4, 0x09, 0x20}}, .name = u"EFI_HII_IMAGE_PROTOCOL"},
    {.guid = {0x1a1241e6, 0x8f19, 0x41a9, {0xbc, 0x0e, 0xe8, 0xef, 0x39, 0xe0, 0x65, 0x46}}, .name = u"EFI_HII_IMAGE_EX_PROTOCOL"},
    {.guid = {0x9E66F251, 0x727C, 0x418C, {0xBF, 0xD6, 0xC2, 0xB4, 0x25, 0x28, 0x18, 0xEA}}, .name = u"EFI_HII_IMAGE_DECODER_PROTOCOL"},
    {.guid = {0xefefd093, 0x0d9b, 0x46eb, {0xa8, 0x56, 0x48, 0x35, 0x07, 0x00, 0xc9, 0x08}}, .name = u"EFI_HII_IMAGE_DECODER_NAME_JPEG"},
    {.guid = {0xaf060190, 0x5e3a, 0x4025, {0xaf, 0xbd, 0xe1, 0xf9, 0x05, 0xbf, 0xaa, 0x4c}}, .name = u"EFI_HII_IMAGE_DECODER_NAME_PNG"},
    {.guid = {0xf7102853, 0x7787, 0x4dc2, {0xa8, 0xa8, 0x21, 0xb5, 0xdd, 0x05, 0xc8, 0x9b}}, .name = u"EFI_HII_FONT_GLYPH_GENERATOR_PROTOCOL"},
    {.guid = {0xef9fc172, 0xa1b2, 0x4693, {0xb3, 0x27, 0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42}}, .name = u"EFI_HII_DATABASE_PROTOCOL"},
    {.guid = {0x14982a4f, 0xb0ed, 0x45b8, {0xa8, 0x11, 0x5a, 0x7a, 0x9b, 0xc2, 0x32, 0xdf}}, .name = u"EFI_HII_SET_KEYBOARD_LAYOUT_EVENT"},
    {.guid = {0x0a8badd5, 0x03b8, 0x4d19, {0xb1, 0x28, 0x7b, 0x8f, 0x0e, 0xda, 0xa5, 0x96}}, .name = u"EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL"},
    {.guid = {0x587e72d7, 0xcc50, 0x4f79, {0x82, 0x09, 0xca, 0x29, 0x1f, 0xc1, 0xa1, 0x0f}}, .name = u"EFI_HII_CONFIG_ROUTING_PROTOCOL"},
    {.guid = {0x330d4706, 0xf2a0, 0x4e4f, {0xa3, 0x69, 0xb6, 0x6f, 0xa8, 0xd5, 0x43, 0x85}}, .name = u"EFI_HII_CONFIG_ACCESS_PROTOCOL"},
    {.guid = {0xb9d4c360, 0xbcfb, 0x4f9b, {0x92, 0x98, 0x53, 0xc1, 0x36, 0x98, 0x22, 0x58}}, .name = u"EFI_FORM_BROWSER2_PROTOCOL"},
    {.guid = {0x93039971, 0x8545, 0x4b04, {0xb4, 0x5e, 0x32, 0xeb, 0x83, 0x26, 0x04, 0x0e}}, .name = u"EFI_HII_PLATFORM_SETUP_FORMSET"},
    {.guid = {0xf22fc20c, 0x8cf4, 0x45eb, {0x8e, 0x06, 0xad, 0x4e, 0x50, 0xb9, 0x5d, 0xd3}}, .name = u"EFI_HII_DRIVER_HEALTH_FORMSET"},
    {.guid = {0x337f4407, 0x5aee, 0x4b83, {0xb2, 0xa7, 0x4e, 0xad, 0xca, 0x30, 0x88, 0xcd}}, .name = u"EFI_HII_USER_CREDENTIAL_FORMSET"},
    {.guid = {0x790217bd, 0xbecf, 0x485b, {0x91, 0x70, 0x5f, 0xf7, 0x11, 0x31, 0x8b, 0x27}}, .name = u"EFI_HII_REST_STYLE_FORMSET"},
    {.guid = {0x4311edc0, 0x6054, 0x46d4, {0x9e, 0x40, 0x89, 0x3e, 0xa9, 0x52, 0xfc, 0xcc}}, .name = u"EFI_HII_POPUP_PROTOCOL"},
    {.guid = {0x6fd5b00c, 0xd426, 0x4283, {0x98, 0x87, 0x6c, 0xf5, 0xcf, 0x1c, 0xb1, 0xfe}}, .name = u"EFI_USER_MANAGER_PROTOCOL"},
    {.guid = {0xe98adb03, 0xb8b9, 0x4af8, {0xba, 0x20, 0x26, 0xe9, 0x11, 0x4c, 0xbc, 0xe5}}, .name = u"EFI_USER_CREDENTIAL2_PROTOCOL"},
    {.guid = {0x15853d7c, 0x3ddf, 0x43e0, {0xa1, 0xcb, 0xeb, 0xf8, 0x5b, 0x8f, 0x87, 0x2c}}, .name = u"EFI_DEFERRED_IMAGE_LOAD_PROTOCOL"},
    {.guid = {0x85b75607, 0xf7ce, 0x471e, {0xb7, 0xe4, 0x2a, 0xea, 0x5f, 0x72, 0x32, 0xee}}, .name = u"EFI_USER_INFO_ACCESS_SETUP_ADMIN"},
    {.guid = {0x1db29ae0, 0x9dcb, 0x43bc, {0x8d, 0x87, 0x5d, 0xa1, 0x49, 0x64, 0xdd, 0xe2}}, .name = u"EFI_USER_INFO_ACCESS_SETUP_NORMAL"},
    {.guid = {0xbdb38125, 0x4d63, 0x49f4, {0x82, 0x12, 0x61, 0xcf, 0x5a, 0x19, 0x0a, 0xf8}}, .name = u"EFI_USER_INFO_ACCESS_SETUP_RESTRICTED"},
    {.guid = {0x42881c98, 0xa4f3, 0x44b0, {0xa3, 0x9d, 0xdf, 0xa1, 0x86, 0x67, 0xd8, 0xcd}}, .name = u"EFI_HASH_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0xc5184932, 0xdba5, 0x46db, {0xa5, 0xba, 0xcc, 0x0b, 0xda, 0x9c, 0x14, 0x35}}, .name = u"EFI_HASH_PROTOCOL"},
    {.guid = {0xda836f8d, 0x217f, 0x4ca0, {0x99, 0xc2, 0x1c, 0xa4, 0xe1, 0x60, 0x77, 0xea}}, .name = u"EFI_HASH2_SERVICE_BINDING_PROTOCOL"},
    {.guid = {0x55b1d734, 0xc5e1, 0x49db, {0x96, 0x47, 0xb1, 0x6a, 0xfb, 0x0e, 0x30, 0x5b}}, .name = u"EFI_HASH2_PROTOCOL"},
    {.guid = {0xEC3A978D, 0x7C4E, 0x48FA, {0x9A, 0xBE, 0x6A, 0xD9, 0x1C, 0xC8, 0xF8, 0x11}}, .name = u"EFI_KMS_PROTOCOL"},
    {.guid = {0xec8a3d69, 0x6ddf, 0x4108, {0x94, 0x76, 0x73, 0x37, 0xfc, 0x52, 0x21, 0x36}}, .name = u"EFI_KMS_FORMAT_GENERIC_128"},
    {.guid = {0xa3b3e6f8, 0xefca, 0x4bc1, {0x88, 0xfb, 0xcb, 0x87, 0x33, 0x9b, 0x25, 0x79}}, .name = u"EFI_KMS_FORMAT_GENERIC_160"},
    {.guid = {0x70f64793, 0xc323, 0x4261, {0xac, 0x2c, 0xd8, 0x76, 0xf2, 0x7c, 0x53, 0x45}}, .name = u"EFI_KMS_FORMAT_GENERIC_256"},
    {.guid = {0x978fe043, 0xd7af, 0x422e, {0x8a, 0x92, 0x2b, 0x48, 0xe4, 0x63, 0xbd, 0xe6}}, .name = u"EFI_KMS_FORMAT_GENERIC_512"},
    {.guid = {0x43be0b44, 0x874b, 0x4ead, {0xb0, 0x9c, 0x24, 0x1a, 0x4f, 0xbd, 0x7e, 0xb3}}, .name = u"EFI_KMS_FORMAT_GENERIC_1024"},
    {.guid = {0x40093f23, 0x630c, 0x4626, {0x9c, 0x48, 0x40, 0x37, 0x3b, 0x19, 0xcb, 0xbe}}, .name = u"EFI_KMS_FORMAT_GENERIC_2048"},
    {.guid = {0xb9237513, 0x6c44, 0x4411, {0xa9, 0x90, 0x21, 0xe5, 0x56, 0xe0, 0x5a, 0xde}}, .name = u"EFI_KMS_FORMAT_GENERIC_3072"},
    {.guid = {0x2156e996, 0x66de, 0x4b27, {0x9c, 0xc9, 0xb0, 0x9f, 0xac, 0x4d, 0x02, 0xbe}}, .name = u"EFI_KMS_FORMAT_GENERIC_DYNAMIC"},
    {.guid = {0x78be11c4, 0xee44, 0x4a22, {0x9f, 0x05, 0x03, 0x85, 0x2e, 0xc5, 0xc9, 0x78}}, .name = u"EFI_KMS_FORMAT_MD2_128"},
    {.guid = {0xf7ad60f8, 0xefa8, 0x44a3, {0x91, 0x13, 0x23, 0x1f, 0x39, 0x9e, 0xb4, 0xc7}}, .name = u"EFI_KMS_FORMAT_MDC2_128"},
    {.guid = {0xd1c17aa1, 0xcac5, 0x400f, {0xbe, 0x17, 0xe2, 0xa2, 0xae, 0x06, 0x67, 0x7c}}, .name = u"EFI_KMS_FORMAT_MD4_128"},
    {.guid = {0x3fa4f847, 0xd8eb, 0x4df4, {0xbd, 0x49, 0x10, 0x3a, 0x0a, 0x84, 0x7b, 0xbc}}, .name = u"EFI_KMS_FORMAT_MDC4_128"},
    {.guid = {0xdcbc3662, 0x9cda, 0x4b52, {0xa0, 0x4c, 0x82, 0xeb, 0x1d, 0x23, 0x48, 0xc7}}, .name = u"EFI_KMS_FORMAT_MD5_128"},
    {.guid = {0x1c178237, 0x6897, 0x459e, {0x9d, 0x36, 0x67, 0xce, 0x8e, 0xf9, 0x4f, 0x76}}, .name = u"EFI_KMS_FORMAT_MD5SHA_128"},
    {.guid = {0x453c5e5a, 0x482d, 0x43f0, {0x87, 0xc9, 0x59, 0x41, 0xf3, 0xa3, 0x8a, 0xc2}}, .name = u"EFI_KMS_FORMAT_SHA1_160"},
    {.guid = {0x6bb4f5cd, 0x8022, 0x448d, {0xbc, 0x6d, 0x77, 0x1b, 0xae, 0x93, 0x5f, 0xc6}}, .name = u"EFI_KMS_FORMAT_SHA256_256"},
    {.guid = {0x4776e33f, 0xdb47, 0x479a, {0xa2, 0x5f, 0xa1, 0xcd, 0x0a, 0xfa, 0xb3, 0x8b}}, .name = u"EFI_KMS_FORMAT_AESXTS_128"},
    {.guid = {0xdc7e8613, 0xc4bb, 0x4db0, {0x84, 0x62, 0x13, 0x51, 0x13, 0x57, 0xab, 0xe2}}, .name = u"EFI_KMS_FORMAT_AESXTS_256"},
    {.guid = {0xa0e8ee6a, 0x0e92, 0x44d4, {0x86, 0x1b, 0x0e, 0xaa, 0x4a, 0xca, 0x44, 0xa2}}, .name = u"EFI_KMS_FORMAT_AESCBC_128"},
    {.guid = {0xd7e69789, 0x1f68, 0x45e8, {0x96, 0xef, 0x3b, 0x64, 0x07, 0xa5, 0xb2, 0xdc}}, .name = u"EFI_KMS_FORMAT_AESCBC_256"},
    {.guid = {0x56417bed, 0x6bbe, 0x4882, {0x86, 0xa0, 0x3a, 0xe8, 0xbb, 0x17, 0xf8, 0xf9}}, .name = u"EFI_KMS_FORMAT_RSASHA1_1024"},
    {.guid = {0xf66447d4, 0x75a6, 0x463e, {0xa8, 0x19, 0x07, 0x7f, 0x2d, 0xda, 0x05, 0xe9}}, .name = u"EFI_KMS_FORMAT_RSASHA1_2048"},
    {.guid = {0xa477af13, 0x877d, 0x4060, {0xba, 0xa1, 0x25, 0xd1, 0xbe, 0xa0, 0x8a, 0xd3}}, .name = u"EFI_KMS_FORMAT_RSASHA256_2048"},
    {.guid = {0x4e1356c2, 0x0eed, 0x463f, {0x81, 0x47, 0x99, 0x33, 0xab, 0xdb, 0xc7, 0xd5}}, .name = u"EFI_KMS_FORMAT_RSASHA256_3072"},
    {.guid = {0x47889fb2, 0xd671, 0x4fab, {0xa0, 0xca, 0xdf, 0x0e, 0x44, 0xdf, 0x70, 0xd6}}, .name = u"EFI_PKCS7_VERIFY_PROTOCOL"},
    {.guid = {0x3152bca5, 0xeade, 0x433d, {0x86, 0x2e, 0xc0, 0x1c, 0xdc, 0x29, 0x1f, 0x44}}, .name = u"EFI_RNG_PROTOCOL"},
    {.guid = {0xa7af67cb, 0x603b, 0x4d42, {0xba, 0x21, 0x70, 0xbf, 0xb6, 0x29, 0x3f, 0x96}}, .name = u"EFI_RNG_ALGORITHM_SP800_90_HASH_256"},
    {.guid = {0xc5149b43, 0xae85, 0x4f53, {0x99, 0x82, 0xb9, 0x43, 0x35, 0xd3, 0xa9, 0xe7}}, .name = u"EFI_RNG_ALGORITHM_SP800_90_HMAC_256"},
    {.guid = {0x44f0de6e, 0x4d8c, 0x4045, {0xa8, 0xc7, 0x4d, 0xd1, 0x68, 0x85, 0x6b, 0x9e}}, .name = u"EFI_RNG_ALGORITHM_SP800_90_CTR_256"},
    {.guid = {0x63c4785a, 0xca34, 0x4012, {0xa3, 0xc8, 0x0b, 0x6a, 0x32, 0x4f, 0x55, 0x46}}, .name = u"EFI_RNG_ALGORITHM_X9_31_3DES"},
    {.guid = {0xacd03321, 0x777e, 0x4d3d, {0xb1, 0xc8, 0x20, 0xcf, 0xd8, 0x88, 0x20, 0xc9}}, .name = u"EFI_RNG_ALGORITHM_X9_31_AES"},
    {.guid = {0x2a4d1adf, 0x21dc, 0x4b81, {0xa4, 0x2f, 0x8b, 0x8e, 0xe2, 0x38, 0x00, 0x60}}, .name = u"EFI_SMART_CARD_READER_PROTOCOL"},
    {.guid = {0xd317f29b, 0xa325, 0x4712, {0x9b, 0xf1, 0xc6, 0x19, 0x54, 0xdc, 0x19, 0x8c}}, .name = u"EFI_SMART_CARD_EDGE_PROTOCOL"},
    {.guid = {0x9317ec24, 0x7cb0, 0x4d0e, {0x8b, 0x32, 0x2e, 0xd9, 0x20, 0x9c, 0xd8, 0xaf}}, .name = u"EFI_PADDING_RSASSA_PKCS1V1P5"},
    {.guid = {0x7b2349e0, 0x522d, 0x4f8e, {0xb9, 0x27, 0x69, 0xd9, 0x7c, 0x9e, 0x79, 0x5f}}, .name = u"EFI_PADDING_RSASSA_PSS"},
    {.guid = {0x3629ddb1, 0x228c, 0x452e, {0xb6, 0x16, 0x09, 0xed, 0x31, 0x6a, 0x97, 0x00}}, .name = u"EFI_PADDING_NONE"},
    {.guid = {0xe1c1d0a9, 0x40b1, 0x4632, {0xbd, 0xcc, 0xd9, 0xd6, 0xe5, 0x29, 0x56, 0x31}}, .name = u"EFI_PADDING_RSAES_PKCS1V1P5"},
    {.guid = {0xc1e63ac4, 0xd0cf, 0x4ce6, {0x83, 0x5b, 0xee, 0xd0, 0xe6, 0xa8, 0xa4, 0x5b}}, .name = u"EFI_PADDING_RSAES_OAEP"},
    {.guid = {0xf4560cf6, 0x40ec, 0x4b4a, {0xa1, 0x92, 0xbf, 0x1d, 0x57, 0xd0, 0xb1, 0x89}}, .name = u"EFI_MEMORY_ATTRIBUTE_PROTOCOL"},
    {.guid = {0x96751a3d, 0x72f4, 0x41a6, {0xa7, 0x94, 0xed, 0x5d, 0x0e, 0x67, 0xae, 0x6b}}, .name = u"EFI_CC_MEASUREMENT_PROTOCOL"},
    {.guid = {0xdd4a4648, 0x2de7, 0x4665, {0x96, 0x4d, 0x21, 0xd9, 0xef, 0x5f, 0xb4, 0x46}}, .name = u"EFI_CC_FINAL_EVENTS_TABLE"},
    {.guid = {0xafbfde41, 0x2e6e, 0x4262, {0xba, 0x65, 0x62, 0xb9, 0x23, 0x6e, 0x54, 0x95}}, .name = u"EFI_TIMESTAMP_PROTOCOL"},
    {.guid = {0x9da34ae0, 0xeaf9, 0x4bbf, {0x8e, 0xc3, 0xfd, 0x60, 0x22, 0x6c, 0x44, 0xbe}}, .name = u"EFI_RESET_NOTIFICATION_PROTOCOL"},
    {.guid = {}, .name = NULL}};

bool guid_cmp(EFI_GUID *a, EFI_GUID *b)
{
    return memcmp(a, b, sizeof(EFI_GUID));
}

const uint16_t *guid_name(EFI_GUID *guid)
{
    for (struct GI *ptr = guids; ptr->name != NULL; ++ptr)
    {
        if (guid_cmp(guid, &ptr->guid))
        {
            return ptr->name;
        }
    }
    return u"Unknown GUID";
}

const uint16_t *status_msg(EFI_STATUS status)
{
    switch (status)
    {
    case EFI_SUCCESS:
        return u"The operation completed successfully.";
        break;
    case EFI_LOAD_ERROR:
        return u"The image failed to load.";
        break;
    case EFI_INVALID_PARAMETER:
        return u"The parameter was incorrect.";
        break;
    case EFI_UNSUPPORTED:
        return u"The operation is not supported.";
        break;
    case EFI_BAD_BUFFER_SIZE:
        return u"The buffer was not the proper size for the request.";
        break;
    case EFI_BUFFER_TOO_SMALL:
        return u"The buffer was not large enough to hold the requested data. The required buffer size is returned in the appropriate parameter when this error occurs.";
        break;
    case EFI_NOT_READY:
        return u"There is no data pending upon return.";
        break;
    case EFI_DEVICE_ERROR:
        return u"The physical device reported an error while attempting the operation.";
        break;
    case EFI_WRITE_PROTECTED:
        return u"The device can not be written to.";
        break;
    case EFI_OUT_OF_RESOURCES:
        return u"The resource has run out.";
        break;
    case EFI_VOLUME_CORRUPTED:
        return u"An inconsistency was detected on the file system causing the operation to fail.";
        break;
    case EFI_VOLUME_FULL:
        return u"There is no more space on the file system.";
        break;
    case EFI_NO_MEDIA:
        return u"The device does not contain any medium to perform the operation.";
        break;
    case EFI_MEDIA_CHANGED:
        return u"The medium in the device has changed since the last access.";
        break;
    case EFI_NOT_FOUND:
        return u"The item was not found.";
        break;
    case EFI_ACCESS_DENIED:
        return u"Access was denied.";
        break;
    case EFI_NO_RESPONSE:
        return u"The server was not found or did not respond to the request.";
        break;
    case EFI_NO_MAPPING:
        return u"A mapping to the device does not exist.";
        break;
    case EFI_TIMEOUT:
        return u"A timeout time expired.";
        break;
    case EFI_NOT_STARTED:
        return u"The protocol has not been started.";
        break;
    case EFI_ALREADY_STARTED:
        return u"The protocol has already been started.";
        break;
    case EFI_ABORTED:
        return u"The operation was aborted.";
        break;
    case EFI_ICMP_ERROR:
        return u"An ICMP error occurred during the network operation.";
        break;
    case EFI_TFTP_ERROR:
        return u"A TFTP error occurred during the network operation.";
        break;
    case EFI_PROTOCOL_ERROR:
        return u"A protocol error occurred during the network operation.";
        break;
    case EFI_INCOMPATIBLE_VERSION:
        return u"A function encountered an internal version that was incompatible with a version requested by the caller.";
        break;
    case EFI_SECURITY_VIOLATION:
        return u"The function was not performed due to a security violation.";
        break;
    case EFI_CRC_ERROR:
        return u"A CRC error was detected.";
        break;
    case EFI_END_OF_MEDIA:
        return u"The beginning or end of media was reached.";
        break;
    case EFI_END_OF_FILE:
        return u"The end of the file was reached.";
        break;
    case EFI_INVALID_LANGUAGE:
        return u"The language specified was invalid.";
        break;
    case EFI_COMPROMISED_DATA:
        return u"The security status of the data is unknown or compromised and the data must be updated or replaced to restore a valid security status.";
        break;
    case EFI_HTTP_ERROR:
        return u"A HTTP error occurred during the network operation.";
        break;
    case EFI_WARN_UNKNOWN_GLYPH:
        return u"The string contained one or more characters that the device could not render and were skipped.";
        break;
    case EFI_WARN_DELETE_FAILURE:
        return u"The handle was closed, but the file was not deleted.";
        break;
    case EFI_WARN_WRITE_FAILURE:
        return u"The handle was closed, but the data to the file was not flushed properly.";
        break;
    case EFI_WARN_BUFFER_TOO_SMALL:
        return u"The resulting buffer was too small, and the data was truncated to the buffer size.";
        break;
    case EFI_WARN_STALE_DATA:
        return u"The data has not been updated within the timeframe set by local policy for this type of data.";
        break;
    case EFI_WARN_FILE_SYSTEM:
        return u"The resulting buffer contains UEFI-compliant file system.";
        break;
    default:
        return u"Unrecognized status code";
        break;
    }
}

void wait_for_keypress_exit()
{
    printf(u"Press any key to exit...\r\n");
    st->ConIn->Reset(st->ConIn, FALSE);
    EFI_EVENT event = st->ConIn->WaitForKey;
    st->BootServices->WaitForEvent(1, &event, NULL);
}

void print_device_path(EFI_SYSTEM_TABLE *st, EFI_DEVICE_PATH_PROTOCOL *start_node)
{
    EFI_DEVICE_PATH_PROTOCOL *node = start_node;

    while (TRUE)
    {
        uint16_t len = *(uint16_t *)node->Length;

        printf(u"Type: %#.2x - Subtype: %#.2x - Length: %d - Address: %#zx\r\n", node->Type, node->SubType, len, node);

        switch (node->Type)
        {
        case 0x01:
            break;
        case 0x02:
            break;
        case 0x03:
            break;
        case MEDIA_DEVICE_PATH:
            switch (node->SubType)
            {
            case MEDIA_HARDDRIVE_DP:;
                HARDDRIVE_DEVICE_PATH *hdd = (HARDDRIVE_DEVICE_PATH *)node;
                uint32_t part_num = hdd->PartitionNumber;
                uint64_t part_start = hdd->PartitionStart;
                uint64_t part_size = hdd->PartitionSize;
                uint8_t *signature = hdd->Signature;
                uint8_t part_format = hdd->MBRType;
                uint8_t signature_type = hdd->SignatureType;

                break;
            case MEDIA_FILEPATH_DP:;
                FILEPATH_DEVICE_PATH *fp = (FILEPATH_DEVICE_PATH *)node;
                printf(u"Filepath: \"%s\"\r\n", fp->PathName);
            }
            break;
        case 0x05:
            break;
        case 0x7F:
            switch (node->SubType)
            {
            case 0x01:
                printf(u"End of current path\r\n");
                break;
            case 0xFF:
                printf(u"End of entire path\r\n");
                return;
            default:
                printf(u"Unknown subtype of Device Path End\r\n");
                break;
            }
            break;
        default:
            break;
        }

        // st->ConIn->Reset(st->ConIn, FALSE);
        // st->BootServices->WaitForEvent(1, &st->ConIn->WaitForKey, NULL);

        node = (EFI_DEVICE_PATH_PROTOCOL *)((char *)node + len);
    }
}

void tree(EFI_FILE *dir, int indent)
{
    uint8_t buffer[1024] = {};
    EFI_FILE_INFO *info = (EFI_FILE_INFO *)buffer;
    EFI_GUID guid = EFI_FILE_INFO_ID;
    UINTN buffer_size = 1024;
    EFI_STATUS status = dir->GetInfo(dir, &guid, &buffer_size, (void *)buffer);

    for (int i = 0; i < indent; i++)
        printf(u"  ");

    if (status != EFI_SUCCESS)
    {
        printf(u"Error: Could not read file info: %s\r\n", status_msg(status));
        printf(u"Needed size: %llu\r\n", buffer_size);
        return;
    }

    if (!(info->Attribute & EFI_FILE_DIRECTORY))
    {
        printf(u"%s\r\n", info->FileName);
        return;
    }

    printf(u"%s/\r\n", info->FileName);
    while (1)
    {
        buffer_size = 1024;
        status = dir->Read(dir, &buffer_size, (void *)buffer);
        if (status != EFI_SUCCESS)
        {
            for (int i = 0; i < indent + 1; i++)
                printf(u"  ");
            printf(u"Error: Could not read directory entry: %s\r\n", status_msg(status));
            printf(u"Needed size: %llu\r\n", buffer_size);
            return;
        }

        if (buffer_size == 0)
        {
            return;
        }

        if (info->FileName[0] == '.' && (info->FileName[1] == 0 || (info->FileName[1] == '.' && info->FileName[2] == 0)))
        {
            continue;
        }

        EFI_FILE *file = NULL;

        buffer_size = 1024;
        status = dir->Open(dir, &file, info->FileName, EFI_FILE_MODE_READ, 0);
        if (status != EFI_SUCCESS)
        {
            for (int i = 0; i < indent + 1; i++)
                printf(u"  ");
            printf(u"Error: Could not open file: %s\r\n", status_msg(status));
            printf(u"Needed size: %llu\r\n", buffer_size);
            return;
        }

        tree(file, indent + 1);

        file->Close(file);
    }
}
#define ERR(msg...)               \
    {                             \
        printf(msg);              \
        wait_for_keypress_exit(); \
        return EFI_ABORTED;       \
    }
#define CONCATENATE(a, b) a##b
#define CALL(func, msg) CALLF(func, msg "%s", u"")
#define CALLF(func, msg, args...)                               \
    {                                                           \
        EFI_STATUS __status = (func);                           \
        if (__status)                                           \
        {                                                       \
            ERR(u"" msg ": %s\r\n", args, status_msg(__status)) \
        }                                                       \
    }

void print_guid(EFI_GUID *guid)
{
    printf(u"%.8x-%.4x-%.4x-", guid->Data1, guid->Data2, guid->Data3);
    for (int i = 0; i < 2; ++i)
    {
        printf(u"%.2x", guid->Data4[i]);
    }
    printf(u"-");
    for (int i = 2; i < 8; ++i)
    {
        printf(u"%.2x", guid->Data4[i]);
    }
}

#ifdef SENDLOADADDR

#endif

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *serial_out = NULL;
void print_serial(void *data, const uint16_t *str)
{
    serial_out->OutputString(serial_out, (uint16_t *)str);
}

void print_page_table(uint64_t *page_table, int level)
{
    for (int i = 0; i < 512; ++i)
    {
        if (page_table[i])
        {
            for (int j = 0; j < level; ++j)
                printf(u"| ");
            printf(u"%3d: %x\r\n", i, page_table[i]);
            if (level < 3 && !(page_table[i] & (1 << 7)))
            {
                print_page_table((uint64_t *)(page_table[i] & 0x000FFFFFFFFFF000), level + 1);
            }
        }
    }
}

#define PHYS_MEMMAP_ENTRY_COUNT 64
struct PhysMemoryMapEntry physmemmap[PHYS_MEMMAP_ENTRY_COUNT];
int physmemmap_count = 0;

EFI_STATUS EFIAPI efi_main(IN EFI_HANDLE image_handle, IN EFI_SYSTEM_TABLE *system_table)
{
    st = system_table;

    EFI_LOADED_IMAGE_PROTOCOL *interface;

    EFI_GUID protocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    CALL(st->BootServices->OpenProtocol(
             image_handle,
             &protocol_guid,
             (void **)&interface,
             image_handle,
             NULL,
             EFI_OPEN_PROTOCOL_EXCLUSIVE),
         "Error getting load address");

#ifdef SENDLOADADDR
    void *addr = interface->ImageBase;

    printf(u"!!%#llx\r\n", addr);

    st->ConIn->Reset(st->ConIn, false);
    st->BootServices->WaitForEvent(1, &st->ConIn->WaitForKey, NULL);

#endif

    st->ConOut->ClearScreen(st->ConOut);

    printf(u"Hello, World!\r\n");

    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;

    CALL(st->BootServices->OpenProtocol(interface->DeviceHandle, &fs_guid, (void **)&fs, image_handle, NULL, EFI_OPEN_PROTOCOL_EXCLUSIVE), "Error");

    EFI_FILE_PROTOCOL *root = NULL;

    CALL(fs->OpenVolume(fs, &root), "Error opening boot filesystem");

    tree(root, 0);

    EFI_FILE *kfile = NULL;

    EFI_STATUS status = root->Open(root, &kfile, u"kernel.elf", EFI_FILE_MODE_READ, 0);

    switch (status)
    {
    case EFI_SUCCESS:
        break;
    case EFI_NOT_FOUND:
        ERR(u"Kernel file not found\r\n");
    default:
        ERR(u"Error: %s\r\n", status_msg(status));
    }

    EFI_GUID info_type = EFI_FILE_INFO_ID;
    uint8_t buffer[512];
    UINTN buffer_size = 1024;

    CALL(kfile->GetInfo(kfile, &info_type, &buffer_size, (void *)buffer), "Error reading kernel file");

    EFI_FILE_INFO *kernel_info = (EFI_FILE_INFO *)buffer;
    if (kernel_info->Attribute & EFI_FILE_DIRECTORY)
    {
        ERR(u"Kernel file is directory");
    }

    uint64_t kernel_filesize = kernel_info->FileSize;

    uint64_t page_count = (kernel_filesize + 4096 - 1) / 4096;
    EFI_PHYSICAL_ADDRESS address = 0;
    CALL(st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, page_count, &address), "Error allocating memory");

    printf(u"Address for loading data: %#x\r\n", address);
    buffer_size = page_count * 4096;
    CALLF(kfile->Read(kfile, &buffer_size, (void *)address), "Error reading %s", kernel_info->FileName);
    kfile->Close(kfile);

    struct Elf64 *elf = (struct Elf64 *)address;

    if (!verify_elf(elf))
    {
        ERR(u"Error verifying ELF; see above");
    }

    FOREACH(struct ProgramHeader, ph_next, program_header_iter(elf))
    {
        printf(u"Program header:\r\n");
        printf(u"  Type: ");
        switch (item->type)
        {
        case 0:
            printf(u"NULL");
            break;
        case 1:
            printf(u"LOAD");
            break;
        case 2:
            printf(u"DYNAMIC");
            break;
        case 3:
            printf(u"INTERP");
            break;
        case 4:
            printf(u"NOTE");
            break;
        case 5:
            printf(u"SHLIB");
            break;
        case 6:
            printf(u"PHDR");
            break;
        case 7:
            printf(u"TLS");
            break;
        default:
            printf(u"UNKNOWN");
            break;
        }
        printf(u"\r\n");
        printf(u"  Virtual address:  %#18llx\r\n", item->vaddr);
        printf(u"  Physical address: %#18llx\r\n", item->paddr);
        printf(u"  Filesize:         %#18llx\r\n", item->filesz);
        printf(u"  Memory size:      %#18llx\r\n", item->memsz);
    }
    ENDFOREACH

    struct
    {
        uint64_t vpage_start;
        uint64_t vpage_size;
    } pages[16];
    int n = 0;

    FOREACH(struct ProgramHeader, ph_next, program_header_iter(elf))
    {
        if (item->type == PT_LOAD)
        {
            if ((item->align > 1 ? item->vaddr & ~((1 << 12) - 1) : item->vaddr) != item->vaddr)
            {
                ERR(u"All program segments in %s must be aligned to 4K pages\r\n", kernel_info->FileName);
            }
            uint64_t page_start = item->vaddr >> 12;
            uint64_t page_end = (item->vaddr + item->memsz - 1) >> 12;
            uint64_t page_size = page_end - page_start + 1;
            printf(u"Loadable program header: %llx..%llx\r\n", page_start, page_end);
            pages[n].vpage_start = page_start;
            pages[n++].vpage_size = page_size;
        }
    }
    ENDFOREACH

    uint64_t cursor = 0;
    for (int i = 0; i < n; ++i)
    {
        if (pages[i].vpage_start < cursor)
        {
            ERR(u"%s contains overlapping loadable segments\r\n", kernel_info->FileName);
        }

        cursor = pages[i].vpage_start + pages[i].vpage_size;
    }
    printf(u"All loadable segments are non-overlapping\r\n");

    st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &address);
    uint64_t *pml4 = (uint64_t *)read_cr3();
    memmove((void *)address, pml4, 4096);
    pml4 = (uint64_t *)address;
    write_cr3((uint64_t)pml4);
    FOREACH(struct ProgramHeader, ph_next, program_header_iter(elf))
    {
        if (item->type == PT_LOAD)
        {
            uint64_t page_start = item->vaddr >> 12;
            uint64_t page_end = (item->vaddr + item->memsz - 1) >> 12;
            uint64_t page_size = page_end - page_start + 1;
            EFI_PHYSICAL_ADDRESS address;
            CALL(st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, page_size, &address), "Error allocating memory for kernel segment");
            size_t skip = item->offset % 4096;
            memmove((void *)address + skip, (void *)elf + item->offset, item->filesz);
            memset((void *)address, 0, skip);
            memset((void *)address + skip + item->filesz, 0, item->memsz - item->filesz);

            printf(u"Page table address: %x\r\n", pml4);

            for (uint64_t page = page_start, addr = address; page <= page_end; ++page, addr += 4096)
            {
                uint64_t l1 = page >> (9 * 3) & 0x1FF;
                uint64_t l2 = page >> (9 * 2) & 0x1FF;
                uint64_t l3 = page >> (9 * 1) & 0x1FF;
                uint64_t l4 = page >> (9 * 0) & 0x1FF;
                printf(u"%d-%d-%d-%d\r\n", l1, l2, l3, l4);
                printf(u"PML4[%d] = %llx\r\n", l1, pml4[l1]);

                uint64_t *map1 = pml4;
                uint64_t *map2;
                uint64_t *map3;
                uint64_t *map4;

                if (!map1[l1])
                {
                    st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, (EFI_PHYSICAL_ADDRESS *)&map2);
                    memset(map2, 0, 4096);
                    map1[l1] = ((uint64_t)map2 & 0x000FFFFFFFFFF000) | 0x03;
                }
                else
                {
                    map2 = (uint64_t *)(map1[l1] & 0x000FFFFFFFFFF000);
                }
                if (!map2[l2])
                {
                    st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, (EFI_PHYSICAL_ADDRESS *)&map3);
                    memset(map3, 0, 4096);
                    map2[l2] = ((uint64_t)map3 & 0x000FFFFFFFFFF000) | 0x03;
                }
                else
                {
                    map3 = (uint64_t *)(map2[l2] & 0x000FFFFFFFFFF000);
                }
                if (!map3[l3])
                {
                    st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, (EFI_PHYSICAL_ADDRESS *)&map4);
                    memset(map4, 0, 4096);
                    map3[l3] = ((uint64_t)map4 & 0x000FFFFFFFFFF000) | 0x03;
                }
                else
                {
                    map4 = (uint64_t *)(map3[l3] & 0x000FFFFFFFFFF000);
                }
                map4[l4] = (addr & 0x000FFFFFFFFFF000) | 0x03;
                printf(u"Mapped %#llx to %#llx\r\n", page << 12, addr);
                printf(u"%llx: L4[%d] = %llx\r\n", map1, l1, map1[l1]);
                printf(u"%llx: L3[%d] = %llx\r\n", map2, l2, map2[l2]);
                printf(u"%llx: L2[%d] = %llx\r\n", map3, l3, map3[l3]);
                printf(u"%llx: L1[%d] = %llx\r\n", map4, l4, map4[l4]);
                printf(u"Tetsing mapping...\r\n");
                uint8_t _ = *(uint8_t *)(page << 12);
                printf(u"Mapping tested.\r\n");
            }
        }
    }
    ENDFOREACH

    write_cr3((uint64_t)pml4);

    uint64_t entry = elf->entry;
    printf(u"Testing call to kernel entry %#llx...\r\n", entry);
    int result = ((int (*)(void))entry)();
    printf(u"Result was %d\r\n", result);

    {
        EFI_GUID protocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
        UINTN handle_count;
        EFI_HANDLE *handles = NULL;
        CALL(st->BootServices->LocateHandleBuffer(ByProtocol, &protocol, NULL, &handle_count, &handles), "Error getting handles supporting ");
        printf(u"%d handles support GOP\r\n", handle_count);

        EFI_GRAPHICS_OUTPUT_PROTOCOL *interface = NULL;
        CALL(st->BootServices->HandleProtocol(handles[0], &protocol, (void **)&interface), "Error opening GOP");

        printf(u"Switching to GOP console; press any key to continue...");
        st->ConIn->Reset(st->ConIn, false);
        st->BootServices->WaitForEvent(1, &st->ConIn->WaitForKey, NULL);

        init_console(interface);

        st->BootServices->FreePool(handles);
    }

    UINTN memmap_size = 0;
    status = st->BootServices->GetMemoryMap(&memmap_size, NULL, NULL, NULL, NULL);
    if (status != EFI_BUFFER_TOO_SMALL)
    {
        ERR(u"Error getting size of memory map: %s\r\n", status_msg(status));
    }
    memmap_size += sizeof(EFI_MEMORY_DESCRIPTOR) * 2;
    printf(u"Memory needed for memory map: %#llx\r\n", memmap_size);
    address = 0;
    CALL(st->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, (memmap_size + 4096 - 1) / 4096, &address), "Error allocating memory for memory map");
    UINTN mapkey;
    UINTN memmap_entry_size;
    uint32_t memmap_entry_version;
    CALL(st->BootServices->GetMemoryMap(&memmap_size, (EFI_MEMORY_DESCRIPTOR *)address, &mapkey, &memmap_entry_size, &memmap_entry_version), "Error getting memory map");

    UINTN memmap_entry_count = memmap_size / memmap_entry_size;
    EFI_MEMORY_DESCRIPTOR *entries = (EFI_MEMORY_DESCRIPTOR *)address;
    EFI_MEMORY_DESCRIPTOR *ptr = entries;

    uint64_t start = 0;
    cursor = 0;
    enum PhysMemoryType type = PhysReserved;
    for (int i = 0; i < memmap_entry_count; ++i, ptr = (EFI_MEMORY_DESCRIPTOR *)((void *)ptr + memmap_entry_size))
    {
        size_t cur_frame = ptr->PhysicalStart >> 12;
        enum PhysMemoryType cur_type;
        switch (ptr->Type)
        {
        case EfiConventionalMemory:
            cur_type = PhysFree;
            break;
        default:
            cur_type = PhysReserved;
            break;
        }

        if (cur_frame == cursor)
        {
            if (cur_type == type)
            {
                cursor += ptr->NumberOfPages;
            }
            else
            {
                physmemmap[physmemmap_count].start_frame = start;
                physmemmap[physmemmap_count].frame_count = cursor - start;
                physmemmap[physmemmap_count].type = type;
                physmemmap_count++;
                start = cur_frame;
                cursor = cur_frame + ptr->NumberOfPages;
                type = cur_type;
            }
        }
        else
        {
            physmemmap[physmemmap_count].start_frame = start;
            physmemmap[physmemmap_count].frame_count = cursor - start;
            physmemmap[physmemmap_count].type = type;
            physmemmap_count++;
            start = cur_frame;
            cursor = cur_frame + ptr->NumberOfPages;
            type = cur_type;
        }

        if (physmemmap_count >= PHYS_MEMMAP_ENTRY_COUNT)
        {
            ERR(u"Error creating physmemmap");
        }
    }

    printf(u"PhysMemmapEntryCount: %d\r\n", physmemmap_count);

    

    wait_for_keypress_exit();
    return EFI_SUCCESS;
}