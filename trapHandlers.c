#include <stdio.h>
#include "processScheduling.h"
#include "memoryManagement.h"
#include "pageTableManagement.h"
#include "processControlBlock.h"
#include "loadProgram.h"
#include "contextSwitch.h"
#include "trapHandlers.h"
#include "terminal.h"

void kernelTrapHandler(ExceptionInfo *info) {
  TracePrintf(1, "trapHandlers: In TRAP_KERNEL interrupt handler...\n");

  int code = info->code;
  int currentPid = getCurrentPid();

  switch (code) {
    case YALNIX_FORK:
      TracePrintf(1, "trapHandlers: Fork requested by process %d.\n", currentPid);
      forkTrapHandler(info);
      break;
    case YALNIX_EXEC:
      TracePrintf(1, "trapHandlers: Exec requested by process %d.\n", currentPid);
      execTrapHandler(info);
      break;
    case YALNIX_EXIT:
      TracePrintf(1, "trapHandlers: Exit requested by process %d.\n", currentPid);
      exitHandler(info, 0);
      break;
    case YALNIX_WAIT:
      TracePrintf(1, "trapHandlers: Wait requested by process %d.\n", currentPid);
      waitTrapHandler(info);
      break;
    case YALNIX_GETPID:
      TracePrintf(1, "trapHandlers: GetPid requested by process %d.\n", currentPid);
      getPidHandler(info);
      break;
    case YALNIX_BRK:
      TracePrintf(1, "trapHandlers: Brk requested by process %d.\n", currentPid);
      brkHandler(info);
      break;
    case YALNIX_DELAY:
      TracePrintf(1, "trapHandlers: Delay requested by process %d.\n", currentPid);
      delayHandler(info);
      break;
    case YALNIX_TTY_READ:
      TracePrintf(1, "trapHandlers: Tty Read requested by process %d.\n", currentPid);
      ttyReadHandler(info);
      break;
    case YALNIX_TTY_WRITE:
      TracePrintf(1, "trapHandlers: Tty Write requested by process %d.\n", currentPid);
      ttyWriteHandler(info);
      break;
  }
}

void waitTrapHandler(ExceptionInfo *info){
	int* statusPtr = (int*)info->regs[1];
	struct processControlBlock* parentPCB = getRunningNode()->pcb;
	if(parentPCB->numChildren == 0){
		if(parentPCB->exitQ == NULL){
			// Never had children
			info->regs[0] = ERROR;
			return;
		}else{
			// Had children, they've all exited
		}
	}else{
		TracePrintf(1, "Parent process %d has %d children\n", parentPCB->pid, parentPCB->numChildren);
		if(parentPCB->exitQ == NULL){
			TracePrintf(1, "Parent process %d's children are all still running. Waiting...\n", parentPCB->pid);
			parentPCB->isWaiting = 1;
			scheduleProcess(0);
		}
		// Either we already had an exited child
		// or one of the children running exited and switched back to here
	}
	struct exitNode* exit = popChildExitNode(parentPCB);
	TracePrintf(1, "Parent process %d has an exited child: pid %d with status %d\n", parentPCB->pid, exit->pid, exit->exitType);
	*statusPtr = exit->exitType;
	info->regs[0] = exit->pid;
	free(exit);
}

void execTrapHandler(ExceptionInfo *info){
	char *filename = (char *)info->regs[1];
	char **argvec = (char **)info->regs[2];
	
	struct scheduleNode *node = getHead();
	
	int loadReturn = LoadProgram(filename, argvec, info, node->pcb);
	if (loadReturn == -1){
		info->regs[0] = ERROR;
	} else if (loadReturn == -2){
		printf("Process %d tried exec-ing file \"%s\" and encountered a non-recoverable error. Exiting process.\n", node->pcb->pid, filename);
		exitHandler(info, 1);
	}
}

void forkTrapHandler(ExceptionInfo *info){
	struct scheduleNode* currentNode = getRunningNode();
	struct processControlBlock* parentPCB = currentNode->pcb;
	
	
	int physicalPagesNeeded = numPagesInUse(parentPCB->pageTable) + KERNEL_STACK_PAGES;
	int physicalPagesAvailable = freePhysicalPageCount();
	if (physicalPagesNeeded > physicalPagesAvailable){
		TracePrintf(1, "Trap Handlers - Fork: In fork handler but not enough free physical pages (%d needed, %d available) for copy\n", physicalPagesNeeded, physicalPagesAvailable);
		//freePageTable(childPCB->pageTable);
		//free(childPCB);
		info->regs[0] = ERROR;
		return;
	}

	int childPid = updateAndGetNextPid();
	int parentPid = getCurrentPid();
	struct processControlBlock *childPCB = createNewProcess(childPid, parentPid, parentPCB);
	
	parentPCB->numChildren++;
	TracePrintf(1, "Trap Handlers - Fork: Parent pcb %d now has %d running children\n", parentPCB->pid, parentPCB->numChildren);
	cloneAndSwitchToProcess(parentPCB, childPCB);
	// Returns as the child first, but we don't need to use that info
	if(getCurrentPid() == childPid){
		info->regs[0] = 0;
	} else {
		info->regs[0] = childPid;
	}
}

