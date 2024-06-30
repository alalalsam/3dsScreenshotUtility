#include <malloc.h>
#include <3ds.h>
#include <3ds/os.h>
#include "draw.h"
#include "fmt.h"
#include "ifile.h"
#include "utils.h"
#include "plugin.h"
#include "menu.h"

//static u32 topWidth;
//static bool is3d;

static u8 *framebufferCache;
static u8 *framebufferCacheEnd;
static MyThread CacheThread;
static MyThread WriteThread;
static u8 CTR_ALIGN(8) WriteThreadStack[0x2000];
static u8 CTR_ALIGN(8) CacheThreadStack[0x2000];
volatile u8 readyToWrite = 0;

#define TRY(expr) if(R_FAILED(res = (expr))) goto end;



static Result CacheToFile(IFile *file)
{
 u64 total;
    Result res = 0;
    u32 lineSize = 3 * 400;
    u32 remaining = lineSize * 240 * 2;
	
	//Draw_FreeFramebufferCache();
	
    //TRY(Draw_AllocateFramebufferCacheForScreenshot(remaining));

    //u8 *framebufferCache = (u8 *)Draw_GetFramebufferCache();
    //u8 *framebufferCacheEnd = framebufferCache + Draw_GetFramebufferCacheSize();

    u8 *buf = framebufferCache;

	//u8 *header = framebufferCache;
	
	//u32 nlines = 480;
    Draw_CreateBitmapHeader(framebufferCache, 400, 480);
    buf += 54;									//header

    u32 y = 0;
    // Our buffer might be smaller than the size of the screenshot...
    while (remaining != 0)
	{
        //s64 t0 = svcGetSystemTick();
        u32 available = (u32)(framebufferCacheEnd - buf);
        u32 size = available < remaining ? available : remaining;
        u32 nlines = size / lineSize;


		Draw_ConvertFrameBufferLines(buf, 400, y, nlines/2 , true, true);
		Draw_ConvertFrameBufferLines(buf, 400, y + 240, nlines/2, true, false);


        //s64 t1 = svcGetSystemTick();
        //timeSpentConvertingScreenshot += t1 - t0;
        TRY(IFile_Write(file, &total, framebufferCache, (y == 0 ? 54 : 0) + lineSize * nlines, 0)); // don't forget to write the header
		//TRY(IFile_Write(file, &total, buf, lineSize * nlines, 0));
        //timeSpentWritingScreenshot += svcGetSystemTick() - t1;
		
        y += nlines;
        remaining -= lineSize * nlines;
        buf = framebufferCache;
    }
	end:

    //Draw_FreeFramebufferCache();
    return res;
}


/*
void TopScreenToCache(void)
{
    Result res = 0;
    u32 lineSize = 3 * 400;
    u32 remaining = lineSize * 240;

    TRY(Draw_AllocateFramebufferCacheForScreenshot(remaining));

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
	
end:
    Draw_FreeFramebufferCache();
}
*/


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
    //Draw_RestoreFramebuffer();
    //Draw_FreeFramebufferCache();

    //svcFlushEntireDataCache();

    //bool is3d;
    //u32 topWidth; // actually Y-dim

    //Draw_GetCurrentScreenInfo(&topWidth, &is3d, true);

    res = FSUSER_OpenArchive(&archive, archiveId, fsMakePath(PATH_EMPTY, ""));
    if(R_SUCCEEDED(res))
    {
        res = FSUSER_CreateDirectory(archive, fsMakePath(PATH_ASCII, "/luma/screenshots"), 0);
        if((u32)res == 0xC82044BE) // directory already exists
            res = 0;
        FSUSER_CloseArchive(archive);
    }

    dateTimeToString(dateTimeStr, osGetTime(), true);

    sprintf(filename, "/luma/screenshots/%s_pair.bmp", dateTimeStr);
    TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, filename), FS_OPEN_CREATE | FS_OPEN_WRITE));
    TRY(CacheToFile(&file));
    TRY(IFile_Close(&file));

end:
    IFile_Close(&file);

    if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
        __builtin_trap(); // We're f***ed if this happens

    //svcFlushEntireDataCache();
    //Draw_SetupFramebuffer();
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
		svcSleepThread(3500000000);		//3.5s 
        if(SliderIsMax()){					//captures gameplay only at full 3d mode
			Draw_Lock();
			svcKernelSetState(0x10000, 2 | 1);		//toggles OS freeze
			svcSleepThread(5 * 1000 * 100LL);

			//idk what im doing anymore
			
			
			
			
			//Draw_FreeFramebufferCache();

			//svcFlushEntireDataCache();
			
			//Draw_GetCurrentScreenInfo(&topWidth, &is3d, true);
			
			Draw_AllocateFramebufferCacheForScreenshot(3 * 400 * 240 *2);	
			
			framebufferCache = (u8 *)Draw_GetFramebufferCache();
			
			framebufferCacheEnd = framebufferCache + Draw_GetFramebufferCacheSize();
			
			//Draw_ConvertFrameBufferLines(framebufferCache, topWidth, 0, 240, true, false);
			//Draw_ConvertFrameBufferLines(framebufferCache, topWidth, 241, 240, true, true);
			
			
			//Draw_ConvertFrameBufferLines(bufR, 400, 0, 240, true, false);
			
			
			
			/*
			if (R_FAILED(Draw_AllocateFramebufferCache(FB_BOTTOM_SIZE)))
			{
				// Oops
				svcKernelSetState(0x10000, 2 | 1);
				svcSleepThread(5 * 1000 * 100LL);
			}
			else
				Draw_SetupFramebuffer();
			svcKernelSetState(0x10000, 2 | 1);
			//TopScreenToCache();		//write to cacheL and cacheR variables
			Draw_RestoreFramebuffer();
			//Draw_FreeFramebufferCache();
			*/
			readyToWrite = 1;
			Draw_Unlock();
			
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
			continue;
		else{
			svcSleepThread(1000000000);
			Draw_Lock();
			createImageFiles();
			svcKernelSetState(0x10000, 2 | 1);		//seems to toggle screen freeze
			svcSleepThread(5 * 1000 * 100LL);
			readyToWrite = 0;
			//Draw_FreeFramebufferCache();
			Draw_Unlock();
		}
	}
}



//this thread uses syscore. writes cache to cacheR and cacheL every few seconds, then calls 2nd thread
MyThread *datasetCapture_CreateCacheThread(void)
{
    if(R_FAILED(MyThread_Create(&CacheThread, ScreenToCacheThreadMain, CacheThreadStack, 0x2000, 52, CORE_SYSTEM)))
        svcBreak(USERBREAK_PANIC);
    return &CacheThread;
}



//uses new 3ds extra cpu core. writes cacheR and cacheL to file after called by other thread.
MyThread *datasetCapture_CreateFileWriteThread(void)
{
	if( R_FAILED(MyThread_Create(&WriteThread, CacheToFileThreadMain, WriteThreadStack, 0x2000, 52, 2)))
		svcBreak(USERBREAK_PANIC);
	return &WriteThread;
}

