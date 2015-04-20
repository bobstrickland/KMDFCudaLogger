#include <ntddk.h>
#include <wdf.h>
//#include <ntddkbd.h>   
#include <ControlDevice.h>
#include <ControlDeviceConstants.h>
#include "AccessBus.h"

WDFDEVICE ControlDevice;
WDFQUEUE                    queue;


// 0 for read 1 for write
NTSTATUS ReadWriteConfigSpace(IN PDEVICE_OBJECT DeviceObject,IN ULONG ReadOrWrite, IN PVOID Buffer,IN ULONG Offset,IN ULONG Length) {
	KEVENT event;
	NTSTATUS status;
	PIRP irp;
	IO_STATUS_BLOCK ioStatusBlock;
	PIO_STACK_LOCATION irpStack;
	PDEVICE_OBJECT targetObject;

	PAGED_CODE();

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	targetObject = IoGetAttachedDeviceReference(DeviceObject);

	KdPrint(("ReadDeviceMemory: %s Config Device [0x%lx] Target [0x%lx] Buffer [0x%lx], Offest [%lu] Length [%lu]\n", (ReadOrWrite ? "Write" : "Read"), DeviceObject, targetObject, *(PULONG)Buffer, Offset, Length));
	irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, targetObject, NULL, 0, NULL, &event, &ioStatusBlock);

	if (irp == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto End;
	}

	irpStack = IoGetNextIrpStackLocation(irp);

	if (ReadOrWrite == 0) {
		irpStack->MinorFunction = IRP_MN_READ_CONFIG;
	}
	else {
		irpStack->MinorFunction = IRP_MN_WRITE_CONFIG;
	}

	irpStack->Parameters.ReadWriteConfig.WhichSpace = PCI_WHICHSPACE_CONFIG;
	irpStack->Parameters.ReadWriteConfig.Buffer = Buffer;
	irpStack->Parameters.ReadWriteConfig.Offset = Offset;
	irpStack->Parameters.ReadWriteConfig.Length = Length;

	// 
	// Initialize the status to error in case the bus driver does not 
	// set it correctly.
	// 

	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	status = IoCallDriver(targetObject, irp);

	if (status == STATUS_PENDING) {

		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = ioStatusBlock.Status;
	}

End:
	// 
	// Done with reference
	// 
	ObDereferenceObject(targetObject);

	return status;

}


NTSTATUS ReadConfigSpace(IN PDEVICE_OBJECT DeviceObject, IN PVOID Buffer, IN ULONG Offset, IN ULONG Length) {
	return ReadWriteConfigSpace(DeviceObject, 0, Buffer, Offset, Length);
}
NTSTATUS WriteConfigSpace(IN PDEVICE_OBJECT DeviceObject, IN PVOID Buffer, IN ULONG Offset, IN ULONG Length) {
	return ReadWriteConfigSpace(DeviceObject, 1, Buffer, Offset, Length);
}

VOID pauseForABit(CSHORT secondsDelay) {

	LARGE_INTEGER systemTime;
	LARGE_INTEGER currentTime;
	TIME_FIELDS formattedTime;

	KeQuerySystemTime(&systemTime);
	ExSystemTimeToLocalTime(&systemTime, &currentTime);
	RtlTimeToTimeFields(&currentTime, &formattedTime);
	CSHORT ExitMinute = formattedTime.Minute;
	CSHORT ExitSecond = formattedTime.Second + secondsDelay;
	while (ExitSecond > 59) {
		ExitSecond = ExitSecond - 60;
		ExitMinute = ExitMinute + 1;
	}
	while (ExitMinute > 59) {
		ExitMinute = ExitMinute - 60;
	}
	CSHORT CurrentMinute = formattedTime.Minute;
	CSHORT CurrentSecond = formattedTime.Second;

	KdPrint(("%d seconds pause begin [%d:%d]", secondsDelay, CurrentMinute, CurrentSecond));

	while (TRUE) {
		KeQuerySystemTime(&systemTime);
		ExSystemTimeToLocalTime(&systemTime, &currentTime);
		RtlTimeToTimeFields(&currentTime, &formattedTime);
		CurrentMinute = formattedTime.Minute;
		CurrentSecond = formattedTime.Second;
		if (CurrentMinute == ExitMinute && CurrentSecond >= ExitSecond) {
			break;
		}
	}
	KdPrint((" - [%d:%d]\n", CurrentMinute, CurrentSecond));
	return;
}


