#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <stdio.h>

#include "processControlBlock.h"
#include "processScheduling.h"
#include "memoryManagement.h"
#include "trapHandlers.h"
#include <comp421/loadinfo.h>
#include "pageTableManagement.h" 
#include "loadProgram.h"
#include "contextSwitch.h"
#include "terminal.h"

void **interruptVectorTable; int isInit = 1;

void KernelStart(ExceptionInfo *frame, unsigned int pmem_size, void *orig_brk, char **cmd_args){
    TracePrintf(1, "kernelStart - start of KernelStart to create %d physical pages.\n", pmem_size/PAGESIZE);
    
    initKernelBrk(orig_brk);
    // Initialize list to keep track of free pages
    initPhysicalPageArray(pmem_size);

    TracePrintf(2, "kernelStart - Free physical page structure initialized with %d free physical pages.\n", freePhysicalPageCount());

    // Mark the kernel stack
    markPagesInRange((void *)KERNEL_STACK_BASE, (void *)KERNEL_STACK_LIMIT);

    // Create the interrupt vector table
    interruptVectorTable = malloc(sizeof(void *) * TRAP_VECTOR_SIZE);
    int i;
    for (i = 0; i < TRAP_VECTOR_SIZE; i++){
        switch(i){
            case TRAP_KERNEL:
                interruptVectorTable[i] = kernelTrapHandler;
                break;
            case TRAP_CLOCK:
                interruptVectorTable[i] = clockTrapHandler;
                break;
            case TRAP_ILLEGAL:
                interruptVectorTable[i] = illegalTrapHandler;
                break;
            case TRAP_MEMORY:
                interruptVectorTable[i] = memoryTrapHandler;
                break;
            case TRAP_MATH:
                interruptVectorTable[i] = mathTrapHandler;
                break;
            case TRAP_TTY_RECEIVE:
                interruptVectorTable[i] = ttyRecieveTrapHandler;
                break;
            case TRAP_TTY_TRANSMIT:
                interruptVectorTable[i] = ttyTransmitTrapHandler;
                break;
            default:
                interruptVectorTable[i] = NULL;
        }
    }
    // Point priveleged register to the trap interrupt table
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) interruptVectorTable);
    TracePrintf(2, "kernelStart: REG_VECTOR_BASE has been set to interrupt vector table\n");

    // Initialize the kernel page table
    struct pte* kernelTable = initKernelPT();
    // set PTR1
    WriteRegister(REG_PTR1, (RCS421RegVal)kernelTable);

    TracePrintf(2, "kernelStart: Kernel Page table has been initialized and set as PTR1. Starting initialization of the first page table record\n");
    initFirstPTRecord();
    TracePrintf(2, "kernelStart: Initialization of the first page table record complete\n");

    // Region 0 page table initialization and creating the idle process
    struct processControlBlock *idlePCB = createNewProcess(IDLE_PID, ORPHAN_PARENT_PID, NULL);
    setIdlePCB(idlePCB);
    // set PTR0
    WriteRegister(REG_PTR0, (RCS421RegVal) idlePCB->pageTable);
    TracePrintf(2, "kernelStart: Idle process created and PTR0 has been set\n");

    
    initVM();
    TracePrintf(2, "kernelStart: Virtual memory now enabled\n");
    setupPageSwapSpace();
    initTerminalBuffers();

    // create the init process
    struct processControlBlock* initPCB = createNewProcess(INIT_PID, ORPHAN_PARENT_PID, idlePCB);

    // TODO: need to implement LoadProgram and ContextSwitch
    // load the idle process
    char *loadargs[2];
    loadargs[0] = "idle";
    loadargs[1] = NULL;
    
    TracePrintf(2, "kernelStart: Starting to load idle program\n");
    if(LoadProgram("idle", loadargs, frame, idlePCB) < 0){
    	TracePrintf(1, "kernelStart: Failed to load idle. Halting...\n");
    }
    TracePrintf(2, "kernelStart: Loaded idle program\n");

    cloneAndSwitchToProcess(idlePCB, initPCB);
    //ContextSwitch(idleInitFunc, &idlePCB->savedContext, (void*)idlePCB, (void*)initPCB);


    // load the init process into one of the copies created by ContextSwitch,
    // then flip "isInit" so idle (using the other copy) doesn't do the same
    if (isInit == 1){
        isInit = 0;
        if (cmd_args[0] != NULL){
            if(LoadProgram(cmd_args[0], cmd_args, frame, initPCB) < 0){
            	printf("kernelStart: Failed to load %s. Halting...\n", cmd_args[0]);
            	Halt();
            }
        }
        else{
            loadargs[0] = "init";
            loadargs[1] = NULL;
            if(LoadProgram(loadargs[0], loadargs, frame, initPCB) < 0){
            	printf("kernelStart: Failed to load %s. Halting...\n", loadargs[0]);
            	Halt();
            }
        }
        TracePrintf(2, "kernelStart: Loaded init program\n");
    }
}
