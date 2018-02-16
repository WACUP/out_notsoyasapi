/*
 * out_yasapi.c
 * Copyright (C) 2015-2016 Peter Belkner <pbelkner@snafu.de>
 *
 * This file is part of out_yasapi.
 *
 * out_yasapi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * out_yasapi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with out_yasapi.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <yasapi.h>

#define GetTime(time) \
  (bAudioClock?time:GetTickCount())
#define PlayerIsUnderfow(pPlayer) \
  (PLAYER_STATE_UNDERFLOW==(pPlayer)->state)
#define PlayerHasChanged(pPlayer,module) \
  wcscmp((pPlayer)->base.pszFileName,module)

int getwrittentime();
int getoutputtime();

int srate, numchan, bps, active;
volatile int64_t writtentime, w_offset;
int64_t start_t;
static int last_pause=0;
static int ref_true=1;
static int loaded=0;

static wchar_t *path;
static PlayerDevice device;
static Player player;
static int bAudioClock;
#if defined (YASAPI_GAPLESS) // {
static int bReset;
#endif // }
static wchar_t out_module[MAX_PATH];

#if defined (YASAPI_GAPLESS) // {
#define OUT_YASAPI_SUBCLASS
#endif // }

static int PlayerSendOpen(int srate, int numchan, int bps)
{
  return PLAYER_SEND(&player,PlayerOpen,&device,srate,numchan,bps);
}

static int PlayerSendClose(void)
{
   return PLAYER_SEND(&player,PlayerClose);
}

#if defined (OUT_YASAPI_SUBCLASS) // {
static WNDPROC WinampProc;

static LRESULT CALLBACK PluginWinampProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
  char *m;

  switch (uMsg) {
  case WM_WA_IPC:
    switch (lParam) {
    case IPC_CB_OUTPUTCHANGED:
	{
      m=(char *)SendMessage(hWnd,WM_WA_IPC,0,IPC_GETOUTPUTPLUGIN);
      DPRINTF(0,"  WM_WA_IPC/IPC_CB_OUTPUTCHANGED: \"%s\"\n",m);

      if (m && *m) {
        MultiByteToWideChar(CP_ACP, 0, m, -1, out_module, ARRAYSIZE(out_module));
	  } else {
        out_module[0] = 0;
      }

      if (PlayerIsUnderfow(&player)&&PlayerHasChanged(&player,out_module))
	  {
        PlayerSendClose();
	  }
      break;
	}
    default:
      break;
    }

    break;
  default:
    break;
  }

  return CallWindowProc(
    WinampProc,   // _In_ WNDPROC lpPrevWndFunc,
    hWnd,         // _In_ HWND    hWnd,
    uMsg,         // _In_ UINT    Msg,
    wParam,       // _In_ WPARAM  wParam,
    lParam        // _In_ LPARAM  lParam
  );
}

static int SubclassWinamp(HWND hWnd)
{
  if (NULL==hWnd||NULL!=WinampProc) {
    DMESSAGE("failing to subclass winamp window");
    return -1;
  }

  WinampProc=(WNDPROC)SetWindowLongPtr(
    hWnd,                   // _In_ HWND hWnd,
    GWLP_WNDPROC,            // _In_ int  nIndex,
    (LONG)PluginWinampProc  // _In_ LONG dwNewLong
  );

  return 0;
}

static int UnsubclassWinamp(HWND hWnd)
{
  if (loaded) {
    if (NULL==hWnd||NULL==WinampProc) {
      DMESSAGE("failing to unsubclass winamp window");
      return -1;
    }

    SetWindowLongPtr(
      hWnd,                   // _In_ HWND hWnd,
      GWLP_WNDPROC,            // _In_ int  nIndex,
      (LONG)WinampProc        // _In_ LONG dwNewLong
    );
  }
  return 0;
}
#endif // }

static int64_t GetAudioTime(void)
{
  int64_t time=0;

  if (PLAYER_STATE_PLAY<=player.state)
    PLAYER_SEND(&player,PlayerGetTime,&time);

  return time;
}

void config(HWND hwnd)
{
  DPRINTF(0,"%s\n",__func__);
  ConfigDialog(&player/*,plugin.hDllInstance*/,hwnd);
}

