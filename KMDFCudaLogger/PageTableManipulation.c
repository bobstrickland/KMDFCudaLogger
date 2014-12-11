#include <PageTableManipulation.h>

VOID printPteHeader() {
	KdPrint(("PTE  [Va|RW|Ow|WT|CD|Ac|Di|PT|Gl|PFN\n"));
}
VOID printPdeHeader() {
	KdPrint(("PDE  [Va|RW|Ow|WT|CD|Ac|Av|LP|Gl|PFN\n"));
}
VOID printPpte(PPTE ppte) {
	KdPrint(("PTE  [ %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu|0x%lx\n",
		ppte->Valid, ppte->Writable, ppte->Owner, ppte->WriteThrough, 
		ppte->CacheDisable, ppte->Accessed, ppte->Dirty, ppte->PageAttributeTable, ppte->Global, ppte->PageFrameNumber));
}

VOID printPpde(PPDE ppde) {
	KdPrint(("PDE  [ %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu|0x%lx\n",
		ppde->Valid, ppde->Writable, ppde->Owner, ppde->WriteThrough,
		ppde->CacheDisable, ppde->Accessed, ppde->Available, ppde->LargePage, ppde->Global, ppde->PageFrameNumber));
}


#ifdef __WINDOWS_7_32
INDEX GetPageTableIndex(GENERIC_POINTER virtualaddr) {
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x03FF;
	return pageTableIndex;
}
INDEX GetPageDirectoryIndex(GENERIC_POINTER virtualaddr) {
	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 22;
	return pageDirectoryIndex;
}
PPTE  GetVirtualPpte(GENERIC_POINTER virtualaddr) {
	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	PPTE pageTable = (PPTE)(PROCESS_PAGE_TABLE_BASE + (pageTableIndex * PTE_SIZE) + (PAGE_SIZE * pageDirectoryIndex));
	return pageTable;
}
PPDE  GetVirtualPpde(GENERIC_POINTER virtualaddr) {
	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	PPDE pageDirectoryTable = (PPDE)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	return pageDirectoryTable;
}
#endif

#ifdef __WINDOWS_7_64

INDEX GetPageTableIndex(GENERIC_POINTER virtualaddr) {
	ULONGLONG pageTableIndex = (ULONGLONG)virtualaddr >> 12 & 0x01FF;
	return pageTableIndex;
}
INDEX GetPageDirectoryIndex(GENERIC_POINTER virtualaddr) {
	ULONGLONG pageDirectoryIndex = (ULONGLONG)virtualaddr >> 21 & 0x01FF;
	return pageDirectoryIndex;
}
PPTE  GetVirtualPpte(GENERIC_POINTER virtualaddr) {
	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	ULONGLONG xPageDirectoryIndex = (ULONGLONG)virtualaddr >> 39 & 0x01FF;
	ULONGLONG pPageDirectoryIndex = (ULONGLONG)virtualaddr >> 30 & 0x01FF;
	PPTE pageTable = (PPTE)(PROCESS_PAGE_TABLE_BASE
		+ (PAGE_SIZE * PAGE_SIZE * PAGE_SIZE * xPageDirectoryIndex)
		+ (PAGE_SIZE * PAGE_SIZE * pPageDirectoryIndex)
		+ (PAGE_SIZE * pageDirectoryIndex)
		+ (pageTableIndex * PTE_SIZE));
	return pageTable;
}
PPDE  GetVirtualPpde(GENERIC_POINTER virtualaddr) {
	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	ULONGLONG xPageDirectoryIndex = (ULONGLONG)virtualaddr >> 39 & 0x01FF;
	ULONGLONG pPageDirectoryIndex = (ULONGLONG)virtualaddr >> 30 & 0x01FF;
	PPDE pageDirectoryTable = (PPDE)(PROCESS_PAGE_DIRECTORY_BASE
		+ (PAGE_SIZE * PAGE_SIZE * xPageDirectoryIndex)
		+ (PAGE_SIZE * pPageDirectoryIndex)
		+ (pageDirectoryIndex * PTE_SIZE));
	return pageDirectoryTable;
}

#endif

#ifdef __WINDOWS_10_32

INDEX GetPageTableIndex(GENERIC_POINTER virtualaddr) {
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	return pageTableIndex;
}
INDEX GetPageDirectoryIndex(GENERIC_POINTER virtualaddr) {
	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21;
	return pageDirectoryIndex;
}
PPTE  GetVirtualPpte(GENERIC_POINTER virtualaddr) {
	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	PPTE pageTable = (PPTE)(PROCESS_PAGE_TABLE_BASE + (pageTableIndex * PTE_SIZE) + (PAGE_SIZE * pageDirectoryIndex));
	return pageTable;
}
PPDE  GetVirtualPpde(GENERIC_POINTER virtualaddr) {
	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	PPDE pageDirectoryTable = (PPDE)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	return pageDirectoryTable;
}

#endif






