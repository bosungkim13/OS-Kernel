#include <comp421/hardware.h>
#include <comp421/yalnix.h>

void getPidHandler(ExceptionInfo *info);
void delayHandler(ExceptionInfo *info);
void exitHandler(ExceptionInfo *info, int error);
void forkTrapHandler(ExceptionInfo *info);
void waitTrapHandler(ExceptionInfo *info);
void execTrapHandler(ExceptionInfo *info);
void ttyReadHandler(ExceptionInfo *info);
void ttyWriteHandler(ExceptionInfo *info);
void kernelTrapHandler(ExceptionInfo *info);
void clockTrapHandler (ExceptionInfo *info);
void illegalTrapHandler (ExceptionInfo *info);
void memoryTrapHandler (ExceptionInfo *info);
void mathTrapHandler (ExceptionInfo *info);
void ttyRecieveTrapHandler (ExceptionInfo *info);
void ttyTransmitTrapHandler (ExceptionInfo *info);
struct exitNode *popChildExitNode(struct processControlBlock *pcb);
