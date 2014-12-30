#include <PageTableManipulation.h>

extern VOID pauseForABit(CSHORT secondsDelay);

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

ULONG getPdeSize() {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return PAE_PDE_SIZE;
	}
	else {
		return X32_PDE_SIZE;
	}
}
ULONG getPteSize() { 
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return PAE_PTE_SIZE;
	}
	else {
		return X32_PTE_SIZE;
	}
}


ULONG getPageDirectoryBase() {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return PAE_PROCESS_PAGE_DIRECTORY_BASE;
	}
	else {
		return X32_PROCESS_PAGE_DIRECTORY_BASE;
	}
}

ULONG getPageTableBase() {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return PAE_PROCESS_PAGE_TABLE_BASE;
	}
	else {
		return X32_PROCESS_PAGE_TABLE_BASE;
	}
}

ULONG64 getPfnDatabaseBase() {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return PAE_PFN_DATABASE_BASE;
	}
	else {
		return X32_PFN_DATABASE_BASE;
	}
}

ULONG64 getPfnSize() {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return PAE_PFN_SIZE;
	}
	else {
		return X32_PFN_SIZE;
	}
}


ULONG GetPageTableIndex(PVOID virtualaddr) {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return (ULONG)virtualaddr >> 12 & 0x01FF;
	}
	else {
		return (ULONG)virtualaddr >> 12 & 0x03FF;
	}
}
ULONG GetPageDirectoryIndex(PVOID virtualaddr) {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return (ULONG)virtualaddr >> 21;
	}
	else {
		return (ULONG)virtualaddr >> 22;
	}
}

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



