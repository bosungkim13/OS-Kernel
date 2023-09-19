#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>

void initKernelBrk(void *origBrk);
void* getKernelBrk();

void initPhysicalPageArray(unsigned int pmem_size);
int freePhysicalPageCount();
void markPagesInRange(void *start, void *end);
unsigned int getFreePhysicalPage();
unsigned int getTopFreePhysicalPage();
void freePhysicalPage(unsigned int pfn);
void brkHandler(ExceptionInfo *info);
void markKernelPagesTo(void *end);
void * virtualToPhysicalAddr(void * va);

void initVM();
void setupPageSwapSpace();
void* getPageSwapSpace();
int growUserStack(ExceptionInfo *info, struct scheduleNode *head);
