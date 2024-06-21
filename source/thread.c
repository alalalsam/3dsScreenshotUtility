// made by alalalsam (kind of)
// copypasted from Luma3DS/sysmodules/pm/source/my_thread.c and menu.c and others
//changes:
// modified functions to support automatic screenshot-taking hopefully
// modified names of menuThread functions to screenshotThread
// removed menu-opening capabilites
// added functions from other files to cut down on dependencies (HandleShellOpened) & (N3DSMenu_UpdateStatus)
// added TickCounter for timing between screenshots

//current contents: a few thread helper functions, and screenshotThreadMain that takes screenshot every 3.5s
#pragma once

#include <3ds.h>
#include <3ds/types.h>
#include "thread.h"
#include "utils.h"
#include "csvc.h"
#include "screenshot.h"
#include "ifile.h"

static MyThread screenshotThread;
static u8 CTR_ALIGN(8) screenshotThreadStack[0x3000];


static void _thread_begin(void* arg)
{
    MyThread *t = (MyThread *)arg;
    t->ep(t->p);
    MyThread_Exit();
}

Result MyThread_Create(MyThread *t, void (*entrypoint)(void), void *stack, u32 stackSize, int prio, int affinity)
{
    t->ep       = entrypoint;
    t->stacktop = (u8 *)stack + stackSize;

    return svcCreateThread(&t->handle, _thread_begin, (u32)t, (u32*)t->stacktop, prio, affinity);
}

Result MyThread_Join(MyThread *thread, s64 timeout_ns)
{
    if (thread == NULL) return 0;
    Result res = svcWaitSynchronization(thread->handle, timeout_ns);
    if(R_FAILED(res)) return res;

    svcCloseHandle(thread->handle);
    thread->handle = (Handle)0;

    return res;
}

void MyThread_Exit(void)
{
    svcExitThread();
}


MyThread *screenshotCreateThread(void)
{
    if(R_FAILED(MyThread_Create(&screenshotThread, screenshotThreadMain, screenshotThreadStack, 0x3000, 52, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);
    return &screenshotThread;
}


void handleShellOpened(void)
{
    // Somtimes this is called before Rosalina thread main executes,
    // sometimes not... how fun :))

    s64 out = 0;
    svcGetSystemInfo(&out, 0x10000, 4);
    u32 multiConfig = (u32)out;
//    u32 forceOp = (multiConfig >> (2 * (u32)FORCEAUDIOOUTPUT)) & 3;

    // We need to check here if GSP has done its init stuff, in particular
    // clock and reset, otherwise we'll cause core1 to be in a waitstate
    // forever (if we access a GPU reg while the GPU block's clock is off).
    // (GSP does its init before registering its services)
    if (isServiceUsable("gsp::Gpu"))
        ScreenFiltersMenu_RestoreSettings();

//    if (forceOp != 0 && isServiceUsable("cdc:CHK"))
//        forceAudioOutput(forceOp);
}

void N3DSMenu_UpdateStatus(void)
{
    svcGetSystemInfo(&clkRate, 0x10001, 0);
    svcGetSystemInfo(&higherClkRate, 0x10001, 1);
    svcGetSystemInfo(&L2CacheEnabled, 0x10001, 2);

    N3DSMenu.items[0].title = L2CacheEnabled ? "Disable L2 cache" : "Enable L2 cache";
    sprintf(clkRateBuf, "Set clock rate to %luMHz", clkRate != 268 ? 268 : (u32)higherClkRate);
}


u32 time0 = 0;
u32 time1 = 0;
//adapted from menuThreadMain
void screenshotThreadMain(void)
{
    if(isN3DS)
        N3DSMenu_UpdateStatus();

    while (!isServiceUsable("ac:u") || !isServiceUsable("hid:USER") || !isServiceUsable("gsp::Gpu") || !isServiceUsable("cdc:CHK"))
        svcSleepThread(250 * 1000 * 1000LL);

    handleShellOpened();

    hidInit(); // assume this doesn't fail
    isHidInitialized = true;

    while(!preTerminationRequested)
    {
        svcSleepThread(50 * 1000 * 1000LL);
		if (!time0)
			time0 = svcGetSystemTick;
		time1 = svcGetSystemTick()
		if(time1 - time0 > 3500){
			screenshot_TakeScreenshot();
			time0 = time1;
		}
		
    }
}


