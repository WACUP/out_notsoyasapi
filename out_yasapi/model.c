/*
 * model-4.c
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
#include <windows.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
typedef struct _Result Result;
typedef struct _Store Store;

typedef struct _Request Request;
typedef struct _QueueStrategy QueueStrategy;
typedef struct _Queue Queue;

typedef struct _Model Model;

///////////////////////////////////////////////////////////////////////////////
struct _Result {
  Result *prPrev;
  int tag;
  HANDLE hEvent;
};

int ResultCreate(Result *pResult);
void ResultDestroy(Result *pResult);

///////////////////////////////////////////////////////////////////////////////
enum {
  STORE_SIZE=16
};

struct _Store {
  Result *prTos;
  Result aResult[STORE_SIZE];
  HANDLE hAvailable;
  HANDLE hMutex;
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
enum {
  REQUEST_DESTROY,
  REQUEST_CREATE,
  REQUEST_TICK
};

struct _Request {
  Result *pResult;
  int tag;
};

///////////////////////////////////////////////////////////////////////////////
struct _QueueStrategy {
  const char *name;
  HANDLE (*GetSemaphoreDown)(Queue *pQueue);
  HANDLE (*GetSemaphoreUp)(Queue *pQueue);
  BOOL (*IsAlertable)(Queue *pQueue);
  Request **(*GetRequest)(Queue *pQueue);
};

extern const QueueStrategy cqsWrite;
extern const QueueStrategy cqsRead;

///////////////////////////////////////////////////////////////////////////////
enum {
  QUEUE_SIZE=16
};

struct _Queue {
  Request aRequest[QUEUE_SIZE];
  Request *rp;
  Request *wp;
  Request *mp;
  HANDLE hAvailable;
  HANDLE hWritten;
  HANDLE hMutex;
};

int QueueCreate(Queue *pQueue);
void QueueDestroy(Queue *pQueue);

Request *QueueLock(Queue *pQueue, const QueueStrategy *pStrategy);
void QueueUnlock(Queue *pQueue, const QueueStrategy *pStrategy,
    Result *pResult);
Request *QueueLockWrite(Queue *pQueue, Result *pResult, int tag);
Request *QueueLockRead(Queue *pQueue);
void QueueUnlockRead(Queue *pQueue, Result *pResult);
void QueueWrite(Queue *pQueue, int tag, Store *pStore);

///////////////////////////////////////////////////////////////////////////////
struct _Model {
  Store store;
  Queue queue;
  HANDLE hTimer;
  HANDLE hThread;
};

int ModelCreate(Model *pModel);
void ModelDestroy(Model *pModel);

VOID CALLBACK ModelTick(LPVOID lpArg, DWORD dwLow, DWORD dwHigh);
void ModelSetTimer(Model *pModel, LONGLONG time);
DWORD WINAPI ModelThread(LPVOID lpParameter);

///////////////////////////////////////////////////////////////////////////////
int ResultCreate(Result *pResult)
{
  /////////////////////////////////////////////////////////////////////////////
  pResult->prPrev=NULL;
  pResult->tag=-1;

  /////////////////////////////////////////////////////////////////////////////
  pResult->hEvent=CreateEvent(
    0,        // _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
    FALSE,    // _In_     BOOL                  bManualReset,
    FALSE,    // _In_     BOOL                  bInitialState,
    NULL      // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pResult->hEvent) {
    printf("error creating event\n");
    goto event;
  }
  
  printf("event created\n");

  /////////////////////////////////////////////////////////////////////////////
  return 0;
// cleanup:
  //CloseHandle(pResult->hEvent);
event:
  return -1;
}

void ResultDestroy(Result *pResult)
{
  printf("destroying event\n");
  CloseHandle(pResult->hEvent);
}

///////////////////////////////////////////////////////////////////////////////
int StoreCreate(Store *pStore)
{
  int i;
  Result *pResult;

  /////////////////////////////////////////////////////////////////////////////
  pStore->prTos=NULL;

  /////////////////////////////////////////////////////////////////////////////
  i=0;

  while (i<STORE_SIZE) {
    pResult=pStore->aResult+i;

    if (ResultCreate(pResult)<0)
      goto result;

    StorePush(pStore,pResult);
    ++i;
    printf("result created\n");
  }

  /////////////////////////////////////////////////////////////////////////////
  pStore->hMutex=CreateMutex(
    NULL,         // _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    FALSE,        // _In_     BOOL                  bInitialOwner,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pStore->hMutex) {
    printf("error creating mutex\n");
    goto mutex;
  }
  
  printf("mutex created\n");

  /////////////////////////////////////////////////////////////////////////////
  pStore->hAvailable=CreateSemaphore(
    0,            // _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    STORE_SIZE,   // _In_     LONG                  lInitialCount,
    STORE_SIZE,   // _In_     LONG                  lMaximumCount,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pStore->hAvailable) {
    printf("error creating available semaphore\n");
    goto available;
  }
  
  printf("available semaphore created\n");

  /////////////////////////////////////////////////////////////////////////////
  return 0;
// cleanup:
// CloseHandle(pStore->hAvailable);
available:
  CloseHandle(pStore->hMutex);
mutex:
  while (0<i) {
    --i;
    ResultDestroy(pStore->aResult+i);
  }
result:
  return -1;
}

void StoreDestroy(Store *pStore)
{
  int i;

  printf("destroying available semaphore\n");
  CloseHandle(pStore->hAvailable);
  printf("destroying mutex\n");
  CloseHandle(pStore->hMutex);

  i=STORE_SIZE;

  while (0<i) {
    --i;
    printf("destroying result\n");
    ResultDestroy(pStore->aResult+i);
  }
}

void StorePush(Store *pStore, Result *pResult)
{
  pResult->prPrev=pStore->prTos;
  pStore->prTos=pResult;
}

Result *StorePop(Store *pStore)
{
  Result *pResult;

  if (NULL!=(pResult=pStore->prTos)) {
    pStore->prTos=pResult->prPrev;
    pResult->prPrev=NULL;
  }

  return pResult;
}

void StoreLock(Store *pStore)
{
  WaitForSingleObjectEx(
    pStore->hMutex,       // _In_ HANDLE hHandle,
    INFINITE,             // _In_ DWORD  dwMilliseconds,
    FALSE                 // _In_ BOOL   bAlertable
  );
}

void StoreUnlock(Store *pStore)
{
  ReleaseMutex(pStore->hMutex);
}

Result *StoreGet(Store *pStore)
{
  Result *pResult;

  WaitForSingleObjectEx(
    pStore->hAvailable,   // _In_ HANDLE hHandle,
    INFINITE,             // _In_ DWORD  dwMilliseconds,
    FALSE                 // _In_ BOOL   bAlertable
  );

  StoreLock(pStore);
  pResult=StorePop(pStore);
  StoreUnlock(pStore);

  return pResult;
}

void StorePut(Store *pStore, Result *pResult)
{
  if (pStore) {
    StoreLock(pStore);
    StorePush(pStore,pResult);
    StoreUnlock(pStore);

    ReleaseSemaphore(pStore->hAvailable,1,NULL);
  }
}

///////////////////////////////////////////////////////////////////////////////
static HANDLE WriteGetSemaphoreDown(Queue *pQueue)
{
  return pQueue->hAvailable;
}

static HANDLE WriteGetSemaphoreUp(Queue *pQueue)
{
  return pQueue->hWritten;
}

static BOOL WriteIsAlertable(Queue *pQueue)
{
  return FALSE;
}

static Request **WriteGetRequest(Queue *pQueue)
{
  return &pQueue->wp;
}

const QueueStrategy cqsWrite={
  "write",
  WriteGetSemaphoreDown,
  WriteGetSemaphoreUp,
  WriteIsAlertable,
  WriteGetRequest
};

///////////////////////////////////////////////////////////////////////////////
static HANDLE ReadGetSemaphoreDown(Queue *pQueue)
{
  return pQueue->hWritten;
}

static HANDLE ReadGetSemaphoreUp(Queue *pQueue)
{
  return pQueue->hAvailable;
}

static BOOL ReadIsAlertable(Queue *pQueue)
{
  return TRUE;
}

static Request **ReadGetRequest(Queue *pQueue)
{
  return &pQueue->rp;
}

const QueueStrategy cqsRead={
  "read",
  ReadGetSemaphoreDown,
  ReadGetSemaphoreUp,
  ReadIsAlertable,
  ReadGetRequest
};

///////////////////////////////////////////////////////////////////////////////
int QueueCreate(Queue *pQueue)
{
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
    printf("error creating available semaphore\n");
    goto available;
  }
  
  printf("available semaphore created\n");

  /////////////////////////////////////////////////////////////////////////////
  pQueue->hWritten=CreateSemaphore(
    0,            // _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    0,            // _In_     LONG                  lInitialCount,
    QUEUE_SIZE,   // _In_     LONG                  lMaximumCount,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pQueue->hWritten) {
    printf("error creating written semaphore\n");
    goto written;
  }
  
  printf("available written created\n");

  /////////////////////////////////////////////////////////////////////////////
  pQueue->hMutex=CreateMutex(
    NULL,         // _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    FALSE,        // _In_     BOOL                  bInitialOwner,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pQueue->hMutex) {
    printf("error creating mutex\n");
    goto mutex;
  }
  
  printf("mutex created\n");

  /////////////////////////////////////////////////////////////////////////////
  return 0;
// cleanup:
  //CloseHandle(pQueue->hMutex);
mutex:
  CloseHandle(pQueue->hWritten);
written:
  CloseHandle(pQueue->hAvailable);
available:
  return -1;
}

void QueueDestroy(Queue *pQueue)
{
  printf("destroying mutex\n");
  CloseHandle(pQueue->hMutex);
  printf("destroying written semaphore\n");
  CloseHandle(pQueue->hWritten);
  printf("destroying available semaphore\n");
  CloseHandle(pQueue->hAvailable);
}

///////////////////////////////////////////////////////////////////////////////
Request *QueueLock(Queue *pQueue, const QueueStrategy *pStrategy)
{
  DWORD dwCode;
retry:
  dwCode=WaitForSingleObjectEx(
    pStrategy->GetSemaphoreDown(pQueue),
                      // _In_ HANDLE hHandle,
    INFINITE,         // _In_ DWORD  dwMilliseconds,
    pStrategy->IsAlertable(pQueue)
                      // _In_ BOOL   bAlertable
  );

  // we need to loop in order to catch alertable events.
  if (WAIT_IO_COMPLETION==dwCode)
    goto retry;

  WaitForSingleObjectEx(
    pQueue->hMutex,   // _In_ HANDLE hHandle,
    INFINITE,         // _In_ DWORD  dwMilliseconds,
    FALSE             // _In_ BOOL   bAlertable
  );

  return *pStrategy->GetRequest(pQueue);
}

void QueueUnlock(Queue *pQueue, const QueueStrategy *pStrategy,
    Result *pResult)
{
  Request **ppRequest;

  ppRequest=pStrategy->GetRequest(pQueue);

  if (++*ppRequest==pQueue->mp)
    *ppRequest=pQueue->aRequest;

  if (pResult)
    SetEvent(pResult->hEvent);

  ReleaseMutex(pQueue->hMutex);
  ReleaseSemaphore(pStrategy->GetSemaphoreUp(pQueue),1,NULL);
}

Request *QueueLockWrite(Queue *pQueue, Result *pResult, int tag)
{
  Request *pRequest;

  pRequest=QueueLock(pQueue,&cqsWrite);
  pRequest->pResult=pResult;
  pRequest->tag=tag;

  return pRequest;
}

void QueueUnlockWrite(Queue *pQueue, Result *pResult)
{
  QueueUnlock(pQueue,&cqsWrite,NULL);

  if (pResult) {
    WaitForSingleObjectEx(
      pResult->hEvent,      // _In_ HANDLE hHandle,
      INFINITE,             // _In_ DWORD  dwMilliseconds,
      FALSE                 // _In_ BOOL   bAlertable
    );
  }
}

Request *QueueLockRead(Queue *pQueue)
{
  return QueueLock(pQueue,&cqsRead);
}

void QueueUnlockRead(Queue *pQueue, Result *pResult)
{
  QueueUnlock(pQueue,&cqsRead,pResult);
}

void QueueWrite(Queue *pQueue, int tag, Store *pStore)
{
  Result *pResult;
  //Request *pRequest;

  pResult=pStore?StoreGet(pStore):NULL;
  /*pRequest=*/QueueLockWrite(pQueue,pResult,tag);
  QueueUnlockWrite(pQueue,pResult);
  StorePut(pStore,pResult);
}

