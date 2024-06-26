
#include <3ds.h>
#include <3ds/os.h>
#include "draw.h"
#include "fmt.h"
#include "ifile.h"
#include "utils.h"
#include "plugin.h"
#include "menu.h"


static MyThread datasetCaptureThread;
static u8 CTR_ALIGN(8) datasetCaptureWriteThreadStack[0x3000];
static u8 CTR_ALIGN(8) datasetCaptureConvertThreadStack[0x3000];

#define TRY(expr) if(R_FAILED(res = (expr))) goto end;

static s64 timeSpentConvertingScreenshot = 0;
static s64 timeSpentWritingScreenshot = 0;

static Result datasetCapture_ConvertScreenshot(IFile *file, u32 width, bool top, bool left)
{
    u64 total;
    Result res = 0;
    u32 lineSize = 3 * width;
    u32 remaining = lineSize * 480;

    TRY(Draw_AllocateFramebufferCacheForScreenshot(remaining));

    u8 *framebufferCache = (u8 *)Draw_GetFramebufferCache();
    u8 *framebufferCacheEnd = framebufferCache + Draw_GetFramebufferCacheSize();

    u8 *buf = framebufferCache;
    Draw_CreateBitmapHeader(framebufferCache, width, 240);
    buf += 54;

    u32 y = 0;
    // Our buffer might be smaller than the size of the screenshot...
    while (remaining != 240)
    {
        s64 t0 = svcGetSystemTick();
        u32 available = (u32)(framebufferCacheEnd - buf);
        u32 size = available < remaining ? available : remaining;
        u32 nlines = size / lineSize;
        Draw_ConvertFrameBufferLines(buf, width, y, nlines, top, true);

        s64 t1 = svcGetSystemTick();
        timeSpentConvertingScreenshot += t1 - t0;
        //TRY(IFile_Write(file, &total, framebufferCache, (y == 0 ? 54 : 0) + lineSize * nlines, 0)); // don't forget to write the header
        //timeSpentWritingScreenshot += svcGetSystemTick() - t1;

        y += nlines;
        remaining -= lineSize * nlines;
        buf = framebufferCache;
    }
	
	while (remaining != 0)
	{
		 s64 t0 = svcGetSystemTick();
        u32 available = (u32)(framebufferCacheEnd - buf);
        u32 size = available < remaining ? available : remaining;
        u32 nlines = size / lineSize;
        Draw_ConvertFrameBufferLines(buf, width, y, nlines, top, false);

        s64 t1 = svcGetSystemTick();
        timeSpentConvertingScreenshot += t1 - t0;
        //TRY(IFile_Write(file, &total, framebufferCache, (y == 0 ? 54 : 0) + lineSize * nlines, 0)); // don't forget to write the header
        //timeSpentWritingScreenshot += svcGetSystemTick() - t1;

        y += nlines;
        remaining -= lineSize * nlines;
        buf = framebufferCache;
	}
    end:

    Draw_FreeFramebufferCache();
    return res;
}


void datasetCapture_TakeScreenshot(void)
{
    IFile file;
    Result res = 0;

    char filename[64];
    char dateTimeStr[32];

    FS_Archive archive;
    FS_ArchiveID archiveId;
    s64 out;
    bool isSdMode;


    if(R_FAILED(svcGetSystemInfo(&out, 0x10000, 0x203))) svcBreak(USERBREAK_ASSERT);
    isSdMode = (bool)out;

    archiveId = isSdMode ? ARCHIVE_SDMC : ARCHIVE_NAND_RW;
    Draw_Lock();
    Draw_RestoreFramebuffer();
    Draw_FreeFramebufferCache();

    svcFlushEntireDataCache();

    bool is3d;
    u32 topWidth; // actually Y-dim

    Draw_GetCurrentScreenInfo(&topWidth, &is3d, true);

    res = FSUSER_OpenArchive(&archive, archiveId, fsMakePath(PATH_EMPTY, ""));
    if(R_SUCCEEDED(res))
    {
        res = FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, "/luma/screenshots"), 0);
        if((u32)res == 0xC82044BE) // directory already exists
            res = 0;
        FSUSER_CloseArchive(archive);
    }

    dateTimeToString(dateTimeStr, osGetTime(), true);

    sprintf(filename, "/luma/screenshots/%s_top.bmp", dateTimeStr);
    TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
    TRY(datasetCapture_WriteScreenshot(&file, topWidth, true, true));
    TRY(IFile_Close(&file));

    sprintf(filename, "/luma/screenshots/%s_top_right.bmp", dateTimeStr);
    TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
	TRY(datasetCapture_WriteScreenshot(&file, topWidth, true, false));
	TRY(IFile_Close(&file));


end:
    IFile_Close(&file);

    if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
        __builtin_trap(); // We're f***ed if this happens

    svcFlushEntireDataCache();
    Draw_SetupFramebuffer();
    Draw_Unlock();


#undef TRY
}


int SliderIsMax(void)
{
	if(osGet3DSliderState() == 1.0)
		return 1;
	return 0;
}


void datasetCapture_ThreadMain(void)
{
	while (!isServiceUsable("ac:u") || !isServiceUsable("hid:USER") || !isServiceUsable("gsp::Gpu") || !isServiceUsable("cdc:CHK"))
        svcSleepThread(250 * 1000 * 1000LL);
	
	while(!preTerminationRequested )			//only collects screenhsots at full 3d mode, for data consistency
    {
        if(SliderIsMax()){
			svcSleepThread(3500000000);		//3.5s 

			Draw_Lock();
			svcKernelSetState(0x10000, 2 | 1);
			svcSleepThread(5 * 1000 * 100LL);
			if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
			{
				// Oops
				svcKernelSetState(0x10000, 2 | 1);
				svcSleepThread(5 * 1000 * 100LL);
			}
			else
				Draw_SetupFramebuffer();
			svcKernelSetState(0x10000, 2 | 1);
			datasetCapture_TakeScreenshot();
			Draw_RestoreFramebuffer();
			Draw_FreeFramebufferCache();
			Draw_Unlock();
		}
		
    }
}


MyThread *datasetCapture_CreateConvertThread(void)
{
    if(R_FAILED(MyThread_Create(&datasetCaptureConvertThread, datasetCapture_ConvertThreadMain, datasetCaptureConvertThreadStack, 0x3000, 52, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);
    return &datasetCaptureConvertThread;
}

MyThread *datasetCapture_CreateWriteThread(void)
{
	if( R_FAILED(MyThread_Create(&datasetCaptureWriteThread