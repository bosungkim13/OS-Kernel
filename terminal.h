#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

struct terminalBuffer *terminalBuffers;

struct terminalBuffer {
    char buffer[TERMINAL_MAX_LINE];
    int read;
    int write;
    int count;
};

void initTerminalBuffers();

int writeBuffer(int term, char *buf, int len, int isScheduling);

int readBuffer(int term, char *buf, int len);

int isNewLineInBuffer(int term);