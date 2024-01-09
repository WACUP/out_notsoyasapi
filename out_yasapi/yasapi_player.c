/*
 * yasapi_player.c
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
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
// Rendering a Stream
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd316756%28v=vs.85%29.aspx

// Exclusive-Mode Streams
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd370844%28v=vs.85%29.aspx

#if ! defined (AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY) // {
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif // }

#if ! defined(AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM) // {
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif // }

///////////////////////////////////////////////////////////////////////////////
static int caTraceIdcs[]={
  IDC_COMBOBOX_DEBUG,
  IDC_CHECKBOX_FILE,
  IDC_COMBOBOX_SLEEP,
  IDC_STATIC_LABEL_DEBUG,
  IDC_STATIC_LABEL_SLEEP
};

static IPlayer gcIPlayer={
  PlayerGetStamp,   // int (*GetStamp)(void *pPlayer);
#if ! defined (YA_EVENT_STACK) // {
  PlayerPostRead,   // PlayerEventProc *EventProc;
  PlayerGetEvent,   // HANDLE (*GetEvent)(Player *pPlayer);
#endif // }
  (sizeof caTraceIdcs)/(sizeof caTraceIdcs[0]),
                    // int nTraceIdcs
  caTraceIdcs       // const int *pTraceIdcs;
};

// Create PLAYER_STATE_RUN ////////////////////////////////////////////////////
int PlayerCreate(Player *pPlayer, HINSTANCE hModule, const wchar_t *path)
{
  int nLen;
  wchar_t aszModuleName[MAX_PATH] = {0};

  DPRINTF(0,"  > %s <\n",__func__);

  /////////////////////////////////////////////////////////////////////////////
  memset(pPlayer,0,sizeof(*pPlayer));

  /////////////////////////////////////////////////////////////////////////////
  pPlayer->options.path=path;
  OptionsCommonLoad(&pPlayer->options.common,L"    ",path);
  DPUTS(0,"  common options loaded\n");

  OptionsDeviceLoad(&pPlayer->options.device,L"    ",
      pPlayer->options.common.szId,path);
  DPUTS(0,"  device options loaded\n");

  /////////////////////////////////////////////////////////////////////////////
  pPlayer->state=PLAYER_STATE_NULL;
  pPlayer->stamp=0;
#if defined (YASAPI_FORCE24BIT) // {
  pPlayer->base.nVolume=YASAPI_MAX_VOLUME;
#else // } {
  pPlayer->base.qVolume=1.0;
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  nLen=GetModuleFileName(
    hModule,                  // _In_opt_ HMODULE hModule,
    aszModuleName,            // _Out_    LPTSTR  lpFilename,
    ARRAYSIZE(aszModuleName)  // _In_     DWORD   nSize
  );

  if (ERROR_INSUFFICIENT_BUFFER==GetLastError()) {
    DMESSAGEV("getting module name (%s)",YASAPI_MODULE);
    goto module;
  }

  //pPlayer->base.pszFileName=safe_wcsdup(basenamew(aszModuleName));
  DPRINTF(0,"  got module name: \"%s\"\n",pPlayer->base.pszFileName);
  //pPlayer->base.hModule=hModule;

  /////////////////////////////////////////////////////////////////////////////
  pPlayer->base.hEvent=CreateEvent(
    0,        // _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
    FALSE,    // _In_     BOOL                  bManualReset,
    FALSE,    // _In_     BOOL                  bInitialState,
    NULL      // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pPlayer->base.hEvent) {
    DMESSAGE("creating event");
    goto event;
  }
  
  DPUTS(0,"  event created\n");

  // create the timer /////////////////////////////////////////////////////////
  pPlayer->base.timer.hRead=CreateWaitableTimer(
    NULL,       // _In_opt_  LPSECURITY_ATTRIBUTES lpTimerAttributes,
    FALSE,      // _In_      BOOL bManualReset,
    NULL        // _In_opt_  LPCTSTR lpTimerName
  );

  if (NULL==pPlayer->base.timer.hRead) {
    DMESSAGE("creating timer for triggering read");
    goto timer_read;
  }

  DPUTS(0,"  waitable timer for triggering read created\n");

#if defined (YASAPI_CHECK_UNDERFLOW) // {
  // create the timer /////////////////////////////////////////////////////////
  pPlayer->base.timer.hUnderflow=CreateWaitableTimer(
    NULL,       // _In_opt_  LPSECURITY_ATTRIBUTES lpTimerAttributes,
    FALSE,      // _In_      BOOL bManualReset,
    NULL        // _In_opt_  LPCTSTR lpTimerName
  );

  if (NULL==pPlayer->base.timer.hUnderflow) {
    DMESSAGE("creating timer for triggering check for underflow");
    goto timer_underflow;
  }

  DPUTS(0,"  waitable timer for triggering check underflow created\n");
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  if (NULL==(pPlayer->base.pStub=PlayerStubCreate(&gcIPlayer,pPlayer))) {
    DMESSAGE("creating player stub");
    goto stub;
  }

  /////////////////////////////////////////////////////////////////////////////
  // PLAYER_STATE_BASE
  ++pPlayer->state;

  // PLAYER_STATE_RUN
  if (PLAYER_STUB_SEND(pPlayer->base.pStub,0,pPlayer->stamp,PlayerRun)<0) {
    DMESSAGE("running player");
    goto run;
  }

  return 0;
run:
  PlayerStubDestroy(pPlayer->base.pStub);
stub:
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  CloseHandle(pPlayer->base.timer.hUnderflow);
timer_underflow:
#endif // }
  CloseHandle(pPlayer->base.timer.hRead);
  pPlayer->base.timer.hRead=NULL;
timer_read:
  CloseHandle(pPlayer->base.hEvent);
  pPlayer->base.hEvent=NULL;
event:
module:
  pPlayer->state=PLAYER_STATE_NULL;
  return 0;
}

void PlayerDestroy(Player *pPlayer)
{
  if (PLAYER_STATE_BASE<pPlayer->state)
	// cppcheck-suppress syntaxError
    PLAYER_SEND(pPlayer,PlayerKillV);

  if (PLAYER_STATE_NULL<pPlayer->state)
    PlayerKill(pPlayer,0,PLAYER_STATE_NULL);
}

///////////////////////////////////////////////////////////////////////////////
int PlayerRun(Player *pPlayer, Request *pRequest)
{
  IMMDeviceEnumerator *pEnumerator = 0;
  HRESULT hr;

  DPRINTF(0,"  > %s <\n",__func__);

  /////////////////////////////////////////////////////////////////////////////
  if (PLAYER_STATE_BASE!=pPlayer->state) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    goto state;
  }

  /////////////////////////////////////////////////////////////////////////////
  memset(&pPlayer->run,0,sizeof pPlayer->run);

  // create a device enumerator ///////////////////////////////////////////////
  hr=CoCreateInstance(
    &CLSID_MMDeviceEnumerator,  // _In_   REFCLSID rclsid,
    NULL,                       // _In_   LPUNKNOWN pUnkOuter,
    CLSCTX_ALL,                 // _In_   DWORD dwClsContext,
    &IID_IMMDeviceEnumerator,   // _In_   REFIID riid,
    (void**)&pEnumerator        // _Out_  LPVOID *ppv
  );

  if (FAILED(hr)) {
    DERROR(REGDB_E_CLASSNOTREG,hr,enumerator);
    DERROR(CLASS_E_NOAGGREGATION,hr,enumerator);
    DERROR(E_NOINTERFACE,hr,enumerator);
    DERROR(E_POINTER,hr,enumerator);
    DMESSAGE("creating device enumerator");
    goto enumerator;
  }

  pPlayer->run.pEnumerator=pEnumerator;
  DPUTS(0,"  device enumerator created\n");

#if defined (YASAPI_NOTIFY) // {
  /////////////////////////////////////////////////////////////////////////////
  if (PlayerAddNotify(pPlayer)<0)
    goto notify;
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  ++pPlayer->state;

  return 0;
#if defined (YASAPI_NOTIFY) // {
  PlayerRemoveNotify(pPlayer);
notify:
#endif // }
  pEnumerator->lpVtbl->Release(pEnumerator);
enumerator:
state:
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
int PlayerCreateConnect(Player *pPlayer, int bNegociate, int bReset)
{
#if defined (YASAPI_EXECUTION_STATE) // {
  enum {
    ES_FLAGS=ES_CONTINUOUS|ES_SYSTEM_REQUIRED|ES_AWAYMODE_REQUIRED
  };
#endif // }

  double qShareSize=pPlayer->options.device.qShareSize;
  WAVEFORMATEXTENSIBLE *pwfxx=&pPlayer->open.wfxx;
  const WAVEFORMATEX *pwfx=&pwfxx->Format;
  IMMDevice *pDevice=pPlayer->device.pDevice;
  const Strategy *pStrategy=pPlayer->open.pStrategy;
  Connection *pConnect=&pPlayer->connect;
  AUDCLNT_SHAREMODE eShareMode = AUDCLNT_SHAREMODE_SHARED;
  REFERENCE_TIME hnsDefaultDevicePeriod;
  REFERENCE_TIME hnsMinimumDevicePeriod;
  REFERENCE_TIME hnsDevicePeriod=0;
  DWORD dwStreamFlags;
  int retry=0;
  WAVEFORMATEX *pClosestMatch;
  IAudioClient *pClient=NULL;
  IAudioRenderClient *pRender;
  IAudioClock *pClock;
  HRESULT hr;

  DPRINTF(0,"  > %s (negociate: %d, reset: %d) <\n",__func__,
      bNegociate,bReset);

  if (bNegociate) {
    // setup share mode ///////////////////////////////////////////////////////
    switch (pPlayer->options.device.eShareMode) {
    case YASAPI_SHAREMODE_EXCLUSIVE:
      DPUTS(0,"  YASAPI_SHAREMODE_EXCLUSIVE\n");
      eShareMode=AUDCLNT_SHAREMODE_EXCLUSIVE;
      break;
    case YASAPI_SHAREMODE_AUTOMATIC:
      DPUTS(0,"  YASAPI_SHAREMODE_AUTOMATIC\n");
      eShareMode=AUDCLNT_SHAREMODE_EXCLUSIVE;
      break;
    default:
      DPUTS(0,"  AUDCLNT_SHAREMODE_SHARED\n");
      eShareMode=AUDCLNT_SHAREMODE_SHARED;
      break;
    }
  }

#if defined (YASAPI_EXECUTION_STATE) // {
  if (0==(pConnect->eEsPrevFlags=SetThreadExecutionState(ES_FLAGS))) {
    DMESSAGE("setting execution state");
    goto execution;
  }

  DPUTS(0,"  execution state set\n");
#endif // }

  /////////////////////////////////////////////////////////////////////////////
retry:
  if (bNegociate) {
    // remember the share mode ////////////////////////////////////////////////
    pPlayer->open.eShareMode=eShareMode;
  }
  else
    eShareMode=pPlayer->open.eShareMode;

  // activate the audio client ////////////////////////////////////////////////
  hr=(pDevice ? pDevice->lpVtbl->Activate(pDevice,
    &IID_IAudioClient,    // [in]   REFIID iid,
    CLSCTX_ALL,           // [in]   DWORD dwClsCtx,
    NULL,                 // [in]   PROPVARIANT *pActivationParams,
    (void **)&pClient     // [out]  void **ppInterface
  ) : E_POINTER);

  if (FAILED(hr)) {
    DERROR(E_NOINTERFACE,hr,client);
    DERROR(E_POINTER,hr,client);
    DERROR(E_INVALIDARG,hr,client);
    DERROR(E_OUTOFMEMORY,hr,client);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,client);
    DUNKNOWN(hr);
    DMESSAGE("activating audio client");
    goto client;
  }

  pConnect->pClient=pClient;
  DPUTS(0,"  audio client created\n");

  // format supported? ////////////////////////////////////////////////////////
  if (bNegociate&&pPlayer->options.common.bFormatSupported) {
    pClosestMatch=NULL;

    hr=pClient->lpVtbl->IsFormatSupported(pClient,
      eShareMode,         // [in]        AUDCLNT_SHAREMODE ShareMode,
      &pwfxx->Format,     // [in]  const WAVEFORMATEX      *pFormat,
      AUDCLNT_SHAREMODE_EXCLUSIVE==eShareMode?NULL:&pClosestMatch
                          // [out]       WAVEFORMATEX      **ppClosestMatch
    );

    if (pClosestMatch) {
      DPRINTF(0,"  closest match: %d, %d\n",
          pClosestMatch->wBitsPerSample,
          pClosestMatch->nSamplesPerSec);
      MemFreeCOM(pClosestMatch);
    }

    switch (hr) {
    case S_OK:
    case S_FALSE:
      break;
    /*case S_FALSE:
      //DERROR(S_FALSE,hr);
      DMESSAGE("format not supported");
      goto support;*/
    default:
      if (FAILED(hr)) {
        DERROR(AUDCLNT_E_UNSUPPORTED_FORMAT,hr,support);
        DERROR(E_POINTER,hr,support);
        DERROR(E_INVALIDARG,hr,support);
        DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,support);
        DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,support);
        DUNKNOWN(hr);
        DMESSAGE("format not supported");
        goto support;
      }

      break;
    }

    DPUTS(0,"  format supported\n");
  }

  // setup the stream flags ///////////////////////////////////////////////////
  dwStreamFlags=0;

  if (AUDCLNT_SHAREMODE_SHARED==eShareMode
        &&pPlayer->options.device.bAutoConvertPCM) {
	  // cppcheck-suppress ConfigurationNotChecked
      dwStreamFlags|=AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;

     if (pPlayer->options.device.bSRCDefaultQuality) {
	   // cppcheck-suppress ConfigurationNotChecked
       dwStreamFlags|=AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
  }
  }

  pPlayer->open.dwStreamFlags=dwStreamFlags;

  // if needed, get the minimum device period (low latency) ///////////////////
  // we may have it already because of not aligned buffer error, cf. below.
  if (!bNegociate)
    hnsDevicePeriod=pPlayer->open.hnsDevicePeriod;
  else if (0==hnsDevicePeriod&&pStrategy->NeedDevicePeriod(pPlayer)) {
    hr=pClient->lpVtbl->GetDevicePeriod(pClient,
      &hnsDefaultDevicePeriod,
                          // [out]  REFERENCE_TIME *phnsDefaultDevicePeriod,
      &hnsMinimumDevicePeriod
                          // [out]  REFERENCE_TIME *phnsMinimumDevicePeriod
    );

    if (FAILED(hr)) {
      DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,period);
      DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,period);
      DERROR(E_POINTER,hr,period);
      DUNKNOWN(hr);
      DMESSAGE("getting device period");
      goto period;
    }

    switch (pPlayer->options.device.eDevicePeriod) {
    case YASAPI_DEVICE_PERIOD_DEFAULT:
      hnsDevicePeriod=hnsDefaultDevicePeriod;
      break;
    case YASAPI_DEVICE_PERIOD_MINIMUM:
      hnsDevicePeriod=hnsMinimumDevicePeriod;
      break;
    default:
      hnsDevicePeriod=hnsMinimumDevicePeriod;
      break;
    }

    DPUTS(0,"  got device period\n");
    DPRINTF(0,"    default: %I64d hns\n",hnsDefaultDevicePeriod);
    DPRINTF(0,"    minimum: %I64d hns\n",hnsMinimumDevicePeriod);
    DPRINTF(0,"    choosen: %I64d hns\n",hnsDevicePeriod);

    if (1.0<qShareSize) {
      hnsDevicePeriod=(REFERENCE_TIME)(qShareSize*hnsDevicePeriod+0.5);
      DPRINTF(0,"  device period scaled by %.02f"
          " (device period: %I64d)\n",
          qShareSize,hnsDevicePeriod);
    }
  }
    pPlayer->open.hnsDevicePeriod=hnsDevicePeriod;

  // initialize the audio client //////////////////////////////////////////////
  DPUTS(0,"  going to initialize\n");
  DWPRINTF(0,L"    \"%s\"\n",pPlayer->device.szId);
  hr=pPlayer->open.pStrategy->Initialize(pPlayer);

  if (FAILED(hr)) {
    if (bNegociate&&AUDCLNT_SHAREMODE_EXCLUSIVE==eShareMode) {
      switch (hr) {
      case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
        if (0<retry)
          break;

        DPUTS(0,"  failed with AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED\n");
        DPUTS(0,"  trying to get an aligned buffer size\n");
        retry=1;

        // 1. Call IAudioClient::GetBufferSize and receive the
        //    next-highest-aligned buffer size (in frames).
        hr=pClient->lpVtbl->GetBufferSize(pClient,
          &pPlayer->connect.uFrames // [out] UINT32 *pNumBufferFrames
        );

        if (FAILED(hr)) {
          DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,init);
          DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,init);
          DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,init);
          DERROR(E_POINTER,hr,init);
          DUNKNOWN(hr);
          DMESSAGE("getting aligned number of frames");
          goto init;
        }

        DPRINTF(0,"  got aligned number of frames: %u\n",
            pPlayer->connect.uFrames);

        // 3. Calculate the aligned buffer size in 100-nansecond units (hns).
        //    The buffer size is (REFERENCE_TIME)((10000.0 * 1000 /
        //    WAVEFORMATEX.nSamplesPerSecond * nFrames) + 0.5). In this
        //    formula, nFrames is the buffer size retrieved by GetBufferSize.
        hnsDevicePeriod=(REFERENCE_TIME)(YASAPI_FRAMES_TIMER(pPlayer->connect.uFrames,pwfx)+0.5);

