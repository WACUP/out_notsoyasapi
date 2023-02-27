/*
 * ya_queue.c
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

// win32 thread-safe queue implementation using native windows API
// http://stackoverflow.com/questions/11706985/win32-thread-safe-queue-implementation-using-native-windows-api

///////////////////////////////////////////////////////////////////////////////
int QueueCreate(Queue *pQueue)
{
#if defined (YA_EVENT_STACK) // {
  /////////////////////////////////////////////////////////////////////////////
  pQueue->nEvents=0;
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  pQueue->wp=pQueue->rp=pQueue->aRequest;
  pQueue->mp=pQueue->aRequest+QUEUE_SIZE;

  /////////////////////////////////////////////////////////////////////////////
  pQueue->hAvailable=CreateSemaphore(
    0,            // _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    QUEUE_SIZE,   // _In_     LONG                  lInitialCount,
    QUEUE_SIZE,   // _In_     LONG                  lMaximumCount,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pQueue->hAvailable) {
    DMESSAGE("creating available semaphore");
    goto available;
  }
  
  DPUTS(0,"  queue available semaphore created\n");

  /////////////////////////////////////////////////////////////////////////////
  pQueue->hWritten=CreateSemaphore(
    0,            // _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    0,            // _In_     LONG                  lInitialCount,
    QUEUE_SIZE,   // _In_     LONG                  lMaximumCount,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pQueue->hWritten) {
    DMESSAGE("creating written semaphore");
    goto written;
  }
  
  DPUTS(0,"  queue written semaphore created\n");

  /////////////////////////////////////////////////////////////////////////////
  pQueue->hMutex=CreateMutex(
    NULL,         // _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    FALSE,        // _In_     BOOL                  bInitialOwner,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pQueue->hMutex) {
    DMESSAGE("creating mutex");
    goto mutex;
  }
  
  DPUTS(0,"  queue mutex created\n");

#if defined (YA_DEBUG) // {
  /////////////////////////////////////////////////////////////////////////////
  pQueue->nMinAvailable=pQueue->nAvailable=QUEUE_SIZE;
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  return 0;
// cleanup:
  //CloseHandle(pQueue->hMutex);
mutex:
  CloseHandle(pQueue->hWritten);
  pQueue->hWritten=NULL;
written:
  CloseHandle(pQueue->hAvailable);
  pQueue->hAvailable = NULL;
available:
  return -1;
}

void QueueDestroy(Queue *pQueue)
{
  DPUTS(0,"  destroying queue mutex\n");
  CloseHandle(pQueue->hMutex);
  pQueue->hMutex=NULL;
  DPUTS(0,"  destroying queue written semaphore\n");
  CloseHandle(pQueue->hWritten);
  pQueue->hWritten=NULL;
  DPUTS(0,"  destroying queue available semaphore\n");
  CloseHandle(pQueue->hAvailable);
  pQueue->hAvailable=NULL;
}

#if defined (YA_EVENT_STACK) // {
///////////////////////////////////////////////////////////////////////////////
int QueuePushEvent(Queue *pQueue, HANDLE hEvent, PlayerEventProc *pOnEvent,
    void *data)
{
  if (pQueue->nEvents<QUEUE_EVENT_SIZE) {
    pQueue->aHandles[pQueue->nEvents]=hEvent;
    pQueue->aQueueEvents[pQueue->nEvents].pOnEvent=pOnEvent;
    pQueue->aQueueEvents[pQueue->nEvents].data=data;
    ++pQueue->nEvents;

    return 0;
  }
  else
    return -1;
}

void QueuePopEvent(Queue *pQueue)
{
  if (0<pQueue->nEvents)
    --pQueue->nEvents;
}
#endif // }

///////////////////////////////////////////////////////////////////////////////
void QueueLockMutex(Queue *pQueue)
{
  if (pQueue && pQueue->hMutex) {
  WaitForSingleObjectEx(
    pQueue->hMutex,   // _In_ HANDLE hHandle,
    1000/*/INFINITE/**/,  // _In_ DWORD  dwMilliseconds,
    TRUE/*/FALSE/**/      // _In_ BOOL   bAlertable
  );
}
}

