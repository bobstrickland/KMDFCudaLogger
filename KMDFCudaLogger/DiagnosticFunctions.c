#include <DiagnosticFunctions.h>

VOID PrintPuapAndMessage(PUSAGE_AND_PAGE puap) {
	if (puap) {
		DbgPrint(" [0x%lx]  [0x%lx] [0x%lx] ", puap, puap->Usage, (PVOID)puap->UsagePage);
	}
	else {
		DbgPrint(" [NULL]  [NULL] [NULL] ");
	}
}
VOID printPmdl(PMDL pmdl) {

	if (pmdl) {
		DbgPrint("      PMDL is [0x%lx] %s\n", pmdl, MmIsAddressValid(pmdl) ? "Valid Address" : "INVALID Address");
		if (MmIsAddressValid(pmdl)) {
			PVOID MappedSystemVa = pmdl->MappedSystemVa;   /* see creators for field size annotations. */
			PVOID StartVa = pmdl->StartVa;   /* see creators for validity; could be address 0.  */
			PVOID Next = pmdl->Next;
			DbgPrint("        PMDL->Size is [%d] \n", pmdl->Size);
			DbgPrint("        PMDL->MdlFlags is [%d] \n", pmdl->MdlFlags);
			DbgPrint("        PMDL->ByteCount is [%lu] \n", pmdl->ByteCount);
			DbgPrint("        PMDL->ByteOffset is [%lu] \n", pmdl->ByteOffset);
			if (MappedSystemVa) {
				DbgPrint("        PMDL->MappedSystemVa is [0x%lx] %s\n", MappedSystemVa, MmIsAddressValid(MappedSystemVa) ? "Valid Address" : "INVALID Address");
			}
			else {
				DbgPrint("        PMDL->MappedSystemVa is NULL \n");
			}
			if (StartVa) {
				DbgPrint("        PMDL->StartVa is [0x%lx] %s \n", StartVa, MmIsAddressValid(MappedSystemVa) ? "Valid Address" : "INVALID Address");
			}
			else {
				DbgPrint("        PMDL->StartVa is NULL \n");
			}
			if (Next) {
				DbgPrint("        PMDL->Next is [0x%lx] %s\n", Next, MmIsAddressValid(MappedSystemVa) ? "Valid Address" : "INVALID Address");
			}
			else {
				DbgPrint("        PMDL->Next is NULL \n");
			}
		}
	}
	else {
		DbgPrint("      PMDL is NULL \n");
	}
}

VOID printPotentialUrb(PURB Urb) {
	if (Urb) {
		if (MmIsAddressValid(Urb)) {
			DbgPrint("  Urb is [0x%lx] \n", Urb);
			struct _URB_HEADER urbHeader = Urb->UrbHeader;
			DbgPrint("    URB Header.Length is           [%u]\n", urbHeader.Length);
			DbgPrint("    URB Header.Function is         [0x%x]\n", urbHeader.Function);
			DbgPrint("    URB Header.Status is           [0x%lx]\n", urbHeader.Status);
			DbgPrint("    URB Header.UsbdDeviceHandle is [0x%lx]\n", urbHeader.UsbdDeviceHandle);
			DbgPrint("    URB Header.UsbdFlags is        [0x%lx]\n", urbHeader.UsbdFlags);
			if (urbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER) { // 0x0009
				struct _URB_BULK_OR_INTERRUPT_TRANSFER  UrbBulkOrInterruptTransfer = Urb->UrbBulkOrInterruptTransfer;
				PVOID TransferBuffer = UrbBulkOrInterruptTransfer.TransferBuffer;
				ULONG TransferBufferLength = UrbBulkOrInterruptTransfer.TransferBufferLength;
				if (TransferBuffer) {
					if (MmIsAddressValid(TransferBuffer)) {
						DbgPrint("    TransferBuffer [0x%lx] is valid.\n", TransferBuffer);
					}
					else {
						DbgPrint("    TransferBuffer [0x%lx] is NOT valid\n", TransferBuffer);
					}
				}
				else {
					DbgPrint("    TransferBuffer is NULL \n");
				}
				DbgPrint("    TransferBufferLength is [%u] \n", TransferBufferLength);
				PMDL pmdl = UrbBulkOrInterruptTransfer.TransferBufferMDL;
				printPmdl(pmdl);
			}
			else {
				DbgPrint("    URB function is not URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER\n");
			}
		}
		else {
			DbgPrint("  Urb is not valid! \n");
		}
	}
	else {
		DbgPrint("  Urb is NULL \n");
	}
}