#if 0 // {
        // save hnsDevicePeriod into device options.
        OptionsSetDevicePeriod(pPlayer->options.hnsDevicePeriod);
#endif // }

        // 2. Call IAudioClient::Release to release the audio
        //    client used in the previous call that returned
        //    AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED.
        DPUTS(0,"  destroying audio client\n");
        pClient->lpVtbl->Release(pClient);

        // 4. Call the IMMDevice::Activate method with parameter iid set
        //    to REFIID IID_IAudioClient to create a new audio client.
        // 5. Call Initialize again on the created audio client and
        //    specify the new buffer size and periodicity.
        DPUTS(0,"  retrying with aligned buffer\n");
        goto retry;
      default:
        if (YASAPI_SHAREMODE_AUTOMATIC!=pPlayer->options.device.eShareMode)
          break;

        DPUTS(0,"  initializing audio client failed \n");
        DPUTS(0,"  destroying audio client\n");
        pClient->lpVtbl->Release(pClient);

        DPUTS(0,"  retrying in shared mode\n");
        eShareMode=AUDCLNT_SHAREMODE_SHARED;
        goto retry;
      }
    }

    DERROR(AUDCLNT_E_ALREADY_INITIALIZED,hr,init);
    DERROR(AUDCLNT_E_WRONG_ENDPOINT_TYPE,hr,init);
    DERROR(AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED,hr,init);
    DERROR(AUDCLNT_E_BUFFER_SIZE_ERROR,hr,init);
    DERROR(AUDCLNT_E_CPUUSAGE_EXCEEDED,hr,init);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,init);
    DERROR(AUDCLNT_E_DEVICE_IN_USE,hr,init);
    DERROR(AUDCLNT_E_ENDPOINT_CREATE_FAILED,hr,init);
    DERROR(AUDCLNT_E_INVALID_DEVICE_PERIOD,hr,init);
    DERROR(AUDCLNT_E_UNSUPPORTED_FORMAT,hr,init);
    DERROR(AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED,hr,init);
    DERROR(AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL,hr,init);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,init);
    DERROR(E_POINTER,hr,init);
    DERROR(E_INVALIDARG,hr,init);
    DERROR(E_OUTOFMEMORY,hr,init);
    DUNKNOWN(hr);
    DMESSAGE("initializing audio client");
    goto init;
  }

  DPUTS(0,"  audio client initialized\n");

  // set the event handle /////////////////////////////////////////////////////
  hr=pStrategy->SetEvent(pPlayer);

  if (FAILED(hr)) {
    DERROR(E_INVALIDARG,hr,event);
    DERROR(AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED,hr,event);
    DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,event);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,event);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,event);
    DUNKNOWN(hr);
    DMESSAGE("setting event handle");
    goto event;
  }

  // get the buffer size //////////////////////////////////////////////////////
  //
  // This method retrieves the length of the endpoint buffer shared
  // between the client application and the audio engine. The length
  // is expressed as the number of audio frames the buffer can hold.
  // The size in bytes of an audio frame is calculated as the number
  // of channels in the stream multiplied by the sample size per channel.
  // For example, the frame size is four bytes for a stereo (2-channel)
  // stream with 16-bit samples.
  hr=pClient->lpVtbl->GetBufferSize(pClient,
    &pPlayer->connect.uFrames // [out]  UINT32 *pNumBufferFrames
  );

  if (FAILED(hr)) {
    DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,size);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,size);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,size);
    DERROR(E_POINTER,hr,size);
    DUNKNOWN(hr);
    DMESSAGE("getting buffer size");
    goto size;
  }

  DPRINTF(0,"  SHARED BUFFER SIZE: %d frames\n",pPlayer->connect.uFrames);