void about(HWND hwnd)
{
  DPRINTF(0,"%s\n",__func__);
#if defined (YASAPI_ABOUT) // {
  AboutDialog(&player,plugin.hDllInstance,hwnd);
#else // } {
  AboutDialog(hwnd);
#endif // }
}

void init()
{
  SetupWasabiServices(&plugin);
}

void quit()
{
  DPRINTF(0,"%s (%s)\n",__func__,player.base.pszFileName);

  PlayerDestroy(&player);
#if defined (OUT_YASAPI_SUBCLASS) // {
  DPUTS(0,"  unsubclassing winamp\n");
  UnsubclassWinamp(plugin.hMainWindow);
#endif // }
#if defined (YASAPI_CO_INITIALIZE) // {
  DPUTS(0,"  destroying com\n");
  CoUninitialize();
#endif // }
#if defined (YA_DEBUG) // {
  TraceDestroy(&trace,trace.nSleep,1);
#elif defined (YA_DUMP) // } {
  DumpDestroy(&dump);
#endif // }
  yafree(path);
}

static void reset(void)
{
#if defined (YASAPI_SWITCH_PAUSE) // {
  int bPause=ref_true==last_pause;
#endif // }

  start_t = GetTime(0);
  w_offset = writtentime = 0;
#if defined (YASAPI_SWITCH_PAUSE) // {
  ref_true=!bPause;
#endif // }
  last_pause=!ref_true;
}

#if defined (YASAPI_GAPLESS) // {
static int drain(int numchan, int srate, int bps)
{
  bReset=0;

  if (PLAYER_STATE_RUN<player.state) {
    while (PLAYER_SEND(&player,PlayerIsPlaying))
      Sleep(1);
  }

  PlayerSendClose();

  return PlayerSendOpen(srate,numchan,bps);
}
#endif // }

int open(int samplerate, int numchannels, int bitspersamp, int bufferlenms,
    int prebufferms)
{
#if defined (YASAPI_GAPLESS) // {
  int bGapless=player.options.common.bGapless;
  WAVEFORMATEXTENSIBLE *pwfxx=&player.open.wfxx;
  WAVEFORMATEX *pwfx=&pwfxx->Format;
  WORD wBitsPerSample=pwfx->wBitsPerSample;
  int nChannels=pwfx->nChannels;
  int nSamplesPerSec=pwfxx->Format.nSamplesPerSec;
  int bChange=0;
#endif // }

  DPRINTF(0,"%s (%s)\n",__func__,player.base.pszFileName);

  bAudioClock=player.options.common.bAudioClock;
  reset();
  lstrcpyn(out_module, player.base.pszFileName, ARRAYSIZE(out_module));

  active=1;
  numchan = numchannels;
  srate = samplerate;
  bps = bitspersamp;

  if (PLAYER_SEND(&player,PlayerDeviceCreateV,&device)<0)
    return -1;

#if defined (YASAPI_GAPLESS) // {
  if (player.state<PLAYER_STATE_PAUSE) {
    bReset=0;

    return PlayerSendOpen(srate,numchan,bps);
  }
  else if (bGapless&&bReset) {
    bReset=0;

    bChange=bChange||wcscmp(device.szId,player.device.szId);
#if defined (YASAPI_SORROUND) // {
    bChange=bChange||player.options.common.bSorround!=player.open.bSorround;
#endif // }
    bChange=bChange||numchannels!=nChannels;
    bChange=bChange||samplerate!=nSamplesPerSec;
    bChange=bChange||bitspersamp!=wBitsPerSample;

    if (bChange)
      return drain(numchan,srate,bps);
    else {
      PLAYER_SEND(&player,PlayerDeviceDestroyV,&device);

      return PLAYER_SEND(&player,PlayerReset);
    }
  }
  else {
    bReset=0;

    return drain(numchan,srate,bps);
  }
#else // } {
  return PlayerSendOpen(srate,numchan,bps);
#endif // }
}

