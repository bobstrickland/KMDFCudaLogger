#ifndef __AccessBus_h__ 
#define __AccessBus_h__ 

#include <ntddk.h>
#include <wdf.h>
NTSTATUS GetBusInterface(IN PDEVICE_OBJECT pcifido,	OUT PPCI_BUS_INTERFACE_STANDARD	busInterface);
NTSTATUS GetStandardBusInterface(IN PDEVICE_OBJECT targetObject, OUT PBUS_INTERFACE_STANDARD busInterface);


#endif
