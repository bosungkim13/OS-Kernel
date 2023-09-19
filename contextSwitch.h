#include <comp421/hardware.h>
#include <comp421/yalnix.h>

void switchToExistingProcess(struct processControlBlock* currentPCB, struct processControlBlock* targetPCB);
void cloneAndSwitchToProcess(struct processControlBlock* currentPCB, struct processControlBlock* targetPCB);
void copyMemory(struct pte* virtPTFrom, struct pte* virtPTTo);
void switchReg0To(void* destPTVirt);