///////////////////////////////////////////////////////////////////////////////
int ModelCreate(Model *pModel)
{
  /////////////////////////////////////////////////////////////////////////////
  if (StoreCreate(&pModel->store)<0) {
    printf("error creating store\n");
    goto store;
  }

  printf("store created\n");

  /////////////////////////////////////////////////////////////////////////////
  if (QueueCreate(&pModel->queue)<0) {
    printf("error creating queue\n");
    goto queue;
  }

  printf("queue created\n");

  /////////////////////////////////////////////////////////////////////////////
  pModel->hTimer=CreateWaitableTimer(
    0,        // _In_opt_ LPSECURITY_ATTRIBUTES lpTimerAttributes,
    FALSE,    // _In_     BOOL                  bManualReset,
    NULL      // _In_opt_ LPCTSTR               lpTimerName
  );

  if (NULL==pModel->hTimer) {
    printf("error creating timer\n");
    goto timer;
  }

  printf("timer created\n");

  /////////////////////////////////////////////////////////////////////////////
  pModel->hThread=CreateThread(
    0,        // _In_opt_  LPSECURITY_ATTRIBUTES  lpThreadAttributes,
    0,        // _In_      SIZE_T                 dwStackSize,
    ModelThread,
              // _In_      LPTHREAD_START_ROUTINE lpStartAddress,
    pModel,   // _In_opt_  LPVOID                 lpParameter,
    0,        // _In_      DWORD                  dwCreationFlags,
    NULL      // _Out_opt_ LPDWORD                lpThreadId
  );

  if (NULL==pModel->hThread) {
    printf("error creating thread\n");
    goto thread;
  }

  printf("thread created\n");

  /////////////////////////////////////////////////////////////////////////////
  QueueWrite(&pModel->queue,REQUEST_CREATE,&pModel->store);
  printf("loop started\n");

  /////////////////////////////////////////////////////////////////////////////
  return 0;
// cleanup:
// CloseHandle(pModel->hThread);
thread:
  CloseHandle(pModel->hTimer);
timer:
  QueueDestroy(&pModel->queue);
queue:
  StoreDestroy(&pModel->store);
store:
  return -1;
}

