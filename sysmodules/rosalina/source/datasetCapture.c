
#include <3ds.h>
#include <3ds/os.h>
#include "draw.h"
#include "fmt.h"
#include "ifile.h"
#include "utils.h"
#include "plugin.h"
#include "menu.h"

static u32 cacheL;
static u32 cacheR;
static MyThread CacheThread;
static MyThread WriteThread;
static u8 CTR_ALIGN(8) WriteThreadStack[0x3000];
static u8 CTR_ALIGN(8) CacheThreadStack[0x3000];
static u8 readyToWrite = 0;

#define TRY(expr) if(R_FAILED(res = (expr))) goto end;

static s64 timeSpentConvertingScreenshot = 0;
static s64 timeSpentWritingScreenshot = 0;


static Result CacheToFile(IFile *file, bool left)
{
    u64 total;
    Result res = 0;
    u32 lineSize = 3 * 400;
    u32 remaining = lineSize * 240;
	u32 size = lineSize * remaining;
	u32 y = 0;
	
    while (remaining != 0)
    {
        u32 nlines = size / lineSize;
		if(left)
			TRY(IFile_Write(file, &total, cacheL, (y == 0 ? 54 : 0) + lineSize * nlines, 0)); // don't forget to write the header
        else TRY(IFile_Write(file, &total, cacheR, (y == 0 ? 54 : 0) + lineSize * nlines, 0));
		
        y += nlines;
        remaining -= lineSize * nlines;
    }
    end:
    return res;	
}
	
	
	

static Result CacheTopScreen(void)
{
    u64 total;
    Result res = 0;
    u32 lineSize = 3 * 400;
    u32 remaining = lineSize * 240;

    TRY(Draw_AllocateFramebufferCacheForScreenshot(remaining ));

    u8 *framebufferCache = (u8 *)Draw_GetFramebufferCache();
    u8 *framebufferCacheEnd = framebufferCache + Draw_GetFramebufferCacheSize();
	

    u8 *bufL = framebufferCache;
	u8 *bufR = framebufferCache;
    Draw_CreateBitmapHeader(framebufferCache, 400, 240);
    bufL += 54;
	bufR += 54;
	
    u32 y = 0;

    u32 available = (u32)(framebufferCacheEnd - bufL);
    u32 size = available < remaining ? available : remaining;
    u32 nlines = size / lineSize;
    Draw_ConvertFrameBufferLines(bufL, 400, y, nlines, true, true);
	Draw_ConvertFrameBufferLines(bufR, 400, y, nlines, true, false);

    //TRY(IFile_Write(file, &total, framebufferCache, (y == 0 ? 54 : 0) + lineSize * nlines, 0)); // don't forget to write the heade
	cacheL = bufL;
	cacheR = bufR;
	
    Draw_FreeFramebufferCache();
    return res;
}


void createImageFiles(void)
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

    //bool is3d;
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
    TRY(CacheToFile(&file, true));
    TRY(IFile_Close(&file));

    sprintf(filename, "/luma/screenshots/%s_top_right.bmp", dateTimeStr);
    TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
	TRY(CacheToFile(&file, false));
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


void ScreenToCacheThreadMain(void)
{
	while (!isServiceUsable("ac:u") || !isServiceUsable("hid:USER") || !isServiceUsable("gsp::Gpu") || !isServiceUsable("cdc:CHK"))
        svcSleepThread(250 * 1000 * 1000LL);
	
	while(!preTerminationRequested )			
    {
        if(SliderIsMax()){					//captures gameplay only at full 3d mode every
			svcSleepThread(3500000000);		//3.5s 

			Draw_Lock();
			svcKernelSetState(0x10000, 2 | 1);		//idk why we do this
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
			CacheTopScreen();		//write to cacheL and cacheR variables
			Draw_RestoreFramebuffer();
			Draw_FreeFramebufferCache();
			Draw_Unlock();
			readyToWrite = 1;
		}
		
    }
}


void CacheToFileThreadMain(void)
{
	while (!isServiceUsable("ac:u") || !isServiceUsable("hid:USER") || !isServiceUsable("gsp::Gpu") || !isServiceUsable("cdc:CHK"))
		svcSleepThread(250 * 1000 * 1000LL);
	while(!preTerminationRequested )			
    {
		if (!readyToWrite)
			continue
		else
			createImageFiles();
		
	}
}

//this thread uses syscore. writes cache to cacheR and cacheL every few seconds, then calls 2nd thread
MyThread *datasetCapture_CreateCacheThread(void)
{
    if(R_FAILED(MyThread_Create(&CacheThread, ScreenToCacheThreadMain, CacheThreadStack, 0x3000, 52, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);
    return &datasetCaptureConvertThread;
}

//uses new 3ds extra cpu core. writes cacheR and cacheL to file after called by other thread.
MyThread *datasetCapture_CreateFileWriteThread(void)
{
	if( R_FAILED(MyThread_Create(&WriteThread, CacheToFileThreadMain, WriteThreadStack, 0x3000, 52, 2)))
		svcBreak(USERBREAK_PANIC);
	return &datasetCaptureConvertThread;
}

