#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "processScheduling.h"
#include "processControlBlock.h"
#include "trapHandlers.h"
#include "pageTableManagement.h"
#include "contextSwitch.h"
#include <stdio.h>

struct processControlBlock* idlePCB;
struct scheduleNode *idleNode = NULL;
struct scheduleNode *head = NULL;
int nextPid = 2; // first process (not init or idle) will have pid of 2
int lastClockTickPID = -1;
int isIdleRunning = 0; // 0 if not running now, 1 if running now
struct scheduleNode* processExitingNow = NULL;

void addToSchedule(struct processControlBlock *pcb){
    struct scheduleNode *newNode = malloc(sizeof(struct scheduleNode));
    newNode->next = head;
    newNode->pcb = pcb;
    head = newNode;
}

void setIdlePCB(struct processControlBlock* pcb){
	idlePCB = pcb;
	idleNode = malloc(sizeof(struct scheduleNode));
	idleNode->next = NULL;
	idleNode->pcb = idlePCB;
}

struct scheduleNode* getHead(){
	return head;
}

struct scheduleNode* getRunningNode(){
	return isIdleRunning ? idleNode : head;
}

int getCurrentPid(){
	return isIdleRunning ? idlePCB->pid : head->pcb->pid;
}

int setAndCheckClockTickPID(){
	int pidRunningNow = isIdleRunning == 1 ? idlePCB->pid : head->pcb->pid;
	int result = (lastClockTickPID == pidRunningNow);
	TracePrintf(5, "SetAndCheckClockTickPID: Previous tick had pid %d, now setting to pid %d, was%s the same\n",
	    lastClockTickPID, pidRunningNow, result == 1 ? "" : " not");
	lastClockTickPID = pidRunningNow;
	return result;
}

// Returns 1 if some process had their delay decreased to 0 so could run now
int decreaseDelay(){
	int wasZeroed = 0;
	TracePrintf(5, "DecreasingDelay - Starting\n");
	struct scheduleNode* current = head;
	while(current != NULL){
		if (current->pcb->delay > 0){
			TracePrintf(5, "DecreaseDelay - Process id %d had delay %d, getting decreased to %d\n",
			    current->pcb->pid, current->pcb->delay, current->pcb->delay - 1);
			current->pcb->delay--;
			if(current->pcb->delay == 0) wasZeroed = 1;
		}
		current = current->next;
	}
	return wasZeroed;
}

int isThisProcessBlocked(struct processControlBlock* pcb){
	return pcb->delay > 0 || pcb->isWaiting != 0 || pcb->isReading != -1 || pcb->isWriting != -1 || pcb->isWaitReading != -1 || pcb->isWaitWriting != -1;
}
void keepOrIdleProcess(struct processControlBlock* currentPCB){
	if(isIdleRunning == 0){
		// Idle is not running now. Switch to it if we need it.
		if(isThisProcessBlocked(currentPCB)){
			TracePrintf(1, "KeepOrIdleProcess: Switching to idle.\n");
			isIdleRunning = 1;
			switchToExistingProcess(currentPCB, idlePCB);
		}
	}else{
		// Idle is running now. Switch away if we can.
		if(currentPCB == idlePCB) return;
		if(!isThisProcessBlocked(currentPCB)){
			TracePrintf(1, "KeepOrIdleProcess: Switching away from idle.\n");
			isIdleRunning = 0;
			switchToExistingProcess(idlePCB, currentPCB);
		}
	}
}

void scheduleProcess(int isExit){
	TracePrintf(1, "processScheduling: Currently running as process %d, scheduling processes...\n", getCurrentPid());
	// if we are scheduling during a NON exit scenario
	if (isExit == 0){
		if(isIdleRunning == 1 && !isThisProcessBlocked(head->pcb)){
			// Happens when there are many processes to run, but they are all blocked,
			// and then the one at the head is unblocked and clock ticks.
			// Without this, the process at the head won't get switched to until another process gets
			// unblocked and switched to first.
			isIdleRunning = 0;
			switchToExistingProcess(idlePCB, head->pcb);
			TracePrintf(2, "ScheduleProcess - Returning from process scheduling (switchToHead case) as %d\n", getCurrentPid());
			return;
		}
		if (head->next == NULL) {
			// There are no other processes.
			// Keep the current process if it's not blocked, switch to idle if it is.
			TracePrintf(1, "ScheduleProcess - No other processes, switching to idle if current is blocked\n");
			keepOrIdleProcess(head->pcb);
			TracePrintf(2, "ScheduleProcess - Returning from process scheduling (head->next == NULL case) as %d\n", getCurrentPid());
			return;
		}
		
		// There are other user processes.
		struct scheduleNode* currentNode = head->next;
		// Determine the next non-blocked process
		while (currentNode != NULL){
			if (!isThisProcessBlocked(currentNode->pcb)) break;
			currentNode = currentNode->next;
		}
		if (currentNode == NULL){
			// All other processes are blocked.
			TracePrintf(1, "ScheduleProcess - All other processes blocked, switching to idle if current is blocked\n");
			keepOrIdleProcess(getRunningNode()->pcb);
			return;
		}
		TracePrintf(1, "ScheduleProcess - First non-blocked process has id %d, switching to it\n", currentNode->pcb->pid);
		
		// Now currentNode->pcb points to a non-blocked process that is not the current process.
		// Keep moving the node at the head to the tail until currentNode == head
		struct scheduleNode* executingNode = getRunningNode();
		struct scheduleNode* tail = head;
		while(tail->next != NULL) tail = tail->next;
		// Rotate
		while(head != currentNode){
			TracePrintf(1, "ScheduleProcess - Rotating list: target pid %d, head pid %d, head->next pid %d, tail pid %d\n",
			    currentNode->pcb->pid, head->pcb->pid, head->next->pcb->pid, tail->pcb->pid);
			tail->next = head;
			tail = tail->next; // Now the process at 'head' is at 'tail'
			head = head->next;
			tail->next = NULL; // Now the 'head' and 'tail' pointers have each moved one
		}
		TracePrintf(1, "ScheduleProcess - After list rotating: target pid %d, head pid %d, tail pid %d\n",
		    currentNode->pcb->pid, head->pcb->pid, tail->pcb->pid);
		isIdleRunning = 0;
		switchToExistingProcess(executingNode->pcb, head->pcb);
		return;
	} else{
		struct scheduleNode* exitingNode = head;
		head = head->next;
		// Switch to idle, then switch away next clock tick
		// This prevents special case handling for switching away from the exiting process
		// which isn't in the list
		isIdleRunning = 1;
		lastClockTickPID = 0;
		switchToExistingProcess(exitingNode->pcb, idlePCB);
	}
}

