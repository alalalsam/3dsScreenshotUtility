//made by alalalsam

#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>


#define THREAD_STACK_SIZE 0x1000

typedef struct MyThread {
    Handle handle;
    void *p;
    void (*ep)(void *p);
    bool finished;
    void* stacktop;
} MyThread;

void _thread_begin(void);
void MyThread_Create(void);
void MyThread_Join(void);
void MyThread_Exit(void);
void screenshotCreateThread(void);
void handleShellOpened(void);
void N3DSMenu_UpdateStatus(void);
void screenshotThreadMain(void);