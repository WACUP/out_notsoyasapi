/*
 * ya.h
 * Copyright (C) 2015-2016 Peter Belkner <pbelkner@snafu.de>
 *
 * This file is part of libya.
 *
 * libya is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libya is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libya.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __YA_H__ // {
#define __YA_H__
#include <windows.h>
#include <strsafe.h>
#include <mmreg.h>
#include <stdarg.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <stddef.h>
//#include <pstdint.h>
#include <wasabi/bfc/platform/types.h>
#include <stdlib.h>
#endif
#include <stdio.h>
#include <math.h>
#include <winamp/wa_ipc.h>

#ifdef __cpluplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
#define YA_EVENT_STACK
#define YA_ABOUT
//#define YA_THREAD_CO_INITIALIZE

#if ! defined (YA_DEBUG) // {
//#define YA_DUMP
#endif // }

///////////////////////////////////////////////////////////////////////////////
// win32 thread-safe queue implementation using native windows API
// http://stackoverflow.com/questions/11706985/win32-thread-safe-queue-implementation-using-native-windows-api

// Mutex vs. Semaphores – Part 1: Semaphores
// https://blog.feabhas.com/2009/09/mutex-vs-semaphores-%E2%80%93-part-1-semaphores/

// Mutex vs. Semaphores – Part 2: The Mutex
// https://blog.feabhas.com/2009/09/mutex-vs-semaphores-%E2%80%93-part-2-the-mutex/

// Mutex vs. Semaphores – Part 3 (final part): Mutual Exclusion Problems
// https://blog.feabhas.com/2009/10/mutex-vs-semaphores-%E2%80%93-part-3-final-part-mutual-exclusion-problems/

// Task Synchronisation
// https://blog.feabhas.com/2009/10/task-synchronisation/

// Task Synchronisation – Part 2: Multiple Tasks and RTOS APIs
// https://blog.feabhas.com/2009/11/task-synchronisation-part-2-multiple-tasks-and-rtos-apis/

///////////////////////////////////////////////////////////////////////////////
//#define YA_THREAD_AVRT
//#define YA_THREAD_CO_INITIALIZE

///////////////////////////////////////////////////////////////////////////////
#if defined (_MSC_VER) && ! defined (__func__) // {
#define __func__                __FUNCTION__
#endif // }

#define YA_WIDEN2(x)            L ## x
#define YA_WIDEN(x)             YA_WIDEN2(x)

#if ! defined (__WFILE__) // {
#define __WFILE__               YA_WIDEN(__FILE__)
#endif // }

#if ! defined (__wfunc__) // {
#define __wfunc__               YA_WIDEN(__func__)
#endif // }

// http://stackoverflow.com/questions/2192416/how-to-convert-concatenated-strings-to-wide-char-with-the-c-preprocessor
#define YA_STR1(x)              #x
#define YA_STR(x)               YA_STR1(x)

#define YA_WSTR1(x)             L ## #x
#define YA_WSTR(x)              YA_WSTR1(x)

// http://stackoverflow.com/questions/7795514/evaluate-preprocessor-token-before-concatenation
#define YA_CONCAT1(A,B)         A ## B
#define YA_CONCAT(A,B)          YA_CONCAT1(A,B)

///////////////////////////////////////////////////////////////////////////////
#define DUNKNOWN(y) \
  messagea(1,0,&y,"UNKNOWN %x: \"%s\" (%d).\n",y,basenamea(__FILE__),__LINE__)
#define DERROR(x,y) \
  messagea(0,x,&y,#x ": \"%s\" (%d).\n",basenamea(__FILE__),__LINE__)
#define DERRORV(x,y,format,...) \
  messagea(0,x,&y,#x ": \"%s\" (%d).\n",basenamea(__FILE__),__LINE__, \
      __VA_ARGS__)
#define DMESSAGE(m) messagea(1,0,NULL,"Error " m ": " \
    "\"%s\" (%d).\n",basenamea(__FILE__),__LINE__)
#define DMESSAGEV(m,...) messagea(1,0,NULL,"Error " m \
    ": \"%s\" (%d).\n",__VA_ARGS__,basenamea(__FILE__),__LINE__)
#define DWMESSAGEV(m,...) messagew(1,0,NULL,L"Error " m \
    L": \"%s\" (%d).\n",__VA_ARGS__,basenamew(__WFILE__),__LINE__)

#if defined (YA_DEBUG) // {
  #define DON(level) \
      ((level)<trace.nDebug)
  #define DPUTS(level,cs) \
      tputs(&trace,level,cs)
  #define DPRINTF(level,cs,...) \
      tprintf(&trace,level,cs,__VA_ARGS__)
  #define DPUTWS(level,ws) \
      tputws(&trace,level,ws)
  #define DWPRINTF(level,ws,...) \
      twprintf(&trace,level,ws,__VA_ARGS__)
  #define DWARNING(m) tprintf(&trace,0,"Warning " m \
      ": \"%s\" (%d).\n",basenamea(__FILE__),__LINE__)
  #define DWARNINGV(m,...) tprintf(&trace,0,"Warning " m \
      ": \"%s\" (%d).\n",__VA_ARGS__,basenamea(__FILE__),__LINE__)
  #define DWWARNINGV(m,...) twprintf(&trace,0,L"Warning " m \
      L": \"%s\" (%d).\n",__VA_ARGS__,basenamew(__WFILE__),__LINE__)
#else // } {
  #define DON(level) 0
  #define DPUTS(level,cs)
  #define DPRINTF(level,cs,...)
  #define DPUTWS(level,ws)
  #define DWPRINTF(level,ws,...)
  #define DWARNING(m)
  #define DWARNINGV(m,...)
  #define DWWARNINGV(m,...)
#endif // }

///////////////////////////////////////////////////////////////////////////////
#if defined (YA_GLOBAL_ALLOC) // {
  #define YA_MALLOC(size)       GlobalAlloc(GMEM_FIXED,size)
  #define YA_FREE(p)            GlobalFree(p)
#else // } {
  #define YA_MALLOC(size)       malloc(size)
  #define YA_FREE(p)            free(p)
#endif // }

#define YA_INT_MAX(n)           (~(~0ull<<((n)-1)))

///////////////////////////////////////////////////////////////////////////////
const char *basenamea(const char *s);
const wchar_t *basenamew(const wchar_t *s);
wchar_t *yapath(wchar_t *file, HWND hWnd);
void yafree(void *p);
int messagea(int force, HRESULT x, HRESULT *y, const char *m, ...);
int messagew(int force, HRESULT x, HRESULT *y, const wchar_t *m, ...);
int crash(const char *format, ...);
#if defined (YA_DEBUG) // {
void ConsoleOpen(void);
void ConsoleClose(void);
#endif // }

void WFXXSetup(WAVEFORMATEXTENSIBLE *pwfxx, int samplerate, int numchannels,
    int bitspersamp, BOOL bSurround, BOOL bFloat);

///////////////////////////////////////////////////////////////////////////////
#define VALUE_INT(pData,offs) \
  (*(int *)(((char *)(pData))+(offs)))
#define VALUE_DOUBLE(pData,offs) \
  (*(double *)(((char *)(pData))+(offs)))
#define VALUE_STRING(pData,offs) \
  ((wchar_t *)(((char *)(pData))+(offs)))

#define PROPERTY_INT(pProperty,pData) \
  VALUE_INT(pData,(pProperty)->offs)
#define PROPERTY_DOUBLE(pProperty,pData) \
  VALUE_DOUBLE(pData,(pProperty)->offs)
#define PROPERTY_STRING(pProperty,pData) \
  VALUE_STRING(pData,(pProperty)->offs)

#define YA_PROPERTY_SIZE        128
#define YA_GROUP_COMMON         L"plugin"
#define YA_PROPERTY_DEBUG       L"debug"
#define YA_PROPERTY_FILE        L"file"
#define YA_PROPERTY_SLEEP       L"sleep"
#if 0 // {
#define YA_PROPERTY_VIS         L"visualization"
#define YA_PROPERTY_PAGE        L"page"
#endif // }
#if defined (YA_ABOUT) // {
#define YA_PROPERTY_CONFIGX     L"posx"
#define YA_PROPERTY_CONFIGY     L"posy"
#define YA_PROPERTY_ABOUTX      L"aboutx"
#define YA_PROPERTY_ABOUTY      L"abouty"
#else // } {
#define YA_PROPERTY_POSX        L"posx"
#define YA_PROPERTY_POSY        L"posy"
#endif // }

///////////////////////////////////////////////////////////////////////////////
#define IDC_IS_DEBUG            -1
#define IDC_IS_FILE             -2
#define IDC_IS_SLEEP            -3
#define IDC_IS_LABEL_DEBUG      -4
#define IDC_IS_LABEL_SLEEP      -5

#if defined (YA_DEBUG) // {
typedef enum _TraceTag TraceTag;
typedef struct _Trace Trace;
#elif defined (YA_DUMP) // {
typedef struct _Dump Dump;
#endif // }

typedef struct _PropertyType PropertyType;
typedef struct _PropertyIOConfig PropertyIOConfig;
typedef struct _Property Property;

typedef struct _Result Result;
typedef struct _Store Store;
typedef struct _Request Request;
typedef struct _QueueStrategy QueueStrategy;
#if defined (YA_EVENT_STACK) // {
typedef struct _QueueEvent QueueEvent;
#endif // }
typedef struct _Queue Queue;
typedef struct _Ring Ring;

typedef struct _ControlCheckBoxConfig ControlCheckBoxConfig;
typedef struct _ControlRadioButtonConfig ControlRadioButtonConfig;
typedef struct _ControlSliderConfig ControlSliderConfig;
typedef struct _ControlComboBoxList ControlComboBoxList;
typedef struct _ControlComboBoxConfig ControlComboBoxConfig;
typedef struct _ControlSliderCascadeList ControlSliderCascadeList;
typedef struct _ControlSliderCascadeConfig ControlSliderCascadeConfig;
typedef struct _ControlType ControlType;
typedef struct _Control Control;

typedef struct _IPlayer IPlayer;
typedef struct _Player Player;
typedef struct _PlayerStub PlayerStub;

typedef int PlayerProc(Player *pPlayer, Request *pRequest);
typedef void PlayerEventProc(void *data);

#if defined (YA_DEBUG) // {
///////////////////////////////////////////////////////////////////////////////
enum _TraceTag {
  TRACE_CLOSE,
  TRACE_CONSOLE,
  TRACE_FILE
};

struct _Trace {
  int nDebug;
  int bFile;
  int nSleep;
  const char *label;
  const wchar_t *path;
  TraceTag tag;
  HANDLE hMutex;

  FILE *f;
};

extern Trace trace;

int TraceCreate(Trace *pTrace, const char *label, const wchar_t *path,
    int bMutex);
void TraceDestroy(Trace *pTrace, int nSleep, int bMutex);

int TraceSwitch(Trace *pTrace);
int TraceFixIdcs(Trace *pTrace, const int *pIdcs);
Trace *TraceDefault();

LRESULT TraceEnableDebug(HWND hDlg);

int tputs(Trace *pTrace, int nLevel, const char *s);
int tputws(Trace *pTrace, int nLevel, const wchar_t *ws);
int vtprintf(Trace *pTrace, const char *format, va_list ap);
int vtwprintf(Trace *pTrace, const wchar_t *format, va_list ap);
int tprintf(Trace *pTrace, int nLevel, const char *format, ...);
int twprintf(Trace *pTrace, int nLevel, const wchar_t *format, ...);
#elif defined (YA_DUMP) // {
///////////////////////////////////////////////////////////////////////////////
struct _Dump {
  const char *label;
  const wchar_t *path;
};

extern Dump dump;

int DumpCreate(Dump *pDump, const char *label, const wchar_t *path);
void DumpDestroy(Dump *pDump);

FILE *DumpOpenFile(Dump *pDump);
#endif // }

#if defined (YA_DEBUG) // {
void TraceControlsInit(HWND hDlg, HINSTANCE hInstance);
#else // } {
void TraceControlsInit(HWND hDlg);
#endif // }

///////////////////////////////////////////////////////////////////////////////
struct _PropertyIOConfig {
  const wchar_t *path;
  void *pData;
  const void *pDefault;
  const wchar_t *group;
  const wchar_t *pfx;
};

struct _PropertyType {
  void (*Load)(const Property *pProperty, PropertyIOConfig *c);
  void (*Save)(const Property *pProperty, const PropertyIOConfig *c);
};

struct _Property {
  wchar_t *key;
  const PropertyType *type;
  int offs;
  int size;
};

extern const PropertyType gcIntType;
extern const PropertyType gcDoubleType;
extern const PropertyType gcStringType;

void PropertySaveInt(const wchar_t *group, const wchar_t *key, int n,
    const wchar_t *path);

void PropertiesLoad(const Property *pProperty, PropertyIOConfig *c);
void PropertiesSave(const Property *pProperty, const PropertyIOConfig *c);

///////////////////////////////////////////////////////////////////////////////
struct _Result {
  Result *prPrev;
  HANDLE hEvent;
  int64_t state;
};

int ResultCreate(Result *pResult);
void ResultDestroy(Result *pResult);

///////////////////////////////////////////////////////////////////////////////
enum {
  STORE_SIZE=12
};

struct _Store {
  Result *prTos;
  Result aResult[STORE_SIZE];
  HANDLE hAvailable;
  HANDLE hMutex;
#if defined (YA_DEBUG) // {
  int nAvailable;
  int nMinAvailable;
#endif // }
};

int StoreCreate(Store *pStore);
void StoreDestroy(Store *pStore);

void StorePush(Store *pStore, Result *pResult);
Result *StorePop(Store *pStore);

void StoreLock(Store *pStore);
void StoreUnlock(Store *pStore);

Result *StoreGet(Store *pStore);
void StorePut(Store *pStore, Result *pResult);

///////////////////////////////////////////////////////////////////////////////
struct _Request {
  Result *pResult;
  int exit;
  int stamp;
#if defined (YA_DEBUG) // {
  const char *id;
#endif // }
  PlayerProc *pPlayerProc;
  va_list ap;
};

///////////////////////////////////////////////////////////////////////////////
struct _QueueStrategy {
  const char *name;
  HANDLE (*GetSemaphoreDown)(Queue *pQueue);
  HANDLE (*GetSemaphoreUp)(Queue *pQueue);
  BOOL (*IsAlertable)(Queue *pQueue);
  Request **(*GetRequest)(Queue *pQueue);
#if defined (YA_DEBUG) // {
  void (*Inc)(Queue *pQueue);
  void (*Dec)(Queue *pQueue);
#endif // }
};

extern const QueueStrategy cqsWrite;
extern const QueueStrategy cqsRead;

///////////////////////////////////////////////////////////////////////////////
enum {
  QUEUE_SIZE=12,
#if defined (YA_EVENT_STACK) // {
  QUEUE_EVENT_SIZE=4,
#endif // }
};

#if defined (YA_EVENT_STACK) // {
struct _QueueEvent {
  PlayerEventProc *pOnEvent;
  void *data;
};
#endif // }

struct _Queue {
#if defined (YA_EVENT_STACK) // {
  int nEvents;
  HANDLE aHandles[QUEUE_EVENT_SIZE];
  QueueEvent aQueueEvents[QUEUE_EVENT_SIZE];
#endif // }
  Request aRequest[QUEUE_SIZE];
  Request *rp;
  Request *wp;
  Request *mp;
  HANDLE hAvailable;
  HANDLE hWritten;
  HANDLE hMutex;
#if defined (YA_DEBUG) // {
  int nAvailable;
  int nMinAvailable;
#endif // }
};

int QueueCreate(Queue *pQueue);
void QueueDestroy(Queue *pQueue);

void QueueLockMutex(Queue *pQueue);
void QueueUnlockMutex(Queue *pQueue);

#if defined (YA_EVENT_STACK) // {
int QueuePushEvent(Queue *pQueue, HANDLE hEvent, PlayerEventProc *pOnEvent,
    void *data);
void QueuePopEvent(Queue *pQueue);
Request *QueueLock(Queue *pQueue, const QueueStrategy *pStrategy);
#else // } {
Request *QueueLock(Queue *pQueue, const QueueStrategy *pStrategy,
    HANDLE hEvent, void (*OnEvent)(void *data), void *data);
#endif // }
void QueueUnlock(Queue *pQueue, const QueueStrategy *pStrategy,
    Result *pResult);
#if defined (YA_DEBUG) // {
Request *QueueLockWrite(Queue *pQueue, Result *pResult, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, va_list ap);
#else // } {
Request *QueueLockWrite(Queue *pQueue, Result *pResult, int exit, int stamp,
    PlayerProc *pPlayerProc, va_list ap);
#endif // }
#if defined (YA_EVENT_STACK) // {
Request *QueueLockRead(Queue *pQueue);
#else // } {
Request *QueueLockRead(Queue *pQueue, HANDLE hEvent,
    void (*OnEvent)(void *data), void *data);
#endif // }
void QueueUnlockRead(Queue *pQueue, Result *pResult);
#if defined (YA_DEBUG) // {
void QueueWrite(Queue *pQueue, Store *pStore, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, va_list ap);
#else // } {
void QueueWrite(Queue *pQueue, Store *pStore, int exit, int stamp,
    PlayerProc *pPlayerProc, va_list ap);
#endif // }

#if defined (YA_DEBUG) // {
void QueueIncAvailable(Queue *pQueue);
void QueueDecAvailable(Queue *pQueue);
void QueueNopAvailable(Queue *pQueue);
#endif // }

void QueueUnlockWrite(Queue *pQueue, Result *pResult);

///////////////////////////////////////////////////////////////////////////////
struct _ControlType {
  const char *magic;
  const char *name;
  void (*Init)(const Control *pControl, HWND hDlg, HWND hWnd);
  void (*Set)(const Control *pControl, HWND hDlg, const void *pData);
  void (*Get)(const Control *pControl, HWND hDlg, void *pData);
  void (*Sync)(const Control *pControl, HWND hDlg);
};

struct _ControlCheckBoxConfig {
  int idc;
  int offset;
  int off;
  int on;
  UINT/*wchar_t **/help;
};

