/*
 * ya_trace.c
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
#include <ya.h>
#if defined (YA_DEBUG) // {

///////////////////////////////////////////////////////////////////////////////
static const Property gcaTraceProperties[]={
  { YA_PROPERTY_DEBUG,&gcIntType,offsetof(Trace,nDebug),0 },
  { YA_PROPERTY_FILE,&gcIntType,offsetof(Trace,bFile),0 },
  { YA_PROPERTY_SLEEP,&gcIntType,offsetof(Trace,nSleep),0 },
  { NULL,NULL,0,0 }
};

///////////////////////////////////////////////////////////////////////////////
static const ControlComboBoxList gcaTraceDebugList[]={
  { L"Off",0 },
  { L"Default",1 },
  { L"Verbose 1",2 },
  { L"Verbose 2",3 },
  { L"Verbose 3",4 },
  { NULL,0 }
};

static ControlComboBoxConfig gTraceDebugConfig=
  { IDC_IS_DEBUG,offsetof(Trace,nDebug),gcaTraceDebugList,
      L"Whether to switch off the debug console or whether to write"
      L" in default or verbose mode to it." };

////////////////
static ControlCheckBoxConfig gTraceFileConfig=
  { IDC_IS_FILE,offsetof(Trace,bFile),0,1,
      L"Whether YA should write the trace to a file or to a console." };

////////////////
static const ControlComboBoxList gcaTraceSleepList[]={
  { L"0 sec",0 },
  { L"2 sec",2*1000 },
  { L"5 sec",5*1000 },
  { L"10 sec",10*1000 },
  { L"20 sec",20*1000 },
  { L"1 min",1*60*1000 },
  { L"2 min",2*60*1000 },
  { L"5 min",5*60*1000 },
  { NULL,0 }
};

static ControlComboBoxConfig gTraceSleepConfig=
  { IDC_IS_SLEEP,offsetof(Trace,nSleep),gcaTraceSleepList,
      L"When switched on, how long should the debug console survive closing"
      L" Winamp." };

const Control gcaTraceControls[]={
  { &gcComboBoxType,&gTraceDebugConfig },
  { &gcCheckBoxType,&gTraceFileConfig },
  { &gcComboBoxType,&gTraceSleepConfig },
  { NULL,0 }
};

///////////////////////////////////////////////////////////////////////////////
Trace trace;

int TraceFixIdcs(Trace *pTrace, const int *pTraceIdcs)
{
  if (pTraceIdcs) {
    int idc;
    if ((idc=gTraceDebugConfig.idc)<0)
      gTraceDebugConfig.idc=pTraceIdcs[-idc-1];

    if ((idc=gTraceFileConfig.idc)<0)
      gTraceFileConfig.idc=pTraceIdcs[-idc-1];

    if ((idc=gTraceSleepConfig.idc)<0)
      gTraceSleepConfig.idc=pTraceIdcs[-idc-1];
  }

  return 0;
}

int TraceCreate(Trace *pTrace, const char *label, const wchar_t *path,
    int bMutex)
{
  PropertyIOConfig c={
    path,                 // const wchar_t *path;
    pTrace,               // void *pData;
    TraceDefault(),       // const void *pDefault;
    YA_GROUP_COMMON,      // const wchar_t *group;
    L"    "               // const wchar_t *pfx;
  };

  wchar_t filepath[MAX_PATH];
  wchar_t *pp;
  size_t len;
  SYSTEMTIME time;

  if (label)
    pTrace->label=label;

  if (path) {
    pTrace->path=path;
    PropertiesLoad(gcaTraceProperties,&c);
  }

  if (bMutex) {
  	pTrace->hMutex=CreateMutex(
    	NULL,       // _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    	FALSE,      // _In_     BOOL                  bInitialOwner,
    	NULL        // _In_opt_ LPCTSTR               lpName
  	);
  }

  if (!pTrace->nDebug) {
    pTrace->tag=TRACE_CLOSE;
    pTrace->f=NULL;
  }
  else if (pTrace->bFile) {
    pTrace->tag=TRACE_FILE;

    wcscpy(filepath,pTrace->path);
    len=wcslen(filepath)-4;
    pp=filepath+len;

    GetSystemTime(&time);

    StringCchPrintf(pp,
      (sizeof filepath)-len,
      L"-%04d"            // wYear
      L"-%02d"            // wMonth
      L"-%02d"            // wDay
      L"-%02d"            // wHour
      L"-%02d"            // wMinute
      L"-%02d"            // wSecond
      L"-%04d"            // wMilliseconds
      L".txt",
      time.wYear,
      time.wMonth,
      time.wDay,
      time.wHour,
      time.wMinute,
      time.wSecond,
      time.wMilliseconds
    );

    pTrace->f=_wfopen(filepath,L"w");;
  }
  else {
    pTrace->tag=TRACE_CONSOLE;
    pTrace->f=stderr;
    ConsoleOpen();
  }

  if (NULL!=pTrace->f&&NULL!=pTrace->label) {
    fprintf(pTrace->f,"%s\n",pTrace->label);
    fflush(pTrace->f);
  }

  return 0;
}

void TraceDestroy(Trace *pTrace, int nSleep, int bMutex)
{
  switch (pTrace->tag) {
  case TRACE_FILE:
    fclose(pTrace->f);
    break;
  case TRACE_CONSOLE:
    if (0<nSleep) {
      fprintf(stderr,"  sleeping %d ms ...\n",nSleep);
      Sleep(nSleep);
    }

    ConsoleClose();
    break;
  default:
    break;
  }

  pTrace->tag=TRACE_CLOSE;

  if (bMutex&&pTrace->hMutex)
    CloseHandle(pTrace->hMutex);
}

int TraceSwitch(Trace *pTrace)
{
  PropertyIOConfig c={
    pTrace->path,         // const wchar_t *path;
    pTrace,               // void *pData;
    NULL,                 // const void *pDefault;
    YA_GROUP_COMMON,      // const wchar_t *group;
    L"    "               // const wchar_t *pfx;
  };

  int tag;

  PropertiesSave(gcaTraceProperties,&c);
  tag=!pTrace->nDebug?TRACE_CLOSE:pTrace->bFile?TRACE_FILE:TRACE_CONSOLE;

  if (tag!=pTrace->tag) {
    TraceDestroy(pTrace,0,0);

    return TraceCreate(pTrace,NULL,NULL,0);
  }
  else
    return 0;
}

static void TraceLock(Trace *pTrace)
{
  if (pTrace->hMutex) {
    WaitForSingleObjectEx(
      pTrace->hMutex,     // _In_ HANDLE hHandle,
      INFINITE,           // _In_ DWORD  dwMilliseconds,
      FALSE               // _In_ BOOL   bAlertable
    );
  }
}

static void TraceUnlock(Trace *pTrace)
{
  if (pTrace->hMutex)
    ReleaseMutex(pTrace->hMutex);
}

int tputs(Trace *pTrace, int nLevel, const char *s)
{
  if (TRACE_CLOSE<pTrace->tag&&nLevel<pTrace->nDebug) {
    TraceLock(pTrace);
    fputs(s,pTrace->f);
    fflush(pTrace->f);
    TraceUnlock(pTrace);
  }

  return 0;
}

int tputws(Trace *pTrace, int nLevel, const wchar_t *ws)
{
  if (TRACE_CLOSE<pTrace->tag&&nLevel<pTrace->nDebug) {
    TraceLock(pTrace);
    fputws(ws,pTrace->f);
    fflush(pTrace->f);
    TraceUnlock(pTrace);
  }

  return 0;
}

int vtprintf(Trace *pTrace, const char *format, va_list ap)
{
  if (TRACE_CLOSE<pTrace->tag) {
    TraceLock(pTrace);
    vfprintf(pTrace->f,format,ap);
    fflush(pTrace->f);
    TraceUnlock(pTrace);
  }

  return 0;
}

int vtwprintf(Trace *pTrace, const wchar_t *format, va_list ap)
{
  if (TRACE_CLOSE<pTrace->tag) {
    TraceLock(pTrace);
    vfwprintf(pTrace->f,format,ap);
    fflush(pTrace->f);
    TraceUnlock(pTrace);
  }

  return 0;
}

int tprintf(Trace *pTrace, int nLevel, const char *format, ...)
{
  if (TRACE_CLOSE<pTrace->tag&&nLevel<pTrace->nDebug) {
    va_list ap;
    va_start(ap,format);
    vtprintf(pTrace,format,ap);
    va_end(ap);
  }

  return 0;
}

int twprintf(Trace *pTrace, int nLevel, const wchar_t *format, ...)
{
  if (TRACE_CLOSE<pTrace->tag&&nLevel<pTrace->nDebug) {
    va_list ap;
    va_start(ap,format);
    vtwprintf(pTrace,format,ap);
    va_end(ap);
  }

  return 0;
}

Trace *TraceDefault()
{
  static int bInitialized;
  static Trace trace;

  if (!bInitialized) {
    trace.nDebug=0;
    trace.bFile=0;
    trace.nSleep=5000;
    bInitialized=1;
  }

  return &trace;
}

LRESULT TraceEnableDebug(HWND hDlg)
{
  const PlayerStub *pStub=PlayerStubGet();
  const int *pIdc=pStub->lpVtbl->pTraceIdcs;
  const int *pMaxIdcs=pIdc+pStub->lpVtbl->nTraceIdcs;
  int idc_file=0;
  int idc_sleep=0;
  int idc_label_sleep=0;
  int nDebug;
  int bFile;

  while (pIdc<pMaxIdcs) {
    switch (-(1+pIdc-pStub->lpVtbl->pTraceIdcs)) {
    case IDC_IS_DEBUG:
      break;
    case IDC_IS_FILE:
      idc_file=*pIdc;
      break;
    case IDC_IS_SLEEP:
      idc_sleep=*pIdc;
      break;
    case IDC_IS_LABEL_DEBUG:
      break;
    case IDC_IS_LABEL_SLEEP:
      idc_label_sleep=*pIdc;
      break;
    }

    ++pIdc;
  }

  ControlsGet(gcaTraceControls,hDlg,&trace);
  nDebug=trace.nDebug;
  bFile=trace.bFile;
  EnableControl(hDlg,idc_file,nDebug);
  EnableControl(hDlg,idc_label_sleep,!bFile&&nDebug);
  EnableControl(hDlg,idc_sleep,!bFile&&nDebug);

  return nDebug;
}
#endif // }

#if defined (YA_DEBUG) // {
void TraceControlsInit(HWND hDlg, HINSTANCE hInstance)
#else // } {
void TraceControlsInit(HWND hDlg)
#endif // }
{
#if defined (YA_DEBUG) // {
  ControlsInit(gcaTraceControls,hDlg,hInstance);
  ControlsSet(gcaTraceControls,hDlg,&trace);
#else // } {
  const PlayerStub *pStub=PlayerStubGet();
  if (pStub != NULL)
  {
	  const int *pIdc = pStub->lpVtbl->pTraceIdcs;
	  const int *pMaxIdcs = pIdc + pStub->lpVtbl->nTraceIdcs;

	  while (pIdc < pMaxIdcs) {
		  switch (-(1 + pIdc - pStub->lpVtbl->pTraceIdcs)) {
		  case IDC_IS_DEBUG:
			  SendDlgItemMessage(hDlg, *pIdc, CB_ADDSTRING, 0, (LPARAM)L"Off");
			  SendDlgItemMessage(hDlg, *pIdc, CB_SETCURSEL, 0, 0);
			  break;
		  case IDC_IS_FILE:
			  break;
		  case IDC_IS_SLEEP:
			  SendDlgItemMessage(hDlg, *pIdc, CB_ADDSTRING, 0, (LPARAM)L"0 sec");
			  SendDlgItemMessage(hDlg, *pIdc, CB_SETCURSEL, 0, 0);
			  break;
		  case IDC_IS_LABEL_DEBUG:
			  break;
		  case IDC_IS_LABEL_SLEEP:
			  break;
		  }

		  EnableWindow(GetDlgItem(hDlg, *pIdc), FALSE);
		  ++pIdc;
	  }
  }
#endif // }
}

#if defined (YA_DEBUG) // {
__declspec(dllexport) const char *TraceGetId(void)
{
  static char id[]={
    0x78,
    0x41,
    0x64,
    0x6b,
    0x6b,
    0x20,
    0x79,
    0x70,
    0x51,
    0x20,
    0x62,
    0x62,
    0x7a,
    0x00
  };

  return id;
}
#endif // }