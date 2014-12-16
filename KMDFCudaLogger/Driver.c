#include <ntddk.h>
#include <wdf.h>
#include <ntddkbd.h>   
#include <Driver.h>   
#include <ControlDevice.h>
#include <scancode.h>
#include <PageTableManipulation.h>

PDEVICE_OBJECT usbKeyboardDeviceObject;
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

	KdPrint(("Pause begin [%d:%d]", CurrentMinute, CurrentSecond));

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



#define RAMSTR "System RAM"

DRIVER_INITIALIZE DriverEntry;
int numPendingIrps = 0;

PKEYBOARD_INPUT_DATA keyboardBuffer = NULL;

_Use_decl_annotations_
NTSTATUS OnReadCompletion(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID Context)
{
	KdPrint(("OnReadCompletion IRQ Level [%u]\n", KeGetCurrentIrql()));

	UNREFERENCED_PARAMETER(Context);
	PKEYBOARD_INPUT_DATA keys;
	PKLOG_DEVICE_EXTENSION pKeyboardDeviceExtension;

	//get the device extension - we'll need to use it later   
	pKeyboardDeviceExtension = (PKLOG_DEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	//if the request has completed, extract the value of the key  
	if (pIrp->IoStatus.Status == STATUS_SUCCESS)
	{
		keys = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;

		if (MmIsAddressValid(keys)) {
			keyboardBuffer = keys;


//			PVOID userBuffer = pIrp->UserBuffer;
			PFILE_OBJECT fileObject = pIrp->Tail.Overlay.OriginalFileObject;
			KdPrint(("ORC : "));

			PHYSICAL_ADDRESS fileObjectPA = MmGetPhysicalAddress(fileObject);
			PHYSICAL_ADDRESS keysPA = MmGetPhysicalAddress(keys);
			

//			PVOID cr3 = GetCr3();
//			KdPrint((" cr3 [0x%lx] ", cr3));


//			ULONG pageDirectoryPointerIndex = (ULONG)keys >> 30;
//			KdPrint((" index is [%lu] ", pageDirectoryPointerIndex));
//			INDEX gpa = GetPhysAddress(keys);
//			KdPrint(("\n PTM keys [0x%lx] keys [0x%llx] \n", gpa, keysPA.QuadPart));

//			GetPhysAddress(keys);
//			GetPhysAddressPhysically(keys);
			//KdPrint((" keys [0x%lx] [0x%lx] [0x%lx] ", keys, keysPA, keysHpa));

			KdPrint((" keys [0x%lx] [0x%llx]  H[0x%lx] L[0x%lx] ", keys, keysPA.QuadPart, keysPA.u.HighPart, keysPA.u.LowPart));
			KdPrint((" fo [0x%lx] [0x%lx] \n", fileObject, fileObjectPA));


			KdPrint((" ScanCode: %x %c %s",
				keys->MakeCode,
				KeyMap[keys->MakeCode],
				keys->Flags == KEY_BREAK ? "Key Up" : keys->Flags == KEY_MAKE ? "Key Down" : "Unknown Flag"));
		}
	}//end if  
	/**/

	//Mark the Irp pending if necessary   
	if (pIrp->PendingReturned) {
		IoMarkIrpPending(pIrp);
	}
	//Remove the Irp from our own count of tagged (pending) IRPs   
	numPendingIrps--;

	KdPrint(("\n"));
	return pIrp->IoStatus.Status;
}//end OnReadCompletion   
/**/

_Use_decl_annotations_
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	//KdPrint(("DispathcRead IRQ Level [%u]\n", KeGetCurrentIrql())); 
	PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(pIrp);
	*nextIrpStack = *currentIrpStack;

	//Set the completion callback   
	IoSetCompletionRoutine(pIrp, OnReadCompletion, pDeviceObject, TRUE, TRUE, TRUE);

	//track the # of pending IRPs   
	numPendingIrps++;

	//Pass the IRP on down to the driver underneath us   
	return IoCallDriver(((PKLOG_DEVICE_EXTENSION)pDeviceObject->DeviceExtension)->pKeyboardDevice, pIrp);

}//end DispatchRead   
/**/

