/*
 * ya_player_stub.c
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
#include <avrt.h>

static PlayerStub stub;

PlayerStub *PlayerStubCreate(const IPlayer *lpVtbl, Player *pPlayer)
{
  PlayerStub *pStub=&stub;

  /////////////////////////////////////////////////////////////////////////////
  if (pStub->lpVtbl) {
    DMESSAGE("vtbl already in use");
    goto vtbl;
  }

  pStub->lpVtbl=lpVtbl;

  if (pStub->pPlayer) {
    DMESSAGE("player already in use");
    goto player;
  }

  pStub->pPlayer=pPlayer;

#if defined (YA_DEBUG) // {
  /////////////////////////////////////////////////////////////////////////////
  TraceFixIdcs(&trace,lpVtbl->pTraceIdcs);
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  if (QueueCreate(&pStub->queue)<0) {
    DMESSAGE("creating queue");
    goto queue;
  }

  DPUTS(0,"  queue created\n");

  /////////////////////////////////////////////////////////////////////////////
  if (StoreCreate(&pStub->store)<0) {
    DMESSAGE("creating store");
    goto store;
  }

  DPUTS(0,"  store created\n");

  // create the thread ////////////////////////////////////////////////////////
  pStub->hThread=CreateThread(
    NULL,               // _In_opt_   LPSECURITY_ATTRIBUTES lpThreadAttributes,
    0,                  // _In_       SIZE_T dwStackSize,
    PlayerStubThread,   // _In_       LPTHREAD_START_ROUTINE lpStartAddress,
    pStub,              // _In_opt_   LPVOID lpParameter,
    0,                  // _In_       DWORD dwCreationFlags,
    NULL                // _Out_opt_  LPDWORD lpThreadId
  );

  if (NULL==pStub->hThread) {
    DMESSAGE("creating thread");
    goto thread;
  }

  DPUTS(0,"  thread created\n");

  /////////////////////////////////////////////////////////////////////////////
#if defined (YA_PLAYER_RUN) // {
  if (PlayerStubSend(pStub,0,0,lpVtbl->RunProc)<0) {
    DMESSAGE("running");
    goto run;
  }
#else // } {
  // cppcheck-suppress syntaxError
  if (PLAYER_STUB_SEND(pStub,0,lpVtbl->GetStamp(pPlayer),PlayerPing)<0) {
    DMESSAGE("running");
    goto run;
  }
#endif // }

  DPUTS(0,"  ping request successfully send\n");

  /////////////////////////////////////////////////////////////////////////////
  return pStub;
run:
  CloseHandle(pStub->hThread);
  pStub->hThread=NULL;
thread:
  StoreDestroy(&pStub->store);
store:
  QueueDestroy(&pStub->queue);
queue:
player:
vtbl:
  return NULL;
}

void PlayerStubDestroy(PlayerStub *pStub)
{
  if (pStub != NULL)
  {
    DPUTS(0,"  sending ping request\n");
#if defined (YA_PLAYER_RUN) // {
    PlayerStubSend(pStub,1,0,pStub->lpVtbl->StopProc);
#else // } {
    PLAYER_STUB_SEND(pStub,1,pStub->lpVtbl->GetStamp(pStub->pPlayer),PlayerPing);
#endif // }
    DPUTS(0,"  waiting for thread to die\n");
    WaitForSingleObject(pStub->hThread,INFINITE);
    DPUTS(0,"  destroying thread\n");
    CloseHandle(pStub->hThread);
	pStub->hThread=NULL;
    DPUTS(0,"  destroying store\n");
    StoreDestroy(&pStub->store);
    DPUTS(0,"  destroying queue\n");
    QueueDestroy(&pStub->queue);
    pStub->pPlayer=NULL;
    pStub->lpVtbl=NULL;
  }
}

const PlayerStub *PlayerStubGet(void)
{
  return &stub;
}

DWORD WINAPI PlayerStubThread(LPVOID lpParameter)
{
  PlayerStub *pStub=lpParameter;
  const IPlayer *lpVtbl=pStub->lpVtbl;
  Player *pPlayer=pStub->pPlayer;
  Queue *pQueue=&pStub->queue;
#if ! defined (YA_EVENT_STACK) // {
  PlayerEventProc *EventProc=lpVtbl->EventProc;
#endif // }
  int exit=0;
#if defined (YA_THREAD_AVRT) // {
  DWORD dwTaskIndex=0;
  HANDLE hTask=AvSetMmThreadCharacteristicsW(L"Pro Audio",&dwTaskIndex);
#endif // }
#if ! defined (YA_EVENT_STACK) // {
  HANDLE hEvent;
#endif // }
  int state;
  HRESULT hr;

  hr=CoInitializeEx(
    NULL,                     // _In_opt_ LPVOID pvReserved
    COINIT_MULTITHREADED      // _In_     DWORD  dwCoInit
  );

  if (FAILED(hr)) {
    /*DERROR(S_FALSE,hr);
    DERROR(RPC_E_CHANGED_MODE,hr);
    DMESSAGE("initializing com failed");*/
    goto coinit;
  }

  while (!exit) {
    Request *pRequest;
#if defined (YA_EVENT_STACK) // {
    pRequest=QueueLockRead(pQueue);
#else // } {
    hEvent=lpVtbl->GetEvent?lpVtbl->GetEvent(pPlayer):NULL;
    pRequest=QueueLockRead(pQueue,hEvent,EventProc,pPlayer);
#endif // }

    if (pRequest->stamp!=lpVtbl->GetStamp(pPlayer)) {
      DWARNINGV("outdated stamp - skipping %s",pRequest->id);
      state=-1;
      goto stamp;
    }

    if (pRequest->pPlayerProc)
      state=pRequest->pPlayerProc(pPlayer,pRequest);
    else
      state=0;

    exit=pRequest->exit;
  stamp:
    if (pRequest->pResult)
      pRequest->pResult->state=state;

    QueueUnlockRead(pQueue,pRequest->pResult);
  }

  CoUninitialize();