#if 0 // [
  pPlayer->connect.time=YASAPI_FRAMES_TIMER(pPlayer->connect.uFrames,pwfx);
#else // ] [
  pPlayer->connect.time=MulDiv(5000000,pPlayer->connect.uFrames,
        pwfx->nSamplesPerSec);
#endif // ]

  // get the render client ////////////////////////////////////////////////////
  hr=pClient->lpVtbl->GetService(pClient,
    &IID_IAudioRenderClient,  // [in]   REFIID riid,
    (void**)&pRender          // [out]  void **ppv
  );

  if (FAILED(hr)) {
    DERROR(E_POINTER,hr,render);
    DERROR(E_NOINTERFACE,hr,render);
    DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,render);
    DERROR(AUDCLNT_E_WRONG_ENDPOINT_TYPE,hr,render);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,render);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,render);
    DUNKNOWN(hr);
    DMESSAGE("getting audio render client");
    goto render;
  }

  pConnect->pRender=pRender;
  DPUTS(0,"  audio render client created\n");

  // get the audio clock //////////////////////////////////////////////////////
  hr=pClient->lpVtbl->GetService(pClient,
    &IID_IAudioClock,         // [in]   REFIID riid,
    (void**)&pClock           // [out]  void **ppv
  );

  if (FAILED(hr)) {
    DERROR(E_POINTER,hr,clock);
    DERROR(E_NOINTERFACE,hr,clock);
    DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,clock);
    DERROR(AUDCLNT_E_WRONG_ENDPOINT_TYPE,hr,clock);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,clock);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,clock);
    DUNKNOWN(hr);
    DMESSAGE("getting audio clock");
    goto clock;
  }

  pConnect->pClock=pClock;
  DPUTS(0,"  audio clock created\n");

  if (bReset) {
    // reset the client ///////////////////////////////////////////////////////
    hr=pClient->lpVtbl->Reset(pClient);

    if (FAILED(hr)) {
      DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,reset);
      DERROR(AUDCLNT_E_NOT_STOPPED,hr,reset);
      DERROR(AUDCLNT_E_BUFFER_OPERATION_PENDING,hr,reset);
      DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,reset);
      DUNKNOWN(hr);
      DMESSAGE("re-setting audio client");
      goto reset;
    }

    DPUTS(0,"  audio client reset\n");
  }

  /////////////////////////////////////////////////////////////////////////////
#if defined (YASAPI_READ_COUNT) // {
  pPlayer->base.timer.nRead=0;
#endif // }
  pPlayer->state=PLAYER_STATE_CONNECTED;

  return 0;
reset:
  pClock->lpVtbl->Release(pClock);
  pConnect->pClock=NULL;
clock:
  pRender->lpVtbl->Release(pRender);
  pConnect->pRender=NULL;
render:
size:
event:
init:
period:
support:
  pClient->lpVtbl->Release(pClient);
  pConnect->pClient=NULL;
client:
#if defined (YASAPI_EXECUTION_STATE) // {
  SetThreadExecutionState(pConnect->eEsPrevFlags);
execution:
#endif // }
  return -1;
}

void PlayerCreateWFXX(Player *pPlayer, Request *pRequest)
{
  extern int srate,numchan,bps;
  WAVEFORMATEXTENSIBLE *pwfxx=&pPlayer->open.wfxx;
  const WAVEFORMATEX *pwfx=&pwfxx->Format;
  int bMono2Stereo=pPlayer->options.common.bMono2Stereo;
#if defined (YASAPI_FORCE24BIT) // {
  int bForce24Bit=pPlayer->options.device.bForce24Bit;
  Convert *pSource=&pPlayer->open.source;
  Convert *pTarget=&pPlayer->open.target;
#endif // }
#if defined (YASAPI_SURROUND) // {
  int bSurround;
#endif // }

  DPRINTF(0,"  > %s (%d, %d, %d) <\n",
      __func__,srate,numchan,bps);

  /////////////////////////////////////////////////////////////////////////////
  DPRINTF(0,"  samplerate: %d\n",samplerate);

#if defined (YASAPI_FORCE24BIT) // {
  if (bForce24Bit) {
    switch (bps) {
    case 8:
      pSource->nBytesPerSample=1;
      pSource->nChannels=numchan;
      pTarget->nBytesPerSample=3;
      pTarget->nChannels=bMono2Stereo&&numchan<2?2:numchan;
      break;
    case 16:
      pSource->nBytesPerSample=2;
      pSource->nChannels=numchan;
      pPlayer->open.target.nBytesPerSample=3;
      pTarget->nChannels=bMono2Stereo&&numchan<2?2:numchan;
      break;
    default:
      pSource->nBytesPerSample=bps>>3;
      pSource->nChannels=numchan;
      pTarget->nBytesPerSample=bps>>3;
      pTarget->nChannels=bMono2Stereo&&numchan<2?2:numchan;
      break;
    }
  }
  else {
    pSource->nBytesPerSample=bps>>3;
    pSource->nChannels=numchan;
    pTarget->nBytesPerSample=bps>>3;
    pTarget->nChannels=bMono2Stereo&&numchan<2?2:numchan;
  }

  pSource->nBytesPerFrame=pSource->nBytesPerSample*pSource->nChannels;
  pTarget->nBytesPerFrame=pTarget->nBytesPerSample*pTarget->nChannels;

  if (pSource->nChannels<pTarget->nChannels)
    DPRINTF(0,"  numchannels: %d -> %d\n",numchannels,
        pTarget->nChannels);
  else
    DPRINTF(0,"  numchannels: %d\n",numchannels);

  if (pSource->nBytesPerSample<pTarget->nBytesPerSample)
    DPRINTF(0,"  bitspersamp: %d -> %d\n",bitspersamp,
        pTarget->nBytesPerSample<<3);
  else
    DPRINTF(0,"  bitspersamp: %d\n",bitspersamp);

#else // } {
  if (bMono2Stereo&&numchannels<2) {
    DPUTS(0,"  numchannels: 1 -> 2\n");
    pPlayer->open.nShift=1;
    numchannels=2;
  }
  else {
    DPRINTF(0,"  numchannels: %d\n",numchannels);
    pPlayer->open.nShift=0;
  }

  DPRINTF(0,"  bitspersamp: %d\n",bitspersamp);
#endif // }

#if defined (YASAPI_SURROUND) // {
  bSurround=pPlayer->open.bSurround=pPlayer->options.common.bSurround;
  WFXXSetup(pwfxx,srate,pTarget->nChannels,(pTarget->nBytesPerSample<<3),bSurround,FALSE);
#else // } {
  WFXXSetup(pwfxx,samplerate,pTarget->nChannels,(pTarget->nBytesPerSample<<3),FALSE,FALSE);
#endif // }
  pPlayer->open.wBytesPerSample=pwfx->wBitsPerSample>>3;
}

