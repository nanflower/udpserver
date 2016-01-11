#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "transcodepool.h"
#include "one_process.h"
#include "outudppool.h"
//#include "Udptrans.h"
#include "values.h"
#include <QFont>

extern transcodepool *g_pCaptureDeviceVec[DEVICE_NUM];
extern outudppool *g_pLoopListBuffer[16];
//extern CUDPTrans *g_pUDPServer;


extern RESOLUTION g_enuResolution;
extern char* g_pNoSignal1280;
extern char* g_pNoSignal1920;
extern char* g_pUnmatchedSignal1280;
extern char* g_pUnmatchedSignal1920;

extern unsigned long g_CapCardPcr;

extern void HandleLongString( QString& longStr, int nWidth, const QFont& font );
#endif // __GLOBAL_H__