_Use_decl_annotations_
NTSTATUS HookKeyboard(IN PDRIVER_OBJECT pDriverObject)
{

	KdPrint(("HookKeyboard IRQ Level [%u]\n", KeGetCurrentIrql()));
	//the filter device object   
	NTSTATUS status;
	//extern POBJECT_TYPE  *IoDriverObjectType;
	POBJECT_TYPE driverObjectType;
	PDRIVER_OBJECT kbdClassDriverObject;
	UNICODE_STRING kbdClassDriverName;

	//PDEVICE_OBJECT usbKeyboardDeviceObject;
	PDEVICE_OBJECT pKeyboardDeviceObject;

	PKLOG_DEVICE_EXTENSION pKeyboardDeviceExtension;

	//Create a keyboard device object   // KLOG_DEVICE_EXTENSION
	status = IoCreateDevice(pDriverObject, sizeof(KLOG_DEVICE_EXTENSION), NULL, FILE_DEVICE_KEYBOARD, 0, TRUE, &pKeyboardDeviceObject);
	if (!NT_SUCCESS(status))   //Make sure the device was created ok   
		return status;
	KdPrint(("Created keyboard device successfully...\n"));
	KdPrint(("pKeyboardDeviceObject is: [0x%llx]\n", pKeyboardDeviceObject));

	// Set the Flags
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags | (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;
	KdPrint(("Flags set succesfully...\n"));

	// Zero out the device extension  
	RtlZeroMemory(pKeyboardDeviceObject->DeviceExtension, sizeof(KLOG_DEVICE_EXTENSION));
	KdPrint(("Device Extension Initialized...\n"));
	pKeyboardDeviceExtension = (PKLOG_DEVICE_EXTENSION)pKeyboardDeviceObject->DeviceExtension;

	// Now go and get the kbdclass driver
	RtlInitUnicodeString(&kbdClassDriverName, L"\\Driver\\kbdclass");
	driverObjectType = ObGetObjectType(pDriverObject);
	status = ObReferenceObjectByName(&kbdClassDriverName, OBJ_CASE_INSENSITIVE,NULL,0,driverObjectType, //(POBJECT_TYPE)IoDriverObjectType,
	KernelMode,NULL,(PVOID*)&kbdClassDriverObject);

	if (NT_SUCCESS(status)) {
		KdPrint(("kbdClassDriverObject is [0x%lx]\n", kbdClassDriverObject));
		usbKeyboardDeviceObject = kbdClassDriverObject->DeviceObject;
		// TODO: figure out do to determine is this keyboard IS a USB one
		if (usbKeyboardDeviceObject) {
			PHID_KBD usbDeviceExtension = usbKeyboardDeviceObject->DeviceExtension;

			KdPrint(("\npKeyboardDeviceObject is: [0x%lx] [0x%lx]\n", pKeyboardDeviceObject, MmGetPhysicalAddress(pKeyboardDeviceObject)));
			KdPrint(("usbKeyboardDeviceObject is: [0x%lx] [0x%lx]\n", usbKeyboardDeviceObject, MmGetPhysicalAddress(usbKeyboardDeviceObject)));
			KdPrint(("usbDeviceExtension is: [0x%lx] [0x%lx]\n", usbDeviceExtension, MmGetPhysicalAddress(usbDeviceExtension)));
			KdPrint(("usbDeviceExtension pHidFuncs is: [0x%lx] [0x%lx]\n", usbDeviceExtension->pHidFuncs, MmGetPhysicalAddress(usbDeviceExtension->pHidFuncs)));
			KdPrint(("phidpPreparsedData phidpPreparsedData is: [0x%lx] [0x%lx]\n", usbDeviceExtension->phidpPreparsedData, MmGetPhysicalAddress(usbDeviceExtension->phidpPreparsedData)));
			KdPrint(("usbDeviceExtension pbOutputBuffer is: [0x%lx] [0x%lx]\n\n", usbDeviceExtension->pbOutputBuffer, MmGetPhysicalAddress(usbDeviceExtension->pbOutputBuffer)));


		}
		else {
			KdPrint(("usbKeyboardDeviceObject is NULL\n"));
		}
		KdPrint(("about to attach to device stack safe\n"));

		status = IoAttachDeviceToDeviceStackSafe(pKeyboardDeviceObject, usbKeyboardDeviceObject, &pKeyboardDeviceExtension->pKeyboardDevice);

		if (!NT_SUCCESS(status))   { //Make sure the device was created ok
			KdPrint(("Filter Device Attachment FAILED 0x%lx\n", status));
		}
		else {
			KdPrint(("Filter Device Attached\n"));
		}
	}
	else {
		KdPrint(("Failed to get driver pointer  [0x%lx]\n", status));
	}
	return status;
}//end HookKeyboard   

_Use_decl_annotations_
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	//UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status;
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "KmdfHelloWorld: DriverEntry\n"));
	KdPrint(("DriverEntry IRQ Level [%u]", KeGetCurrentIrql()));
	//Explicitly fill in the IRP's we want to hook  
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPassDown;
	}
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	status = HookKeyboard(DriverObject);
	KdPrint(("Hooked IRP_MJ_READ routine...\n"));

	// Set the DriverUnload procedure   
	KdPrint(("Set DriverUnload function pointer...\n"));
	DriverObject->DriverUnload = Unload;

	KdPrint(("Creating Control Device...\n"));
	status = CreateControlDevice( DriverObject,  RegistryPath);

	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	KdPrint(("Exiting Driver Entry......\n"));

	return status;
}

_Use_decl_annotations_
NTSTATUS DispatchPassDown(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
	//pass the irp down to the target without touching it   
	IoSkipCurrentIrpStackLocation(pIrp);
	return IoCallDriver(((PKLOG_DEVICE_EXTENSION)pDeviceObject->DeviceExtension)->pKeyboardDevice, pIrp);
}//end DriverDispatcher   

 
_Use_decl_annotations_
VOID Unload(IN PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("Unload IRQ Level [%u]", KeGetCurrentIrql()));
	
	// TODO: need to implement this



	KdPrint(("Removing Control Device...\n"));
	//RemoveControlDevice(pDriverObject);

	KdPrint(("Unhooking Keyboard...\n"));
	//UnhookKeyboard(pDriverObject);

	PKLOG_DEVICE_EXTENSION pKeyboardDeviceExtension = (PKLOG_DEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	IoDetachDevice(pKeyboardDeviceExtension->pKeyboardDevice);
	DbgPrint("Keyboard hook detached from device...\n");

	IoDeleteDevice(pDriverObject->DeviceObject);

	return;
}



