//made by alalalsam
#pragma once

#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>


#define THREAD_STACK_SIZE 0x1000
#define CORE_SYSTEM 	  1

typedef struct MyThread {
    Handle handle;
    void *p;
    void (*ep)(void *p);
    bool finished;
    void* stacktop;
} MyThread;


Result MyThread_Create(MyThread *t, void (*entrypoint)(void), void *stack, u32 stackSize, int prio, int affinity);
Result MyThread_Join(MyThread *thread, s64 timeout_ns);
MyThread *screenshotCreateThread(void);
void MyThread_Exit(void);
void handleShellOpened(void);
void N3DSMenu_UpdateStatus(void);
void screenshotThreadMain(void);