int PlayerCreateRing(Player *pPlayer)
{
  Connection *pConnect=&pPlayer->connect;
  const WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;
  Ring *pRing=&pPlayer->open.ring;
  double qRingFill=pPlayer->options.device.ring.qFill;
  double qRingSize=pPlayer->options.device.ring.qSize;
#if defined (YASAPI_FORCE24BIT) // {
  const Convert *pSource=&pPlayer->open.source;
  const Convert *pTarget=&pPlayer->open.target;
  int nNumerator;
  int nDenominator;
  int nGcd;
#endif // }
  UINT32 uBufSize;

  DPRINTF(0,"  > %s <\n",__func__);

  // setup the ring buffer ////////////////////////////////////////////////////
  pConnect->uFramesMin=(UINT32)(qRingFill*pConnect->uFrames+0.5);
  DPRINTF(0,"  START AUDIO CLIENT: %d frames\n",pConnect->uFramesMin);

  uBufSize=(UINT32)(qRingSize*pConnect->uFrames+0.5);
  // "nBlockAlign" reflects "bMono2Stereo", i.e. is in target units.
  uBufSize*=pwfx->nBlockAlign;

#if defined (YASAPI_FORCE24BIT) // {
  nNumerator=pSource->nBytesPerFrame;
  nDenominator=pTarget->nBytesPerFrame;

  if (1<(nGcd=gcd(nNumerator,nDenominator))) {
    nNumerator/=nGcd;
    nDenominator/=nGcd;
  }

  DPRINTF(0,"  numerator=%d, denominator=%d\n",nNumerator,nDenominator);

  if (RingCreate(pRing,uBufSize,nNumerator,nDenominator)<0) {
    DMESSAGE("creating ring buffer\n");
    goto ring;
  }
#else // } {
  if (RingCreate(pRing,uBufSize,Player->open.nShift)<0) {
    DMESSAGE("creating ring buffer\n");
    goto ring;
  }
#endif // }

  pRing->Copy=PlayerCopyMemory;
  pRing->pClient=pPlayer;
#if defined (YASAPI_RING_REALLOC) // {
  pPlayer->open.dwBufSize=RingGetSize(pRing);
#endif // }
  DPRINTF(0,"  ring buffer created (%d bytes)\n",RingGetSize(pRing));

  return 0;
//cleanup:
  //RingDestroy(&pPlayer->open.ring);
ring:
  return -1;
}

#if defined (YASAPI_NOTIFY) // {
int PlayerReallocRing(Player *pPlayer)
{
  const WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;
  double qRingFill=pPlayer->options.device.ring.qFill;
  double qRingSize=pPlayer->options.device.ring.qSize;
  UINT32 uBufSize;

  DPRINTF(0,"  > %s <\n",__func__);

  // setup the ring buffer ////////////////////////////////////////////////////
  pPlayer->connect.uFramesMin=(UINT32)(qRingFill*pPlayer->connect.uFrames+0.5);
  DPRINTF(0,"  START AUDIO CLIENT: %d frames\n",
      pPlayer->connect.uFramesMin);

  uBufSize=(UINT32)(qRingSize*pPlayer->connect.uFrames+0.5);
  // "nBlockAlign" reflects "bMono2Stereo".
  uBufSize*=pwfx->nBlockAlign;

  if (RingRealloc(&pPlayer->open.ring,uBufSize)<0) {
    DMESSAGE("re-allocationg ring buffer\n");
    goto ring;
  }

  pPlayer->open.dwBufSize=uBufSize;
  DPRINTF(0,"  ring buffer re-allocated (%d bytes)\n",uBufSize);

  return 0;
//cleanup:
  //RingDestroy(&pPlayer->open.ring);
ring:
  return -1;
}
#endif // }

int PlayerGetMaxLatency(const Player *pPlayer)
{
  const Connection *pConnect=&pPlayer->connect;
  const WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;
  return MulDiv(1000,pConnect->uFramesMin,pwfx->nSamplesPerSec);
}

int PlayerOpen(Player *pPlayer, Request *pRequest)
{
#if 0 // {
  enum {
    ES_FLAGS=ES_CONTINUOUS|ES_SYSTEM_REQUIRED|ES_AWAYMODE_REQUIRED
  };
#endif // }

  const PlayerState state=pPlayer->state;
  const wchar_t *path=pPlayer->options.path;

  DPRINTF(0,"  > %s <\n",__func__);

  /////////////////////////////////////////////////////////////////////////////
  if (pPlayer->state<PLAYER_STATE_RUN) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    goto state;
  }

  /////////////////////////////////////////////////////////////////////////////
  memset(&pPlayer->open,0,sizeof pPlayer->open);
  memset(&pPlayer->connect,0,sizeof pPlayer->connect);
  memset(&pPlayer->time,0,sizeof pPlayer->time);

  /////////////////////////////////////////////////////////////////////////////
  OptionsDeviceLoad(&pPlayer->options.device,
      L"    ",pPlayer->device.szId,path);
  DPUTS(0,"  device options loaded\n");

  /////////////////////////////////////////////////////////////////////////////
  pPlayer->open.pStrategy
      =pPlayer->options.device.bPull?&gcStrategyPull:&gcStrategyPush;
  DPRINTF(0,"  %s\n",pPlayer->options.common.bDisconnect
      ?"DISCONNECT ENABLED":"DISCONNECT DISABLED");
  pPlayer->open.pDisconnect
      =pPlayer->options.common.bDisconnect?&gcDisconnectYes:&gcDisconnectNo;

  /////////////////////////////////////////////////////////////////////////////
  if (!*&pPlayer->device.pDevice
      &&PlayerDeviceGet(&pPlayer->device,pPlayer->run.pEnumerator)<0) {
    DMESSAGE("getting the device");
    goto device;
  }
  DPUTS(0,"  got the device\n");

#if 0 // {
  /////////////////////////////////////////////////////////////////////////////
  OptionsDeviceLoad(&pPlayer->options.device,L"    ",
      pPlayerDevice->szId,path);
  DPUTS(0,"  device options loaded\n");
#endif // }

  // Create WAVEFORMATEXTENSIBLE //////////////////////////////////////////////
  PlayerCreateWFXX(pPlayer,pRequest);

/*
#if defined (YASAPI_EXECUTION_STATE) // {
  if (0==(pPlayer->open.ePrevState=SetThreadExecutionState(ES_FLAGS))) {
    DMESSAGE("setting execution state");
    goto execution;
  }

  DPUTS(0,"  execution state set\n");
#endif // }
*/

  // Connect //////////////////////////////////////////////////////////////////
  // PLAYER_STATE_CONNECTED
  // TODO improve the error reporting for this
  if (PlayerCreateConnect(pPlayer,1,1)<0) {
    DMESSAGE("connecting ");
    goto connect;
  }

  // Create ring //////////////////////////////////////////////////////////////
  if (PlayerCreateRing(pPlayer)<0) {
    DMESSAGE("creating ring");
    goto ring;
  }

  if (TimeReset(&pPlayer->time,pPlayer->options.common.eTimeTag,
      &pPlayer->connect)<0) {
    DMESSAGE("re-setting time offset");
    goto time;
  }
  return PlayerGetMaxLatency(pPlayer);
time:
  RingDestroy(&pPlayer->open.ring);
ring:
  pPlayer->open.pDisconnect->Destroy(pPlayer,0);
connect:
#if 0 // {
#if defined (YASAPI_EXECUTION_STATE) // {
  SetThreadExecutionState(pPlayer->open.ePrevState);
execution:
#endif // }
#endif // }
  PlayerDeviceDestroy(&pPlayer->device);
device:
state:
  pPlayer->state=state;
  return -1;
}

#if defined (YASAPI_NOTIFY) // {
int PlayerMigrate(Player *pPlayer, Request *pRequest)
{
  LPCWSTR pwstrId=va_arg(pRequest->ap,LPCWSTR);
  const wchar_t *path=pPlayer->options.path;
  Connection *pConnect=&pPlayer->connect;
  PlayerDevice device={0}, *pPlayerDevice=&device;

  DPRINTF(0,"  > %s <\n",__func__);

  ////////////////////////////////////////////////////////////////////////////
  if (pPlayer->state<PLAYER_STATE_PAUSE) {
    DWARNINGV("illegal state: %d",pPlayer->state);
    goto state;
  }

  ////////////////////////////////////////////////////////////////////////////
  if (pwstrId!=NULL && pPlayerDevice->szId && 0==wcscmp(pwstrId,pPlayerDevice->szId)) {
    DWARNING("device not changed");
    goto device1;
  }

  if (PlayerDeviceCreate(pPlayerDevice,pwstrId,pPlayer->run.pEnumerator)<0) {
    DWARNING("getting the default device");
    goto device2;
  }

  if (PlayerDeviceGet(pPlayerDevice,pPlayer->run.pEnumerator)<0) {
    DWARNING("getting the device");
    goto device3;
  }

  DPUTS(0,"  got the device\n");

  /////////////////////////////////////////////////////////////////////////////
  ConnectionSetInvalid(pConnect,1);
  PlayerKill(pPlayer,0,PLAYER_STATE_PREPARED);
  ConnectionSetInvalid(pConnect,0);
  
  pPlayer->device=device;
  memset(pPlayerDevice,0,sizeof *pPlayerDevice);
  pPlayerDevice=&pPlayer->device;

  /////////////////////////////////////////////////////////////////////////////
  OptionsDeviceLoad(&pPlayer->options.device,
      L"    ",
      pPlayer->device.szId,
      path);
  DPUTS(0,"  device options loaded\n");

  // Connect //////////////////////////////////////////////////////////////////
  // PLAYER_STATE_CONNECTED
  if (PlayerCreateConnect(pPlayer,1,1)<0) {
    DMESSAGE("connecting ");
    goto connect;
  }

  // Realloc ring /////////////////////////////////////////////////////////////
  if (PlayerReallocRing(pPlayer)<0) {
    DMESSAGE("re-allocating ring");
    goto ring;
  }

  /////////////////////////////////////////////////////////////////////////////
#if defined (YASAPI_CONVERT_TIME) // {
  if (TimeMigrate(&pPlayer->time,pPlayer->options.common.eTimeTag,
      &pPlayer->connect)<0) {
    DMESSAGE("getting time");
    goto time;
  }
#else // } {
  if (TimeMigrate(&pPlayer->time,&pPlayer->connect)<0) {
    DMESSAGE("getting time");
    goto time;
  }
#endif // }

  PlayerTryStart(pPlayer,0);

  return PlayerGetMaxLatency(pPlayer);
time:
  RingDestroy(&pPlayer->open.ring);
ring:
connect:
device3:
  PlayerDeviceDestroy(pPlayerDevice);
device2:
device1:
  pPlayer->state=PLAYER_STATE_RUN;
state:
  return -1;
}
#endif // }

