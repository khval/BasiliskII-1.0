#ifndef LOCALE_H
#define LOCALE_H


/****************************************************************************/


/* This file was created automatically by CatComp.
 * Do NOT edit by hand!
 */


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifdef CATCOMP_CODE
#ifndef CATCOMP_BLOCK
#define CATCOMP_ARRAY
#endif
#endif

#ifdef CATCOMP_ARRAY
#ifndef CATCOMP_NUMBERS
#define CATCOMP_NUMBERS
#endif
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif

#ifdef CATCOMP_BLOCK
#ifndef CATCOMP_STRINGS
#define CATCOMP_STRINGS
#endif
#endif


/****************************************************************************/


#ifdef CATCOMP_NUMBERS

#define MSG_AUTHOR_INFORMATION 0
#define MSG_PORT_INFORMATION 1
#define MSG_GFX_INFORMATION 2
#define MSG_TRANSLATION_INFORMATION 3
#define MSG_DESCRIPTION 4
#define MSG_QUIT_WHILE_RUNING_INFORMATION 5
#define MSG_OK_GAD 6
#define MSG_CANCEL_GAD 7
#define MSG_PREFS_TYPE_GAD 8
#define MSG_PREFS_READ_ONLY_GAD 9
#define MSG_PREFS_FILE_GAD 10
#define MSG_PREFS_DEVICE_GAD 11
#define MSG_PREFS_CREATE_GAD 12
#define MSG_PREFS_UNIT_GAD 13
#define MSG_PREFS_FLAGS_GAD 14
#define MSG_PREFS_START_BLOCK_GAD 15
#define MSG_PREFS_BLOCKS_GAD 16
#define MSG_PREFS_BLOCK_SIZE_GAD 17
#define MSG_PREFS_RAMSIZE_GAD 18
#define MSG_PREFS_ADD_GAD 19
#define MSG_PREFS_EDIT_GAD 20
#define MSG_PREFS_REMOVE_GAD 21
#define MSG_PREFS_CD_DEVICE_GAD 22
#define MSG_PREFS_CD_UNIT_GAD 23
#define MSG_PREFS_CD_BOOT_GAD 24
#define MSG_PREFS_CD_DISABLE_DRIVER_GAD 25
#define MSG_PREFS_MORPHOS_ROOT_GAD 26
#define MSG_PREFS_VOLUMETYPE_FILE_GAD 27
#define MSG_PREFS_VOLUMETYPE_DEVICE_GAD 28
#define MSG_PREFS_ID_0_DEVICE_GAD 29
#define MSG_PREFS_ID_1_DEVICE_GAD 30
#define MSG_PREFS_ID_2_DEVICE_GAD 31
#define MSG_PREFS_ID_3_DEVICE_GAD 32
#define MSG_PREFS_ID_4_DEVICE_GAD 33
#define MSG_PREFS_ID_5_DEVICE_GAD 34
#define MSG_PREFS_ID_6_DEVICE_GAD 35
#define MSG_PREFS_SCSI_UNIT_GAD 36
#define MSG_PREFS_MODEM_DEVICE_GAD 37
#define MSG_PREFS_MODEM_UNIT_GAD 38
#define MSG_PREFS_MODEM_PARALLEL_GAD 39
#define MSG_PREFS_ETHERNET_DEVICE_GAD 40
#define MSG_PREFS_ETHERNET_UNIT_GAD 41
#define MSG_PREFS_PRINTER_DEVICE_GAD 42
#define MSG_PREFS_PRINTER_UNIT_GAD 43
#define MSG_PREFS_PRINTER_PARALLEL_GAD 44
#define MSG_PREFS_GFX_WIDTH_GAD 45
#define MSG_PREFS_GFX_HEIGHT_GAD 46
#define MSG_PREFS_GFX_FULLSCREEN_GAD 47
#define MSG_PREFS_GFX_8BIT_GAD 48
#define MSG_PREFS_DISABLE_SOUND 49
#define MSG_PREFS_SYSTEM_RAM_GAD 50
#define MSG_PREFS_SYSTEM_MODEL_GAD 51
#define MSG_PREFS_SYSTEM_ROM_GAD 52
#define MSG_PREFS_START_GAD 53
#define MSG_PREFS_QUIT_GAD 54
#define MSG_MAC_VOLUMES 55
#define MSG_CDROM 56
#define MSG_VIRTUAL_SCSI_DEVICES 57
#define MSG_MODEM 58
#define MSG_ETHERNET 59
#define MSG_PRINTER 60
#define MSG_GRAPHICS 61
#define MSG_SOUND 62
#define MSG_SYSTEM 63
#define MSG_SIZE 64
#define MSG_MB 65
#define MSG_TITLE_BASILISK_SETTINGS 66
#define MSG_TITLE_ADD_VOLUME 67
#define MSG_TITLE_EDIT_VOLUME 68
#define MSG_PAGE_VOLUMES 69
#define MSG_PAGE_SCSI 70
#define MSG_PAGE_COMMUNICATION 71
#define MSG_PAGE_EMULATION 72
#define MSG_MENU_TITLE_BASILISK 73
#define MSG_MENU_BASILISK_ABOUT 74
#define MSG_MENU_BASILISK_START 75
#define MSG_MENU_BASILISK_QUIT 76
#define ID_GRAPHICS 77
#define ID_PREFS_GFX_MODE_GAD 78
#define ID_PREFS_GFX_WIDTH_GAD 79
#define ID_PREFS_GFX_HEIGHT_GAD 80
#define ID_PREFS_GFX_MODE_ID_SELECT_GAD 81
#define ID_PREFS_GFX_MODE_ID_GAD 82
#define ID_PREFS_GFX_WINDOW_DEPTH_GAD 83
#define ID_PREFS_GFX_RENDER_METHOD_GAD 84
#define ID_PREFS_GFX_LOCK_GAD 85
#define ID_ACTIVE_WINDOW_GAD 86
#define ID_ACTIVE_WINDOW_FRAMESKIP_GAD 87
#define ID_ACTIVE_WINDOW_LINESKIP_GAD 88
#define ID_DEACTIVE_WINDOW_GAD 89
#define ID_DEACTIVE_WINDOW_FRAMESKIP_GAD 90
#define ID_DEACTIVE_WINDOW_LINESKIP_GAD 91
#define ID_SOUND 92
#define ID_PREFS_DISABLE_SOUND 93
#define ID_VIRTUAL_SCSI_DEVICES 94
#define ID_PREFS_ID_0_DEVICE_GAD 95
#define ID_PREFS_ID_0_UNIT_GAD 96
#define ID_PREFS_ID_1_DEVICE_GAD 97
#define ID_PREFS_ID_1_UNIT_GAD 98
#define ID_PREFS_ID_2_DEVICE_GAD 99
#define ID_PREFS_ID_2_UNIT_GAD 100
#define ID_PREFS_ID_3_DEVICE_GAD 101
#define ID_PREFS_ID_3_UNIT_GAD 102
#define ID_PREFS_ID_4_DEVICE_GAD 103
#define ID_PREFS_ID_4_UNIT_GAD 104
#define ID_PREFS_ID_5_DEVICE_GAD 105
#define ID_PREFS_ID_5_UNIT_GAD 106
#define ID_PREFS_ID_6_DEVICE_GAD 107
#define ID_PREFS_ID_6_UNIT_GAD 108
#define ID_PREFS_START_GAD 109
#define ID_PREFS_QUIT_GAD 110
#define ID_PREFS_EDIT_GAD 111
#define ID_PREFS_REMOVE_GAD 112
#define ID_HARDWARE 113
#define ID_CPU 114
#define ID_PREFS_SYSTEM_CPU_GAD 115
#define ID_PREFS_SYSTEM_FPU_GAD 116
#define ID_CPU_PRI 117
#define ID_CPU_ACTIVE_GAD 118
#define ID_CPU_INACTIVE_GAD 119
#define ID_RAM 120
#define ID_PREFS_SYSTEM_RAM_GAD 121
#define ID_SYSTEM 122
#define ID_PREFS_SYSTEM_MODEL_GAD 123
#define ID_PREFS_SYSTEM_ROM_SELECT_GAD 124
#define ID_PREFS_SYSTEM_ROM_GAD 125
#define ID_MODEM 126
#define ID_PREFS_MODEM_DEVICE_GAD 127
#define ID_PREFS_MODEM_UNIT_GAD 128
#define ID_PREFS_MODEM_PARALLEL_GAD 129
#define ID_PRINTER 130
#define ID_PREFS_PRINTER_DEVICE_GAD 131
#define ID_PREFS_PRINTER_UNIT_GAD 132
#define ID_PREFS_PRINTER_PARALLEL_GAD 133
#define ID_ETHERNET 134
#define ID_PREFS_ETHERNET_DEVICE_SELECT_GAD 135
#define ID_PREFS_ETHERNET_DEVICE_GAD 136
#define ID_PREFS_ETHERNET_UNIT_GAD 137
#define ID_MAC_VOLUMES 138
#define ID_PREFS_ADD_BOOTDISK_GAD 139
#define ID_PREFS_ADD_GAD 140
#define ID_PREFS_CREATE_GAD 141
#define ID_CDROM 142
#define ID_PREFS_CD_DEVICE_GAD 143
#define ID_PREFS_CD_UNIT_GAD 144
#define ID_PREFS_CD_BOOT_GAD 145
#define ID_PREFS_CD_DISABLE_DRIVER_GAD 146
#define ID_PREFS_AMIGAOS4_ROOT_SELECT_GAD 147
#define ID_PREFS_AMIGAOS4_ROOT_GAD 148
#define ID_PREFS_VOLUMETYPE_FILE_GAD 149
#define ID_PREFS_VOLUMETYPE_DEVICE_GAD 150
#define ID_PREFS_TYPE_GAD 151
#define ID_PREFS_DEVICE_GAD 152
#define ID_PREFS_UNIT_GAD 153
#define ID_PREFS_PARTITION_NAME_GAD 154
#define ID_PREFS_READ_ONLY_GAD 155
#define ID_PREFS_FILE_GAD 156
#define ID_PREFS_FILE_SELECT_GAD 157
#define ID_CREATE_NAME_ASL_GAD 158
#define ID_CREATE_NAME_GAD 159
#define ID_CREATE_OK_GAD 160
#define ID_CREATE_CANCEL_GAD 161
#define ID_CREATE_SIZE_GAD 162
#define ID_OK_GAD 163
#define ID_CANCEL_GAD 164
#define ID_END 165