coinit:

#if defined (YA_THREAD_AVRT) // {
  if (hTask)
    AvRevertMmThreadCharacteristics(hTask);
#endif // }

  if (pStub->hThread)
  {
	  CloseHandle(pStub->hThread);
	  pStub->hThread=NULL;
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
#if defined (YA_DEBUG) // {
int PlayerStubSendV(PlayerStub *pStub, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, va_list ap)
#else // } {
int PlayerStubSendV(PlayerStub *pStub, int exit, int stamp,
    PlayerProc *pPlayerProc, va_list ap)
#endif // }
{
  if (pStub) {
  Store *pStore=&pStub->store;
  Queue *pQueue=&pStub->queue;
  int state;

  Result *pResult=StoreGet(pStore);
#if defined (YA_DEBUG) // {
  /*pRequest=*/QueueLockWrite(pQueue,pResult,exit,stamp,id,pPlayerProc,ap);
#else // } {
  /*pRequest=*/QueueLockWrite(pQueue,pResult,exit,stamp,pPlayerProc,ap);
#endif // }
  QueueUnlockWrite(pQueue,pResult);
    state=pResult->state;
  StorePut(pStore,pResult);

  return state;
  }
  return 0;
}

#if defined (YA_DEBUG) // {
int PlayerStubSend(PlayerStub *pStub, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, ...)
#else // } {
int PlayerStubSend(PlayerStub *pStub, int exit, int stamp,
    PlayerProc *pPlayerProc, ...)
#endif // }
{
  va_list ap;
  int state;

  va_start(ap,pPlayerProc);
#if defined (YA_DEBUG) // {
  state=PlayerStubSendV(pStub,exit,stamp,id,pPlayerProc,ap);
#else // } {
  state=PlayerStubSendV(pStub,exit,stamp,pPlayerProc,ap);
#endif // }
  va_end(ap);

  return state;
}

#if defined (YA_DEBUG) // {
void PlayerStubPost(PlayerStub *pStub, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc)
#else // } {
void PlayerStubPost(PlayerStub *pStub, int exit, int stamp,
    PlayerProc *pPlayerProc)
#endif // }
{
  Queue *pQueue=&pStub->queue;

#if defined (YA_DEBUG) // {
  QueueLockWrite(pQueue,NULL,exit,stamp,id,pPlayerProc,NULL);
#else // } {
  QueueLockWrite(pQueue,NULL,exit,stamp,pPlayerProc,NULL);
#endif // }
  QueueUnlockWrite(pQueue,NULL);
}

#if defined (YA_DEBUG) // {
void PlayerStubSyncDebug(PlayerStub *pStub, int stamp, HWND hDlg)
{
  TraceEnableDebug(hDlg);
#if defined (YA_DEBUG) // {
  PlayerStubSend(pStub,0,stamp,"PlayerWriteDebug",PlayerWriteDebug);
#else // } {
  PlayerStubSend(pStub,0,stamp,PlayerWriteDebug);
#endif // }
}
#endif // }