NTSTATUS CreateControlDevice(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath) {
	KdPrint(("CreateControlDevice IRQ Level [%u]", KeGetCurrentIrql()));

	NTSTATUS status;
	PWDFDEVICE_INIT ControlDeviceInit;
	WDF_OBJECT_ATTRIBUTES ControlDeviceAttributes;
	WDFDRIVER Driver;
	WDF_DRIVER_CONFIG       DriverConfig;
	WDF_IO_QUEUE_CONFIG         ioQueueConfig;
	//WDFQUEUE                    queue;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ControlDeviceAttributes, CONTROL_DEVICE_EXTENSION);
	WDF_DRIVER_CONFIG_INIT(&DriverConfig, WDF_NO_EVENT_CALLBACK);

	//WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
	// TODO: Attributes.EvtCleanupCallback = EvtDriverContextCleanup;

	status = WdfDriverCreate(DriverObject, RegistryPath, &ControlDeviceAttributes, &DriverConfig, &Driver);

	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDriverCreate FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDriverCreate Succeededx\n"));
	}

	ControlDeviceInit = WdfControlDeviceInitAllocate(Driver, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX);

	DECLARE_CONST_UNICODE_STRING(ntDeviceName, L"\\Device\\DeviceMemDump");
	DECLARE_CONST_UNICODE_STRING(symbolicLinkName, L"\\DosDevices\\DeviceMemDump");

	status = WdfDeviceInitAssignName(ControlDeviceInit, &ntDeviceName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceInitAssignName FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDeviceInitAssignName Succeeded\n"));
	}

	status = WdfDeviceCreate(&ControlDeviceInit, &ControlDeviceAttributes, &ControlDevice);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceCreate FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDeviceCreate Succeeded\n"));
	}

	status = WdfDeviceCreateSymbolicLink(ControlDevice, &symbolicLinkName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfDeviceCreateSymbolicLink FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfDeviceCreateSymbolicLink Succeeded\n"));
	}

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
	ioQueueConfig.DefaultQueue = TRUE;
	ioQueueConfig.EvtIoDefault = ReadDeviceMemory;
	status = WdfIoQueueCreate(ControlDevice, &ioQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue); // pointer to default queue
	if (!NT_SUCCESS(status)) {
		KdPrint(("WdfIoQueueCreate FAILED 0x%lx\n", status));
		return status;
	}
	else {
		KdPrint(("WdfIoQueueCreate Succeeded\n"));
	}
	WdfControlFinishInitializing(ControlDevice);

	KdPrint(("WdfControlFinishInitializing finished\n"));
	return status;
	// ControlDevice
}


NTSTATUS DestroyControlDevice(PDRIVER_OBJECT  DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);
	KdPrint(("DestroyControlDevice IRQ Level [%u]", KeGetCurrentIrql()));
	NTSTATUS status = STATUS_SUCCESS;
	//status = 
		WdfIoQueuePurge(queue, NULL, NULL);
	// TODO: here!!!
		if (HostMem) {
			MmUnmapIoSpace(HostMem, sizeof(ULONG));
			KdPrint(("ReadDeviceMemory HostMem unmapped\n"));
		}
		else {
			KdPrint(("ReadDeviceMemory HostMem never mapped\n"));
		}
		if (Pramin) {
			MmUnmapIoSpace(Pramin, 0x100000);
			KdPrint(("ReadDeviceMemory Pramin unmapped\n"));
		}
		else {
			KdPrint(("ReadDeviceMemory Pramin never mapped\n"));
		}
		return status;
}