#endif /* CATCOMP_NUMBERS */


/****************************************************************************/


#ifdef CATCOMP_STRINGS

#define MSG_AUTHOR_INFORMATION_STR "\33cBasilisk II 1.0 by Christian Bauer\n Copyright 1997-2001\nDistribtion under General Public License (GPL)"
#define MSG_PORT_INFORMATION_STR "AmigaOS 4.X (PowerPC) port by Kjetil Hvalstrand, 2007-2020"
#define MSG_GFX_INFORMATION_STR "Basilisk icons by Christian Rosentereter"
#define MSG_TRANSLATION_INFORMATION_STR "Translation by: xxxxx"
#define MSG_DESCRIPTION_STR "68k MacIntosh emulator"
#define MSG_QUIT_WHILE_RUNING_INFORMATION_STR "Quiting this way can result in corrupt MacOS files or filesystem.\nThis is the same as pulling the power plug while computer is running."
#define MSG_OK_GAD_STR "_Ok"
#define MSG_CANCEL_GAD_STR "_Cancel"
#define MSG_PREFS_TYPE_GAD_STR "_Type"
#define MSG_PREFS_READ_ONLY_GAD_STR "_Read Only"
#define MSG_PREFS_FILE_GAD_STR "_File"
#define MSG_PREFS_DEVICE_GAD_STR "_Device"
#define MSG_PREFS_CREATE_GAD_STR "Create"
#define MSG_PREFS_UNIT_GAD_STR "_Unit"
#define MSG_PREFS_FLAGS_GAD_STR "_Flags"
#define MSG_PREFS_START_BLOCK_GAD_STR "_Start Block"
#define MSG_PREFS_BLOCKS_GAD_STR "_Blocks"
#define MSG_PREFS_BLOCK_SIZE_GAD_STR "Block Size"
#define MSG_PREFS_RAMSIZE_GAD_STR "%ldMB"
#define MSG_PREFS_ADD_GAD_STR "_Add..."
#define MSG_PREFS_EDIT_GAD_STR "_Edit..."
#define MSG_PREFS_REMOVE_GAD_STR "_Remove"
#define MSG_PREFS_CD_DEVICE_GAD_STR "_Device"
#define MSG_PREFS_CD_UNIT_GAD_STR "_Unit"
#define MSG_PREFS_CD_BOOT_GAD_STR "_Boot from CD"
#define MSG_PREFS_CD_DISABLE_DRIVER_GAD_STR "Disable CD-ROM driver"
#define MSG_PREFS_MORPHOS_ROOT_GAD_STR "AmigaOS root"
#define MSG_PREFS_VOLUMETYPE_FILE_GAD_STR "File"
#define MSG_PREFS_VOLUMETYPE_DEVICE_GAD_STR "Device"
#define MSG_PREFS_ID_0_DEVICE_GAD_STR "ID _0 Device"
#define MSG_PREFS_ID_1_DEVICE_GAD_STR "ID _1 Device"
#define MSG_PREFS_ID_2_DEVICE_GAD_STR "ID _2 Device"
#define MSG_PREFS_ID_3_DEVICE_GAD_STR "ID _3 Device"
#define MSG_PREFS_ID_4_DEVICE_GAD_STR "ID _4 Device"
#define MSG_PREFS_ID_5_DEVICE_GAD_STR "ID _5 Device"
#define MSG_PREFS_ID_6_DEVICE_GAD_STR "ID _6 Device"
#define MSG_PREFS_SCSI_UNIT_GAD_STR "Unit"
#define MSG_PREFS_MODEM_DEVICE_GAD_STR "Device"
#define MSG_PREFS_MODEM_UNIT_GAD_STR "Unit"
#define MSG_PREFS_MODEM_PARALLEL_GAD_STR "Parallel device"
#define MSG_PREFS_ETHERNET_DEVICE_GAD_STR "Device"
#define MSG_PREFS_ETHERNET_UNIT_GAD_STR "Unit"
#define MSG_PREFS_PRINTER_DEVICE_GAD_STR "Device"
#define MSG_PREFS_PRINTER_UNIT_GAD_STR "Unit"
#define MSG_PREFS_PRINTER_PARALLEL_GAD_STR "Parallel device"
#define MSG_PREFS_GFX_WIDTH_GAD_STR "_Width"
#define MSG_PREFS_GFX_HEIGHT_GAD_STR "_Height"
#define MSG_PREFS_GFX_FULLSCREEN_GAD_STR "_Full screen"
#define MSG_PREFS_GFX_8BIT_GAD_STR "_8bit mode"
#define MSG_PREFS_DISABLE_SOUND_STR "_Disable sound"
#define MSG_PREFS_SYSTEM_RAM_GAD_STR "MacOS RAM"
#define MSG_PREFS_SYSTEM_MODEL_GAD_STR "Mac Model"
#define MSG_PREFS_SYSTEM_ROM_GAD_STR "_ROM File"
#define MSG_PREFS_START_GAD_STR "Start"
#define MSG_PREFS_QUIT_GAD_STR "Quit"
#define MSG_MAC_VOLUMES_STR "Mac Volumes"
#define MSG_CDROM_STR "CD-ROM"
#define MSG_VIRTUAL_SCSI_DEVICES_STR "Virtual SCSI devices"
#define MSG_MODEM_STR "Modem"
#define MSG_ETHERNET_STR "Ethernet"
#define MSG_PRINTER_STR "Printer"
#define MSG_GRAPHICS_STR "Graphics"
#define MSG_SOUND_STR "Sound"
#define MSG_SYSTEM_STR "System message"
#define MSG_SIZE_STR "Size"
#define MSG_MB_STR "MB"
#define MSG_TITLE_BASILISK_SETTINGS_STR "Basilisk II Settings"
#define MSG_TITLE_ADD_VOLUME_STR "Add Volume"
#define MSG_TITLE_EDIT_VOLUME_STR "Edit Volume"
#define MSG_PAGE_VOLUMES_STR "Volumes"
#define MSG_PAGE_SCSI_STR "SCSI"
#define MSG_PAGE_COMMUNICATION_STR "Communication"
#define MSG_PAGE_EMULATION_STR "Emulation"
#define MSG_MENU_TITLE_BASILISK_STR "Basilisk"
#define MSG_MENU_BASILISK_ABOUT_STR "?\0About..."
#define MSG_MENU_BASILISK_START_STR "S\0Start"
#define MSG_MENU_BASILISK_QUIT_STR "Q\0Quit"
#define ID_GRAPHICS_STR "Graphics"
#define ID_PREFS_GFX_MODE_GAD_STR "Gfx mode"
#define ID_PREFS_GFX_WIDTH_GAD_STR "Gfx Width"
#define ID_PREFS_GFX_HEIGHT_GAD_STR "Gfx height"
#define ID_PREFS_GFX_MODE_ID_SELECT_GAD_STR "Select screen mode"
#define ID_PREFS_GFX_MODE_ID_GAD_STR "Screen mode id"
#define ID_PREFS_GFX_WINDOW_DEPTH_GAD_STR "Window depth"
#define ID_PREFS_GFX_RENDER_METHOD_GAD_STR "Render mode"
#define ID_PREFS_GFX_LOCK_GAD_STR "Screen lock"
#define ID_ACTIVE_WINDOW_GAD_STR "Active window"
#define ID_ACTIVE_WINDOW_FRAMESKIP_GAD_STR "Frame skip"
#define ID_ACTIVE_WINDOW_LINESKIP_GAD_STR "Line skip"
#define ID_DEACTIVE_WINDOW_GAD_STR "Deactive window"
#define ID_DEACTIVE_WINDOW_FRAMESKIP_GAD_STR "Frame skip"
#define ID_DEACTIVE_WINDOW_LINESKIP_GAD_STR "Line skip"
#define ID_SOUND_STR "Sound"
#define ID_PREFS_DISABLE_SOUND_STR "Disable sound"
#define ID_VIRTUAL_SCSI_DEVICES_STR "Scsi devices"
#define ID_PREFS_ID_0_DEVICE_GAD_STR "Device 0"
#define ID_PREFS_ID_0_UNIT_GAD_STR "Unit 0"
#define ID_PREFS_ID_1_DEVICE_GAD_STR "Device 1"
#define ID_PREFS_ID_1_UNIT_GAD_STR "Unit 1"
#define ID_PREFS_ID_2_DEVICE_GAD_STR "Device 2"
#define ID_PREFS_ID_2_UNIT_GAD_STR "Unit 2"
#define ID_PREFS_ID_3_DEVICE_GAD_STR "Device 3"
#define ID_PREFS_ID_3_UNIT_GAD_STR "Unit 3"
#define ID_PREFS_ID_4_DEVICE_GAD_STR "Device 4"
#define ID_PREFS_ID_4_UNIT_GAD_STR "Unit 4"
#define ID_PREFS_ID_5_DEVICE_GAD_STR "Device 5"
#define ID_PREFS_ID_5_UNIT_GAD_STR "Unit 5"
#define ID_PREFS_ID_6_DEVICE_GAD_STR "Device 6"
#define ID_PREFS_ID_6_UNIT_GAD_STR "Unit 6"
#define ID_PREFS_START_GAD_STR "Start"
#define ID_PREFS_QUIT_GAD_STR "Quit"
#define ID_PREFS_EDIT_GAD_STR "Edit"
#define ID_PREFS_REMOVE_GAD_STR "Remove"
#define ID_HARDWARE_STR "Hardware"
#define ID_CPU_STR "CPU"
#define ID_PREFS_SYSTEM_CPU_GAD_STR "CPU"
#define ID_PREFS_SYSTEM_FPU_GAD_STR "FPU"
#define ID_CPU_PRI_STR "CPU priority"
#define ID_CPU_ACTIVE_GAD_STR "CPU active window"
#define ID_CPU_INACTIVE_GAD_STR "CPU inactive window"
#define ID_RAM_STR "RAM"
#define ID_PREFS_SYSTEM_RAM_GAD_STR "RAM"
#define ID_SYSTEM_STR "System"
#define ID_PREFS_SYSTEM_MODEL_GAD_STR "Mac model"
#define ID_PREFS_SYSTEM_ROM_SELECT_GAD_STR "Select rom"
#define ID_PREFS_SYSTEM_ROM_GAD_STR "Rom"
#define ID_MODEM_STR "Modem"
#define ID_PREFS_MODEM_DEVICE_GAD_STR "Modem device"
#define ID_PREFS_MODEM_UNIT_GAD_STR "Modem unit"
#define ID_PREFS_MODEM_PARALLEL_GAD_STR "Modem parallel"
#define ID_PRINTER_STR "Printer"
#define ID_PREFS_PRINTER_DEVICE_GAD_STR "Printer device"
#define ID_PREFS_PRINTER_UNIT_GAD_STR "Printer unit"
#define ID_PREFS_PRINTER_PARALLEL_GAD_STR "Printer parallel"
#define ID_ETHERNET_STR "Ethernet"
#define ID_PREFS_ETHERNET_DEVICE_SELECT_GAD_STR "Select ethernet device"
#define ID_PREFS_ETHERNET_DEVICE_GAD_STR "Ethernet device"
#define ID_PREFS_ETHERNET_UNIT_GAD_STR "Ethernet unit"
#define ID_MAC_VOLUMES_STR "Volumes"
#define ID_PREFS_ADD_BOOTDISK_GAD_STR "Boot disk"
#define ID_PREFS_ADD_GAD_STR "Add"
#define ID_PREFS_CREATE_GAD_STR "Create"
#define ID_CDROM_STR "Cdrom"
#define ID_PREFS_CD_DEVICE_GAD_STR "Cd device"
#define ID_PREFS_CD_UNIT_GAD_STR "Cd unit"
#define ID_PREFS_CD_BOOT_GAD_STR "Cd boot"
#define ID_PREFS_CD_DISABLE_DRIVER_GAD_STR "Disable cd device"
#define ID_PREFS_AMIGAOS4_ROOT_SELECT_GAD_STR "Shared directory"
#define ID_PREFS_AMIGAOS4_ROOT_GAD_STR "Shared directory"
#define ID_PREFS_VOLUMETYPE_FILE_GAD_STR "File volume type"
#define ID_PREFS_VOLUMETYPE_DEVICE_GAD_STR "Device volume type"
#define ID_PREFS_TYPE_GAD_STR "Type"
#define ID_PREFS_DEVICE_GAD_STR "Device"
#define ID_PREFS_UNIT_GAD_STR "Unit"
#define ID_PREFS_PARTITION_NAME_GAD_STR "Partition name"
#define ID_PREFS_READ_ONLY_GAD_STR "Read only"
#define ID_PREFS_FILE_GAD_STR "File"
#define ID_PREFS_FILE_SELECT_GAD_STR "File"
#define ID_CREATE_NAME_ASL_GAD_STR "Name"
#define ID_CREATE_NAME_GAD_STR "Name"
#define ID_CREATE_OK_GAD_STR "Ok"
#define ID_CREATE_CANCEL_GAD_STR "Cancel"
#define ID_CREATE_SIZE_GAD_STR "Size"
#define ID_OK_GAD_STR "Ok"
#define ID_CANCEL_GAD_STR "Cancel"
#define ID_END_STR "--- EOF ---"