#if defined (YASAPI_GAPLESS) // {
int PlayerReset(Player *pPlayer, Request *pRequest)
{
  const Disconnect *pDisconnect=pPlayer->open.pDisconnect;
  Connection *pConnect=&pPlayer->connect;
#if 0 // {
  IAudioClient *pClient=pConnect->pClient;
  WAVEFORMATEXTENSIBLE *pwfxx=&pPlayer->open.wfxx;
  WAVEFORMATEX *pFormat=&pwfxx->Format;
  UINT32 u32FramesPadding;
  UINT32 u32FramesWritten;
  UINT32 u32FramesWaiting;
#endif // }

  DPRINTF(0,"  > %s <\n",__func__);
  DPUTS(0,"  re-using previous device\n");

  if (pPlayer->state<PLAYER_STATE_PAUSE) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    goto state;
  }

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

#if 0 // {
  if (!pConnect->pClient) {
    DMESSAGE("null pointer");
    goto null;
  }
#endif // }

  if (pDisconnect->Reconnect(pPlayer)<0) {
    // TODO consider this doing a timed
    //      messagebox instead of this!
    DMESSAGE("connecting");
    goto connect;
  }

  if (TimeGapless(&pPlayer->time,pConnect)<0) {
    DMESSAGE("getting time");
    goto reset;
  }

#if 0 // {
  if ((u32FramesPadding=PlayerGetFramesPadding(pPlayer))<0) {
    DMESSAGE("getting frames padding");
    goto padding;
  }

  // get the number of frames written to the ring buffer
  u32FramesWritten=pPlayer->open.ring.dwWritten/pFormat->nBlockAlign;
  // get the number of frames padding and written to the ring buffer
  u32FramesWaiting=u32FramesPadding+u32FramesWritten;

  if (TimeAddFrames(pOffset1,u32FramesWaiting,pFormat,pConnect)<0) {
    DMESSAGE("adding frames");
    goto add;
  }
#endif // }

  return PlayerGetMaxLatency(pPlayer);
//add:
//padding:
reset:
connect:
#if 0 // {
null:
#endif // }
invalid:
state:
  return -1;
}
#endif // }

int PlayerClose(Player *pPlayer, Request *pRequest)
{
  DPRINTF(0,"  > %s <\n",__func__);

  if (PLAYER_STATE_RUN<pPlayer->state)
    PlayerKill(pPlayer,0,PLAYER_STATE_RUN);

  return 0;
}

int PlayerCanWrite(Player *pPlayer, Request *pRequest)
{
  enum { DEBUG=3 };
  Ring *pRing=&pPlayer->open.ring;
  DWORD dwAvailable;

  DPRINTF(DEBUG,"  > %s (%d) <\n",__func__,pPlayer->state);

  if (pPlayer->state<PLAYER_STATE_PAUSE) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    dwAvailable=0;
  }
  else if (pPlayer->state<PLAYER_STATE_UNDERFLOW) {
    DPUTS(DEBUG,"  pause\n");
    dwAvailable=0;
  }
  else {
#if defined (YASAPI_RING_REALLOC) // {
    DPRINTF(DEBUG,"  buf size: %d, written: %d\n",
        pPlayer->open.dwBufSize,pPlayer->open.ring.dwWritten);

    if (pPlayer->open.dwBufSize<pPlayer->open.ring.dwWritten)
      dwAvailable=0;
    else
      dwAvailable=(DWORD)(pPlayer->open.dwBufSize-pPlayer->open.ring.dwWritten);
#else // } {
    dwAvailable=pPlayer->open.ring.dwAvailable;
#endif // }
    dwAvailable=RingSourceSize(pRing,dwAvailable);
  }

  return dwAvailable;
}

int PlayerGetWriteSize(Player *pPlayer, UINT32 uFramesPadding,
    int bUnderflow, int nDebug, PlayConfig *pc)
{
  int bNeedPadding=pPlayer->open.pStrategy->NeedPadding(pPlayer);
  const WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;

  DPRINTF(nDebug,"  > %s (%d) <\n",__func__,pPlayer->state);

  pc->uFramesWrite=pPlayer->connect.uFrames;

  if (!bNeedPadding)
  	pc->uFramesWrite-=uFramesPadding;

  pc->dwSize=pc->uFramesWrite*pwfx->nBlockAlign;
  pc->dwWrite=(DWORD)pPlayer->open.ring.dwWritten;

  if (pc->dwWrite<pc->dwSize) {
    if (bUnderflow&&pPlayer->state<PLAYER_STATE_DRAIN_1) {
      if (bNeedPadding||0==(pc->dwWriteSize=pc->dwWrite))
        goto underflow;
    }

    pc->dwWriteSize=pc->dwWrite;

    if (!bNeedPadding)
      pc->uFramesWrite=pc->dwWriteSize/pwfx->nBlockAlign;
  }
  else
    pc->dwWriteSize=pc->dwSize;

  return 0;
underflow:
  return -1;
}

static void RingErrorReleaseBuffer(RingIOError *pError)
{
  Player *pPlayer=pError->pPlayer;
  IAudioRenderClient *pRender=pPlayer->connect.pRender;

  pRender->lpVtbl->ReleaseBuffer(pRender,
    pError->uFramesWrite, // [in]  UINT32 NumFramesWritten,
    AUDCLNT_BUFFERFLAGS_SILENT
                          // [in]  DWORD dwFlags
  );

  PlayerKill(pPlayer,1,PLAYER_STATE_RUN);
}

static void RingErrorNop(RingIOError *pError)
{
}

// http://qrqcwnet.ning.com/profiles/blogs/virtual-audio-cable-by-vb-audio?id=1993813%3ABlogPost%3A18709&page=2
int PlayerPlay(Player *pPlayer, UINT32 uFramesPadding, int bUnderflow)
{
  enum { DEBUG=3 };
  const WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;
  IAudioRenderClient *pRender=pPlayer->connect.pRender;
  Ring *pRing=&pPlayer->open.ring;
  Connection *pConnect=&pPlayer->connect;
  int bNeedPadding=pPlayer->open.pStrategy->NeedPadding(pPlayer);
  BYTE *pData=NULL;
  DWORD dwFlags=0;
  PlayConfig c;
  DWORD dwFramesRead;
  WORD wRing,wShared;
  RingIOError error;
  HRESULT hr;

  DPRINTF(DEBUG,"  > %s (state: %d, undeflow: %d) <\n",__func__,
      pPlayer->state,bUnderflow);

  /////////////////////////////////////////////////////////////////////////////
  if (pPlayer->state<PLAYER_STATE_CONNECTED) {
    DWARNINGV("unexpected state %d in %s",&pPlayer->state,__func__);
    PlayerKill(pPlayer,1,PLAYER_STATE_RUN);
    goto state;
  }

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid1;
  }

  if (PlayerGetWriteSize(pPlayer,uFramesPadding,bUnderflow,DEBUG,&c)<0) {
    DWARNINGV("underflow in %s",__func__);
    PlayerKill(pPlayer,1,PLAYER_STATE_UNDERFLOW);
    goto underflow;
  }

  if (PlayerGetDlgConfig(pPlayer)) {
    wRing=(WORD)MulDiv(USHRT_MAX,(int)pRing->dwWritten,(int)(pRing->pMax-pRing->pRep));
    wShared=(WORD)MulDiv(USHRT_MAX,uFramesPadding,pConnect->uFrames);
    PlayerPostUpdate(pPlayer,wRing,wShared);
  }

  if (0==c.uFramesWrite)
    goto nop;

  if (c.uFramesWrite*pwfx->nBlockAlign!=c.dwWriteSize) {
    DMESSAGE("uFrameWrite does not match dwWriteSize");
    goto match;
  }

  // get the render buffer ////////////////////////////////////////////////////
  hr=pRender->lpVtbl->GetBuffer(pRender,
    c.uFramesWrite,     // [in]   UINT32 NumFramesRequested,
    &pData              // [out]  BYTE **ppData
  );

  if (FAILED(hr)) {
    if (AUDCLNT_E_DEVICE_INVALIDATED==hr&&ConnectionSetInvalid(pConnect,1)) {
      DWARNINGV("invalid device on getting buffer in %s",__func__);
      goto invalid2;
    }

    DERROR(AUDCLNT_E_BUFFER_ERROR,hr,get);
    DERROR(AUDCLNT_E_BUFFER_TOO_LARGE,hr,get);	// https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/fe2da61d-1be5-43c9-b4d6-30f97615a298/win-7-core-audio-api-buffer-allocation-failed?forum=windowspro-audiodevelopment
    DERROR(AUDCLNT_E_BUFFER_SIZE_ERROR,hr,get);
    DERROR(AUDCLNT_E_OUT_OF_ORDER,hr,get);
    DERROR(AUDCLNT_E_OUT_OF_ORDER,hr,get);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,get);
    DERROR(AUDCLNT_E_BUFFER_OPERATION_PENDING,hr,get);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,get);
    DERROR(E_POINTER,hr,get);
    DUNKNOWN(hr);
    DMESSAGE("getting render buffer");
    goto get;
  }

  // copy the data ////////////////////////////////////////////////////////////
  error.Cleanup=RingErrorReleaseBuffer;
  error.pPlayer=pPlayer;
  error.uFramesWrite=c.uFramesWrite;

  dwFramesRead=RingReadEx(pRing,(LPSTR)pData,c.dwWriteSize,RING_COPY,&error);

  if (dwFramesRead<0) {
    DMESSAGEV("copying data from ring buffer to device buffer");
    goto copy;
  }
  else if (0<dwFramesRead)
    dwFramesRead/=pwfx->nBlockAlign;

  if (c.dwWriteSize<c.dwSize&&bNeedPadding)
    memset(pData+c.dwWriteSize,0,c.dwSize-c.dwWriteSize);

  // release the render buffer ////////////////////////////////////////////////
  hr=pRender->lpVtbl->ReleaseBuffer(pRender,
    0<dwFramesRead?dwFramesRead:0,
                        // [in]  UINT32 NumFramesWritten,
    dwFlags             // [in]  DWORD dwFlags
  );

  if (FAILED(hr)) {
    if (AUDCLNT_E_DEVICE_INVALIDATED==hr&&ConnectionSetInvalid(pConnect,1)) {
      DWARNINGV("invalid device on releasing buffer in %s",__func__);
      goto invalid3;
    }

    DERROR(AUDCLNT_E_INVALID_SIZE,hr,release);
    DERROR(AUDCLNT_E_BUFFER_SIZE_ERROR,hr,release);
    DERROR(AUDCLNT_E_OUT_OF_ORDER,hr,release);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,release);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,release);
    DERROR(E_INVALIDARG,hr,release);
    DUNKNOWN(hr);
    DMESSAGE("releasing render buffer");
    goto release;
  }

