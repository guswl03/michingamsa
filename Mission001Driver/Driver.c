#include <ntddk.h>
#include <wdf.h>
#include <wdmsec.h>

#include "../Shared/Ioctl.h"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD Mission001EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL Mission001EvtIoDeviceControl;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
        "[Mission001] DriverEntry: driver loaded.\n");

    WDF_DRIVER_CONFIG_INIT(&config, Mission001EvtDeviceAdd);
    return WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE);
}

NTSTATUS
Mission001EvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    DECLARE_CONST_UNICODE_STRING(deviceName, L"\\Device\\Mission001");
    DECLARE_CONST_UNICODE_STRING(symbolicLink, L"\\DosDevices\\Mission001");
    DECLARE_CONST_UNICODE_STRING(sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");

    // Only SYSTEM and administrators may open the exercise device.
    status = WdfDeviceInitAssignSDDLString(DeviceInit, &sddl);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfDeviceInitAssignName(DeviceInit, &deviceName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    status = WdfDeviceCreate(
        &DeviceInit,
        WDF_NO_OBJECT_ATTRIBUTES,
        &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = Mission001EvtIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE);

    if (NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
            "[Mission001] Device ready: \\\\.\\Mission001\n");
    }

    return status;
}

VOID
Mission001EvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PMISSION001_PROCESS_INFO info = NULL;
    size_t bufferLength = 0;

    if (IoControlCode == IOCTL_MISSION001_SEND_PROCESS_INFO) {
        if (InputBufferLength < sizeof(MISSION001_PROCESS_INFO)) {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            status = WdfRequestRetrieveInputBuffer(
                Request,
                sizeof(MISSION001_PROCESS_INFO),
                (PVOID*)&info,
                &bufferLength);

            if (NT_SUCCESS(status)) {
                DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,
                    "[Mission001] IOCTL received: PID=%lu, VirtualAddress=0x%016llX\n",
                    (ULONG)info->ProcessId,
                    (unsigned long long)info->VirtualAddress);
            }
        }
    }

    WdfRequestComplete(Request, status);
}
