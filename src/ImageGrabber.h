#include <windows.h>

#include "dib.h"
#include "dmtx.h"
#include "twain.h"     // Standard TWAIN header.
//#include "TwainException.h"

TW_BOOL initGrabber();
DmtxImage* acquire();
void selectSourceAsDefault();
static DmtxImage* createDmtxImage(HANDLE hMem);
void unloadTwain();
void freeHandle();
int GetPaletteSize(BITMAPINFOHEADER& bmInfo);