#if 0 // {
  if (dwFramesRead<0) {
    DMESSAGE("reading ring buffer");
    goto read;
  }
#endif // }

  // no critical section needed in order to block unexpected write
  // requests to the ring buffer because all requests are serialized.
  memset(&error,0,sizeof error);
  error.Cleanup=RingErrorNop;
  RingReadEx(pRing,(LPSTR)pData,c.dwWriteSize,RING_COMMIT,&error);
nop:
  if (PLAYER_STATE_DRAIN_1==pPlayer->state&&0==pPlayer->open.ring.dwWritten)
    ++pPlayer->state;

  pPlayer->open.pStrategy->SetTimer(pPlayer,pConnect->time,FALSE);
underflow:
state:
  return 0;
#if 0 // {
read:
#endif // }
release:
invalid3:
copy:
get:
invalid2:
match:
invalid1:
  return -1;
}

int PlayerTryStart(Player *pPlayer, int bUnderflow)
{
  const PlayerState state=pPlayer->state;
  const Strategy *pStrategy=pPlayer->open.pStrategy;
  const Disconnect *pDisconnect=pPlayer->open.pDisconnect;
  const WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;
  const Ring *pRing=&pPlayer->open.ring;
  UINT32 uFramesRingWritten=(UINT32)pRing->dwWritten/pwfx->nBlockAlign;
  HANDLE hTask=NULL;
  IAudioClient *pClient = NULL;
  UINT32 uFramesPadding;
  HRESULT hr=AUDCLNT_E_NOT_INITIALIZED;

  PlayerSetThreadCharacteristics(pPlayer,&hTask);

  DPRINTF(0,"  > %s (state: %d, underflow: %d) <\n",__func__,
      pPlayer->state,bUnderflow);

  if (pPlayer->state<PLAYER_STATE_UNDERFLOW) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    goto state;
  }

  if (pPlayer->state<PLAYER_STATE_PLAY) {
#if defined (YA_DEBUG) // {
    if (PLAYER_STATE_UNDERFLOW==state)
      DPUTS(0,"  ============================================\n");
#endif // }

    if (pDisconnect->Reconnect(pPlayer)<0) {
      // TODO consider this doing a timed
      //      messagebox instead of this!
      DMESSAGE("connecting");
      goto connect;
    }

    pClient=pPlayer->connect.pClient;

    if ((uFramesPadding=pStrategy->GetFramesPadding(pPlayer))<0) {
      DMESSAGE("getting frames padding");
      goto padding;
    }

    if (uFramesRingWritten+uFramesPadding<pPlayer->connect.uFramesMin) {
      DWARNINGV("underflow in %s",__func__);
      goto underflow;
    }

    // https://www.gittprogram.com/question/399078_wasapi-exclusive-event-mode-causes-buzzing.html
    ///////////////////////////////////////////////////////////////////////////
    pPlayer->state=PLAYER_STATE_PLAY;

    ///////////////////////////////////////////////////////////////////////////
    if (PlayerPlay(pPlayer,uFramesPadding,bUnderflow)<0) {
      DMESSAGEV("playing");
      goto play;
    }

    // start the audio client /////////////////////////////////////////////////
	// changed - if we've an underflow state then
	// we cannot access pClient as it's gone away
	// TODO need to improve the checking in general
	if (pPlayer->state == PLAYER_STATE_UNDERFLOW) {
		goto underflow;
	} else {
		hr=pClient->lpVtbl->Start(pClient);
	}

    if (FAILED(hr)) {
      DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,start);
      DERROR(AUDCLNT_E_NOT_STOPPED,hr,start);
      DERROR(AUDCLNT_E_EVENTHANDLE_NOT_SET,hr,start);
      DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,start);
      DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,start);
      DUNKNOWN(hr);
      DMESSAGE("starting audio client");
      goto start;
    }

    DPRINTF(0,"  %s: audio client started\n",__func__);

#if 0 // {
    ///////////////////////////////////////////////////////////////////////////
    pPlayer->state=PLAYER_STATE_PLAY;

    ///////////////////////////////////////////////////////////////////////////
    if (PlayerPlay(pPlayer,uFramesPadding,bUnderflow)<0) {
      DMESSAGEV("playing");
      goto play;
    }
#endif // }

    DPRINTF(0,"  %s: start playing: %d\n",__func__,pPlayer->state);
  }
underflow:
//frames:
  PlayerRevertThreadCharacteristics(pPlayer,&hTask);
  return 0;
#if 0 // {
play:
#endif // }
start:
play:
connect:
padding:
  pPlayer->state=state;
state:
  PlayerRevertThreadCharacteristics(pPlayer,&hTask);
  return -1;
}

int PlayerWrite(Player *pPlayer, Request *pRequest)
{
  enum { DEBUG=3 };
  const char *buf=va_arg(pRequest->ap,const char *);
  int len=va_arg(pRequest->ap,int);
  Connection *pConnect=&pPlayer->connect;
  Ring *pRing=&pPlayer->open.ring;
#if defined (YASAPI_FORCE24BIT) // {
  const Convert *pSource=&pPlayer->open.source;
#endif // }

  DPRINTF(DEBUG,"  > %s (%d) <\n",__func__,pPlayer->state);

#if defined (YASAPI_FORCE24BIT) // {
  if (len && pSource->nBytesPerFrame && 0!=(len%pSource->nBytesPerFrame)) {
    //DMESSAGE("source data not frame aligned");
    goto align;
  }
#endif // }

  if (pPlayer->state<PLAYER_STATE_PAUSE) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    goto state;
  }

  if (PLAYER_STATE_PAUSE==pPlayer->state)
    goto pause;

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

#if defined (YASAPI_RING_REALLOC) // {
  if (RingReallocAvailable(pRing,len)<0) {
    DMESSAGE("reallocating");
    goto realloc;
  }
#endif // }

  if (RingWrite(pRing,buf,len)<0) {
    DMESSAGE("writing");
    goto write;
  }

  if (pPlayer->state<PLAYER_STATE_PLAY)
    PlayerTryStart(pPlayer,1);
pause:
  return 0;
write:
#if defined (YASAPI_RING_REALLOC) // {
realloc:
#endif // }
invalid:
state:
#if defined (YASAPI_FORCE24BIT) // {
align:
#endif // }
  return -1;
}

int PlayerFlush(Player *pPlayer, Request *pRequest)
{
  double ms=0.0;

  DPRINTF(0,"  > %s (%d) <\n",__func__,pPlayer->state);

  if (pPlayer->state<PLAYER_STATE_PAUSE) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    goto state;
  }

  if (ConnectionIsInvalid(&pPlayer->connect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

#if 0 // {
  // we may be in PLAYER_STATE_PAUSE
  if (!pPlayer->connect.pClient&&pDisconnect->Reconnect(pPlayer)<0) {
    DMESSAGE("reconnecting");
    goto reconnect;
  }
#endif // }

  RingReset(&pPlayer->open.ring);
  
  if (PLAYER_STATE_UNDERFLOW<pPlayer->state) {
    if (TimeGetMS(&pPlayer->time,&pPlayer->connect,&ms)<0) {
      DMESSAGE("getting time\n");
      goto get;
    }

    PlayerKill(pPlayer,1,PLAYER_STATE_CONNECTED);
    return (int)(ms + 0.5);
  }
  else
    return 0;
get:
state:
//reconnect:
invalid:
  return 0;
}

int PlayerIsPlaying(Player *pPlayer, Request *pRequest)
{
  enum { DEBUG=3 };

  DPRINTF(DEBUG,"  > %s (%d) <\n",__func__,pPlayer->state);

  if (pPlayer->state<PLAYER_STATE_PAUSE) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    return 0;
  }
  else if (pPlayer->open.ring.dwWritten) {
    pPlayer->state=PLAYER_STATE_DRAIN_1;
    DPUTS(0,"  isplaying\n");
    return 1;
  }
  else {
    switch (pPlayer->state) {
    case PLAYER_STATE_PAUSE:
    case PLAYER_STATE_UNDERFLOW:
    case PLAYER_STATE_CONNECTED:
      // Don't increase state without re-connecting!
#if 0 // {
      pPlayer->state=PLAYER_STATE_EOT;
#endif // }
      DPUTS(0,"  isplaying (eot)\n");
      return 0;
    case PLAYER_STATE_PLAY:
      pPlayer->state=PLAYER_STATE_DRAIN_2;
      DPUTS(0,"  isplaying\n");
      return 1;
    case PLAYER_STATE_DRAIN_1:
    case PLAYER_STATE_DRAIN_2:
    case PLAYER_STATE_DRAIN_3:
      DPRINTF(0,"  isplaying (drain %d)\n",
          1+pPlayer->state-PLAYER_STATE_DRAIN_1);
      return 1;
    case PLAYER_STATE_EOT:
    default:
      DPUTS(0,"  isplaying (eot)\n");
      return 0;
    }
  }
}