_Use_decl_annotations_
VOID ReadDeviceMemory(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request) {

	UNREFERENCED_PARAMETER(Queue);

	KdPrint(("ReadDeviceMemory IRQ Level [%u]\n", KeGetCurrentIrql()));
	NTSTATUS  status;
	WDFMEMORY memoryHandle;
	ULONG     Length;

	Length = sizeof(SHARED_MEMORY_STRUCT);

	WDF_OBJECT_ATTRIBUTES  attributes;
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);


	KdPrint(("ReadDeviceMemory: get memory buffer\n"));
	status = WdfRequestRetrieveOutputMemory(Request, &memoryHandle);

	if (!NT_SUCCESS(status)) {
		KdPrint(("ReadDeviceMemory Could not create memory buffer 0x%x\n", status));
		WdfVerifierDbgBreakPoint();
		WdfRequestCompleteWithInformation(Request, status, 0L);
		return;
	}
	else {
		KdPrint(("ReadDeviceMemory memory buffer created.\n"));
	}

	PSHARED_MEMORY_STRUCT userSharedMemory = (PSHARED_MEMORY_STRUCT)WdfMemoryGetBuffer(memoryHandle, NULL);
	if (!(userSharedMemory)) {
		KdPrint(("ReadDeviceMemory WdfMemoryGetBuffer failed:\n"));
		WdfVerifierDbgBreakPoint();
		WdfRequestCompleteWithInformation(Request, status, 0L);
		return;
	}
	else {

		KdPrint(("ReadDeviceMemory Preparing for memory mapping\n"));
		ULONG originalHostMemValue;
		if (!HostMem) {
			PHYSICAL_ADDRESS HostMemPhysicalAddress;
			HostMemPhysicalAddress.HighPart = 0;
			HostMemPhysicalAddress.LowPart = HostMemAddress;
			HostMem = (PULONG)MmMapIoSpace(HostMemPhysicalAddress, sizeof(ULONG), MmNonCached);
			if (HostMem) {
				KdPrint(("ReadDeviceMemory: HostMem mapped [0x%llx] to [0x%lx]\n", HostMemPhysicalAddress.QuadPart, HostMem));
			}
			else {
				KdPrint(("ReadDeviceMemory: HostMem mapping failed\n"));
			}
		}
		if (!Pramin) {
			PHYSICAL_ADDRESS PraminPhysicalAddress;
			PraminPhysicalAddress.HighPart = 0;
			PraminPhysicalAddress.LowPart = WindowAddress;
			Pramin = (PULONG)MmMapIoSpace(PraminPhysicalAddress, PRAMIN_LENGTH, MmNonCached);
			if (Pramin) {
				KdPrint(("ReadDeviceMemory: PRAMIN mapped [0x%llx] to [0x%lx]\n", PraminPhysicalAddress.QuadPart, Pramin));
			}
			else {
				KdPrint(("ReadDeviceMemory: PRAMIN mapping failed\n"));
			}
		}

		if (HostMem && Pramin) {
			BOOLEAN ContinueMapping = TRUE;
			KdPrint(("ReadDeviceMemory HostMem Mapped. Address [0x%lx] Value was [0x%lx]\n", HostMem, *HostMem));
			originalHostMemValue = *HostMem;
			*HostMem = userSharedMemory->BufferOffset;
			KdPrint(("ReadDeviceMemory HostMem Mapped. Address [0x%lx] Value is [0x%lx]\n", HostMem, *HostMem));
			while (ContinueMapping) {
				memcpy(userSharedMemory->ClientMemory, Pramin, PRAMIN_LENGTH);
				if (*HostMem == userSharedMemory->BufferOffset) {
					*HostMem = originalHostMemValue;
					ContinueMapping = FALSE;
				}
				else {
					// something changed the mapping....so let's reset it and try this again....
					originalHostMemValue = *HostMem;
					*HostMem = userSharedMemory->BufferOffset;
					KdPrint(("ReadDeviceMemory: encountered HostMem reset\n"));
				}
			}
		}
		else {
			KdPrint(("ReadDeviceMemory HostMem or Pramin Map failed\n"));
		}
		KdPrint(("ReadDeviceMemory: EXIT \n"));
		status = STATUS_SUCCESS;
	}

	// Set transfer information
	KdPrint(("ReadDeviceMemory returning to user length [%u] status [0x%lx]\n", Length, status));
	WdfRequestCompleteWithInformation(Request, status, &Length);
	return;

}
