/*
 * yasapi_strategy.c
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

///////////////////////////////////////////////////////////////////////////////
static HRESULT InitializePush(Player *pPlayer)
{
  AUDCLNT_SHAREMODE eShareMode=pPlayer->open.eShareMode;
  DWORD dwStreamFlags=pPlayer->open.dwStreamFlags;
  REFERENCE_TIME hnsDevicePeriod=pPlayer->open.hnsDevicePeriod;
  IAudioClient *pClient=pPlayer->connect.pClient;
  WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;

  DPRINTF(0,"  intitializing audio client %s/PUSH with %I64d/0 hns\n",
      YASAPI_SHAREMODE_LABEL(eShareMode),hnsDevicePeriod);

  return pClient->lpVtbl->Initialize(pClient,
    eShareMode,           // [in]  AUDCLNT_SHAREMODE ShareMode,
    dwStreamFlags,        // [in]  DWORD StreamFlags,
    hnsDevicePeriod,      // [in]  REFERENCE_TIME hnsBufferDuration,
    0,                    // [in]  REFERENCE_TIME hnsPeriodicity,
    pwfx,                 // [in]  const WAVEFORMATEX *pFormat,
    NULL                  // [in]  LPCGUID AudioPlayerGuid
  );
}

static LONGLONG IntervalPush(Player *pPlayer, LONGLONG time)
{
  return MulDiv((int)time,1,2);
}

static void SetTimerPush(Player *pPlayer, LONGLONG time, BOOL bFlush)
{
#if 0 // [
  PlayerSetTimerRead(pPlayer,IntervalPush(pPlayer,time));
#else // ] [
  PlayerSetTimerRead(pPlayer,time);
#endif // ]
}

static BOOL NeedDevicePeriodPush(Player *pPlayer)
{
  return TRUE;
}

static BOOL NeedPaddingPush(Player *pPlayer)
{
  return FALSE;
}

static HRESULT SetEventPush(Player *pPlayer)
{
  return S_OK;
}

static HANDLE GetEventPush(Player *pPlayer)
{
  return NULL;
}

static UINT32 GetFramesPaddingPush(Player *pPlayer)
{
  return PlayerGetFramesPadding(pPlayer);
}

const Strategy gcStrategyPush={
  InitializePush,
  IntervalPush,
  SetTimerPush,
  NeedDevicePeriodPush,
  NeedPaddingPush,
  SetEventPush,
  GetEventPush,
  GetFramesPaddingPush
};

///////////////////////////////////////////////////////////////////////////////
static HRESULT InitializePull(Player *pPlayer)
{
  AUDCLNT_SHAREMODE eShareMode=pPlayer->open.eShareMode;
  DWORD dwStreamFlags=pPlayer->open.dwStreamFlags;
  REFERENCE_TIME hnsDevicePeriod=pPlayer->open.hnsDevicePeriod;
  IAudioClient *pClient=pPlayer->connect.pClient;
  WAVEFORMATEX *pwfx=&pPlayer->open.wfxx.Format;

  dwStreamFlags|=AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

  switch (eShareMode) {
  case AUDCLNT_SHAREMODE_SHARED:
    // For a shared-mode stream that uses event-driven buffering, the
    // caller must set both hnsPeriodicity and hnsBufferDuration to 0. The
    // Initialize method determines how large a buffer to allocate based on
    // the scheduling period of the audio engine. Although the client's buffer
    // processing thread is event driven, the basic buffer management process,
    // as described previously, is unaltered. Each time the thread awakens,
    // it should call IAudioClient::GetCurrentPadding to determine how much
    // data to write to a rendering buffer or read from a capture buffer. In
    // contrast to the two buffers that the Initialize method allocates for an
    // exclusive-mode stream that uses event-driven buffering, a shared-mode
    // stream requires a single buffer.
    DPUTS(0,"  intitializing audio client SHARED/PULL with 0/0 hns\n");

    return pClient->lpVtbl->Initialize(pClient,
      eShareMode,         // [in]  AUDCLNT_SHAREMODE ShareMode,
      dwStreamFlags,      // [in]  DWORD StreamFlags,
      0,                  // [in]  REFERENCE_TIME hnsBufferDuration,
      0,                  // [in]  REFERENCE_TIME hnsPeriodicity,
      pwfx,               // [in]  const WAVEFORMATEX *pFormat,
      NULL                // [in]  LPCGUID AudioPlayerGuid
    );
  case AUDCLNT_SHAREMODE_EXCLUSIVE:
    // For an exclusive-mode stream that uses event-driven buffering,
    // the caller must specify nonzero values for hnsPeriodicity
    // and hnsBufferDuration, and the values of these two parameters must
    // be equal. The Initialize method allocates two buffers for the stream.
    // Each buffer is equal in duration to the value of the hnsBufferDuration
    // parameter. Following the Initialize call for a rendering stream, the
    // caller should fill the first of the two buffers before starting the
    // stream. For a capture stream, the buffers are initially empty, and
    // the caller should assume that each buffer remains empty until the
    // event for that buffer is signaled. While the stream is running, the
    // system alternately sends one buffer or the other to the client—this
    // form of double buffering is referred to as "ping-ponging". Each time
    // the client receives a buffer from the system (which the system
    // indicates by signaling the event), the client must process the entire
    // buffer. For example, if the client requests a packet size from the
    // IAudioRenderClient::GetBuffer method that does not match the buffer
    // size, the method fails. Calls to the IAudioClient::GetCurrentPadding
    // method are unnecessary because the packet size must always equal the
    // buffer size. In contrast to the buffering modes discussed previously,
    // the latency for an event-driven, exclusive-mode stream depends directly
    // on the buffer size.
    DPRINTF(0,"  intitializing audio client EXCLUSIVE/PULL"
        " with %I64d/%I64d hns\n",hnsDevicePeriod,hnsDevicePeriod);

    return pClient->lpVtbl->Initialize(pClient,
      eShareMode,         // [in]  AUDCLNT_SHAREMODE ShareMode,
      dwStreamFlags,      // [in]  DWORD StreamFlags,
      hnsDevicePeriod,    // [in]  REFERENCE_TIME hnsBufferDuration,
      hnsDevicePeriod,    // [in]  REFERENCE_TIME hnsPeriodicity,
      pwfx,               // [in]  const WAVEFORMATEX *pFormat,
      NULL                // [in]  LPCGUID AudioPlayerGuid
    );
  default:
    return E_FAIL;
  }
}

static LONGLONG IntervalPull(Player *pPlayer, LONGLONG time)
{
    switch (pPlayer->open.eShareMode) {
    case AUDCLNT_SHAREMODE_SHARED:
      return MulDiv((int)time,1,2);
    case AUDCLNT_SHAREMODE_EXCLUSIVE:
      return time;
    default:
      return MulDiv((int)time,1,2);
    }
}

static void SetTimerPull(Player *pPlayer, LONGLONG time, BOOL bFlush)
{
  if (bFlush) {
#if 0 // {
    PlayerSetTimer(pPlayer,IntervalPull(pPlayer,time));
#endif // }
  }
}

static BOOL NeedDevicePeriodPull(Player *pPlayer)
{
  switch (pPlayer->open.eShareMode) {
  case AUDCLNT_SHAREMODE_SHARED:
    return FALSE;
  case AUDCLNT_SHAREMODE_EXCLUSIVE:
    return TRUE;
  default:
    return FALSE;
  }
}

static BOOL NeedPaddingPull(Player *pPlayer)
{
  switch (pPlayer->open.eShareMode) {
  case AUDCLNT_SHAREMODE_SHARED:
    return FALSE;
  case AUDCLNT_SHAREMODE_EXCLUSIVE:
    return TRUE;
  default:
    return FALSE;
  }
}

static HRESULT SetEventPull(Player *pPlayer)
{
  IAudioClient *pClient=pPlayer->connect.pClient;
  HRESULT hr;

  hr=pClient->lpVtbl->SetEventHandle(pClient,
    pPlayer->base.hEvent    // [in] HANDLE eventHandle
  );

  if (!FAILED(hr))
    DPUTS(0,"  event handle set\n");

  return hr;
}

static HANDLE GetEventPull(Player *pPlayer)
{
  return pPlayer->state<PLAYER_STATE_PLAY?NULL:pPlayer->base.hEvent;
}

static UINT32 GetFramesPaddingPull(Player *pPlayer)
{
  switch (pPlayer->open.eShareMode) {
  case AUDCLNT_SHAREMODE_SHARED:
    return PlayerGetFramesPadding(pPlayer);
  case AUDCLNT_SHAREMODE_EXCLUSIVE:
    return 0;
  default:
    return PlayerGetFramesPadding(pPlayer);
  }
}

const Strategy gcStrategyPull={
  InitializePull,
  IntervalPull,
  SetTimerPull,
  NeedDevicePeriodPull,
  NeedPaddingPull,
  SetEventPull,
  GetEventPull,
  GetFramesPaddingPull
};