int PlayerPause(Player *pPlayer, Request *pRequest)
{
  int bPause=PLAYER_STATE_PAUSE==pPlayer->state;
  double ms=0.0;

  DPRINTF(0,"  > %s (%d) <\n",__func__,pPlayer->state);

  if (pPlayer->state<PLAYER_STATE_PAUSE) {
    DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);
    goto state;
  }

  if (ConnectionIsInvalid(&pPlayer->connect)) {
    DWARNINGV("invalid connection in %s",__func__);
  }
  else if (bPause) {
    ++pPlayer->state;

    if (PlayerTryStart(pPlayer,0)<0) {
      DMESSAGE("starting");
      goto start;
    }

    if (TimePause(&pPlayer->time,&pPlayer->connect)<0) {
      DMESSAGE("getting time");
      goto time1;
    }

    if (TimeGetMS(&pPlayer->time,&pPlayer->connect,&ms)<0) {
      DMESSAGE("getting time");
      goto get1;
    }
  }
  else {
    if (!pPlayer->connect.pClient) {
      DMESSAGE("null pointer");
      goto null;
    }

    if (TimeGetMS(&pPlayer->time,&pPlayer->connect,&ms)<0) {
      DMESSAGE("getting time");
      goto get2;
    }

    if (TimePause(&pPlayer->time,&pPlayer->connect)<0) {
      DMESSAGE("getting time");
      goto time2;
    }

    PlayerKill(pPlayer,0,PLAYER_STATE_PAUSE);
#if 0 // {
    pPlayer->state=PLAYER_STATE_PAUSE;
#endif // }
  }

  return (int)(ms + 0.5);
time2:
get2:
get1:
null:
time1:
start:
state:
  return -1;
}

int PlayerGetTime(Player *pPlayer, Request *pRequest)
{
  enum { DEBUG=3 };
  double ms=0.0;

  DPRINTF(DEBUG,"  > %s (%d) <\n",__func__,pPlayer->state);

  if (pPlayer->state<PLAYER_STATE_CONNECTED) {
    if (DON(DEBUG))
      DWARNINGV("unexpected state %d in %s",pPlayer->state,__func__);

    goto state;
  }

  if (ConnectionIsInvalid(&pPlayer->connect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pPlayer->connect.pClient) {
    DMESSAGE("null pointer");
    goto null;
  }

  if (TimeGetMS(&pPlayer->time,&pPlayer->connect,&ms)<0) {
    DMESSAGE("getting time");
    goto time;
  }
null:
invalid:
  return (int)(ms + 0.5);
time:
state:
  return -1;
}

///////////////////////////////////////////////////////////////////////////////
void PlayerSetThreadCharacteristics(Player *pPlayer, HANDLE *pTask)
{
  DWORD dwTaskIndex=0;
  
  if (AUDCLNT_SHAREMODE_EXCLUSIVE==pPlayer->open.eShareMode)
    *pTask=AvSetMmThreadCharacteristicsW(L"Pro Audio",&dwTaskIndex);
  else
    *pTask=NULL;
}

void PlayerRevertThreadCharacteristics(Player *pPlayer, HANDLE *pTask)
{
  if (*pTask) {
    AvRevertMmThreadCharacteristics(*pTask);
    *pTask=NULL;
  }
}

int PlayerRead(Player *pPlayer, Request *pRequest)
{
  enum { DEBUG=3 };
  UINT32 uFramesPadding;
  HANDLE hTask=NULL;

  DPRINTF(DEBUG,"  > %s (%d) <\n",__func__,pPlayer->state);

  if (pPlayer->state<PLAYER_STATE_CONNECTED) {
    DPRINTF(0,"Warning: unexpected state: %d (%s, \"%s\", %d)\n",
        pPlayer->state,__func__,__FILE__,__LINE__);
    goto state;
  }
  else if (ConnectionIsInvalid(&pPlayer->connect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }
  else if (!pPlayer->connect.pClient) {
    DMESSAGE("null pointer");
    goto null;
  }
  else if (PLAYER_STATE_DRAIN_2==pPlayer->state) {
    pPlayer->state=PLAYER_STATE_DRAIN_3;
    pPlayer->open.pStrategy->SetTimer(pPlayer,pPlayer->connect.time,TRUE);
  }
  else if (PLAYER_STATE_DRAIN_3==pPlayer->state) {
    pPlayer->state=PLAYER_STATE_EOT;
    goto eof;
  }
  else if (pPlayer->state<PLAYER_STATE_UNDERFLOW)
    goto pause;
  else {
    PlayerSetThreadCharacteristics(pPlayer,&hTask);

    if ((uFramesPadding=PlayerGetFramesPadding(pPlayer))<0) {
      DMESSAGE("getting frames padding");
      PlayerRevertThreadCharacteristics(pPlayer,&hTask);
      goto padding;
    }

    if (PlayerPlay(pPlayer,uFramesPadding,1)<0) {
      DMESSAGE("playing");
      PlayerRevertThreadCharacteristics(pPlayer,&hTask);
      goto play;
    }

    PlayerRevertThreadCharacteristics(pPlayer,&hTask);
  }
pause:
eof:
  return 0;
play:
padding:
null:
invalid:
state:
  return -1;
}

#if defined (YASAPI_CHECK_UNDERFLOW) // {
int PlayerShutdownUnderflow(Player *pPlayer, Request *pRequest)
{
  if (PLAYER_STATE_UNDERFLOW==pPlayer->state)
    PlayerClose(pPlayer,pRequest);

  return 0;
}
#endif // }

///////////////////////////////////////////////////////////////////////////////
UINT32 PlayerGetFramesPadding(Player *pPlayer)
{
  Connection *pConnect=&pPlayer->connect;
  IAudioClient *pClient;
  UINT32 uFramesPadding;
  HRESULT hr;

  if (ConnectionIsInvalid(&pPlayer->connect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid1;
  }

  if (NULL==(pClient=pPlayer->connect.pClient)) {
    DMESSAGE("null pointer");
    goto null;
  }

  hr=pClient->lpVtbl->GetCurrentPadding(pClient,
    &uFramesPadding       // [out]  UINT32 *pNumPaddingFrames
  );

  if (FAILED(hr)) {
    if (AUDCLNT_E_DEVICE_INVALIDATED==hr&&ConnectionSetInvalid(pConnect,1)) {
      DWARNINGV("invalid device on getting padding in %s",__func__);
      goto invalid2;
    }

    DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,padding);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr,padding);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,padding);
    DERROR(E_POINTER,hr,padding);
    DUNKNOWN(hr);
    DMESSAGE("getting padding");
    goto padding;
  }

  return uFramesPadding;
padding:
invalid2:
null:
invalid1:
  return (UINT32)-1;
}

///////////////////////////////////////////////////////////////////////////////
HWND PlayerGetDlgConfig(Player *pPlayer)
{
  return pPlayer->options.common.bVisualization?pPlayer->hDlgConfig:NULL;
}

void PlayerPostUpdate(Player *pPlayer, WORD wRing, WORD wShared)
{
  HWND hDlgConfig=PlayerGetDlgConfig(pPlayer);

  if (hDlgConfig)
    PostMessage(hDlgConfig,WM_CONFIG_UPDATE,0,MAKELONG(wRing,wShared));
}

///////////////////////////////////////////////////////////////////////////////
#if defined (YA_DEBUG) // {
int PlayerSend(Player *pPlayer, const char *id, PlayerProc *pPlayerProc, ...)
#else // } {
int PlayerSend(Player *pPlayer, PlayerProc *pPlayerProc, ...)
#endif // }
{
  if (!pPlayer->base.pStub)
    return -1;

  if (!pPlayer->base.pStub->hThread) {
    if (!PlayerStubCreate(&gcIPlayer, pPlayer)) {
      return -1;
    }
  }

  va_list ap;
  int state;

  va_start(ap,pPlayerProc);
#if defined (YA_DEBUG) // {
  state=PlayerStubSendV(pPlayer->base.pStub,0,pPlayer->stamp,
      id,pPlayerProc,ap);
#else // } {
  state=PlayerStubSendV(pPlayer->base.pStub,0,pPlayer->stamp,pPlayerProc,ap);
#endif // }
  va_end(ap);

  return state;
}

#if defined (YA_DEBUG) // {
void PlayerPost(Player *pPlayer, const char *id, PlayerProc *pPlayerProc)
{
  PlayerStubPost(pPlayer->base.pStub,0,pPlayer->stamp,id,pPlayerProc);
}
#else // } {
void PlayerPost(Player *pPlayer, PlayerProc *pPlayerProc)
{
  PlayerStubPost(pPlayer->base.pStub,0,pPlayer->stamp,pPlayerProc);
}
#endif // }

