

#ifndef   __DIAGNOSTIC_FUNCTIONS_H__
#define   __DIAGNOSTIC_FUNCTIONS_H__

#include <ntddk.h>
#include <usb.h> 
#include <hidusage.h>
#include <hidpi.h>

/** /
typedef struct _USAGE_AND_PAGE
{
	USAGE Usage;
	USAGE UsagePage;
} USAGE_AND_PAGE, *PUSAGE_AND_PAGE;
/**/

extern VOID PrintPuapAndMessage(PUSAGE_AND_PAGE puap);
extern VOID printPmdl(PMDL pmdl);
extern VOID printPotentialUrb(PURB Urb);


#endif
