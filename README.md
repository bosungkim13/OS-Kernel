# Operating System Kernel

KernelStart: This file contains the kernelStart entry procedure

ContextSwitch: This file contains functions that are used by the provided ContextSwitch
  switchToExistingProcess: procedure that swithces to an existing context
  cloneAndSwitchToProcess: procedure used to clone a process and switch to that process, used for fork

loadProgram: variant of provided load program tempalte with added parameters:
  1. ExctptionInfo *info
    The info that contains the current stack pointer and program counter
  2. struct processControlBlock *pcb
    Process control block that is the process we are loading
    
pageTableManagement: Functions to manage page tables
  We have a linked list of page table entries that correspond to free half physical pages to use when allocating
  memory. These physical pages are at the top of Region 1 and we split each physical page in half because each page
  table is PAGESIZE/2.
  
 processControlBlock: Contains the functions to create and manage PCBs
  We defined a PCB as:
    int pid: The process ID. 0 is init and 1 is idle
    struct pte* pageTable: page table for the process
    SavedContext savedContext: The saved context of the process
    int delay: delay of the process
    int parentPid: Process ID of the parent
    int isWaiting: Used to flag when process is waiting
    int numChildren: Number of children
    int isReading: Flag for when process is currently reading from a terminal. isReading is the 
                    to the terminal number it is reading from. -1 if it is not reading.
    int isWriting: Flag for when process is currently writing to a terminal. isWriting is the 
                to the terminal number it is writing to. -1 if it is not reading.
    int isWaitReading: Flag for when process is currently waiting to reading from a terminal. isWaitReading is the 
                    to the terminal number it is waiting to read from. -1 if it is not waiting to read.
    int isWaitWriting: Flag for when process is currently waiting to write to a terminal. isWaitWriting is the 
                    to the terminal number it is waiting to write to. -1 if it is not waiting to write.
    exitNode *exitQ: list of child processes that have exited
    
memoryManagement: Functions for dealing with memory.
  In addition to memory management, it contains handlers for the Brk kernel call and growing user stack.
  
processScheduling: This contains procedures and structs for scheduling the running of processes
  We created a linked list of nodes that contain pcbs of all non-terminated and non-idle processes. We schedule 
  a process by traversing the linked list and choosing the first ready process.

  
terminal: Contains all code related to terminal i/o.

trapHandlers: This contains all code related to interrupt trap handlers.

