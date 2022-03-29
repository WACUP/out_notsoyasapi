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
  (player.options.common.bAudioClock?time:GetTickCount64())
#if defined (OUT_YASAPI_SUBCLASS) // {
#define PlayerIsUnderfow(pPlayer) \
  (PLAYER_STATE_UNDERFLOW==(pPlayer)->state)
#endif // }
#define PlayerHasChanged(pPlayer,module) \
  !module/*||wcscmp((pPlayer)->base.pszFileName,module)*/

int getwrittentime(void);
int getoutputtime(void);

int srate=0, numchan=0, bps=0;
volatile time_t writtentime=0, w_offset=0;
time_t start_t=0;
static int last_pause=0;
static int ref_true=1;
static int loaded=0;

static wchar_t *path;
Player player={0};
#if defined (YASAPI_GAPLESS) // {
static int bReset;
#endif // }
//static wchar_t *out_module;
static int out_module;

static prefsDlgRecW* output_prefs;

#if defined (YASAPI_GAPLESS) // {
//#define OUT_YASAPI_SUBCLASS
#endif // }

#if defined (OUT_YASAPI_SUBCLASS) // {

static LRESULT CALLBACK PluginWinampProc(HWND hWnd, UINT uMsg, WPARAM wParam,
					LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
  switch (uMsg) {
  case WM_WA_IPC:
    switch (lParam) {
    case IPC_CB_OUTPUTCHANGED:
	{
      wchar_t *m=(wchar_t *)SendMessage(hWnd,WM_WA_IPC,0,IPC_GETOUTPUTPLUGINW);
      DPRINTF(0,"  WM_WA_IPC/IPC_CB_OUTPUTCHANGED: \"%s\"\n",m);

      if (m && *m) {
        wcsncpy(out_module, m, ARRAYSIZE(out_module));
	  } else {
        out_module[0] = 0;
      }

      if (PlayerIsUnderfow(&player)&&PlayerHasChanged(&player,out_module))
	  {
		// cppcheck-suppress syntaxError
		PLAYER_SEND(&player,PlayerClose);
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

  return DefSubclass(
    hWnd,         // _In_ HWND    hWnd,
    uMsg,         // _In_ UINT    Msg,
    wParam,       // _In_ WPARAM  wParam,
    lParam        // _In_ LPARAM  lParam
  );
}
#endif // }

extern __declspec(dllexport) void __cdecl winampGetOutModeChange(int mode);
void config(HWND hwnd)
{
  DPRINTF(0, "%s\n", __func__);
  if (output_prefs != NULL)
  {
	  OpenPrefsPage((WPARAM)output_prefs);
  }
}

// grab the function from wasabi.cpp
// as that resolves 64-bit issues in
// how the string related parts work
// to avoid some odd crashes seen :(
void about(HWND hwnd);

void init(void)
{
  SetupWasabiServices(&plugin);
}

void quit(void)
{
  DPRINTF(0,"%s (%s)\n",__func__,player.base.pszFileName);

  PlayerDestroy(&player);
#if defined (OUT_YASAPI_SUBCLASS) // {
  DPUTS(0,"  unsubclassing winamp\n");
  UnSubclass(plugin.hMainWindow, PluginWinampProc);
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

  reset();
  /*if (out_module) {
	  free(out_module);
  }
  out_module = _wcsdup(player.base.pszFileName);*/

  numchan = numchannels;
  srate = samplerate;
  bps = bitspersamp;

  if (PLAYER_SEND(&player,PlayerDeviceCreateV)<0)
    return -1;

    bReset=0;

#if defined (YASAPI_GAPLESS) // {
  if (player.state<PLAYER_STATE_PAUSE) {
    return PLAYER_SEND(&player,PlayerOpen);
  }
  else if (bGapless&&bReset) {
    bChange=bChange/*||wcscmp(device.szId,player.device.szId)*/;
#if defined (YASAPI_SURROUND) // {
    bChange=bChange||player.options.common.bSurround!=player.open.bSurround;
#endif // }
    bChange=bChange||numchannels!=nChannels;
    bChange=bChange||samplerate!=nSamplesPerSec;
    bChange=bChange||bitspersamp!=wBitsPerSample;

    if (!bChange) {
      PLAYER_SEND(&player,PlayerDeviceDestroyV);
      return PLAYER_SEND(&player,PlayerReset);
    }
  }

  if (PLAYER_STATE_RUN<player.state) {
	// cppcheck-suppress syntaxError
    while (PLAYER_SEND(&player,PlayerIsPlaying))
      Sleep(1);
  }

  PLAYER_SEND(&player,PlayerClose);
#endif // }
  return PLAYER_SEND(&player,PlayerOpen);
}

void close(void)
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
    PLAYER_SEND(&player,PlayerClose);
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  else if (bDisconnect)
    PlayerSetTimerCheckUnderflow(&player);
#endif // }
#else // } {
  PLAYER_SEND(&player,PlayerClose);
#endif // }
}

int write(char *buf, int len)
{
#if 0 // {
  writtentime += len;
  return 0;
#else // } {
  DPRINTF(3,"%s (%s)\n",__func__,player.base.pszFileName);

  if ((ref_true!=last_pause)&&PLAYER_SEND(&player,PlayerWrite,buf,len)<0)
    return 1;
  else {
    writtentime+=len;
    return 0;
  }
#endif // }
}

int canwrite(void)
{
  int bytes;

  DPRINTF(3,"%s (%s)\n",__func__,player.base.pszFileName);
  bytes=((ref_true==last_pause)?0:PLAYER_SEND(&player,PlayerCanWrite));
  DPRINTF(3,"  bytes: %d\n",bytes);

  return bytes;
}

int isplaying(void)
{
#if defined (YASAPI_GAPLESS) // {
  int bGapless=player.options.common.bGapless;
#endif // }
  int bPlaying;

  DPRINTF(0,"%s (%s)\n",__func__,player.base.pszFileName);
  //DPRINTF(0,"  %s\n",module);

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

  DPRINTF(0,"%s (%s: %d, %d)\n",__func__,player.base.pszFileName,pause,bPause);

  if (last_pause!=pause) {
    time_t time=PLAYER_SEND(&player,PlayerPause);

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
  time_t time;

  DPRINTF(0,"%s\n",__func__);
  time=PLAYER_SEND(&player,PlayerFlush);
  w_offset=t;
  start_t=GetTime(time);
  writtentime=0;
}
  
int getoutputtime(void)
{
  return ((ref_true != last_pause) ? GetTime(((PLAYER_STATE_PLAY <= player.state) ?
		  PLAYER_SEND(&player, PlayerGetTime) : 0)) - start_t + w_offset : w_offset);
}

int getwrittentime(void)
{
  const int t=srate*numchan;
  int ms=writtentime;

  if (t) {
    int l = ms%t;
    ms /= t;
    ms *= 1000;
    ms += (l*1000)/t;

    ms/=(bps/8);

    ms += w_offset;
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

__declspec(dllexport) Out_Module * winampGetOutModule(void)
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
#if ! defined (OUT_YASAPI_SUBCLASS) // {
		case OUT_UNSET:
		{
			out_module = FALSE;
			// we've been unloaded so we can 
			// reset everything just in-case
			break;
		}
#endif // }
		case OUT_SET:
		{
			if (!loaded)
			{
				// if we're not being used then there's no
				// need to be loading anything until then
				#if defined (YASAPI_CO_INITIALIZE) // {
				  HRESULT hr;
				#endif // }

				  if (NULL==(path=yapath(YASAPI_PROPERTY_FILE_NAME)))
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
				  if (!Subclass(plugin.hMainWindow, PluginWinampProc)) {
					DMESSAGE("  failed to subclass winamp");
					goto subclass;
				  }

				  DPUTS(0,"  winamp subclassed\n");
				#endif // }

				  if (PlayerCreate(&player,plugin.hDllInstance,path)<0) {
					DMESSAGE("creating player");
					goto player;
				  }

				#if ! defined (OUT_YASAPI_SUBCLASS) // {
				  out_module = TRUE;
				#endif // }
				  loaded = TRUE;
				  return;
				player:
				#if defined (OUT_YASAPI_SUBCLASS) // {
				  UnSubclass(plugin.hMainWindow, PluginWinampProc);
				subclass:
				#else
				  out_module = FALSE;
				#endif // }
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

HINSTANCE WASABI_API_LNG_HINST = NULL;
INT_PTR CALLBACK ConfigProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern LPWSTR GetLangString(const UINT id);

__declspec(dllexport) BOOL __cdecl winampGetOutPrefs(prefsDlgRecW* prefs)
{
	// this is called when the preferences window is being created
	// and is used for the delayed registering of a native prefs
	// page to be placed as a child of the 'Output' node (why not)
	if (prefs)
	{
		// TODO localise
		prefs->hInst = plugin.hDllInstance/*WASABI_API_LNG_HINST*/;
		prefs->dlgID = IDD_CONFIG;
		prefs->name = _wcsdup(GetLangString(IDS_WASAPI));
		prefs->proc = (void *)ConfigProc;
		prefs->where = 9;
		prefs->_id = 52;
		output_prefs = prefs;
		return TRUE;
	}
	return FALSE;
}