void QueueUnlockMutex(Queue *pQueue)
{
  if (pQueue && pQueue->hMutex) {
  ReleaseMutex(pQueue->hMutex);
}
}

///////////////////////////////////////////////////////////////////////////////
#if defined (YA_EVENT_STACK) // {
Request *QueueLock(Queue *pQueue, const QueueStrategy *pStrategy)
{
  BOOL bAlertable=pStrategy->IsAlertable(pQueue);
  HANDLE aHandles[1+QUEUE_EVENT_SIZE]={0};
  //HANDLE *pHandles=aHandles;
  DWORD dwCode=0;

  aHandles[0]=pStrategy->GetSemaphoreDown(pQueue);

  if (0<pQueue->nEvents)
    memcpy(aHandles+1,pQueue->aHandles,pQueue->nEvents*(sizeof *aHandles));
retry:
  if (pQueue)
  dwCode=WaitForMultipleObjectsEx(
    1+pQueue->nEvents,    // _In_       DWORD  nCount,
    aHandles,             // _In_ const HANDLE *lpHandles,
    FALSE,                // _In_       BOOL   bWaitAll,
    INFINITE,             // _In_       DWORD  dwMilliseconds,
    bAlertable            // _In_       BOOL   bAlertable
  );
  else
    return 0;

  switch (dwCode) {
  case WAIT_IO_COMPLETION:
    // alertable event (timer): we need to loop.
    goto retry;
  case WAIT_OBJECT_0+0:
    break;
  case WAIT_OBJECT_0+1:
  case WAIT_OBJECT_0+2:
  case WAIT_OBJECT_0+3:
  case WAIT_OBJECT_0+4:
    QueueEvent *pQueueEvent;
    pQueueEvent=pQueue->aQueueEvents+(dwCode-1)-WAIT_OBJECT_0;
    pQueueEvent->pOnEvent(pQueueEvent->data);
    // event: we need to loop.
    goto retry;
  default:
    break;
  }

  QueueLockMutex(pQueue);
#if defined (YA_DEBUG) // {
  pStrategy->Dec(pQueue);
#endif // }

  Request* pRequest;
  pRequest=*pStrategy->GetRequest(pQueue);
  return pRequest;
}
#else // } {
Request *QueueLock(Queue *pQueue, const QueueStrategy *pStrategy,
    HANDLE hEvent, void (*OnEvent)(void *data), void *data)
{
  BOOL bAlertable=pStrategy->IsAlertable(pQueue);
  DWORD nCount=0;
  HANDLE ahHandles[2];
  DWORD dwCode;
  Request *pRequest;

  ahHandles[nCount++]=pStrategy->GetSemaphoreDown(pQueue);

  if (hEvent)
    ahHandles[nCount++]=hEvent;
retry:
  if (pQueue)
  dwCode=WaitForMultipleObjectsEx(
    nCount,           // _In_       DWORD  nCount,
    ahHandles,        // _In_ const HANDLE *lpHandles,
    FALSE,            // _In_       BOOL   bWaitAll,
    INFINITE,         // _In_       DWORD  dwMilliseconds,
    bAlertable        // _In_       BOOL   bAlertable
  );
  else
    return 0;

  switch (dwCode) {
  case WAIT_IO_COMPLETION:
    // alertable event (timer): we need to loop.
    goto retry;
  case WAIT_OBJECT_0+0:
    break;
  case WAIT_OBJECT_0+1:
    if (OnEvent)
      OnEvent(data);

    // event: we need to loop.
    goto retry;
  default:
    break;
  }

  QueueLockMutex(pQueue);
#if defined (YA_DEBUG) // {
  pStrategy->Dec(pQueue);
#endif // }

  pRequest=*pStrategy->GetRequest(pQueue);
  return pRequest;
}
#endif // }

