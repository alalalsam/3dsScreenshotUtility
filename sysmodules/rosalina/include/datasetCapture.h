
#pragma once

#include <3ds/os.h>
#include <3ds/types.h>
#include <3ds/services/hid.h>
#include "MyThread.h"
#include "utils.h"



int SliderIsMax(void);
static Result CacheToFile(IFile *file, bool left);
static Result CacheTopScreen(void);
MyThread *datasetCapture_CreateCacheThread(void);
MyThread *datasetCapture_CreateFileWriteThread(void);
void createImageFiles(void);
void datasetCapture_TakeScreenshot(void);
void CacheScreenThreadMain(void);
void CacheToFileThreadMain(void);