/** /
PPTE  GetVirtualPpte(PVOID virtualaddr) {
	ULONG pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	ULONG pageTableIndex = GetPageTableIndex(virtualaddr);
	PPTE pageTable = (PPTE)(getPageTableBase() + (pageTableIndex * getPteSize()) + (PAGE_SIZE * pageDirectoryIndex));
	return pageTable;
}
PPDE  GetVirtualPpde(PVOID virtualaddr) {
	ULONG pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	PPDE pageDirectoryTable = (PPDE)(getPageDirectoryBase() + (pageDirectoryIndex * getPdeSize()));
	return pageDirectoryTable;
}
/**/


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
	ULONG pageDirectoryPointerIndex = (ULONG)virtualaddr >> 30;
	ULONG pageDirectoryIndex = (ULONG)virtualaddr >> 21 & 0x01FF;
	ULONG pageTableIndex = (ULONG)virtualaddr >> 12 & 0x01FF;
	ULONG offset = (ULONG)virtualaddr & 0x0fff;
	DbgPrint("\n\nVirtualAddress [0x%lx] is  [0x%lx] [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryPointerIndex, pageDirectoryIndex, pageTableIndex, offset);
	DbgPrint("Looking physical \n");

	// get the PageDirectoryPointerTable Entry
	PHYSICAL_ADDRESS pageDirPointerTablePA = GetPDBRPhysicalAddress();
	pageDirPointerTablePA.QuadPart = pageDirPointerTablePA.QuadPart + (pageDirectoryPointerIndex*sizeof(PHYSICAL_ADDRESS));
	PPTE pageDirectoryPointerTable = MmMapIoSpace(pageDirPointerTablePA, sizeof(PTE), MmNonCached);
	if (MmIsAddressValid(pageDirectoryPointerTable)) {

		// get the PageDirectoryTable Entry
		ULONG pdpPFN = pageDirectoryPointerTable->PageFrameNumber;
		ULONG pdBaseAddress = pdpPFN << 12;
		ULONG pdPhysicalAddress = pdBaseAddress + (pageDirectoryIndex * getPdeSize());
		DbgPrint("pageDirectoryPointerTable   [0x%lx][0x%lx] PageFrameNumber is [0x%lx] [0x%lx] [0x%lx]\n", pageDirectoryPointerTable, pageDirPointerTablePA, pdpPFN, pdBaseAddress, pdPhysicalAddress);
		PHYSICAL_ADDRESS pdtPhysicalAddress;
		pdtPhysicalAddress.u.HighPart = 0x0;
		pdtPhysicalAddress.u.LowPart = pdPhysicalAddress;
		PPDE pageDirectoryTable = MmMapIoSpace(pdtPhysicalAddress, sizeof(PDE), MmNonCached);
		if (MmIsAddressValid(pageDirectoryTable)) {
			if (pageDirectoryTable->LargePage) {
				ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
				ULONG basePhysicalAddress = pdPFN << 12;
				ULONG specificAddress = basePhysicalAddress + offset;
				DbgPrint("pageDirectoryTable   [0x%lx][0x%lx] is LARGE.  PageFrameNumber is [0x%lx] [0x%lx] [0x%lx]\n", pageDirectoryTable, pdPhysicalAddress, pdPFN, basePhysicalAddress, specificAddress);
				DbgPrint("Physical address for [0x%lx] is [0x%lx]\n", virtualaddr, specificAddress);
				MmUnmapIoSpace(pageDirectoryTable, sizeof(PDE));
				MmUnmapIoSpace(pageDirectoryPointerTable, sizeof(PTE));
				return specificAddress;
			}
			else {


				// get the PageTable Entry
				ULONG pdPFN = pageDirectoryTable->PageFrameNumber;
				ULONG ptBasePhysicalAddress = pdPFN << 12;
				ULONG ptSpecificAddress = ptBasePhysicalAddress + (pageTableIndex * getPteSize());
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
					MmUnmapIoSpace(pageDirectoryTable, sizeof(PDE));
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

PPTE  GetPteAddress(PVOID virtualaddr) {
	ULONG pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	ULONG pageTableIndex = GetPageTableIndex(virtualaddr); 
	PPTE pageTable = (PPTE)(getPageTableBase() + (pageTableIndex * getPteSize()) + (PAGE_SIZE * pageDirectoryIndex));
	DbgPrint("VirtualAddress [0x%lx] pageDirectoryIndex is [0x%lx]  pageTableIndex is [0x%lx]  pageTable is [0x%lx]\n",
		virtualaddr, pageDirectoryIndex, pageTableIndex, pageTable);
	if (MmIsAddressValid(pageTable)) {
		return pageTable;
	}
	else {
		DbgPrint("pageTable   [0x%lx] is INVALID\n", pageTable);
		return NULL;
	}
}

PPDE  GetPdeAddress(PVOID virtualaddr){
	ULONG pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	PPDE pageDirectoryTable = (PPDE)(getPageDirectoryBase() + (pageDirectoryIndex * getPdeSize()));
	DbgPrint("\n\nVirtualAddress [0x%lx] pageDirectoryIndex [0x%lx] pageDirectoryTable [0x%lx] ", 
		virtualaddr, pageDirectoryIndex, pageDirectoryTable);
	if (MmIsAddressValid(pageDirectoryTable)) {
		return pageDirectoryTable;
	}
	else {
		DbgPrint("pageDirectoryTable   [0x%lx] is INVALID\n", pageDirectoryTable);
		return NULL;
	}
}



ULONG GetPhysAddress(PVOID virtualaddr)
{

	ULONG pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	ULONG pageTableIndex = GetPageTableIndex(virtualaddr);
	PPTE pageTable = GetPteAddress(virtualaddr);
	ULONG offset = (ULONG)virtualaddr & 0x0fff;

	DbgPrint("\n\nVirtualAddress [0x%lx] is [0x%lx] [0x%lx] [0x%lx]\n", virtualaddr, pageDirectoryIndex, pageTableIndex, offset);
	PPTE pageDirectoryTable = (PPTE)(getPageDirectoryBase() + (pageDirectoryIndex * getPdeSize()));
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


NTSTATUS Remap(PVOID clientDataPointer, PPDE clientPageDirectory, PPTE clientPageTable, PVOID kmdfDataPointer)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG64 PfnDatabase = getPfnDatabaseBase();
	ULONG64 pfnSize = getPfnSize(); 
	ULONG64 pfnForUsbAddress;
	ULONG   kmdfPhysicalAddress;
	ULONG   kmdfBaseAddress;

	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		KdPrint(("PAE Enabled\n"));
	}
	else {
		KdPrint(("No PAE\n"));
	}

//	ULONG pageDirectoryIndex = GetPageDirectoryIndex(clientDataPointer);
#ifdef __WINDOWS_7_64
	ULONGLONG extendedPageDirectoryIndex = (ULONGLONG)clientDataPointer >> 39 & 0x01FF;
	ULONGLONG pageDirectoryPointerIndex  = (ULONGLONG)clientDataPointer >> 30 & 0x01FF;
#endif
//	ULONG pageTableIndex = GetPageTableIndex(clientDataPointer); //(ULONG)clientDataPointer >> 12 & 0x01FF;
	ULONG clientOffset = (ULONG)clientDataPointer & 0x0fff;
	ULONG kmdfOffset = (ULONG)kmdfDataPointer & 0x0fff;

	ULONG clientPageFrameNumber = clientPageTable->PageFrameNumber;
	ULONG clientBaseAddress     = clientPageFrameNumber << 12;
	ULONG finalPhysicalAddress  = clientBaseAddress + clientOffset;


	KdPrint(("ReadKeyboardBuffer Client Buffer Address is [0x%lx]\n", finalPhysicalAddress));

	KdPrint(("GetPdeAddress\n"));
	PPDE  kmdfPageDirectory = GetPdeAddress(kmdfDataPointer);
	if (!kmdfPageDirectory) {
		KdPrint(("KMDF PDE is null\n"));
		return STATUS_CRC_ERROR;
	}
	KdPrint(("Got KMDF PDE\n"));
	if (kmdfPageDirectory->LargePage) {

		KdPrint(("Pre Alter\n"));
		printPdeHeader();
		printPpde(kmdfPageDirectory);
		printPpde(clientPageDirectory);
		printPteHeader();
		printPpte(clientPageTable);
		pauseForABit(60);
		ULONG kmdfPfnValue = kmdfPageDirectory->rawValue & ~(0xfff);
		//ULONG clientPfnValue = clientPageDirectory->rawValue & ~(0xfff);
		ULONG clientPfnValue = clientPageTable->rawValue & ~(0xfff);


		kmdfBaseAddress = kmdfPfnValue << 12;
		kmdfPhysicalAddress = kmdfBaseAddress + kmdfOffset;

		ULONG newClientBaseAddress = kmdfPhysicalAddress - clientOffset;


		//ULONG clientToKmdfMask = (kmdfPfnValue ^ clientPfnValue);
		ULONG clientToKmdfMask = (kmdfPfnValue ^ clientPfnValue);





		//(*((PULONG)clientPageTable)) |= 0x80; // set Large Page
		// remap the PDE right here!
		//(*((PULONG)clientPageDirectory)) ^= clientToKmdfMask;


		(*((PULONG)clientPageTable)) |= 0x1; // set present
		(*((PULONG)clientPageTable)) |= 0x20; // set accessed ?
		(*((PULONG)clientPageTable)) |= 0x40; // set dirty
		(*((PULONG)clientPageTable)) &= ~(0x100); // Clear global
		(*((PULONG)clientPageTable)) |= 0x200; // set copy on write
		//(*((PULONG)clientPageTable)) &= ~(0x40);  // clear dirty
		// This line right here gived a BSOD with the error MEMORY_MANAGEMENT
		(*((PULONG)clientPageTable)) ^= clientToKmdfMask;




		KdPrint(("Post Alter\n"));
		printPdeHeader();
		printPpde(kmdfPageDirectory);
		printPpde(clientPageDirectory);
		printPteHeader();
		printPpte(clientPageTable);
		pauseForABit(60);

		pfnForUsbAddress = PfnDatabase + (kmdfPageDirectory->PageFrameNumber * pfnSize);
		KdPrint(("PFNDatabase=0x%llx\n", PfnDatabase));
		KdPrint(("pfnSize=0x%llx\n", pfnSize));
		KdPrint(("kmdfPfnValue=0x%lx\n", kmdfPageDirectory->PageFrameNumber));
		KdPrint(("PFN [0x%llx]\n", pfnForUsbAddress));
		// TODO: do we need to decrement the PFN for the address the client originally had????
	}
	else {
		PPTE  kmdfPageTable;

		kmdfPageTable = GetPteAddress(kmdfDataPointer);
		if (!kmdfPageTable) {
			KdPrint(("KMDF PTE is null\n"));
			return STATUS_CRC_ERROR;
		}
		KdPrint(("Got KMDF PTE\n"));

		KdPrint(("Pre Alter\n"));
		printPteHeader();
		printPpte(kmdfPageTable);
		printPpte(clientPageTable);

		// Create a bit mask so we can set the page table index all at once
		ULONG kmdfPfnValue = kmdfPageTable->rawValue & ~(0xfff);
		ULONG clientPfnValue = clientPageTable->rawValue & ~(0xfff);
		ULONG clientToKmdfMask = (kmdfPfnValue ^ clientPfnValue);

		(*((PULONG)clientPageTable)) |= 0x1; // set present
		//(*((PULONG)clientPageTable)) &= ~(0x2);  // clear writable
		//(*((PULONG)clientPageTable)) |= (0x2);  // set writable
		//(*((PULONG)clientPageTable)) |= 0x10; // set cache disabled
		(*((PULONG)clientPageTable)) |= 0x20; // set accessed ?
		(*((PULONG)clientPageTable)) |= 0x40; // set dirty
		(*((PULONG)clientPageTable)) &= ~(0x100); // Clear global
		//(*((PULONG)clientPageTable)) |= 0x100; // set global
		(*((PULONG)clientPageTable)) |= 0x200; // set copy on write
		//(*((PULONG)clientPageTable)) &= ~(0x40);  // clear dirty
		// This line right here gived a BSOD with the error MEMORY_MANAGEMENT
		(*((PULONG)clientPageTable)) ^= clientToKmdfMask;


		KdPrint(("Post Alter\n"));
		printPteHeader();
		printPpte(kmdfPageTable);
		printPpte(clientPageTable);


		pfnForUsbAddress = PfnDatabase + (kmdfPageTable->PageFrameNumber * pfnSize);
		KdPrint(("PFNDatabase=0x%llx\n", PfnDatabase));
		KdPrint(("pfnSize=0x%llx\n", pfnSize));
		KdPrint(("kmdfPfnValue=0x%lx\n", kmdfPageTable->PageFrameNumber));
		KdPrint(("PFN [0x%llx]\n", pfnForUsbAddress));
	}

	KdPrint(("Flush TLB\n"));
	__asm __volatile
	{
		cli
		invlpg  clientPageTable; // flush the TLB
		sti
	}
	(*((PULONG)clientPageTable)) |= 0x100; // set global


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
	KdPrint(("finished with remap function\n"));

	return status;
}