void QueueUnlock(Queue *pQueue, const QueueStrategy *pStrategy,
    Result *pResult)
{
  Request **ppRequest;

  ppRequest=pStrategy->GetRequest(pQueue);

  if (ppRequest)
  if (++*ppRequest==pQueue->mp)
    *ppRequest=pQueue->aRequest;

  if (pResult)
    SetEvent(pResult->hEvent);

#if defined (YA_DEBUG) // {
  pStrategy->Inc(pQueue);
#endif // }
  QueueUnlockMutex(pQueue);
  ReleaseSemaphore(pStrategy->GetSemaphoreUp(pQueue),1,NULL);
}

#if defined (YA_DEBUG) // {
Request *QueueLockWrite(Queue *pQueue, Result *pResult, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, va_list ap)
#else // } {
Request *QueueLockWrite(Queue *pQueue, Result *pResult, int exit, int stamp,
    PlayerProc *pPlayerProc, va_list ap)
#endif // }
{
  if (pQueue) {
  Request *pRequest;

#if defined (YA_EVENT_STACK) // {
  pRequest=QueueLock(pQueue,&cqsWrite);
#else // } {
  pRequest=QueueLock(pQueue,&cqsWrite,NULL,NULL,NULL);
#endif // }
    if (pRequest) {
  pRequest->pResult=pResult;
  pRequest->exit=exit;
  pRequest->stamp=stamp;
#if (YA_DEBUG) // {
  pRequest->id=id;
#endif // }
  pRequest->pPlayerProc=pPlayerProc;
  pRequest->ap=ap;

  return pRequest;
}
  }
  return NULL;
}

void QueueUnlockWrite(Queue *pQueue, Result *pResult)
{
  QueueUnlock(pQueue,&cqsWrite,NULL);

  if (pResult && pResult->hEvent) {
    WaitForSingleObjectEx(
      pResult->hEvent,      // _In_ HANDLE hHandle,
      1000/*/INFINITE/**/,  // _In_ DWORD  dwMilliseconds,
      TRUE/*/FALSE/**/      // _In_ BOOL   bAlertable
    );
  }
}

#if defined (YA_EVENT_STACK) // {
Request *QueueLockRead(Queue *pQueue)
{
  Request *pRequest;

  pRequest=QueueLock(pQueue,&cqsRead);

  return pRequest;
}
#else // } {
Request *QueueLockRead(Queue *pQueue, HANDLE hEvent,
    void (*OnEvent)(void *data), void *data)
{
  Request *pRequest;

  pRequest=QueueLock(pQueue,&cqsRead,hEvent,OnEvent,data);

  return pRequest;
}
#endif // }

void QueueUnlockRead(Queue *pQueue, Result *pResult)
{
  QueueUnlock(pQueue,&cqsRead,pResult);
}

#if defined (YA_DEBUG) // {
void QueueWrite(Queue *pQueue, Store *pStore, int exit, int stamp,
    const char *id, PlayerProc *pPlayerProc, va_list ap)
#else // } {
void QueueWrite(Queue *pQueue, Store *pStore, int exit, int stamp,
    PlayerProc *pPlayerProc, va_list ap)
#endif // }
{
  //Request *pRequest;

  Result *pResult=pStore?StoreGet(pStore):NULL;
#if defined (YA_DEBUG) // {
  /*pRequest=*/QueueLockWrite(pQueue,pResult,exit,stamp,id,pPlayerProc,ap);
#else // } {
  /*pRequest=*/QueueLockWrite(pQueue,pResult,exit,stamp,pPlayerProc,ap);
#endif // }
  QueueUnlockWrite(pQueue,pResult);
  StorePut(pStore,pResult);
}

#if defined (YA_DEBUG) // {
///////////////////////////////////////////////////////////////////////////////
void QueueIncAvailable(Queue *pQueue)
{
  ++pQueue->nAvailable;
}

void QueueDecAvailable(Queue *pQueue)
{
  if (--pQueue->nAvailable<pQueue->nMinAvailable)
    pQueue->nMinAvailable=pQueue->nAvailable;
}

void QueueNopAvailable(Queue *pQueue)
{
}
#endif // }
