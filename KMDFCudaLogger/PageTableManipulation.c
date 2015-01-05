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
ULONG GetPageDirectoryPointerIndex(PVOID virtualaddr) {
	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		return (ULONG)virtualaddr >> 30;
	}
	else {
		return NULL;
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



PHYSICAL_ADDRESS GetPDBRPhysicalAddress()
{
	ULONG pdbrValue = GetPageDirectoryBaseRegister();
	PHYSICAL_ADDRESS returnValue;
	returnValue.u.HighPart = 0x0;
	returnValue.u.LowPart = pdbrValue;
	return returnValue;
}

BOOLEAN IsLargePage(PVOID virtualAddress) {
	PPDE  ppde = GetPdeAddress(virtualAddress);
	return ppde->LargePage;
}

ULONG GetOffset(PVOID virtualAddress) {
	if (IsLargePage(virtualAddress)) {
		return(ULONG)virtualAddress & 0x1fffff;
	} else {
		return(ULONG)virtualAddress & 0xfff;
	}
}

PPTE  GetPteAddress(PVOID virtualaddr) {
	ULONG pageDirectoryIndex = GetPageDirectoryIndex(virtualaddr);
	ULONG pageTableIndex = GetPageTableIndex(virtualaddr); 
	PPTE pageTable = (PPTE)(getPageTableBase() + (pageTableIndex * getPteSize()) + (PAGE_SIZE * pageDirectoryIndex));
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

NTSTATUS Remap(PVOID kmdfDataPointer, PVOID clientDataPointer)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG64 PfnDatabase = getPfnDatabaseBase();
	ULONG64 pfnSize = getPfnSize(); 
	 
	PPFN    clientOriginalPfn;
	ULONG64 pfnForUsbAddress;
	ULONG   kmdfPhysicalAddress;
	ULONG   kmdfBaseAddress;

	if (ExIsProcessorFeaturePresent(PF_PAE_ENABLED)) {
		KdPrint(("RE_MAP PAE Enabled\n"));
	}
	else {
		KdPrint(("RE_MAP No PAE\n"));
	}

	ULONG clientOffset = (ULONG)clientDataPointer & 0x0fff;
	ULONG kmdfOffset = (ULONG)kmdfDataPointer & 0x0fff;


	PPDE  kmdfPageDirectory = GetPdeAddress(kmdfDataPointer);
	if (!kmdfPageDirectory) {
		KdPrint(("KMDF PDE is null\n"));
		return STATUS_CRC_ERROR;
	}
	KdPrint(("Got KMDF PDE\n"));
	if (kmdfPageDirectory->LargePage) {
		KdPrint(("LARGE page\n"));

		PPDE clientPageDirectory = GetPdeAddress(clientDataPointer);
		PPTE clientPageTable     = GetPteAddress(clientDataPointer);
		PPDE kmdfPageDirectory   = GetPdeAddress(kmdfDataPointer);
		PHYSICAL_ADDRESS kmdfPA = MmGetPhysicalAddress(kmdfDataPointer);
		ULONG offset = (ULONG)clientDataPointer & 0x0fff;
		ULONG kmdfXX = kmdfPA.LowPart;
		ULONG target = kmdfXX - offset;
		ULONG targetPFN = kmdfPA.LowPart >> 12;

		KdPrint(("FUBAR [0x%llx][0x%lx][0x%lx] target PFN [0x%lx]   \n", 
			kmdfPA.QuadPart, offset, kmdfXX, targetPFN));

		ULONG targetPFNvalue = targetPFN << 12;

		// Create a bit mask so we can set the page table index all at once
		ULONG clientPfnValue = clientPageTable->rawValue & ~(0xfff);
		ULONG clientToKmdfMask = (targetPFNvalue ^ clientPfnValue);

		KdPrint(("fobar Target [0x%lx] Client [0x%lx]  \n",
			targetPFNvalue, clientPfnValue));

		KdPrint(("Pre Alter\n"));
		printPteHeader();
		printPpte(clientPageTable);


		(*((PULONG)clientPageTable)) |= 0x1; // set present
		(*((PULONG)clientPageTable)) |= 0x20; // set accessed ?
		(*((PULONG)clientPageTable)) |= 0x40; // set dirty
		(*((PULONG)clientPageTable)) &= ~(0x100); // Clear global
		(*((PULONG)clientPageTable)) |= 0x200; // set copy on write
		(*((PULONG)clientPageTable)) ^= clientToKmdfMask;

		KdPrint(("Post Alter\n"));
		printPteHeader();
		printPpte(clientPageTable);

		pfnForUsbAddress = PfnDatabase + (targetPFN * pfnSize);
		KdPrint(("PFNDatabase=0x%llx\n", PfnDatabase));
		KdPrint(("pfnSize=0x%llx\n", pfnSize));
		KdPrint(("clientPfnValue=0x%lx\n", clientPageTable->PageFrameNumber));
		KdPrint(("PFN [0x%llx]\n", pfnForUsbAddress));
		KdPrint(("Flush TLB\n"));
		__asm __volatile
		{
			cli
			invlpg  clientPageTable; // flush the TLB
			sti
		}
		(*((PULONG)clientPageTable)) |= 0x100; // set global
	}
	else {
		// TODO: implement the new way of doing things for small pages too
		PPDE clientPageDirectory = GetPdeAddress(clientDataPointer);
		PPDE kmdfPageDirectory = GetPdeAddress(kmdfDataPointer);
		PPTE clientPageTable = GetPteAddress(clientDataPointer);
		PPTE kmdfPageTable = GetPteAddress(kmdfDataPointer);

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
		KdPrint(("Flush TLB\n"));
		__asm __volatile
		{
			cli
			invlpg  clientPageTable; // flush the TLB
			sti
		}
		(*((PULONG)clientPageTable)) |= 0x100; // set global
	}

	KdPrint(("About to look at PFN [0x%x]\n", pfnForUsbAddress));
	pauseForABit(10);
	PPFN pfnForUsb = (PPFN)pfnForUsbAddress;
	if (MmIsAddressValid(pfnForUsb)) {
		KdPrint(("PFN is valid, so let's increment it\n"));
		pauseForABit(10);
		KdPrint(("flink [0x%lx] blink [0x%lx] pteaddress [0x%lx] flags [0x%x]\n"
			, pfnForUsb->flink, pfnForUsb->blink, pfnForUsb->pteaddress, pfnForUsb->flags
			));
		KdPrint(("page_state [0x%x] reference_count [0x%x] restore_pte [0x%lx] containing_page [0x%x]\n"
			, pfnForUsb->page_state, pfnForUsb->reference_count, pfnForUsb->restore_pte, pfnForUsb->containing_page));
		pauseForABit(10);
		pfnForUsb->reference_count++;
		KdPrint(("page_state [0x%x] reference_count [0x%x] restore_pte [0x%lx] containing_page [0x%x]\n"
			, pfnForUsb->page_state, pfnForUsb->reference_count, pfnForUsb->restore_pte, pfnForUsb->containing_page));
		pauseForABit(10);
	}
	else {
		KdPrint(("PFN [0x%x] was invalid\n", pfnForUsbAddress));
	}
	KdPrint(("finished with remap function\n"));
	pauseForABit(5);

	return status;
}

