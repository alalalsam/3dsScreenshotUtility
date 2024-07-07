
#pragma once

#include <3ds/os.h>
#include <3ds/types.h>
#include <3ds/services/hid.h>
#include "MyThread.h"
#include "utils.h"



int SliderIsMax(void);
MyThread *datasetCapture_CreateCacheThread(void);
MyThread *datasetCapture_CreateFileThread(void);
void ConvertFrameBufferLines(u8 *buf);
void TopScreenToCache(void);
void createImageFiles(void);
void datasetCapture_TakeScreenshot(void);
void CacheScreenThreadMain(void);
void CacheToFileThreadMain(void);