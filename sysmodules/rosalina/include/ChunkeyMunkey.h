
#pragma once

#include <3ds/os.h>
#include <3ds/types.h>
#include <3ds/services/hid.h>
#include "MyThread.h"
#include "utils.h"




MyThread *chunkeyMunkeyCreateThread(void);
void chunkeyMunkey_TakeScreenshot(void);
void chunkeyMunkeyThreadMain(void);