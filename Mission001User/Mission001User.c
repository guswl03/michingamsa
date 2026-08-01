#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "../Shared/Ioctl.h"

int wmain(int argc, wchar_t** argv)
{
    volatile UINT64 sampleValue = 0x1122334455667788ULL;
    MISSION001_PROCESS_INFO info = { 0 };

    if (argc == 1) {
        info.ProcessId = GetCurrentProcessId();
        info.VirtualAddress = (UINT64)(ULONG_PTR)&sampleValue;
    } else if (argc == 3) {
        info.ProcessId = (UINT32)wcstoul(argv[1], NULL, 0);
        info.VirtualAddress = _wcstoui64(argv[2], NULL, 0);
    } else {
        fwprintf(stderr, L"Usage: Mission001User.exe [PID VirtualAddress]\n");
        fwprintf(stderr, L"Example: Mission001User.exe 1234 0x7FF612341000\n");
        return 2;
    }

    HANDLE device = CreateFileW(
        L"\\\\.\\Mission001",
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (device == INVALID_HANDLE_VALUE) {
        fwprintf(stderr,
            L"[FAIL] Cannot open \\\\.\\Mission001 (Win32 error %lu).\n"
            L"       Run as administrator and confirm the driver service is running.\n",
            GetLastError());
        return 1;
    }

    wprintf(L"[SEND] PID=%lu, VirtualAddress=0x%016llX\n",
        (unsigned long)info.ProcessId,
        (unsigned long long)info.VirtualAddress);

    DWORD bytesReturned = 0;
    BOOL ok = DeviceIoControl(
        device,
        IOCTL_MISSION001_SEND_PROCESS_INFO,
        &info,
        (DWORD)sizeof(info),
        NULL,
        0,
        &bytesReturned,
        NULL);

    if (!ok) {
        fwprintf(stderr, L"[FAIL] DeviceIoControl failed (Win32 error %lu).\n", GetLastError());
        CloseHandle(device);
        return 1;
    }

    wprintf(L"[OK] The driver accepted the IOCTL. Check WinDbg.\n");
    CloseHandle(device);
    return 0;
}
