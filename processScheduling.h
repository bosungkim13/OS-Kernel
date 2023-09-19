#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

#define INIT_PID 1
#define IDLE_PID 0
#define IDLE_DELAY -1

struct scheduleNode {
  struct scheduleNode *next;
  struct processControlBlock *pcb;
};

void addToSchedule(struct processControlBlock *pcb);
void setIdlePCB(struct processControlBlock* pcb);
struct scheduleNode* getHead();
struct scheduleNode* getRunningNode();
int getCurrentPid();

int setAndCheckClockTickPID();
int decreaseDelay();

void scheduleProcess(int isExit);
void removeExitingProcess();
int updateAndGetNextPid();
void tryFreeSwitchedAwayExitingProcess();
struct processControlBlock* getWritingPCB(int term);
void wakeUpReader(int term);
void wakeUpWriter(int term);