#endif /* CATCOMP_STRINGS */


/****************************************************************************/


#ifdef CATCOMP_ARRAY

struct CatCompArrayType
{
    LONG         cca_ID;
    CONST_STRPTR cca_Str;
};

STATIC CONST struct CatCompArrayType CatCompArray[] =
{
    {MSG_AUTHOR_INFORMATION,(CONST_STRPTR)MSG_AUTHOR_INFORMATION_STR},
    {MSG_PORT_INFORMATION,(CONST_STRPTR)MSG_PORT_INFORMATION_STR},
    {MSG_GFX_INFORMATION,(CONST_STRPTR)MSG_GFX_INFORMATION_STR},
    {MSG_TRANSLATION_INFORMATION,(CONST_STRPTR)MSG_TRANSLATION_INFORMATION_STR},
    {MSG_DESCRIPTION,(CONST_STRPTR)MSG_DESCRIPTION_STR},
    {MSG_QUIT_WHILE_RUNING_INFORMATION,(CONST_STRPTR)MSG_QUIT_WHILE_RUNING_INFORMATION_STR},
    {MSG_OK_GAD,(CONST_STRPTR)MSG_OK_GAD_STR},
    {MSG_CANCEL_GAD,(CONST_STRPTR)MSG_CANCEL_GAD_STR},
    {MSG_PREFS_TYPE_GAD,(CONST_STRPTR)MSG_PREFS_TYPE_GAD_STR},
    {MSG_PREFS_READ_ONLY_GAD,(CONST_STRPTR)MSG_PREFS_READ_ONLY_GAD_STR},
    {MSG_PREFS_FILE_GAD,(CONST_STRPTR)MSG_PREFS_FILE_GAD_STR},
    {MSG_PREFS_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_DEVICE_GAD_STR},
    {MSG_PREFS_CREATE_GAD,(CONST_STRPTR)MSG_PREFS_CREATE_GAD_STR},
    {MSG_PREFS_UNIT_GAD,(CONST_STRPTR)MSG_PREFS_UNIT_GAD_STR},
    {MSG_PREFS_FLAGS_GAD,(CONST_STRPTR)MSG_PREFS_FLAGS_GAD_STR},
    {MSG_PREFS_START_BLOCK_GAD,(CONST_STRPTR)MSG_PREFS_START_BLOCK_GAD_STR},
    {MSG_PREFS_BLOCKS_GAD,(CONST_STRPTR)MSG_PREFS_BLOCKS_GAD_STR},
    {MSG_PREFS_BLOCK_SIZE_GAD,(CONST_STRPTR)MSG_PREFS_BLOCK_SIZE_GAD_STR},
    {MSG_PREFS_RAMSIZE_GAD,(CONST_STRPTR)MSG_PREFS_RAMSIZE_GAD_STR},
    {MSG_PREFS_ADD_GAD,(CONST_STRPTR)MSG_PREFS_ADD_GAD_STR},
    {MSG_PREFS_EDIT_GAD,(CONST_STRPTR)MSG_PREFS_EDIT_GAD_STR},
    {MSG_PREFS_REMOVE_GAD,(CONST_STRPTR)MSG_PREFS_REMOVE_GAD_STR},
    {MSG_PREFS_CD_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_CD_DEVICE_GAD_STR},
    {MSG_PREFS_CD_UNIT_GAD,(CONST_STRPTR)MSG_PREFS_CD_UNIT_GAD_STR},
    {MSG_PREFS_CD_BOOT_GAD,(CONST_STRPTR)MSG_PREFS_CD_BOOT_GAD_STR},
    {MSG_PREFS_CD_DISABLE_DRIVER_GAD,(CONST_STRPTR)MSG_PREFS_CD_DISABLE_DRIVER_GAD_STR},
    {MSG_PREFS_MORPHOS_ROOT_GAD,(CONST_STRPTR)MSG_PREFS_MORPHOS_ROOT_GAD_STR},
    {MSG_PREFS_VOLUMETYPE_FILE_GAD,(CONST_STRPTR)MSG_PREFS_VOLUMETYPE_FILE_GAD_STR},
    {MSG_PREFS_VOLUMETYPE_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_VOLUMETYPE_DEVICE_GAD_STR},
    {MSG_PREFS_ID_0_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ID_0_DEVICE_GAD_STR},
    {MSG_PREFS_ID_1_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ID_1_DEVICE_GAD_STR},
    {MSG_PREFS_ID_2_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ID_2_DEVICE_GAD_STR},
    {MSG_PREFS_ID_3_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ID_3_DEVICE_GAD_STR},
    {MSG_PREFS_ID_4_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ID_4_DEVICE_GAD_STR},
    {MSG_PREFS_ID_5_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ID_5_DEVICE_GAD_STR},
    {MSG_PREFS_ID_6_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ID_6_DEVICE_GAD_STR},
    {MSG_PREFS_SCSI_UNIT_GAD,(CONST_STRPTR)MSG_PREFS_SCSI_UNIT_GAD_STR},
    {MSG_PREFS_MODEM_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_MODEM_DEVICE_GAD_STR},
    {MSG_PREFS_MODEM_UNIT_GAD,(CONST_STRPTR)MSG_PREFS_MODEM_UNIT_GAD_STR},
    {MSG_PREFS_MODEM_PARALLEL_GAD,(CONST_STRPTR)MSG_PREFS_MODEM_PARALLEL_GAD_STR},
    {MSG_PREFS_ETHERNET_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_ETHERNET_DEVICE_GAD_STR},
    {MSG_PREFS_ETHERNET_UNIT_GAD,(CONST_STRPTR)MSG_PREFS_ETHERNET_UNIT_GAD_STR},
    {MSG_PREFS_PRINTER_DEVICE_GAD,(CONST_STRPTR)MSG_PREFS_PRINTER_DEVICE_GAD_STR},
    {MSG_PREFS_PRINTER_UNIT_GAD,(CONST_STRPTR)MSG_PREFS_PRINTER_UNIT_GAD_STR},
    {MSG_PREFS_PRINTER_PARALLEL_GAD,(CONST_STRPTR)MSG_PREFS_PRINTER_PARALLEL_GAD_STR},
    {MSG_PREFS_GFX_WIDTH_GAD,(CONST_STRPTR)MSG_PREFS_GFX_WIDTH_GAD_STR},
    {MSG_PREFS_GFX_HEIGHT_GAD,(CONST_STRPTR)MSG_PREFS_GFX_HEIGHT_GAD_STR},
    {MSG_PREFS_GFX_FULLSCREEN_GAD,(CONST_STRPTR)MSG_PREFS_GFX_FULLSCREEN_GAD_STR},
    {MSG_PREFS_GFX_8BIT_GAD,(CONST_STRPTR)MSG_PREFS_GFX_8BIT_GAD_STR},
    {MSG_PREFS_DISABLE_SOUND,(CONST_STRPTR)MSG_PREFS_DISABLE_SOUND_STR},
    {MSG_PREFS_SYSTEM_RAM_GAD,(CONST_STRPTR)MSG_PREFS_SYSTEM_RAM_GAD_STR},
    {MSG_PREFS_SYSTEM_MODEL_GAD,(CONST_STRPTR)MSG_PREFS_SYSTEM_MODEL_GAD_STR},
    {MSG_PREFS_SYSTEM_ROM_GAD,(CONST_STRPTR)MSG_PREFS_SYSTEM_ROM_GAD_STR},
    {MSG_PREFS_START_GAD,(CONST_STRPTR)MSG_PREFS_START_GAD_STR},
    {MSG_PREFS_QUIT_GAD,(CONST_STRPTR)MSG_PREFS_QUIT_GAD_STR},
    {MSG_MAC_VOLUMES,(CONST_STRPTR)MSG_MAC_VOLUMES_STR},
    {MSG_CDROM,(CONST_STRPTR)MSG_CDROM_STR},
    {MSG_VIRTUAL_SCSI_DEVICES,(CONST_STRPTR)MSG_VIRTUAL_SCSI_DEVICES_STR},
    {MSG_MODEM,(CONST_STRPTR)MSG_MODEM_STR},
    {MSG_ETHERNET,(CONST_STRPTR)MSG_ETHERNET_STR},
    {MSG_PRINTER,(CONST_STRPTR)MSG_PRINTER_STR},
    {MSG_GRAPHICS,(CONST_STRPTR)MSG_GRAPHICS_STR},
    {MSG_SOUND,(CONST_STRPTR)MSG_SOUND_STR},
    {MSG_SYSTEM,(CONST_STRPTR)MSG_SYSTEM_STR},
    {MSG_SIZE,(CONST_STRPTR)MSG_SIZE_STR},
    {MSG_MB,(CONST_STRPTR)MSG_MB_STR},
    {MSG_TITLE_BASILISK_SETTINGS,(CONST_STRPTR)MSG_TITLE_BASILISK_SETTINGS_STR},
    {MSG_TITLE_ADD_VOLUME,(CONST_STRPTR)MSG_TITLE_ADD_VOLUME_STR},
    {MSG_TITLE_EDIT_VOLUME,(CONST_STRPTR)MSG_TITLE_EDIT_VOLUME_STR},
    {MSG_PAGE_VOLUMES,(CONST_STRPTR)MSG_PAGE_VOLUMES_STR},
    {MSG_PAGE_SCSI,(CONST_STRPTR)MSG_PAGE_SCSI_STR},
    {MSG_PAGE_COMMUNICATION,(CONST_STRPTR)MSG_PAGE_COMMUNICATION_STR},
    {MSG_PAGE_EMULATION,(CONST_STRPTR)MSG_PAGE_EMULATION_STR},
    {MSG_MENU_TITLE_BASILISK,(CONST_STRPTR)MSG_MENU_TITLE_BASILISK_STR},
    {MSG_MENU_BASILISK_ABOUT,(CONST_STRPTR)MSG_MENU_BASILISK_ABOUT_STR},
    {MSG_MENU_BASILISK_START,(CONST_STRPTR)MSG_MENU_BASILISK_START_STR},
    {MSG_MENU_BASILISK_QUIT,(CONST_STRPTR)MSG_MENU_BASILISK_QUIT_STR},
    {ID_GRAPHICS,(CONST_STRPTR)ID_GRAPHICS_STR},
    {ID_PREFS_GFX_MODE_GAD,(CONST_STRPTR)ID_PREFS_GFX_MODE_GAD_STR},
    {ID_PREFS_GFX_WIDTH_GAD,(CONST_STRPTR)ID_PREFS_GFX_WIDTH_GAD_STR},
    {ID_PREFS_GFX_HEIGHT_GAD,(CONST_STRPTR)ID_PREFS_GFX_HEIGHT_GAD_STR},
    {ID_PREFS_GFX_MODE_ID_SELECT_GAD,(CONST_STRPTR)ID_PREFS_GFX_MODE_ID_SELECT_GAD_STR},
    {ID_PREFS_GFX_MODE_ID_GAD,(CONST_STRPTR)ID_PREFS_GFX_MODE_ID_GAD_STR},
    {ID_PREFS_GFX_WINDOW_DEPTH_GAD,(CONST_STRPTR)ID_PREFS_GFX_WINDOW_DEPTH_GAD_STR},
    {ID_PREFS_GFX_RENDER_METHOD_GAD,(CONST_STRPTR)ID_PREFS_GFX_RENDER_METHOD_GAD_STR},
    {ID_PREFS_GFX_LOCK_GAD,(CONST_STRPTR)ID_PREFS_GFX_LOCK_GAD_STR},
    {ID_ACTIVE_WINDOW_GAD,(CONST_STRPTR)ID_ACTIVE_WINDOW_GAD_STR},
    {ID_ACTIVE_WINDOW_FRAMESKIP_GAD,(CONST_STRPTR)ID_ACTIVE_WINDOW_FRAMESKIP_GAD_STR},
    {ID_ACTIVE_WINDOW_LINESKIP_GAD,(CONST_STRPTR)ID_ACTIVE_WINDOW_LINESKIP_GAD_STR},
    {ID_DEACTIVE_WINDOW_GAD,(CONST_STRPTR)ID_DEACTIVE_WINDOW_GAD_STR},
    {ID_DEACTIVE_WINDOW_FRAMESKIP_GAD,(CONST_STRPTR)ID_DEACTIVE_WINDOW_FRAMESKIP_GAD_STR},
    {ID_DEACTIVE_WINDOW_LINESKIP_GAD,(CONST_STRPTR)ID_DEACTIVE_WINDOW_LINESKIP_GAD_STR},
    {ID_SOUND,(CONST_STRPTR)ID_SOUND_STR},
    {ID_PREFS_DISABLE_SOUND,(CONST_STRPTR)ID_PREFS_DISABLE_SOUND_STR},
    {ID_VIRTUAL_SCSI_DEVICES,(CONST_STRPTR)ID_VIRTUAL_SCSI_DEVICES_STR},
    {ID_PREFS_ID_0_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ID_0_DEVICE_GAD_STR},
    {ID_PREFS_ID_0_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ID_0_UNIT_GAD_STR},
    {ID_PREFS_ID_1_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ID_1_DEVICE_GAD_STR},
    {ID_PREFS_ID_1_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ID_1_UNIT_GAD_STR},
    {ID_PREFS_ID_2_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ID_2_DEVICE_GAD_STR},
    {ID_PREFS_ID_2_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ID_2_UNIT_GAD_STR},
    {ID_PREFS_ID_3_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ID_3_DEVICE_GAD_STR},
    {ID_PREFS_ID_3_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ID_3_UNIT_GAD_STR},
    {ID_PREFS_ID_4_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ID_4_DEVICE_GAD_STR},
    {ID_PREFS_ID_4_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ID_4_UNIT_GAD_STR},
    {ID_PREFS_ID_5_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ID_5_DEVICE_GAD_STR},
    {ID_PREFS_ID_5_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ID_5_UNIT_GAD_STR},
    {ID_PREFS_ID_6_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ID_6_DEVICE_GAD_STR},
    {ID_PREFS_ID_6_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ID_6_UNIT_GAD_STR},
    {ID_PREFS_START_GAD,(CONST_STRPTR)ID_PREFS_START_GAD_STR},
    {ID_PREFS_QUIT_GAD,(CONST_STRPTR)ID_PREFS_QUIT_GAD_STR},
    {ID_PREFS_EDIT_GAD,(CONST_STRPTR)ID_PREFS_EDIT_GAD_STR},
    {ID_PREFS_REMOVE_GAD,(CONST_STRPTR)ID_PREFS_REMOVE_GAD_STR},
    {ID_HARDWARE,(CONST_STRPTR)ID_HARDWARE_STR},
    {ID_CPU,(CONST_STRPTR)ID_CPU_STR},
    {ID_PREFS_SYSTEM_CPU_GAD,(CONST_STRPTR)ID_PREFS_SYSTEM_CPU_GAD_STR},
    {ID_PREFS_SYSTEM_FPU_GAD,(CONST_STRPTR)ID_PREFS_SYSTEM_FPU_GAD_STR},
    {ID_CPU_PRI,(CONST_STRPTR)ID_CPU_PRI_STR},
    {ID_CPU_ACTIVE_GAD,(CONST_STRPTR)ID_CPU_ACTIVE_GAD_STR},
    {ID_CPU_INACTIVE_GAD,(CONST_STRPTR)ID_CPU_INACTIVE_GAD_STR},
    {ID_RAM,(CONST_STRPTR)ID_RAM_STR},
    {ID_PREFS_SYSTEM_RAM_GAD,(CONST_STRPTR)ID_PREFS_SYSTEM_RAM_GAD_STR},
    {ID_SYSTEM,(CONST_STRPTR)ID_SYSTEM_STR},
    {ID_PREFS_SYSTEM_MODEL_GAD,(CONST_STRPTR)ID_PREFS_SYSTEM_MODEL_GAD_STR},
    {ID_PREFS_SYSTEM_ROM_SELECT_GAD,(CONST_STRPTR)ID_PREFS_SYSTEM_ROM_SELECT_GAD_STR},
    {ID_PREFS_SYSTEM_ROM_GAD,(CONST_STRPTR)ID_PREFS_SYSTEM_ROM_GAD_STR},
    {ID_MODEM,(CONST_STRPTR)ID_MODEM_STR},
    {ID_PREFS_MODEM_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_MODEM_DEVICE_GAD_STR},
    {ID_PREFS_MODEM_UNIT_GAD,(CONST_STRPTR)ID_PREFS_MODEM_UNIT_GAD_STR},
    {ID_PREFS_MODEM_PARALLEL_GAD,(CONST_STRPTR)ID_PREFS_MODEM_PARALLEL_GAD_STR},
    {ID_PRINTER,(CONST_STRPTR)ID_PRINTER_STR},
    {ID_PREFS_PRINTER_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_PRINTER_DEVICE_GAD_STR},
    {ID_PREFS_PRINTER_UNIT_GAD,(CONST_STRPTR)ID_PREFS_PRINTER_UNIT_GAD_STR},
    {ID_PREFS_PRINTER_PARALLEL_GAD,(CONST_STRPTR)ID_PREFS_PRINTER_PARALLEL_GAD_STR},
    {ID_ETHERNET,(CONST_STRPTR)ID_ETHERNET_STR},
    {ID_PREFS_ETHERNET_DEVICE_SELECT_GAD,(CONST_STRPTR)ID_PREFS_ETHERNET_DEVICE_SELECT_GAD_STR},
    {ID_PREFS_ETHERNET_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_ETHERNET_DEVICE_GAD_STR},
    {ID_PREFS_ETHERNET_UNIT_GAD,(CONST_STRPTR)ID_PREFS_ETHERNET_UNIT_GAD_STR},
    {ID_MAC_VOLUMES,(CONST_STRPTR)ID_MAC_VOLUMES_STR},
    {ID_PREFS_ADD_BOOTDISK_GAD,(CONST_STRPTR)ID_PREFS_ADD_BOOTDISK_GAD_STR},
    {ID_PREFS_ADD_GAD,(CONST_STRPTR)ID_PREFS_ADD_GAD_STR},
    {ID_PREFS_CREATE_GAD,(CONST_STRPTR)ID_PREFS_CREATE_GAD_STR},
    {ID_CDROM,(CONST_STRPTR)ID_CDROM_STR},
    {ID_PREFS_CD_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_CD_DEVICE_GAD_STR},
    {ID_PREFS_CD_UNIT_GAD,(CONST_STRPTR)ID_PREFS_CD_UNIT_GAD_STR},
    {ID_PREFS_CD_BOOT_GAD,(CONST_STRPTR)ID_PREFS_CD_BOOT_GAD_STR},
    {ID_PREFS_CD_DISABLE_DRIVER_GAD,(CONST_STRPTR)ID_PREFS_CD_DISABLE_DRIVER_GAD_STR},
    {ID_PREFS_AMIGAOS4_ROOT_SELECT_GAD,(CONST_STRPTR)ID_PREFS_AMIGAOS4_ROOT_SELECT_GAD_STR},
    {ID_PREFS_AMIGAOS4_ROOT_GAD,(CONST_STRPTR)ID_PREFS_AMIGAOS4_ROOT_GAD_STR},
    {ID_PREFS_VOLUMETYPE_FILE_GAD,(CONST_STRPTR)ID_PREFS_VOLUMETYPE_FILE_GAD_STR},
    {ID_PREFS_VOLUMETYPE_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_VOLUMETYPE_DEVICE_GAD_STR},
    {ID_PREFS_TYPE_GAD,(CONST_STRPTR)ID_PREFS_TYPE_GAD_STR},
    {ID_PREFS_DEVICE_GAD,(CONST_STRPTR)ID_PREFS_DEVICE_GAD_STR},
    {ID_PREFS_UNIT_GAD,(CONST_STRPTR)ID_PREFS_UNIT_GAD_STR},
    {ID_PREFS_PARTITION_NAME_GAD,(CONST_STRPTR)ID_PREFS_PARTITION_NAME_GAD_STR},
    {ID_PREFS_READ_ONLY_GAD,(CONST_STRPTR)ID_PREFS_READ_ONLY_GAD_STR},
    {ID_PREFS_FILE_GAD,(CONST_STRPTR)ID_PREFS_FILE_GAD_STR},
    {ID_PREFS_FILE_SELECT_GAD,(CONST_STRPTR)ID_PREFS_FILE_SELECT_GAD_STR},
    {ID_CREATE_NAME_ASL_GAD,(CONST_STRPTR)ID_CREATE_NAME_ASL_GAD_STR},
    {ID_CREATE_NAME_GAD,(CONST_STRPTR)ID_CREATE_NAME_GAD_STR},
    {ID_CREATE_OK_GAD,(CONST_STRPTR)ID_CREATE_OK_GAD_STR},
    {ID_CREATE_CANCEL_GAD,(CONST_STRPTR)ID_CREATE_CANCEL_GAD_STR},
    {ID_CREATE_SIZE_GAD,(CONST_STRPTR)ID_CREATE_SIZE_GAD_STR},
    {ID_OK_GAD,(CONST_STRPTR)ID_OK_GAD_STR},
    {ID_CANCEL_GAD,(CONST_STRPTR)ID_CANCEL_GAD_STR},
    {ID_END,(CONST_STRPTR)ID_END_STR},
};

#endif /* CATCOMP_ARRAY */


/****************************************************************************/



#endif /* LOCALE_H */
