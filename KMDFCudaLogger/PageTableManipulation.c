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
//	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
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
//	INDEX offset = (INDEX)virtualaddr & 0x0fff;

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
//	INDEX pageTableIndex = GetPageTableIndex(virtualaddr);
//	PPTE pageTable = GetVirtualPpte(virtualaddr);
//	INDEX offset = (INDEX)virtualaddr & 0x0fff;

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

//	INDEX pageDirectoryIndex = GetPageDirectoryIndex(clientDataPointer);
#ifdef __WINDOWS_7_64
	ULONGLONG extendedPageDirectoryIndex = (ULONGLONG)clientDataPointer >> 39 & 0x01FF;
	ULONGLONG pageDirectoryPointerIndex  = (ULONGLONG)clientDataPointer >> 30 & 0x01FF;
#endif
//	INDEX pageTableIndex = GetPageTableIndex(clientDataPointer); //(INDEX)clientDataPointer >> 12 & 0x01FF;
	INDEX clientOffset = (INDEX)clientDataPointer & 0x0fff;
//	INDEX kmdfOffset = (INDEX)kmdfDataPointer & 0x0fff;

	ULONG clientPageFrameNumber = clientPageTable->PageFrameNumber;
	ULONG clientBaseAddress     = clientPageFrameNumber << 12;
	ULONG finalPhysicalAddress  = clientBaseAddress + clientOffset;

	KdPrint(("\nReadKeyboardBuffer Client Buffer Address is [0x%lx]\n\n", finalPhysicalAddress));

	PPDE  kmdfPageDirectory = GetPdeAddress(kmdfDataPointer);
	PPTE  kmdfPageTable = GetPteAddress(kmdfDataPointer, kmdfPageDirectory);
	//PPDE  clientPageDirectory = GetPdeAddress(clientDataPointer);
	//PPTE  clientPageTable = GetPteAddress(clientDataPointer, clientPageDirectory);
//	ULONG kmdfPageFrameNumber = kmdfPageTable->PageFrameNumber;
//	ULONG kmdfBaseAddress = kmdfPageFrameNumber << 12;

	KdPrint(("Pre Alter\n"));
	//printPdeHeader();
	//printPpde(kmdfPageDirectory);
	//printPpde(clientPageDirectory);
	printPteHeader();
	printPpte(kmdfPageTable);
	printPpte(clientPageTable);

	// Create a bit mask so we can set the page table index all at once
	INDEX kmdfPfnValue = kmdfPageTable->rawValue & ~(0xfff);
	INDEX clientPfnValue = clientPageTable->rawValue & ~(0xfff);
	INDEX clientToKmdfMask = (kmdfPfnValue ^ clientPfnValue);

	(*((INDEX_POINTER)clientPageTable)) |= 0x1; // set present
	//(*((INDEX_POINTER)clientPageTable)) &= ~(0x2);  // clear writable
	//(*((INDEX_POINTER)clientPageTable)) |= (0x2);  // set writable
	//(*((INDEX_POINTER)clientPageTable)) |= 0x10; // set cache disabled
	(*((INDEX_POINTER)clientPageTable)) |= 0x20; // set accessed ?
	(*((INDEX_POINTER)clientPageTable)) |= 0x40; // set dirty
	(*((INDEX_POINTER)clientPageTable)) &= ~(0x100); // Clear global
	//(*((INDEX_POINTER)clientPageTable)) |= 0x100; // set global
	(*((INDEX_POINTER)clientPageTable)) |= 0x200; // set copy on write
	//(*((INDEX_POINTER)clientPageTable)) &= ~(0x40);  // clear dirty
	// This line right here gived a BSOD with the error MEMORY_MANAGEMENT
	(*((INDEX_POINTER)clientPageTable)) ^= clientToKmdfMask;


	KdPrint(("Post Alter\n"));
	printPteHeader();
	printPpte(kmdfPageTable);
	printPpte(clientPageTable);

	__asm __volatile
	{
		cli
		invlpg  clientPageTable; // flush the TLB
		sti
	}
	(*((INDEX_POINTER)clientPageTable)) |= 0x100; // set global

	ULONG64 PfnDatabase = PFN_DATABASE_BASE; 
	ULONG64 pfnSize = 0x18;
	ULONG64 pfnForUsbAddress = PfnDatabase + (kmdfPageTable->PageFrameNumber * pfnSize);
	KdPrint(("PFNDatabase=0x%llx\n", PfnDatabase));
	KdPrint(("pfnSize=0x%llx\n", pfnSize));
	KdPrint(("kmdfPfnValue=0x%lx\n", kmdfPageTable->PageFrameNumber));

	KdPrint(("PFN [0x%llx]\n", pfnForUsbAddress));
	PPFN pfnForUsb = (PPFN)pfnForUsbAddress;
	if (MmIsAddressValid(pfnForUsb)) {
		KdPrint(("flink [0x%lx] blink [0x%lx] pteaddress [0x%lx] flags [0x%x]\n"
			, pfnForUsb->flink, pfnForUsb->blink, pfnForUsb->pteaddress, pfnForUsb->flags
			));
		KdPrint(("page_state [0x%x] reference_count [0x%x] restore_pte [0x%lx] containing_page [0x%x]\n"
			, pfnForUsb->page_state, pfnForUsb->reference_count, pfnForUsb->restore_pte, pfnForUsb->containing_page));
		pfnForUsb->reference_count++;
		KdPrint(("page_state [0x%x] reference_count [0x%x] restore_pte [0x%lx] containing_page [0x%x]\n"
			, pfnForUsb->page_state, pfnForUsb->reference_count, pfnForUsb->restore_pte, pfnForUsb->containing_page));
	}
	else {
		KdPrint(("PFN was invalid\n"));
	}

	return status;
}