void ModelDestroy(Model *pModel)
{
  printf("stopping loop\n");
  QueueWrite(&pModel->queue,REQUEST_DESTROY,&pModel->store);
  printf("waiting for thread to return\n");
  WaitForSingleObject(pModel->hThread,INFINITE);
  printf("destroying thread\n");
  CloseHandle(pModel->hThread);
  printf("destroying timer\n");
  CloseHandle(pModel->hTimer);
  printf("destroying queue\n");
  QueueDestroy(&pModel->queue);
  printf("destroying store\n");
  StoreDestroy(&pModel->store);
}

VOID CALLBACK ModelTick(LPVOID lpArg, DWORD dwLow, DWORD dwHigh)
{
  Model *pModel=lpArg;

  printf("  X\n");
  QueueWrite(&pModel->queue,REQUEST_TICK,NULL);
}

void ModelSetTimer(Model *pModel, LONGLONG time)
{
  HANDLE hTimer=pModel->hTimer;
  PTIMERAPCROUTINE pfnCompletionRoutine=time?ModelTick:NULL;
  LPVOID lpArgToCompletionRoutine=time?pModel:NULL;
  LARGE_INTEGER liDueTime;

  liDueTime.QuadPart=-time;

  SetWaitableTimer(
    hTimer,       // _In_           HANDLE           hTimer,
    &liDueTime,   // _In_     const LARGE_INTEGER    *pDueTime,
    0,            // _In_           LONG             lPeriod,
    pfnCompletionRoutine,
                  // _In_opt_       PTIMERAPCROUTINE pfnCompletionRoutine,
    lpArgToCompletionRoutine,
                  // _In_opt_       LPVOID           lpArgToCompletionRoutine,
    FALSE         // _In_           BOOL             fResume
  );
}

