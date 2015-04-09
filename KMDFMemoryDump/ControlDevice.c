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
//	size_t    memoryLength;


	//PHYSICAL_ADDRESS PRAMIN_CONTROL_ADDRESS = { 0x1700, 0x0 };
	//PHYSICAL_ADDRESS PRAMIN_ADDRESS = { 0x700000, 0x0 }; //at the address 0x700000-0x7fffff
	//ULONG            PRAMIN_LENGTH = 0x100000;









	/** /

	if (pciVideoDeviceObject) {
		KdPrint(("pciVideoDeviceObject is [0x%lx]\n", pciVideoDeviceObject));
		pauseForABit(10);

//		PCI_COMMON_CONFIG commonConfig;
		PCI_COMMON_HEADER PciHeader;
		PPCI_COMMON_CONFIG PciConfig = (PPCI_COMMON_CONFIG)&PciHeader;
		status = ReadConfigSpace(pciVideoDeviceObject, PciConfig, 0, sizeof(PCI_COMMON_HEADER));
		if (NT_SUCCESS(status)) {
			if (PciConfig) {
				KdPrint(("ReadConfigSpace: Type %d\n", PciHeader.HeaderType));
				pauseForABit(10);
				Bar0Address = PciHeader.u.type0.BaseAddresses[0];
				HostMemAddress = Bar0Address + 0x1700;
				WindowAddress = Bar0Address + 0x700000;
				KdPrint(("   Base Address 0   [0x%lx]\n", Bar0Address));
				KdPrint(("   Host Mem Address [0x%lx]\n", HostMemAddress));
				KdPrint(("   Window Address   [0x%lx]\n\n", WindowAddress));
			}
			else {
				KdPrint(("ReadConfigSpace failed :-( \n"));
			}
		}
		else {
			KdPrint(("ReadConfigSpace failed ntstatus [0x%lx] \n", status));
		}
		pauseForABit(10);
	}
	/**/










	


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
			HostMemPhysicalAddress.LowPart = HostMemAddress; // 0xf6001700;
			HostMem = (PULONG)MmMapIoSpace(HostMemPhysicalAddress, sizeof(ULONG), MmNonCached);
		}


		KdPrint(("ReadDeviceMemory BufferOffset was %lu.\n", userSharedMemory->BufferOffset));
		if (HostMem) {
			KdPrint(("ReadDeviceMemory HostMem Mapped. Address [0x%lx] Value was [0x%lx]\n", HostMem, *HostMem));
			if (userSharedMemory->BufferOffset != 666) {
				originalHostMemValue = *HostMem;
				*HostMem = userSharedMemory->BufferOffset;
				KdPrint(("ReadDeviceMemory HostMem Mapped. Address [0x%lx] Value is [0x%lx]\n", HostMem, *HostMem));
			}
		}
		else {
			KdPrint(("ReadDeviceMemory HostMem Map failed\n"));

		}

		if (!Pramin) {
			PHYSICAL_ADDRESS PraminPhysicalAddress;
			PraminPhysicalAddress.HighPart = 0;
			PraminPhysicalAddress.LowPart = WindowAddress; // 0xf6700000;
			Pramin = (PULONG)MmMapIoSpace(PraminPhysicalAddress, PRAMIN_LENGTH, MmNonCached);
		}

		if (Pramin) {
			KdPrint(("ReadDeviceMemory Pramin Mapped\n"));
			memcpy(userSharedMemory->ClientMemory, Pramin, PRAMIN_LENGTH);
			KdPrint(("ReadDeviceMemory Pramin Coppied\n"));
		}
		else {
			KdPrint(("ReadDeviceMemory Pramin Map failed\n"));
		}


		if (userSharedMemory->BufferOffset != 666) {
			*HostMem = originalHostMemValue;
			KdPrint(("ReadDeviceMemory HostMem Mapped. Address [0x%lx] Value now [0x%lx]\n", HostMem, *HostMem));
		}


		/** /
		PHYSICAL_ADDRESS baseAddress;
		baseAddress.HighPart = 0;
		baseAddress.LowPart = 0xf6000000;
		ULONG nbytes =  0xffffff;
		PUCHAR membase = (PUCHAR)MmMapIoSpace(baseAddress, nbytes, MmNonCached);
		if (membase) {
			KdPrint(("ReadDeviceMemory Memory Mapped\n"));
			memcpy(userSharedMemory->ClientMemory, membase, nbytes);
			KdPrint(("ReadDeviceMemory Memory Coppied\n"));
		}
		else {
			KdPrint(("ReadDeviceMemory Memory Map failed\n"));
		}
		/**/




		/** /
		if (pciVideoDeviceObject) {
			KdPrint(("pciVideoDeviceObject is [0x%lx]\n", pciVideoDeviceObject));
			pauseForABit(10);
			BUS_INTERFACE_STANDARD busInterface;
			status = GetBusInterface(pciVideoDeviceObject, &busInterface);
			if (!NT_SUCCESS(status)) {
				KdPrint(("Failed to get Bus Interface [0x%lx]\n", status));
			} // STATUS_NOT_SUPPORTED
			else {
				PCI_COMMON_HEADER Header;
				PCI_COMMON_CONFIG *pPciConfig = (PCI_COMMON_CONFIG *)&Header;
				//UCHAR CapabilityOffset;
				busInterface.GetBusData(busInterface.Context, PCI_WHICHSPACE_CONFIG, pPciConfig, 0, sizeof(PCI_COMMON_HEADER)); // just 64 bytes
				if (pPciConfig) {
					KdPrint(("PCI_CONFIG::  BaseClass [0x%x]  BIST [0x%x]  CacheLineSize [0x%x]  Command [0x%x]  DeviceID [0x%x]  HeaderType [0x%x]  LatencyTimer [0x%x]  ProgIf [0x%x]  RevisionID [0x%x]  Status [0x%x]  SubClass [0x%x]  VendorID [0x%x] \n",
						Header.BaseClass, Header.BIST, Header.CacheLineSize, Header.Command,
						Header.DeviceID, Header.HeaderType, Header.LatencyTimer, Header.ProgIf,
						Header.RevisionID, Header.Status, Header.SubClass, Header.VendorID
						));
				}
				else {
					KdPrint(("FAILED to get PCI_CONFIG\n"));
				}
			}

		}
		/**/


		//	device id  10DE 1381 1381196E
		// PCI Bus 1 device 0 function 0
		//pciVideoDeviceObject->DriverObject->


		// Bar0Address;
		// HostMemAddress;
		// WindowAddress;
		/** /
		ULONG SavedValue = *HostMemPointer;
		*HostMemPointer = userSharedMemory->BufferOffset;
		memcpy(userSharedMemory->ClientMemory, WindowPointer, PRAMIN_LENGTH);
		*HostMemPointer = SavedValue;
		/**/
		// WindowPointer;

		/**/
		// ok we got a good call

		/* begin current code HERE * /
		ULONG WindowOffset = userSharedMemory->WindowOffset;
		ULONG BufferOffset = userSharedMemory->BufferOffset;

		PULONG HostMemPointer = HostMemAddress;
		KdPrint(("   Host Mem Pointer [0x%lx] [0x%lx]\n", HostMemPointer, *HostMemPointer));
		ULONG saveValue = *HostMemPointer;
		*HostMemPointer = WindowOffset;
		KdPrint(("   Host Mem Pointer [0x%lx] [0x%lx]\n", HostMemPointer, *HostMemPointer));


		PCHAR WindowPointer = WindowAddress;
		KdPrint(("   Window Pointer [0x%lx] [0x%lx]\n", WindowPointer, *WindowPointer));
		memcpy(userSharedMemory->ClientMemory, WindowPointer, PRAMIN_LENGTH);

		*HostMemPointer = saveValue;

		KdPrint(("   Host Mem Pointer [0x%lx] [0x%lx]\n", HostMemPointer, *HostMemPointer));
		/* end current code HERE */




		/** /
		ULONG ob;
		ULONG vb;
		PULONG originalBuffer = &ob;
		PULONG verifyBuffer = &vb;
		//PULONG originalBuffer = (PULONG)ExAllocatePoolWithTag(NonPagedPool, sizeof(ULONG), 'evol');
		//PULONG verifyBuffer = (PULONG)ExAllocatePoolWithTag(NonPagedPool, sizeof(ULONG), 'evol');
		//ULONG BufferOffset = 0x1700;
		//ULONG WindowOffset = 0x700000;
		ULONG Length = sizeof(ULONG);

		status = ReadConfigSpace(pciVideoDeviceObject, originalBuffer, BufferOffset, Length);
		if (!NT_SUCCESS(status)) {
			KdPrint(("ReadDeviceMemory Could not ReadConfigSpace 0x%x\n", status));
		}
		status = WriteConfigSpace(pciVideoDeviceObject, &WindowOffset, BufferOffset, Length);
		if (!NT_SUCCESS(status)) {
			KdPrint(("ReadDeviceMemory Could not WriteConfigSpace 0x%x\n", status));
		}
		status = ReadConfigSpace(pciVideoDeviceObject, verifyBuffer, BufferOffset, Length);
		if (!NT_SUCCESS(status)) {
			KdPrint(("ReadDeviceMemory Could not ReadConfigSpace 0x%x\n", status));
		}

		/** /
		KdPrint(("ReadDeviceMemory:  map PRAMIN\n"));
		PCHAR PRAMIN = (PCHAR)MmMapIoSpace(PRAMIN_ADDRESS, PRAMIN_LENGTH, MmNonCached);

		KdPrint(("MEMORY COPY : %X%X%X%X%X%X%X%X%X%X%X%X\n", PRAMIN[0], PRAMIN[1], PRAMIN[2], PRAMIN[3], PRAMIN[4], PRAMIN[5], PRAMIN[6], PRAMIN[7], PRAMIN[8], PRAMIN[9], PRAMIN[10], PRAMIN[11]));
		memcpy(userSharedMemory->ClientMemory, PRAMIN, PRAMIN_LENGTH);
		/** /

		// now set it back to it';s original value
		status = WriteConfigSpace(pciVideoDeviceObject, originalBuffer, BufferOffset, Length);
		if (!NT_SUCCESS(status)) {
			KdPrint(("ReadDeviceMemory Could not WriteConfigSpace 0x%x\n", status));
		}
		KdPrint(("ReadDeviceMemory: Buffer Window was [0x%lx] ==changed=to==> [0x%lx]     Buffer Offset [0x%lx]  Window Offset [0x%lx]\n", *originalBuffer, *verifyBuffer, BufferOffset, WindowOffset));
		/**/
		KdPrint(("ReadDeviceMemory: EXIT \n"));
		status = STATUS_SUCCESS;
	}

	// Set transfer information
	KdPrint(("ReadDeviceMemory returning to user length [%u] status [0x%lx]\n", Length, status));
	WdfRequestCompleteWithInformation(Request, status, &Length);
	return;

}
