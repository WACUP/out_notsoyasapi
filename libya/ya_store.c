/*
 * ya_store.c
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

    if (ResultCreate(pResult)<0) {
      DMESSAGE("creating result");
      goto result;
    }

    StorePush(pStore,pResult);
    ++i;
    DPUTS(2,"  store result created\n");
  }

  /////////////////////////////////////////////////////////////////////////////
  pStore->hMutex=CreateMutex(
    NULL,         // _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    FALSE,        // _In_     BOOL                  bInitialOwner,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pStore->hMutex) {
    DMESSAGE("creating mutex");
    goto mutex;
  }
  
  DPUTS(0,"  store mutex created\n");

  /////////////////////////////////////////////////////////////////////////////
  pStore->hAvailable=CreateSemaphore(
    0,            // _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    STORE_SIZE,   // _In_     LONG                  lInitialCount,
    STORE_SIZE,   // _In_     LONG                  lMaximumCount,
    NULL          // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pStore->hAvailable) {
    DMESSAGE("creating available semaphore");
    goto available;
  }
  
  DPUTS(0,"  store available semaphore created\n");

#if defined (YA_DEBUG) // {
  /////////////////////////////////////////////////////////////////////////////
  pStore->nMinAvailable=pStore->nAvailable=STORE_SIZE;
#endif // }

  /////////////////////////////////////////////////////////////////////////////
  return 0;
// cleanup:
//  CloseHandle(pStore->hAvailable);
available:
  CloseHandle(pStore->hMutex);
  pStore->hMutex=NULL;
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

  DPUTS(0,"  destroying store available semaphore\n");
  CloseHandle(pStore->hAvailable);
  pStore->hAvailable=NULL;
  DPUTS(0,"  destroying store mutex\n");
  CloseHandle(pStore->hMutex);
  pStore->hMutex=NULL;

  i=STORE_SIZE;

  while (0<i) {
    --i;
    DPUTS(2,"  destroying store result\n");
    ResultDestroy(pStore->aResult+i);
  }
}

void StorePush(Store *pStore, Result *pResult)
{
  pResult->prPrev=pStore->prTos;
  pStore->prTos=pResult;
#if defined (YA_DEBUG) // {
  ++pStore->nAvailable;
#endif // }
}

Result *StorePop(Store *pStore)
{
  Result *pResult;

  if (NULL!=(pResult=pStore->prTos)) {
    pStore->prTos=pResult->prPrev;
    pResult->prPrev=NULL;
#if defined (YA_DEBUG) // {
    if (--pStore->nAvailable<pStore->nMinAvailable)
      pStore->nMinAvailable=pStore->nAvailable;
#endif // }
  }

  return pResult;
}

void StoreLock(Store *pStore)
{
  WaitForSingleObjectEx(
    pStore->hMutex,       // _In_ HANDLE hHandle,
    1000/*/INFINITE/**/,             // _In_ DWORD  dwMilliseconds,
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
    1000/*/INFINITE/**/,             // _In_ DWORD  dwMilliseconds,
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
