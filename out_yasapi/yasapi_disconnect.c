/*
 * yasapi_disconnect.c
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
static void PlayerDestroyConnect(Player *pPlayer)
{
  Connection *pConnect=&pPlayer->connect;
  IAudioClient *pClient;
  IAudioRenderClient *pRender;
  IAudioClock *pClock;

  DPRINTF(0,"  > %s <\n",__func__);

#if 0 // {
  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }
#endif // }

  if (NULL==(pClock=pConnect->pClock)) {
    DMESSAGE("audio clock null pointer");
    goto clock;
  }

  DPUTS(0,"  destroying audio clock\n");
  pClock->lpVtbl->Release(pClock);

  if (NULL==(pRender=pConnect->pRender)) {
    DMESSAGE("render client null pointer");
    goto render;
  }

  DPUTS(0,"  destroying render client\n");
  pRender->lpVtbl->Release(pRender);

  if (NULL==(pClient=pConnect->pClient)) {
    DMESSAGE("audio client null pointer");
    goto client;
  }

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    DWARNING("destroying audio client skipped");
    goto invalid;
  }

  DPUTS(0,"  destroying audio client\n");
  pClient->lpVtbl->Release(pClient);

#if defined (YASAPI_EXECUTION_STATE) // {
  if (!pConnect->eEsPrevFlags) {
    DMESSAGE("null flags");
    goto flags;
  }

  DPUTS(0,"  re-setting execution state\n");
  SetThreadExecutionState(pConnect->eEsPrevFlags);
flags:
#endif // }
invalid:
client:
render:
clock:
#if 0 // {
invalid:
#endif // }
  memset(&pPlayer->connect,0,sizeof pPlayer->connect);
}

static void DisconnectAllDisconnect(Player *pPlayer, int bReset)
{
  IAudioClient *pClient=pPlayer->connect.pClient;

  if (pClient&&bReset&&!ConnectionIsInvalid(&pPlayer->connect)) {
    HRESULT hr;
    TimeFlush(&pPlayer->time);
    hr=pClient->lpVtbl->Reset(pClient);

    if (FAILED(hr)) {
      DERROR(AUDCLNT_E_NOT_INITIALIZED,hr,destroy);
      DERROR(AUDCLNT_E_NOT_STOPPED,hr,destroy);
      DERROR(AUDCLNT_E_BUFFER_OPERATION_PENDING,hr,destroy);
      DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr,destroy);
      DUNKNOWN(hr);
      DPRINTF(0,"  Warning: re-setting audio client failed.\n");
    }
    else
      DPRINTF(0,"  audio client reset\n");
  }

// cppcheck-suppress unusedLabel
destroy:
  PlayerDestroyConnect(pPlayer);
}

///////////////////////////////////////////////////////////////////////////////
static void DisconnectNoDisconnect(Player *pPlayer, int bReset)
{
  // don't disconnect.
}

static int DisconnectNoReconnect(Player *pPlayer)
{
  if (!pPlayer->connect.pClient) {
    DMESSAGE("null pointer");
    goto client;
  }

  // don't connect, just set state accordingly.
  if (pPlayer->state<PLAYER_STATE_CONNECTED)
    pPlayer->state=PLAYER_STATE_CONNECTED;

  return 0;
client:
  return -1;
}

const Disconnect gcDisconnectNo={
  DisconnectNoDisconnect,
  DisconnectNoReconnect,
  DisconnectAllDisconnect,
};

///////////////////////////////////////////////////////////////////////////////
static int DisconnectYesReconnect(Player *pPlayer)
{
  return pPlayer->state<PLAYER_STATE_CONNECTED
      ?PlayerCreateConnect(pPlayer,0,0)
      :0;
}

static int DisconnectYesDestroy(Player *pPlayer, int bReset)
{
  // already disconnected.
  return 0;
}

const Disconnect gcDisconnectYes={
  DisconnectAllDisconnect,
  DisconnectYesReconnect,
  DisconnectYesDestroy,
};