PPTE  GetPteAddress(GENERIC_POINTER virtualaddr, PPDE pageDirectoryTable){

	PPTE pageTable = GetVirtualPpte(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	INDEX offset = (INDEX)virtualaddr & 0x0fff;

	DbgPrint("\n\nVirtualAddress [0x%lx] pageTableIndex is [0x%lx]\n", virtualaddr, pageTableIndex);
	ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
	DbgPrint("  PageFrameNumber is [0x%lx]\n", pdPFN);
	DbgPrint("pageTable   [0x%lx] ", pageTable);
	if (MmIsAddressValid(pageTable)) {
		DbgPrint("\n");
		return pageTable;
	}
	else {
		DbgPrint(" is INVALID\n");
		return NULL;
	}
}

PPDE  GetPdeAddress(GENERIC_POINTER virtualaddr){

	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	PPDE pageDirectoryTable = GetVirtualPpde(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	PPTE pageTable = GetVirtualPpte(virtualaddr);
	INDEX offset = (INDEX)virtualaddr & 0x0fff;

	DbgPrint("\n\nVirtualAddress [0x%lx] pageDirectoryTable   [0x%lx] pageDirectoryIndex [0x%lx] ", virtualaddr, pageDirectoryTable, pageDirectoryIndex);
	if (MmIsAddressValid(pageDirectoryTable)) {
		DbgPrint("\n");
		return pageDirectoryTable;
	}
	else {
		DbgPrint(" is INVALID\n");
		return NULL;
	}
}



INDEX GetPhysAddress(GENERIC_POINTER virtualaddr)
{

	INDEX pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
	PPTE pageTable = GetVirtualPpte(virtualaddr);
	INDEX offset = (INDEX)virtualaddr & 0x0fff;

	DbgPrint("\n\nVirtualAddress [0x%lx] is [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryIndex, pageTableIndex, offset);
	PPTE pageDirectoryTable = (PPTE)(PROCESS_PAGE_DIRECTORY_BASE + (pageDirectoryIndex * PTE_SIZE));
	DbgPrint("pageDirectoryTable   [0x%lx]", pageDirectoryTable);
	if (MmIsAddressValid(pageDirectoryTable)) {
		DbgPrint("[0x%lx] ", MmGetPhysicalAddress(pageDirectoryTable));
		ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
		DbgPrint("  PageFrameNumber is [0x%lx]\n", pdPFN);
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


NTSTATUS Remap(GENERIC_POINTER clientDataPointer, PPDE clientPageDirectory, PPTE clientPageTable, GENERIC_POINTER kmdfDataPointer)
{
	NTSTATUS status = STATUS_SUCCESS;

	INDEX pageDirectoryIndex = GetPageDirectoryIndex(clientDataPointer);
#ifdef __WINDOWS_7_64
	ULONGLONG extendedPageDirectoryIndex = (ULONGLONG)clientDataPointer >> 39 & 0x01FF;
	ULONGLONG pageDirectoryPointerIndex  = (ULONGLONG)clientDataPointer >> 30 & 0x01FF;
#endif
	INDEX pageTableIndex = GetPageTableIndex(clientDataPointer); //(INDEX)clientDataPointer >> 12 & 0x01FF;
	INDEX clientOffset = (INDEX)clientDataPointer & 0x0fff;
	INDEX kmdfOffset = (INDEX)kmdfDataPointer & 0x0fff;

	ULONG clientPageFrameNumber = clientPageTable->PageFrameNumber;
	ULONG clientBaseAddress     = clientPageFrameNumber << 12;
	ULONG finalPhysicalAddress  = clientBaseAddress + clientOffset;

	KdPrint(("\nReadKeyboardBuffer Client Buffer Address is [0x%lx]\n\n", finalPhysicalAddress));

	PPDE  kmdfPageDirectory = GetPdeAddress(kmdfDataPointer);
	PPTE  kmdfPageTable = GetPteAddress(kmdfDataPointer, kmdfPageDirectory);
	ULONG kmdfPageFrameNumber = kmdfPageTable->PageFrameNumber;
	ULONG kmdfBaseAddress = kmdfPageFrameNumber << 12;

	KdPrint(("Pre Alter\n"));
	VOID printPdeHeader();
	printPpde(kmdfPageDirectory);
	printPpde(clientPageDirectory);
	VOID printPteHeader();
	printPpte(kmdfPageTable);
	printPpte(clientPageTable);

	// Create a bit mask so we can set the page table index all at once
	INDEX kmdfPfnValue = kmdfPageTable->rawValue & ~(0xfff);
	INDEX clientPfnValue = clientPageTable->rawValue & ~(0xfff);
	INDEX clientToKmdfMask = (kmdfPfnValue ^ clientPfnValue);

	(*((INDEX_POINTER)clientPageTable)) |= 0x1; // set present
	//(*((INDEX_POINTER)clientPageTable)) |= 0x10; // set cache disabled
	(*((INDEX_POINTER)clientPageTable)) |= 0x20; // set accessed ?
	(*((INDEX_POINTER)clientPageTable)) |= 0x40; // set dirty
	(*((INDEX_POINTER)clientPageTable)) |= 0x100; // set global
	(*((INDEX_POINTER)clientPageTable)) |= 0x200; // set copy on write
	//(*((INDEX_POINTER)clientPageTable)) &= ~(1 << 6);  // clear dirty
	// This line right here gived a BSOD with the error MEMORY_MANAGEMENT
	(*((INDEX_POINTER)clientPageTable)) ^= clientToKmdfMask;


	KdPrint(("Post Alter\n"));
	VOID printPdeHeader();
	printPpde(kmdfPageDirectory);
	printPpde(clientPageDirectory);
	VOID printPteHeader();
	printPpte(kmdfPageTable);
	printPpte(clientPageTable);



	__asm __volatile
	{
		invlpg  clientPageTable; // flush the TLB
	}


	return status;
}

#ifndef _WIN64
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

#endif