void close()
{
#if defined (YASAPI_GAPLESS) // {
  int bGapless=player.options.common.bGapless;
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  int bDisconnect=player.options.common.bDisconnect;
#endif // }
#endif // }

  DPRINTF(0,"%s (%s)\n",__func__,player.base.pszFileName);
#if defined (YASAPI_GAPLESS) // {
  if (!bReset||!bGapless)
    PlayerSendClose();
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  else if (bDisconnect)
    PlayerSetTimerCheckUnderflow(&player);
#endif // }
#else // } {
  PlayerSendClose();
#endif // }
}

int write(char *buf, int len)
{
#if 0 // {
  writtentime += len;
  return 0;
#else // } {
  int bPause=ref_true==last_pause;

  DPRINTF(3,"%s (%s)\n",__func__,player.base.pszFileName);

  if (!bPause&&PLAYER_SEND(&player,PlayerWrite,buf,len)<0)
    return 1;
  else {
    writtentime+=len;
    return 0;
  }
#endif // }
}

int canwrite()
{
  int bPause=ref_true==last_pause;
  int bytes;

  DPRINTF(3,"%s (%s)\n",__func__,player.base.pszFileName);
  bytes=bPause?0:PLAYER_SEND(&player,PlayerCanWrite);
  DPRINTF(3,"  bytes: %d\n",bytes);

  return bytes;
}

int isplaying()
{
#if defined (YASAPI_GAPLESS) // {
  int bGapless=player.options.common.bGapless;
#endif // }
  int bPlaying;

  DPRINTF(0,"%s (%s)\n",__func__,player.base.pszFileName);
  DPRINTF(0,"  %s\n",module);

#if defined (YASAPI_GAPLESS) // {
  if (!bGapless||PlayerHasChanged(&player,out_module))
    bPlaying=PLAYER_SEND(&player,PlayerIsPlaying);
  else {
    bReset=1;
    bPlaying=0;
  }
#else // } {
  bPlaying=PLAYER_SEND(&player,PlayerIsPlaying);
#endif // }

  return bPlaying;
}

int pause(int pause)
{
#if 0 // {
  int t=last_pause;
  if (!last_pause && pause) { w_offset+=GetTickCount()-start_t;  writtentime=0; }
  if (last_pause && !pause) { start_t=GetTickCount(); }
  last_pause=pause;
  return t;
#else // } {
  int bPause=ref_true==pause;
  int t=last_pause;
  int64_t time=0;

  DPRINTF(0,"%s (%s: %d, %d)\n",__func__,player.base.pszFileName,pause,bPause);

  if (last_pause!=pause) {
    PLAYER_SEND(&player,PlayerPause,&time);

    if (bPause) {
      w_offset+=GetTime(time)-start_t;
      writtentime=0;
    }
    else
      start_t=GetTime(time);
  }

  last_pause=pause;

  return t;
#endif // }
}

void setvolume(int volume)
{
  enum { DEBUG=3 };

  if (player.options.common.bVolume&&-666!=volume) {
#if defined (YASAPI_FORCE24BIT) // {
    player.base.nVolume=volume;
    DPRINTF(DEBUG,"%s: %.2f\n",__func__,(double)volume/YASAPI_MAX_VOLUME);
#else // } {
    player.base.qVolume=255==volume?1.0:(double)volume/255;
    DPRINTF(DEBUG,"%s: %.2f\n",__func__,player.base.qVolume);
#endif // }
  }
}

void setpan(int pan)
{
}

void flush(int t)
{
  int64_t time=0;
  
  DPRINTF(0,"%s\n",__func__);
  PLAYER_SEND(&player,PlayerFlush,&time);
  w_offset=t;
  start_t=GetTime(time);
  writtentime=0;
}
  
int getoutputtime()
{
#if 0 // {
  if (last_pause)
    return w_offset;
  return GetTickCount()-start_t + w_offset;
#else // } {
  int bPause=ref_true==last_pause;

  if (bPause)
    return w_offset;
  return GetTime(GetAudioTime())-start_t + w_offset;
#endif // }
}