void removeExitingProcess(){
	if(processExitingNow != NULL){
		// Only happens for pid 2, the first process made by fork, for some reason. All others
		// use the one in switchToExistingProcess as intended
		TracePrintf(1, "ProcessScheduling - Remove Exiting Process: Trying to exit with process %d, but have not yet cleaned up previously exited process %d! Cleaning up now...\n", head->pcb->pid, processExitingNow->pcb->pid);
		Halt();
	}
	processExitingNow = head; // The head is the node running now. Idle is not stored in
	// the list, but idle calling Exit() was handled already.
	if(processExitingNow->next == NULL){
		printf("The final user process is exiting. Halting...\n");
		Halt();
	}
	TracePrintf(1, "ProcessScheduling - Remove Exiting Process: Before context switch away from exiting process %d\n", processExitingNow->pcb->pid);
	scheduleProcess(1);
	// CODE BELOW HERE WILL NEVER RUN
	// PUT CODE YOU WANT TO PUT HERE IN tryFreeSwitchedAwayExitingProcess
}
void tryFreeSwitchedAwayExitingProcess(){
	if(processExitingNow == NULL) return;
	TracePrintf(1, "ProcessScheduling: Starting tryFreeSwitchedAwayExitingProcess\n");
	
	int id = processExitingNow->pcb->pid;
	TracePrintf(1, "ProcessScheduling - Remove Exiting Process: After context switch away from exiting process %d\n", id);
	TracePrintf(1, "processScheduling: with scheduleNode: %p\n", processExitingNow);
	TracePrintf(1, "processScheduling: with PCB: %p\n", processExitingNow->pcb);
	TracePrintf(1, "processScheduling: with page table: %p\n", processExitingNow->pcb->pageTable);
	/*
	while(processExitingNow->pcb->exitQ != NULL){
		TracePrintf(1, "ProcessScheduling - Remove Exiting Process %d: Current exit queue %p, exit queue->next %p\n", id, processExitingNow->pcb->exitQ, processExitingNow->pcb->exitQ->next);
		struct exitNode* nowOrphanedExitedChild = processExitingNow->pcb->exitQ;
		processExitingNow->pcb->exitQ = nowOrphanedExitedChild->next;
		free(nowOrphanedExitedChild);
	}
	*/
	TracePrintf(1, "ProcessScheduling - Remove Exiting Process %d: Freed exit queue of children\n", id);
	freePageTable(processExitingNow->pcb->pageTable);
	TracePrintf(1, "ProcessScheduling - Remove Exiting Process %d: Freed page table\n", id);
	free(processExitingNow->pcb);
	TracePrintf(1, "ProcessScheduling - Remove Exiting Process %d: Freed pcb\n", id);
	free(processExitingNow);
	TracePrintf(2, "processScheduling: Completed removal of exiting process %d.\n", id);
	
	processExitingNow = NULL;
}

struct processControlBlock* getWritingPCB(int term) {
	TracePrintf(1, "processScheduling: looking for process writing to terminal %d\n", term);  
	struct scheduleNode *current = getHead();

	while (current != NULL) {
		TracePrintf(2, "processScheduling: current has pid of %d\n", current->pcb->pid);
		TracePrintf(2, "processScheduling: current is writing to terminal: %d\n", current->pcb->isWriting);
		struct processControlBlock *pcb = current->pcb;
		if (pcb->isWriting == term) {
			return pcb;
		}
		current = current->next;
	}
	TracePrintf(3, "processScheduling: No matching process writing to terminal %d\n", term);
	return NULL;
}

int updateAndGetNextPid(){
  return nextPid++;
}

void wakeUpReader(int term){
	struct scheduleNode *currNode = getHead();
	while (currNode != NULL) {
		struct processControlBlock *pcb = currNode->pcb;
		if (pcb->isWaitReading == term) {
			pcb->isWaitReading = -1;
			return;
		}
		currNode = currNode->next;
	}
	return;
}

void wakeUpWriter(int term){
	struct scheduleNode *currNode = getHead();
	while (currNode != NULL) {
		struct processControlBlock *pcb = currNode->pcb;
		if (pcb->isWaitWriting == term) {
			pcb->isWaitWriting = -1;
			return;
		}
		currNode = currNode->next;
	}
	return;
}
