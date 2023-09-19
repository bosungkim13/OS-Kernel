#include <comp421/hardware.h>
#include <comp421/yalnix.h>

#define ORPHAN_PARENT_PID -1

struct processControlBlock
{
  int pid;
  struct pte *pageTable;
  SavedContext savedContext;
  int delay; // In clock ticks
  void *brk;
  void *userStackLimit;
  int isWaiting; // 1 if blocked due to a Wait call. 0 otherwise.
  int numChildren; // The number alive and running now. Children that have exited but not been Wait() for are not counted here
  int parentPid;
  int isReading;
  int isWaitReading;
  int isWriting;
  int isWaitWriting;
  struct exitNode *exitQ; // If not null, needs to be freed when this process exits

  // probably need to add some more stuff for the terminal blocking for reading and writing
};

struct exitNode
{
  int pid;
  int exitType;
  struct exitNode *next;
};

struct processControlBlock* createNewProcess(int pid, int parentPid, struct processControlBlock* parentPCB);
struct processControlBlock* getPCB(int pid);
void appendChildExitNode(struct processControlBlock* parentPCB, int pid, int exitType);