///////////////////////////////////////////////////////////////////////////////
VOID CALLBACK PlayerFireRead(LPVOID lpArg, DWORD dwLow, DWORD dwHigh)
{
#if defined (YASAPI_READ_COUNT) // {
  --((Player *)lpArg)->base.timer.nRead;
#endif // }
  PLAYER_POST(lpArg, PlayerRead);
}

void PlayerSetTimerRead(Player *pPlayer, LONGLONG time)
{
  HANDLE hTimer=pPlayer->base.timer.hRead;
  LARGE_INTEGER ui;

#if defined (YASAPI_READ_COUNT)
  if (0==pPlayer->base.timer.nRead) {
    ++pPlayer->base.timer.nRead;
#endif

  // http://msdn.microsoft.com/en-us/library/windows/desktop/ms687012%28v=vs.85%29.aspx
  // Positive values indicate absolute time. 
  // Negative values indicate relative time.
  // in 100 nanosecond intervals
  ui.QuadPart=-time;

  SetWaitableTimer(
    hTimer,           // _In_      HANDLE hTimer,
    &ui,              // _In_      const LARGE_INTEGER *pDueTime,
    0,                // _In_      LONG lPeriod,
    time?PlayerFireRead:NULL,
                      // _In_opt_  PTIMERAPCROUTINE pfnCompletionRoutine,
    time?pPlayer:NULL,
                      // _In_opt_  LPVOID lpArgToCompletionRoutine,
    FALSE             // _In_      BOOL fResume
  );
#if defined (YASAPI_READ_COUNT)
  }
#endif
}

#if defined (YASAPI_CHECK_UNDERFLOW) // {
///////////////////////////////////////////////////////////////////////////////
VOID CALLBACK PlayerFireShutdownUnderflow(LPVOID lpArg, DWORD dwLow,
    DWORD dwHigh)
{
  PlayerPost(lpArg,0,PlayerShutdownUnderflow);
}

void PlayerSetTimerCheckUnderflow(Player *pPlayer)
{
  int ms=pPlayer->options.common.nCheckUnderflow;
  HANDLE hTimer=pPlayer->base.timer.hUnderflow;
  LARGE_INTEGER ui;

  // http://msdn.microsoft.com/en-us/library/windows/desktop/ms687012%28v=vs.85%29.aspx
  // Positive values indicate absolute time. 
  // Negative values indicate relative time.
  // in 100 nanosecond intervals
  ui.QuadPart=-1000*1000*ms/100;

  SetWaitableTimer(
    hTimer,           // _In_      HANDLE hTimer,
    &ui,              // _In_      const LARGE_INTEGER *pDueTime,
    0,                // _In_      LONG lPeriod,
    ms?PlayerFireShutdownUnderflow:NULL,
                      // _In_opt_  PTIMERAPCROUTINE pfnCompletionRoutine,
    ms?pPlayer:NULL,
                      // _In_opt_  LPVOID lpArgToCompletionRoutine,
    FALSE             // _In_      BOOL fResume
  );
}
#endif // }

///////////////////////////////////////////////////////////////////////////////
HANDLE PlayerGetEvent(Player *pPlayer)
{
  return pPlayer->state<PLAYER_STATE_PLAY
      ?NULL:pPlayer->open.pStrategy->GetEvent(pPlayer);
}

int PlayerGetStamp(const Player *pPlayer)
{
  return pPlayer->stamp;
}

///////////////////////////////////////////////////////////////////////////////
#if defined (YASAPI_FORCE24BIT) // {
static int PlayerFormatChanged(Player *pPlayer)
{
  const Convert *pSource=&pPlayer->open.source;
  const Convert *pTarget=&pPlayer->open.target;

  if (pSource->nBytesPerSample-pTarget->nBytesPerSample)
    return 1;

  if (pSource->nChannels-pTarget->nChannels)
    return 1;

  return 0;
}

static void CopySampleDirect(char *wp, int m, const char *rp, int k)
{
  int l=m-k;

  if (0<l) {
    memset(wp,0,l);
    wp+=l;
  }

  // copy source to the temorary location.
  memcpy(
    wp,                   // _In_  PVOID Destination,
    rp,                   // _In_  const VOID *Source,
    k                     // _In_  SIZE_T Length
  );

  //wp+=k;
}

static void CopySampleIndirect(char *wp, int m, const char *rp, int k,
    int nVolume)
{
  int32_t i32=0;
  char *i32wp=((char *)&i32)+(sizeof i32)-k;
  const char *i32rp=((const char *)&i32)+(sizeof i32)-m;

  // copy source to the temorary location.
  memcpy(
    i32wp,                // _In_  PVOID Destination,
    rp,                   // _In_  const VOID *Source,
    k                     // _In_  SIZE_T Length
  );

  i32=MulDiv(i32,nVolume,YASAPI_MAX_VOLUME);

  // copy the temorary location to the target.
  memcpy(
    wp,                   // _In_  PVOID Destination,
    i32rp,                // _In_  const VOID *Source,
    m                     // _In_  SIZE_T Length
  );
}
#endif // }

void PlayerCopyMemory(PVOID p, PVOID Destination, const VOID *Source,
    SIZE_T Length)
{
  enum { DEBUG=3 };
  if (!p) return;
  Player *pPlayer=p;
#if defined (YASAPI_FORCE24BIT) // {
  const Convert *pSource=&pPlayer->open.source;
  const Convert *pTarget=&pPlayer->open.target;
  int nVolume=pPlayer->options.common.bVolume
      ?pPlayer->base.nVolume:YASAPI_MAX_VOLUME;
  int k;
#else // } {
  double qVolume=pPlayer->base.qVolume;
  int n;
  double q;
  int32_t i32;
#endif // }
  char *wp=Destination;
  const char *lwp=wp, *mp=wp+Length, *rp=Source;
  int m;

  DPRINTF(DEBUG," > %s <\n",__func__);

#if defined (YASAPI_FORCE24BIT) // {
  // source bytes per sample.
  k=pSource->nBytesPerSample;
  // target bytes per sample.
  m=pTarget->nBytesPerSample;
#else // } {
  // bits per sample.
  n=pPlayer->open.wfxx.Format.wBitsPerSample;
  // bytes per sample.
  m=n>>3;
  // attenuation factor.
  q=pOptions->common.bVolume?qVolume*YA_INT_MAX(n)/YA_INT_MAX(32):1.0;
  // bits to shift the 32-bit result.
  n=32-n;
#endif // }

#if defined (YASAPI_FORCE24BIT) // {
  if (!PlayerFormatChanged(pPlayer)) {
    if (YASAPI_MAX_VOLUME==nVolume) {
      memcpy(
        Destination,            // _In_  PVOID Destination,
        Source,                 // _In_  const VOID *Source,
        Length                  // _In_  SIZE_T Length
      );
    }
    else {
      while (wp<mp) {
        CopySampleIndirect(wp,m,rp,k,nVolume);
        rp+=k;
        wp+=m;
      }
    }
  }
  else {
    while (wp<mp) {
      int nChannel;
      for (nChannel=0;nChannel<pTarget->nChannels;++nChannel) {
        if (nChannel<pSource->nChannels) {
          // remember the last write pointer (lwp)
          lwp=wp;

          if (YASAPI_MAX_VOLUME==nVolume)
            CopySampleDirect(wp,m,rp,k);
          else
            CopySampleIndirect(wp,m,rp,k,nVolume);

          rp+=k;
          wp+=m;
        }
        else {
          // mono: duplicate the last channel (lwp is last wp)
          memcpy(
            wp,                 // _In_  PVOID Destination,
            lwp,                // _In_  const VOID *Source,
            m                   // _In_  SIZE_T Length
          );

          // increase write pointer
          wp+=m;
        }
      }
    }
  }
#else // } {
  if (!pPlayer->open.nShift) {
    if (!pOptions->common.bVolume||1.0==qVolume) {
      memcpy(
        Destination,            // _In_  PVOID Destination,
        Source,                 // _In_  const VOID *Source,
        Length                  // _In_  SIZE_T Length
      );
    }
    else {
      while (wp<mp) {
        i32=0;

        memcpy(
          &i32,                 // _In_  PVOID Destination,
          rp,                   // _In_  const VOID *Source,
          m                     // _In_  SIZE_T Length
        );

        i32<<=n;
        i32=q*i32+0.5;

        memcpy(
          wp,                   // _In_  PVOID Destination,
          &i32,                 // _In_  const VOID *Source,
          m                     // _In_  SIZE_T Length
        );

        wp+=m;
        rp+=m;
      }
    }
  }
  else {
    if (!pOptions->common.bVolume||1.0==qVolume) {
      while (wp<mp) {
        memcpy(
          wp,                   // _In_  PVOID Destination,
          rp,                   // _In_  const VOID *Source,
          m                     // _In_  SIZE_T Length
        );

        wp+=m;

        memcpy(
          wp,                   // _In_  PVOID Destination,
          rp,                   // _In_  const VOID *Source,
          m                     // _In_  SIZE_T Length
        );

        wp+=m;
        rp+=m;
      }
    }
    else {
      while (wp<mp) {
        i32=0;

        memcpy(
          &i32,                 // _In_  PVOID Destination,
          rp,                   // _In_  const VOID *Source,
          m                     // _In_  SIZE_T Length
        );

        i32<<=n;
        i32=q*i32+0.5;

        memcpy(
          wp,                   // _In_  PVOID Destination,
          &i32,                 // _In_  const VOID *Source,
          m                     // _In_  SIZE_T Length
        );

        wp+=m;

        memcpy(
          wp,                   // _In_  PVOID Destination,
          &i32,                 // _In_  const VOID *Source,
          m                     // _In_  SIZE_T Length
        );

        wp+=m;
        rp+=m;
      }
    }
  }
#endif // }
}
