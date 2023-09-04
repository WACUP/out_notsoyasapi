/*
 * yasapi_kill.c
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

static void PlayerDestroyEventSource(Player *pPlayer)
{
  Connection *pConnect=&pPlayer->connect;

  DPRINTF(0,"  > %s <\n",__func__);

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
#if 0 // {
    goto invalid;
#endif // }
  }

  if (!pConnect->pClient) {
    DMESSAGEV("null pointer in state %d",pPlayer->state);
    goto null;
  }

  DPRINTF(0,"  %s: cancelling timer\n",__func__);
  CancelWaitableTimer(pPlayer->base.timer.hRead);
#if defined (YASAPI_READ_COUNT) // {
  pPlayer->base.timer.nRead=0;
#endif // }

  DPRINTF(0,"  %s: stopping audio client\n",__func__);
  pConnect->pClient->lpVtbl->Stop(pConnect->pClient);

  PlayerPostUpdate(pPlayer,0,0);
null:
#if 0 // {
invalid:
#endif // }
  ;
}

static void PlayerDestroyPrepared(Player *pPlayer)
{
  DPUTS(0,"  destroying ring buffer\n");
  RingDestroy(&pPlayer->open.ring);
}

static void PlayerDestroyPause(Player *pPlayer)
{
  pPlayer->open.pDisconnect->Destroy(pPlayer,0);
#if 0 // {
#if defined (YASAPI_EXECUTION_STATE) // {
  DPUTS(0,"  re-setting execution state\n");
  SetThreadExecutionState(pPlayer->open.ePrevState);
#endif // }
#endif // }
  //PlayerDeviceDestroy(&pPlayer->device);
}

///////////////////////////////////////////////////////////////////////////////
static void PlayerDestroyRun(Player *pPlayer)
{
  IMMDeviceEnumerator *pEnumerator=pPlayer->run.pEnumerator;
  // for some WINE based setups this may not be working
  // so we'll abort instead of continuing & crashing...
  if (pEnumerator) {
#if defined (YASAPI_NOTIFY) // {
  /////////////////////////////////////////////////////////////////////////////
  DPUTS(0,"  removing notify\n");
  PlayerRemoveNotify(pPlayer);
#endif // }

  DPUTS(0,"  destroying device enumerator\n");
  pEnumerator->lpVtbl->Release(pEnumerator);
  }
}

static void PlayerDestroyBase(Player *pPlayer)
{
  DPUTS(0,"  destroying player stub\n");
  PlayerStubDestroy(pPlayer->base.pStub);

#if defined (YASAPI_CHECK_UNDERFLOW) // {
  DPUTS(0,"  destroying timer for underflow\n");
  CloseHandle(pPlayer->base.timer.hUnderflow);
#endif // }

  DPUTS(0,"  destroying timer for read\n");
  CloseHandle(pPlayer->base.timer.hRead);
  pPlayer->base.timer.hRead=NULL;

  DPUTS(0,"  destroying event\n");
  CloseHandle(pPlayer->base.hEvent);
  pPlayer->base.hEvent=NULL;
}

void PlayerKill(Player *pPlayer, int bReset, const PlayerState state)
{
  //HRESULT hr;

  DPRINTF(0,"  > %s (state: %d -> %d, reset: %d) <\n",__func__,
      pPlayer->state,state,bReset);

  ++pPlayer->stamp;

  if (state<pPlayer->state) {
    switch (pPlayer->state) {
    case PLAYER_STATE_EOT:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      // intentional fall through, no break!
    case PLAYER_STATE_DRAIN_3:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      // intentional fall through, no break!
    case PLAYER_STATE_DRAIN_2:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      // intentional fall through, no break!
    case PLAYER_STATE_DRAIN_1:
      if (state==pPlayer->state)
        break;
  
      --pPlayer->state;
      // intentional fall through, no break!
    case PLAYER_STATE_PLAY:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      PlayerDestroyEventSource(pPlayer);
      // intentional fall through, no break!
    case PLAYER_STATE_CONNECTED:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      pPlayer->open.pDisconnect->Disconnect(pPlayer,bReset);
      // intentional fall through, no break!
    case PLAYER_STATE_UNDERFLOW:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      // intentional fall through, no break!
    case PLAYER_STATE_PAUSE:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      PlayerDestroyPause(pPlayer);
      // intentional fall through, no break!
    case PLAYER_STATE_PREPARED:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      PlayerDestroyPrepared(pPlayer);
      // intentional fall through, no break!
    case PLAYER_STATE_RUN:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      // intentional fall through, no break!
      PlayerDestroyRun(pPlayer);
    case PLAYER_STATE_BASE:
      if (state==pPlayer->state)
        break;

      --pPlayer->state;
      PlayerDestroyBase(pPlayer);
      // intentional fall through, no break!
    default:
      break;
    }
  }
}

int PlayerKillV(Player *pPlayer, Request *pRequest)
{
  DPRINTF(0,"  > %s (state: %d -> %d, reset: %d) <\n",__func__,
      pPlayer->state,0,PLAYER_STATE_BASE);
  PlayerKill(pPlayer,0,PLAYER_STATE_BASE);

  return 0;
}