struct _ControlRadioButtonConfig {
  int idc;
  int offset;
  int val;
  UINT/*wchar_t **/help;
};

struct _ControlSliderConfig {
  int idcSlider;
  int offset;
  double min;
  double max;
  LONG lMaxRange;
  int idcStatic;
  UINT/*const wchar_t **/format;
  UINT/*wchar_t **/help;
};

struct _ControlComboBoxList {
  UINT label;
  int val;
};

struct _ControlComboBoxConfig {
  int idc;
  int offset;
  const ControlComboBoxList *list;
  UINT/*wchar_t **/help;
};

struct _ControlSliderCascadeList {
  int idcSlider;
  int offset;
  int idcStatic;
  UINT/*const wchar_t **/format;
  UINT/*wchar_t **/help;
};

struct _ControlSliderCascadeConfig {
  double min;
  double max;
  LONG lMaxRange;
  const ControlSliderCascadeList *list;
};

struct _Control {
  const ControlType *type;
  const void *config;
};

extern const ControlType gcCheckBoxType;
extern const ControlType gcRadioButtonType;
extern const ControlType gcSliderType;
extern const ControlType gcComboBoxType;
extern const ControlType gcSliderCascadeType;

HWND ControlCreateToolTip(HWND hDlg/*, HINSTANCE hInstance*/);
void ControlAddToolTip(HWND hDlg, HWND hWnd, int idc, UINT idText/*PWSTR pszText*/);