void clockTrapHandler (ExceptionInfo *info) {
	TracePrintf(1, "trapHandlers: begin clockTraphandler with PID: %d\n", getCurrentPid());
	int delayWasSetToZero = decreaseDelay();
	if(setAndCheckClockTickPID() || (delayWasSetToZero == 1 && getCurrentPid() == IDLE_PID)) {
		TracePrintf(1, "trapHandlers: time to switch... scheduling processes now\n");
		scheduleProcess(0);
	}
	TracePrintf(1, "ClockTrapHandler: Exiting as process %d\n", getCurrentPid());
}

void illegalTrapHandler (ExceptionInfo *info) {
	TracePrintf(1, "trapHandlers: Illegal trap handler \n");
	
	if (info -> code == TRAP_ILLEGAL_ILLOPC) printf("Process %d terminating - Illegal Opcode\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_ILLOPN) printf("Process %d terminating - Illegal Operand\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_ILLADR) printf("Process %d terminating - Illegal address mode\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_ILLTRP) printf("Process %d terminating - Illegal software trap\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_PRVOPC) printf("Process %d terminating - Privileged opcode\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_PRVREG) printf("Process %d terminating - Privileged register\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_COPROC) printf("Process %d terminating - Coprocessor error\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_BADSTK) printf("Process %d terminating - Bad stack\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_KERNELI) printf("Process %d terminating - Linux kernel sent SIGILL\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_USERIB) printf("Process %d terminating - Received SIGILL or SIGBUS from user\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_ADRALN) printf("Process %d terminating - Invalid address alignment\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_ADRERR) printf("Process %d terminating - Non-existent physical address\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_OBJERR) printf("Process %d terminating - Object-specific HW error\n", getCurrentPid());
	else if (info -> code == TRAP_ILLEGAL_KERNELB) printf("Process %d terminating - Linux kernel sent SIGBUS\n", getCurrentPid());
	else printf("Process %d terminating - Some other illegal trap occured\n", getCurrentPid());
	
	exitHandler(info, 1);
}

void memoryTrapHandler (ExceptionInfo *info) {
	TracePrintf(1, "MemoryTrapHandler starting for process %d at address %p\n", getRunningNode()->pcb->pid, info->addr);
	if(growUserStack(info, getRunningNode()) != 1){
		printf("Process %d attempted to access invalid memory at %p. Exiting process.\n", getRunningNode()->pcb->pid, info->addr);
		exitHandler(info, 1);
	}
}

void mathTrapHandler (ExceptionInfo *info) {

	TracePrintf(1, "Exception: Math\n");
	
	if (info -> code == TRAP_MATH_INTDIV) printf("Process %d terminating - %s\n", getCurrentPid(), "Integer divide by zero");
	else if (info -> code == TRAP_MATH_INTOVF) printf("Process %d terminating - %s\n", getCurrentPid(), "Integer overflow");
	else if (info -> code == TRAP_MATH_FLTDIV) printf("Process %d terminating - %s\n", getCurrentPid(), "Floating divide by zero");
	else if (info -> code == TRAP_MATH_FLTOVF) printf("Process %d terminating - %s\n", getCurrentPid(), "Floating overflow");
	else if (info -> code == TRAP_MATH_FLTUND) printf("Process %d terminating - %s\n", getCurrentPid(), "Floating underflow");
	else if (info -> code == TRAP_MATH_FLTRES) printf("Process %d terminating - %s\n", getCurrentPid(), "Floating inexact result");
	else if (info -> code == TRAP_MATH_FLTINV) printf("Process %d terminating - %s\n", getCurrentPid(), "Invalid floating operation");
	else if (info -> code == TRAP_MATH_FLTSUB) printf("Process %d terminating - %s\n", getCurrentPid(), "FP subscript out of range");
	else if (info -> code == TRAP_MATH_KERNEL) printf("Process %d terminating - %s\n", getCurrentPid(), "Linux kernel sent SIGFPE");
	else if (info -> code == TRAP_MATH_USER) printf("Process %d terminating - %s\n", getCurrentPid(), "Received SIGFPE from user");
	else printf("Process %d terminating - %s\n", getCurrentPid(), "Some other math error occured");
	
	exitHandler(info, 1);
}

