#include "terminal.h"
#include "processScheduling.h"
#include "processControlBlock.h"
#include "trapHandlers.h"

int isNewLineBuffer(int term);

void
initTerminalBuffers() {
    TracePrintf(1, "terminal: Initializing terminalBuffers\n");

    terminalBuffers = malloc(sizeof(struct terminalBuffer) * NUM_TERMINALS);

    int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        terminalBuffers[i].read = 0;
        terminalBuffers[i].write = 0;
    }
}


int
writeBuffer(int term, char *buf, int len, int isScheduling) {
    int i;
    // we want to block if there is someone already writing to this terminal
    // this is a while loop, because even if this process is woken up, some process could be
    // ahead in the queue who also wants to write.
    if (isScheduling){
        while (getWritingPCB(term) != NULL) {
            struct scheduleNode *item = getRunningNode();
            struct processControlBlock *currPCB = item->pcb;

            // block the process
            currPCB->isWaitWriting = term;

            // context switch.
            scheduleProcess(0);
            // TODO reset time and schedule the next process

        }

    }
    int numWritten = 0;

    for (i = 0; i < len; i++) {
        // drop characters if our buffer is full.
        if (terminalBuffers[term].count != TERMINAL_MAX_LINE) {
            terminalBuffers[term].buffer[terminalBuffers[term].write] = buf[i];
            terminalBuffers[term].write = (terminalBuffers[term].write + 1) % TERMINAL_MAX_LINE;
            terminalBuffers[term].count++;
            numWritten++;
        }
    }
    return numWritten;
}

int
readBuffer(int term, char *buf, int len) {
    int i;
    struct scheduleNode *item = getRunningNode();
    struct processControlBlock *currPCB = item->pcb;
    // we want to block if there is not a new line in the terminal's buffer
    // this is a while loop, because even if this process is woken up, some process could be
    // ahead in the queue who also wants to read.
    while (!isNewLineInBuffer(term)) {
        TracePrintf(3, "terminal: Waiting to read a line from terminal %d\n", term);
        // block the process
        currPCB->isWaitReading = term;
        // context switch.
        scheduleProcess(0);

    }
    // there is new line that needs to be read
    int numRead = 0;
    int bufferIdx = terminalBuffers[term].read;
    for (i = 0; i < len; i++) {
        if (terminalBuffers[term].count > 0) {
            buf[i] = terminalBuffers[term].buffer[bufferIdx];
            bufferIdx = (bufferIdx + 1) % TERMINAL_MAX_LINE;
            terminalBuffers[term].count--;
            numRead++;
        } else {
        break;
        }
    }
    TracePrintf(2, "terminal: readBuffer read %d characters\n", numRead);
    return numRead;
}

int
isNewLineInBuffer(int term) {
    int i;
    int bufferIdx = terminalBuffers[term].read;
    for (i = 0; i < terminalBuffers[term].count; i++) {
        TracePrintf(3, "terminals: terminal %d running through buffer...: %c\n", term, terminalBuffers[term].buffer[bufferIdx]);
        if (terminalBuffers[term].buffer[bufferIdx] == '\n') {
            return 1;
        }
        bufferIdx = (bufferIdx + 1) % TERMINAL_MAX_LINE;
    }
    return 0;
}
