//made by alalalsam

#pragma once

#include <3ds/types.h>
#include <3ds/gfx.h>
#include "utils.h"
#include "ifile.h"

static Result screenshot_WriteScreenshot(IFile *file, u32 width, bool top, bool left);
void screenshot_TakeScreenshot(void);