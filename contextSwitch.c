#include "processScheduling.h"
#include "processControlBlock.h"
#include "memoryManagement.h"
#include "pageTableManagement.h"

#include "contextSwitch.h"

SavedContext* switchProcessFunction(SavedContext* context, void* p1, void* p2){
	struct processControlBlock* pcbFrom = (struct processControlBlock*)p1;
	struct processControlBlock* pcbTo = (struct processControlBlock*)p2;
	switchReg0To(pcbTo->pageTable);
	TracePrintf(1, "ContextSwitch - SwitchToExistingProcess - Switched from process id %d to process id %d\n", pcbFrom->pid, pcbTo->pid);
	return &pcbTo->savedContext;
}
void switchToExistingProcess(struct processControlBlock* currentPCB, struct processControlBlock* targetPCB){
	ContextSwitch(switchProcessFunction, &currentPCB->savedContext, (void*)currentPCB, (void*)targetPCB);
	tryFreeSwitchedAwayExitingProcess();
}

// First copy process 1's kernel stack over to process 2. Then, switches context from process 1 to process 2.
SavedContext* copyProcessFunction(SavedContext *ctxp, void* p1, void* p2){
	struct processControlBlock* pcbFrom = (struct processControlBlock *)p1;
	struct processControlBlock* pcbTo = (struct processControlBlock *)p2;
	copyMemory(pcbFrom->pageTable, pcbTo->pageTable);
	switchReg0To(pcbTo->pageTable);
	return &pcbFrom->savedContext;
}
void cloneAndSwitchToProcess(struct processControlBlock* currentPCB, struct processControlBlock* targetPCB){
	ContextSwitch(copyProcessFunction, &currentPCB->savedContext, (void*)currentPCB, (void*)targetPCB);
	tryFreeSwitchedAwayExitingProcess();
}

// Assumes REG_PTR0 points to the physical address that is the base of virtPTFrom
// and returns with REG_PTR0 pointed to that as well
// Assumes that there are enough free physical pages available to make the copy
void copyMemory(struct pte* virtPTFrom, struct pte* virtPTTo){
	void* pageSwapSpace = getPageSwapSpace();
	int uprot, kprot;
	long vpn = 0;
	for(; vpn < VMEM_0_LIMIT/PAGESIZE; vpn++)
		if(virtPTFrom[vpn].valid == 1){
			TracePrintf(1, "ContextSwitch Memory Copy - VPN %d is valid, setting up copy\n", vpn);
			memcpy(pageSwapSpace, (void*)(vpn * PAGESIZE), PAGESIZE);
			uprot = virtPTFrom[vpn].uprot;
			kprot = virtPTFrom[vpn].kprot;
			switchReg0To(virtPTTo);
			
			virtPTTo[vpn].valid = 1;
			virtPTTo[vpn].pfn = getFreePhysicalPage();
			memcpy((void*)(vpn * PAGESIZE), pageSwapSpace, PAGESIZE);
			virtPTFrom[vpn].uprot = uprot;
			virtPTFrom[vpn].kprot = kprot;
			switchReg0To(virtPTFrom);
			TracePrintf(1, "ContextSwitch Memory Copy - Finished copying VPN %d into pfn %d\n", vpn, virtPTTo[vpn].pfn);
		}
}
void switchReg0To(void* destPTVirt){
	WriteRegister(REG_PTR0, (RCS421RegVal)virtualToPhysicalAddr(destPTVirt));
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
}
