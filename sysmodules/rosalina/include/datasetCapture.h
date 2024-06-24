
#pragma once

#include <3ds/os.h>
#include <3ds/types.h>
#include <3ds/services/hid.h>
#include "MyThread.h"
#include "utils.h"



int SliderIsMax(void);
MyThread *datasetCapture_CreateThread(void);
void datasetCapture_TakeScreenshot(void);
void datasetCapture_ThreadMain(void);