DWORD WINAPI ModelThread(LPVOID lpParameter)
{
  Model *pModel=lpParameter;
  Request *pRequest;
  Result *pResult;

  for (;;) {
    pRequest=QueueLockRead(&pModel->queue);
    pResult=pRequest->pResult;

    switch (pRequest->tag) {
    case REQUEST_DESTROY:
      printf("REQUEST_DESTROY\n");
      printf("killing timer\n");
      ModelSetTimer(pModel,0ll);
      fflush(stdout);
      QueueUnlockRead(&pModel->queue,pResult);
      goto end;
    case REQUEST_CREATE:
      printf("REQUEST_CREATE\n");
      fflush(stdout);
      QueueUnlockRead(&pModel->queue,pResult);
      break;
    case REQUEST_TICK:
      printf("REQUEST_TICK\n");
      ModelSetTimer(pModel,1000ll);
      fflush(stdout);
      QueueUnlockRead(&pModel->queue,pResult);
      break;
    default:
      printf("ERROR\n");
      fflush(stdout);
      QueueUnlockRead(&pModel->queue,pResult);
      break;
    }
  }
end:
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
  Model model;

  // create the model.
  if (ModelCreate(&model)<0) {
    printf("error creating model\n");
    goto model;
  }

  printf("model created\n");

  QueueWrite(&model.queue,REQUEST_TICK,&model.store);
  Sleep(25);

// cleanup:
  printf("destroying model\n");
  ModelDestroy(&model);
model:
  return 0;
}
