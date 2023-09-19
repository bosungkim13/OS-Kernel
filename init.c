#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

void testDelay(int amount){
	printf("Init id %d: Calling Delay with parameter %d\n", GetPid(), amount);
	printf("Init id %d: Delay returned with value %d\n", GetPid(), Delay(amount));
}
void delayCountdownLoop(int start, int busyWorkInc){
	int i = start;
	for(; i >= 0; i--){
		printf("Init id %d: Delay loop i = %d\n", GetPid(), i);
		testDelay(i);
		
		printf("Init id %d: Doing busy work. Idle should not run until Delay!\n", GetPid());
		int j;
		for(j = 200; !(j >= 0 && j <= 100); j += busyWorkInc); // Will int overflow. Adjust inc for speed
	}
}

// Returns true if child
int testFork(){
	printf("Init id %d: Testing fork...\n", GetPid());

	int childPid = Fork();

	printf("Init id %d: Fork returned\n", GetPid());
	printf("Init id %d: childPid is %d\n", GetPid(), childPid);
	return childPid == 0;
}

// Returns 1 if it got a child
int testWait(){
	printf("Init id %d: Waiting for child process...\n", GetPid());
	int childStatus = 0;
	int childPid = Wait(&childStatus);
	printf("Init id %d: Wait returned, with child process id %d and status %d\n", GetPid(), childPid, childStatus);
	return childPid != ERROR;
}

void testExit(int status){
	printf("Init id %d: Testing exit with status %d\n", GetPid(), status);
	Exit(status);
}

int main() {
	//printf("Init id %d: Initialized and running.\n", GetPid());

	// Test invalid delay
	//testDelay(-10);

	// All calls test
	/*int i = 0;
	for(; i < 5; i++){
		printf("Init id %d: Main loop iteration %d\n", GetPid(), i);
		testFork();
		testDelay(i);//delayCountdownLoop(i, 8-i);
	}
	while(testWait());
	testExit(GetPid());*/
	/*
	int i = 0;
	for(; i < 100; i++){
		char* testPointer1 = malloc(10000);
		printf("Init id %d: Malloced test pointer one at %p\n", GetPid(), testPointer1);
		testFork();
		testFork();
		//testFork();
		// 4 processes
		char* testPointer2 = malloc(10000);
		printf("Init id %d: Malloced test pointer two at %p\n", GetPid(), testPointer2);
		
		free(testPointer1);
		printf("Init id %d: Freed test pointer one at %p\n", GetPid(), testPointer1);
		free(testPointer2);
		printf("Init id %d: Freed test pointer two at %p\n", GetPid(), testPointer2);
		
		if(malloc(10000000) == NULL) printf("Init id %d: Tried mallocing 10M, returned null as expected.\n", GetPid());
		
		while(testWait());
		if(GetPid() != 1) testExit(GetPid());
	}*/
	
	// TODO The true contents of init, once we're done testing.
	while(1) Pause();
	
	return 0;
}