///////////////////////////////////////////////////////////////////////////////
#if defined (YA_DEBUG) // {
extern const Control gcaTraceControls[];
#endif // }

HWND ControlsInit(const Control *pControl, HWND hDlg/*, HINSTANCE hInstance*/);
void ControlsSet(const Control *pControl, HWND hDlg, const void *pData);
void ControlsGet(const Control *pControl, HWND hDlg, void *pData);
void ControlsSync(const Control *pControl, HWND hDlg);

///////////////////////////////////////////////////////////////////////////////
int PlayerPing(Player *pPlayer, Request *pRequest);
#if defined (YA_DEBUG) // {
int PlayerWriteDebug(Player *pPlayer, Request *pRequest);
#endif // }

///////////////////////////////////////////////////////////////////////////////
#if defined (YA_DEBUG) // {
  #define PLAYER_STUB_SEND(pStub,exit,stamp,pPlayerProc,...) \
      PlayerStubSend(pStub,exit,stamp,#pPlayerProc,pPlayerProc,__VA_ARGS__)
#else // } {
  #define PLAYER_STUB_SEND(pStub,exit,stamp,pPlayerProc,...) \
      PlayerStubSend(pStub,exit,stamp,pPlayerProc,__VA_ARGS__)
#endif // }

struct _IPlayer {
#if defined (YA_PLAYER_RUN) // {
  PlayerProc *RunProc;
  PlayerProc *StopProc;
#endif // }
  int (*GetStamp)(void *pPlayer);
#if ! defined (YA_EVENT_STACK) // {
  void (*EventProc)(void *pPlayer);
  HANDLE (*GetEvent)(Player *pPlayer);
#endif // }
  int nTraceIdcs;
  const int *pTraceIdcs;
};

struct _PlayerStub {
  const IPlayer *lpVtbl;
  Player *pPlayer;
  Store store;
  Queue queue;
  HANDLE hThread;
};

PlayerStub *PlayerStubCreate(const IPlayer *lpVtbl, Player *pPlayer);
void PlayerStubDestroy(PlayerStub *pStub);

#if defined (YA_DEBUG) // {
void PlayerStubSyncDebug(PlayerStub *pStub, int stamp, HWND hDlg);
#endif // }


const PlayerStub *PlayerStubGet(void);

DWORD WINAPI PlayerStubThread(LPVOID lpParameter);

#if defined (YA_DEBUG) // {
int PlayerStubSendV(PlayerStub *pStub, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, va_list ap);
int PlayerStubSend(PlayerStub *pStub, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, ...);
void PlayerStubPost(PlayerStub *pStub, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc);
#else // } {
int PlayerStubSendV(PlayerStub *pStub, int exit, int stamp,
    PlayerProc *pPlayerProc, va_list ap);
int PlayerStubSend(PlayerStub *pStub, int exit, int stamp,
    PlayerProc *pPlayerProc, ...);
void PlayerStubPost(PlayerStub *pStub, int exit, int stamp,
    PlayerProc *pPlayerProc);
#endif // }

#ifdef __cpluplus
}
#endif
#endif // }