void ttyRecieveTrapHandler (ExceptionInfo *info) {
  TracePrintf(1, "trapHandlers: Entering TRAP_TTY_RECEIVE interrupt handler...\n");

  int term = info->code;  
  char *receivedChars = malloc(sizeof(char) * TERMINAL_MAX_LINE);

  int numReceivedChars = TtyReceive(term, receivedChars, TERMINAL_MAX_LINE);

  writeBuffer(term, receivedChars, numReceivedChars, 0);

  if (isNewLineInBuffer(term)) {
    TracePrintf(3, "trapHandlers: There is a new line in buffer... wake up a reader\n");
    wakeUpReader(term);
  }

  TracePrintf(1, "trapHandlers: Received %d chars from terminal %d.\n", numReceivedChars, term);
}

void
ttyTransmitTrapHandler (ExceptionInfo *info) {
  TracePrintf(1, "trapHandlers: Entering TRAP_TTY_TRANSMIT interrupt handler...\n");

  int term = info->code;  

  struct processControlBlock *oldWritingPCB = getWritingPCB(term);

  if (oldWritingPCB == NULL) {
    TracePrintf(3, "trapHandlers: Can't find the process writing to terminal %d during ttyTransmit.\n", term);
  } else {
    TracePrintf(3, "trapHandlers: Process with pid %d has been writing to terminal %d.\n", oldWritingPCB->pid, term);

    // reset its status.
    oldWritingPCB->isWriting = -1;
    TracePrintf(3, "trapHandlers: Process with pid %d is as done writing to terminal.\n", oldWritingPCB->pid);
  }
  wakeUpWriter(term);

  TracePrintf(1, "trapHandlers: TRAP_TTY_TRANSMIT handler finished.\n");
}

void
ttyReadHandler(ExceptionInfo *info) {
  int term = info->regs[1];
  if(term < 0 || term > NUM_TERMINALS){
    info->regs[0] = ERROR;
    return;
  }
  void *buf = (void *)info->regs[2];
  int len = info->regs[3];

  int numRead = readBuffer(term, buf, len);

  if (numRead >= 0) {
    info->regs[0] = numRead;
  } else {
    info->regs[0] = ERROR;
  }
}

void
ttyWriteHandler(ExceptionInfo *info) {
  int term = info->regs[1];
  if(term < 0 || term > NUM_TERMINALS){
    info->regs[0] = ERROR;
    return;
  }
  void *buf = (void *)info->regs[2];
  int len = info->regs[3];

  struct scheduleNode *item = getRunningNode();
  struct processControlBlock *currPCB = item->pcb;

  // this call blocks the process if someone is already writing to terminal.
  int numWritten = writeBuffer(term, buf, len, 1);

  TtyTransmit(term, buf, numWritten);

  // now that we are waiting for the io to finish, mark it as writing.
  currPCB->isWriting = term;

  if (numWritten >= 0) {
    info->regs[0] = numWritten;
  } else {
    info->regs[0] = ERROR;
  }
}

void getPidHandler(ExceptionInfo *info) {
	info->regs[0] = getCurrentPid();
}

void delayHandler(ExceptionInfo *info) {
	int ticksToGo = info->regs[1];
	struct processControlBlock *currPCB = getHead()->pcb;
	
	// check that we have a valid delay
	if (ticksToGo < 0){
		info->regs[0] = ERROR;
		return;
	}
	// set the delay in current process
	currPCB->delay = ticksToGo;
	info->regs[0] = 0;
	
	if(ticksToGo > 0){
		TracePrintf(1, "trapHandlers: In delayHandler... initiating a context switch.\n");
		scheduleProcess(0);
	}
	TracePrintf(1, "DelayHandler: Exiting as process %d\n", getCurrentPid());
	return;
}

void exitHandler(ExceptionInfo *info, int calledDueToProgramError) {
	int currentPid = getCurrentPid();
	if(currentPid == IDLE_PID){
		printf("Idle process called Exit(). Halting...\n");
		Halt();
	}
	
	int parentPid = getRunningNode()->pcb->parentPid;
	int exitType = calledDueToProgramError ? ERROR : (int)(info->regs[1]);
	
	TracePrintf(1, "trapHandlers: Process with pid %d attempting to exit, has parent %d\n", currentPid, parentPid);
	
	// Check that pcb is not an orphan process
	if (parentPid != ORPHAN_PARENT_PID) {
		struct processControlBlock* parentPCB = getPCB(parentPid);
		TracePrintf(3, "trapHandlers: parent: %d\n", parentPCB->pid);
		parentPCB->numChildren--;
		parentPCB->isWaiting = 0;
		// need to do more bookkeeping to keep track of exiting processes
		appendChildExitNode(parentPCB, getCurrentPid(), exitType);
	}
	// Orphan this process's children
	struct scheduleNode* checkingNode = getRunningNode();
	for(; checkingNode != NULL; checkingNode = checkingNode->next){
		if(checkingNode->pcb->parentPid == currentPid){
			checkingNode->pcb->parentPid = ORPHAN_PARENT_PID;
		}
	}
	// Remove the current node and perform scheduling to pick a new process
	removeExitingProcess();
}
