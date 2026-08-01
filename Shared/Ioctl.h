#pragma once

// User mode and kernel mode both include this file after their Windows/WDK headers.
#define IOCTL_MISSION001_SEND_PROCESS_INFO \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MISSION001_PROCESS_INFO {
    UINT32 ProcessId;
    UINT32 Reserved;
    UINT64 VirtualAddress;
} MISSION001_PROCESS_INFO, *PMISSION001_PROCESS_INFO;