int getwrittentime()
{
  int t=srate*numchan;
  int ms=writtentime;

  if (t) {
    int l = ms%t;
    ms /= t;
    ms *= 1000;
    ms += (l*1000)/t;

    ms/=(bps/8);

    return ms + w_offset;
  }
   return ms;
}

Out_Module plugin =
{
	OUT_VER_U,		// module version (OUT_VER)
	(LPSTR)L"",//(LPSTR)TEXT("Not So YASAPI Output v") TEXT(PLUGIN_VERSION),		// description of module, with version string
	YASAPI_WA_ID,	// module id. each input module gets its own.
					// non-nullsoft modules should be >= 65536.
	NULL,			// winamp's main window (filled in by winamp)
	NULL,			// DLL instance handle (filled in by winamp)
	config,			// configuration dialog
	about,			// about dialog
	init,			// called when loaded
	quit,			// called when unloaded
	open,
	close,			// close the ol' output device.
	write,
	canwrite,
	isplaying,
	pause,			// returns previous pause state
	setvolume,		// volume is 0-255
	setpan,			// pan is -128 to 128
	flush,
	getoutputtime,	// returns played time in MS
	getwrittentime	// returns time written in MS (used for synching up vis stuff)
};

__declspec(dllexport) Out_Module * winampGetOutModule()
{
	return &plugin;
}

__declspec(dllexport) int __cdecl winampUninstallPlugin(HINSTANCE hDllInst, HWND hwndDlg, int param)
{
	return OUT_PLUGIN_UNINSTALL_REBOOT;
}

__declspec(dllexport) void __cdecl winampGetOutModeChange(int mode)
{
	// just look at the set / unset state
	switch (mode & ~0xFF0)
	{
		case OUT_UNSET:
		{
			// we've been unloaded so we can 
			// reset everything just in-case
			break;
		}
		case OUT_SET:
		{
			if (!loaded)
			{
				// if we're not being used then there's no
				// need to be loading anything until then
				#if defined (YASAPI_CO_INITIALIZE) // {
				  HRESULT hr;
				#endif // }

				  if (NULL==(path=yapath(YASAPI_PROPERTY_FILE_NAME,plugin.hMainWindow)))
					goto path;

				#if defined (YA_DEBUG) // {
				  TraceCreate(&trace,PI_LABEL,path,1);
				#elif defined (YA_DUMP) // } {
				  DumpCreate(&dump,PI_LABEL,path);
				#endif // }

				  DPRINTF(0,"%s (%s)\n",__func__,YASAPI_MODULE);
				  DWPRINTF(0,L"  \"%s\"\n",path);

				#if defined (YASAPI_CO_INITIALIZE) // {
				  hr=CoInitializeEx(
					NULL,                     // _In_opt_ LPVOID pvReserved
					COINIT_MULTITHREADED      // _In_     DWORD  dwCoInit
				  );

				  if (FAILED(hr)) {
					DERROR(S_FALSE,hr);
					DERROR(RPC_E_CHANGED_MODE,hr);
					DMESSAGE("initializing com failed");
					goto com;
				  }

				  DPUTS(0,"  com initialized\n");
				#endif // }

				#if defined (OUT_YASAPI_SUBCLASS) // {
				  if (SubclassWinamp(plugin.hMainWindow)<0) {
					DMESSAGE("  failed to subclass winamp");
					goto subclass;
				  }

				  DPUTS(0,"  winamp subclassed\n");
				#endif // }

				  if (PlayerCreate(&player,plugin.hDllInstance,path)<0) {
					DMESSAGE("creating player");
					goto player;
				  }

				  loaded = TRUE;
				  return;
				//cleanup:
				player:
				#if defined (OUT_YASAPI_SUBCLASS) // {
				  UnsubclassWinamp(plugin.hMainWindow);
				#endif // }
				subclass:
				#if defined (YASAPI_CO_INITIALIZE) // {
				  CoUninitialize();
				com:
				#endif // }
				  yafree(path);
				path:
				  return;
			}
			break;
		}
	}
}

#ifndef _DEBUG
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}
#endif