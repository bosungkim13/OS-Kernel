#
#	Sample Makefile for COMP 421 Yalnix kernel and user programs.
#
#	The Yalnix kernel built will be named "yalnix".  *ALL* kernel
#	Makefiles for this lab must have a "yalnix" rule in them, and
#	must produce a kernel executable named "yalnix" -- we will run
#	your Makefile and will grade the resulting executable
#	named "yalnix".
#
#	Your project must be implemented using the C programming
#	language (e.g., not in C++ or other languages).
#

#
#	Define the list of everything to be made by this Makefile.
#	The list should include "yalnix" (the name of your kernel),
#	plus the list of user test programs you also want to be mae
#	by this Makefile.  For example, the definition below
#	specifies to make Yalnix test user programs test1, test2,
#	and test3.  You should modify this list to the list of your
#	own test programs.
#
#	For each user test program, the Makefile will make the
#	program out of a single correspondingly named sourc file.
#	For example, the Makefile will make test1 out of test1.c,
#	if you have a file named test1.c in this directory.
#
ALL = yalnix idle init providedTests/bigstack providedTests/blowstack providedTests/brktest providedTests/console providedTests/delaytest providedTests/exectest providedTests/forktest0 providedTests/forktest1 providedTests/forktest1b providedTests/forktest2 providedTests/forktest2b providedTests/forktest3 providedTests/forkwait0c providedTests/forkwait0p providedTests/forkwait1 providedTests/forkwait1b providedTests/forkwait1c providedTests/forkwait1d providedTests/init providedTests/init1 providedTests/init2 providedTests/init3 providedTests/shell providedTests/trapillegal providedTests/trapmath providedTests/trapmemory providedTests/ttyread1 providedTests/ttywrite1 providedTests/ttywrite2 providedTests/ttywrite3 testTTYRead testTTYWrite testTTYWrite2

#
#	You must modify the KERNEL_OBJS and KERNEL_SRCS definitions
#	below.  KERNEL_OBJS should be a list of the .o files that
#	make up your kernel, and KERNEL_SRCS should  be a list of
#	the corresponding source files that make up your kernel.
#
KERNEL_OBJS = kernelStart.o memoryManagement.o pageTableManagement.o processControlBlock.o processScheduling.o trapHandlers.o loadProgram.o contextSwitch.o terminal.o
KERNEL_SRCS = kernelStart.c memoryManagement.c pageTableManagement.c processControlBlock.c processScheduling.c trapHandlers.c loadProgram.c contextSwitch.c providedTests/bigstack.c providedTests/blowstack.c providedTests/brktest.c providedTests/console.c providedTests/delaytest.c providedTests/exectest.c providedTests/forktest0.c providedTests/forktest1.c providedTests/forktest1b.c providedTests/forktest2.c providedTests/forktest2b.c providedTests/forktest3.c providedTests/forkwait0c.c providedTests/forkwait0p.c providedTests/forkwait1.c providedTests/forkwait1b.c providedTests/forkwait1c.c providedTests/forkwait1d.c providedTests/init.c providedTests/init1.c providedTests/init2.c providedTests/init3.c providedTests/shell.c providedTests/trapillegal.c providedTests/trapmath.c providedTests/trapmemory.c providedTests/ttyread1.c providedTests/ttywrite1.c providedTests/ttywrite2.c providedTests/ttywrite3.c terminal.c

#
#	You should not have to modify anything else in this Makefile
#	below here.  If you want to, however, you may modify things
#	such as the definition of CFLAGS, for example.
#

PUBLIC_DIR = /clear/courses/comp421/pub

CPPFLAGS = -I$(PUBLIC_DIR)/include
CFLAGS = -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable

LANG = gcc

%: %.o
	$(LINK.o) -o $@ $^ $(LOADLIBES) $(LDLIBS)

LINK.o = $(PUBLIC_DIR)/bin/link-user-$(LANG) $(LDFLAGS) $(TARGET_ARCH)

%: %.c
%: %.cc
%: %.cpp

all: $(ALL)

yalnix: $(KERNEL_OBJS)
	$(PUBLIC_DIR)/bin/link-kernel-$(LANG) -o yalnix $(KERNEL_OBJS)

clean:
	rm -f $(KERNEL_OBJS) $(ALL)

depend:
	$(CC) $(CPPFLAGS) -M $(KERNEL_SRCS) > .depend

#include .depend
