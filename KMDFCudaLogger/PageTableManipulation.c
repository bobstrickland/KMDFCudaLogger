#include <PageTableManipulation.h>

ULONG GetPageDirectoryBaseRegister()
{
	ULONG returnValue = NULL;
	__asm
	{
		cli                   // disable interrupts
		pushad                // save the registers
		mov eax, cr3          // read the Page Directory Base Register from CR3
		mov returnValue, eax  // set returnValue with that value
		popad                 // restore the registers
		sti                   // re-enable interrupts
	}
	return returnValue;
}

PHYSICAL_ADDRESS GetPDBRPhysicalAddress()
{
	ULONG pdbrValue = GetPageDirectoryBaseRegister();
	PHYSICAL_ADDRESS returnValue;
	returnValue.u.HighPart = 0x0;
	returnValue.u.LowPart = pdbrValue;
	return returnValue;
}


PPTE GetPteAddress(PVOID virtualaddr){

	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21;
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	ULONG offset = (ULONG)virtualaddr & 0x0fff;
	DbgPrint("\n\nVirtualAddress [0x%lx] is [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryIndex, pageTableIndex, offset);
	DbgPrint("Looking conventional \n");

	PPTE pageDirectoryTable = (PPTE)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	DbgPrint("pageDirectoryTable   [0x%lx]", pageDirectoryTable);
	if (MmIsAddressValid(pageDirectoryTable)) {
		DbgPrint("[0x%lx] ", MmGetPhysicalAddress(pageDirectoryTable));
		ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
		DbgPrint("  PageFrameNumber is [0x%lx]\n", pdPFN);
		PPTE pageTable = (PPTE)(PROCESS_PAGE_TABLE_BASE + (pageTableIndex * PTE_SIZE) + (PAGE_SIZE * pageDirectoryIndex));
		DbgPrint("pageTable   [0x%lx] ", pageTable);
		if (MmIsAddressValid(pageTable)) {
			return pageTable;
		}
		else {
			DbgPrint(" is INVALID\n");
			return NULL;
		}
	}
	else {
		DbgPrint(" is INVALID\n");
		return NULL;
	}
}

ULONG GetPhysAddress(PVOID virtualaddr)
{
	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21;
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	ULONG offset = (ULONG)virtualaddr & 0x0fff;
	DbgPrint("\n\nVirtualAddress [0x%lx] is [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryIndex, pageTableIndex, offset);
	DbgPrint("Looking conventional \n");

	PPTE pageDirectoryTable = (PPTE)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	DbgPrint("pageDirectoryTable   [0x%lx]", pageDirectoryTable);
	if (MmIsAddressValid(pageDirectoryTable)) {
		DbgPrint("[0x%lx] ", MmGetPhysicalAddress(pageDirectoryTable));
		ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
		DbgPrint("  PageFrameNumber is [0x%lx]\n", pdPFN);
		PPTE pageTable = (PPTE)(PROCESS_PAGE_TABLE_BASE + (pageTableIndex * PTE_SIZE) + (PAGE_SIZE * pageDirectoryIndex));
		DbgPrint("pageTable   [0x%lx] ", pageTable);
		if (MmIsAddressValid(pageTable)) {
			DbgPrint("[0x%lx] ", MmGetPhysicalAddress(pageTable));
			ULONG ptPFN = pageTable->PageFrameNumber;
			ULONG baseAddress = ptPFN << 12;
			ULONG finalPhysicalAddress = baseAddress + offset;
			DbgPrint("  PageFrameNumber is [0x%lx] [0x%lx] [0x%lx]\n", ptPFN, baseAddress, finalPhysicalAddress);
			DbgPrint("Physical address for [0x%lx] is [0x%lx]\n", virtualaddr, finalPhysicalAddress);
			return finalPhysicalAddress;
		}
		else {
			DbgPrint(" is INVALID\n");
			return NULL;
		}
	}
	else {
		DbgPrint(" is INVALID\n");
		return NULL;
	}
}

ULONG GetPhysAddressPhysically(PVOID virtualaddr)
{
	PHYSICAL_ADDRESS pageDirPointerTablePA = GetPDBRPhysicalAddress();
	return GetPhysAddressPhysicallyWithPDPT(virtualaddr, pageDirPointerTablePA);
}


ULONG GetPhysAddressPhysicallyWithProcessHandle(PVOID virtualaddr, HANDLE processHandle)
{
	PNEPROCESS peProcess = NULL; // IoGetProcessId(processHandle);
	return GetPhysAddressPhysicallyWithProcess(virtualaddr, peProcess);
}

ULONG GetPhysAddressPhysicallyWithProcess(PVOID virtualaddr, PNEPROCESS peProcess)
{
	ULONG pdbrValue = peProcess->Pcb.DirectoryTableBase + 0x10;
	PHYSICAL_ADDRESS pageDirPointerTablePA;
	pageDirPointerTablePA.u.HighPart = 0x0;
	pageDirPointerTablePA.u.LowPart = pdbrValue;

	return GetPhysAddressPhysicallyWithPDPT(virtualaddr, pageDirPointerTablePA);
}

ULONG GetPhysAddressPhysicallyWithPDPT(PVOID virtualaddr, PHYSICAL_ADDRESS pageDirPointerTablePA)
{
	ULONG pageDirectoryPointerIndex = (ULONG)virtualaddr >> 30;
	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21 & 0x01FF;
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	ULONG offset = (ULONG)virtualaddr & 0x0fff;
	DbgPrint("\n\nVirtualAddress [0x%lx] is  [0x%lx] [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryPointerIndex, pageDirectoryIndex, pageTableIndex, offset);
	DbgPrint("Looking physical \n");

	// get the PageDirectoryPointerTable Entry
	
	pageDirPointerTablePA.QuadPart = pageDirPointerTablePA.QuadPart + (pageDirectoryPointerIndex*sizeof(PHYSICAL_ADDRESS));
	PPTE pageDirectoryPointerTable = MmMapIoSpace(pageDirPointerTablePA, sizeof(PTE), MmNonCached);
	if (MmIsAddressValid(pageDirectoryPointerTable)) {

		// get the PageDirectoryTable Entry
		ULONG pdpPFN = pageDirectoryPointerTable->PageFrameNumber;
		ULONG pdBaseAddress = pdpPFN << 12;
		ULONG pdPhysicalAddress = pdBaseAddress + (pageDirectoryIndex * PTE_SIZE);
		DbgPrint("pageDirectoryPointerTable   [0x%lx][0x%lx] PageFrameNumber is [0x%lx] [0x%lx] [0x%lx]\n", pageDirectoryPointerTable, pageDirPointerTablePA, pdpPFN, pdBaseAddress, pdPhysicalAddress);
		PHYSICAL_ADDRESS pdtPhysicalAddress;
		pdtPhysicalAddress.u.HighPart = 0x0;
		pdtPhysicalAddress.u.LowPart = pdPhysicalAddress;
		PPTE pageDirectoryTable = MmMapIoSpace(pdtPhysicalAddress, sizeof(PTE), MmNonCached);
		if (MmIsAddressValid(pageDirectoryTable)) {

			// get the PageTable Entry
			ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
			ULONG ptBasePhysicalAddress = pdPFN << 12;
			ULONG ptSpecificAddress = ptBasePhysicalAddress + (pageTableIndex * PTE_SIZE);
			DbgPrint("pageDirectoryTable   [0x%lx][0x%lx] PageFrameNumber is [0x%lx] [0x%lx] [0x%lx]\n", pageDirectoryTable, pdPhysicalAddress, pdPFN, ptBasePhysicalAddress, ptSpecificAddress);
			PHYSICAL_ADDRESS ptPhysicalAddress;
			ptPhysicalAddress.u.HighPart = 0x0;
			ptPhysicalAddress.u.LowPart = ptSpecificAddress;
			PPTE pageTable = MmMapIoSpace(ptPhysicalAddress, sizeof(PTE), MmNonCached);
			if (MmIsAddressValid(pageTable)) {

				// get the physical address
				ULONG ptPFN = pageTable->PageFrameNumber;
				ULONG basePhysicalAddress = ptPFN << 12;
				ULONG specificAddress = basePhysicalAddress + offset;
				DbgPrint("pageTable   [0x%lx][0x%lx] PageFrameNumber is [0x%lx] [0x%lx] [0x%lx]\n", pageTable, ptSpecificAddress, ptPFN, basePhysicalAddress, specificAddress);
				DbgPrint("Physical address for [0x%lx] is [0x%lx]\n", virtualaddr, specificAddress);
				MmUnmapIoSpace(pageTable, sizeof(PTE));
				MmUnmapIoSpace(pageDirectoryTable, sizeof(PTE));
				MmUnmapIoSpace(pageDirectoryPointerTable, sizeof(PTE));
				return specificAddress;
			}
			else {
				DbgPrint("pageTable is INVALID\n");
				if (pageTable) {
					MmUnmapIoSpace(pageTable, sizeof(PTE));
				}
				MmUnmapIoSpace(pageDirectoryTable, sizeof(PTE));
				MmUnmapIoSpace(pageDirectoryPointerTable, sizeof(PTE));
				return NULL;
			}
		}
		else {
			DbgPrint("pageDirectoryTable is INVALID\n");
			if (pageDirectoryTable) {
				MmUnmapIoSpace(pageDirectoryTable, sizeof(PTE));
			}
			MmUnmapIoSpace(pageDirectoryPointerTable, sizeof(PTE));
			return NULL;
		}
	}
	else {
		DbgPrint(" pageDirectoryPointerTable is INVALID\n");
		if (pageDirectoryPointerTable) {
			MmUnmapIoSpace(pageDirectoryPointerTable, sizeof(PTE));
		}
		return NULL;
	}
}
