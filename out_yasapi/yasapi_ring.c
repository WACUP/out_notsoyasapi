/*
 * yasapi_ring.c
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

#if defined (YA_DEBUG) // {
#define YASAPI_RING_DEBUG
#endif // }

#if defined (YASAPI_RING_DEBUG) // {
#define RING_CONSISTENCY(pRing,dwSize,func,line,pError,label) \
  if (RingConsistency(pRing,dwSize,func,line,pError)<0) { \
    DMESSAGEV("%s: consistency",__func__); \
    goto label; \
  }
#else // } {
#define RING_CONSISTENCY(pRing,dwSize,func,line,pError,label)
#endif // }

typedef struct _RingWriteConfig RingWriteConfig;
typedef struct _RingReadConfig RingReadConfig;

struct _RingWriteConfig {
  LPCSTR pData;
  SIZE_T dwSourceSize;
  SIZE_T dwTargetSize;
};

struct _RingReadConfig {
  LPSTR pRep;
  LPSTR pMax;
  LPSTR pData;
  SIZE_T dwSize;
  UINT uFlags;
  RingIOError *pError;
};

#if defined (YASAPI_FORCE24BIT) // {
DWORD RingCreate(Ring *pRing, SIZE_T dwSize, int nNumerator, int nDenominator)
#else // } {
DWORD RingCreate(Ring *pRing, SIZE_T dwSize, SIZE_T nShift)
#endif // }
{
#if defined (YASAPI_FORCE24BIT) // {
  pRing->nNumerator=nNumerator;
  pRing->nDenominator=nDenominator;
#else // } {
  pRing->nShift=nShift;
#endif // }
  pRing->Copy=RingCopyMemory;
  pRing->pClient=pRing;

  // create ring buffer.
  if (NULL==(pRing->pRep=YA_MALLOC(dwSize))) {
    DMESSAGE("allocating ring buffer");
    goto rep;
  }
  
  pRing->pMax=pRing->pRep+dwSize;
  RingReset(pRing);

  return dwSize;
// cleanup:
  //YA_FREE(pRing->pRep);
rep:
  return (DWORD)-1;
}

void RingDestroy(Ring *pRing)
{
  if (pRing->pRep)
  {
    YA_FREE(pRing->pRep);
    pRing->pRep = NULL;
  }
}

int RingGetSize(const Ring *pRing)
{
  return pRing->pMax-pRing->pRep;
}

void RingReset(Ring *pRing)
{
  pRing->pHead=pRing->pRep;
  pRing->pTail=pRing->pRep;
  pRing->dwWritten=0;
  pRing->dwAvailable=pRing->pMax-pRing->pRep;
}

#if defined (YASAPI_FORCE24BIT) // {
DWORD RingSourceSize(const Ring *pRing, const DWORD dwTargetSize)
{
  return MulDiv(dwTargetSize,pRing->nNumerator,pRing->nDenominator);
}

DWORD RingTargetSize(const Ring *pRing, const DWORD dwSourceSize)
{
  return MulDiv(dwSourceSize,pRing->nDenominator,pRing->nNumerator);
}
#else // } {
DWORD RingSourceSize(const Ring *pRing, const DWORD dwTargetSize)
{
  return dwTargetSize>>pRing->nShift;
}

DWORD RingTargetSize(const Ring *pRing, const DWORD dSourcewSize)
{
  return dwSourceSize<<pRing->nShift;
}
#endif // }

#if defined (YASAPI_RING_REALLOC) // {
int RingRealloc(Ring *pRing, SIZE_T uSize)
{
  // all sizes in target units.
  enum { DEBUG=0 };
  UINT32 uBufSize=RingGetSize(pRing);
  DWORD dwAvailable;
  DWORD dwOffs1;
  DWORD dwOffs2;
  LPSTR pRep;
  LPSTR pMax;
  LPSTR pHead;
  LPSTR pTail;

  if (uBufSize<uSize) {
    DPRINTF(DEBUG,"  > %s <\n",__func__);
    RING_CONSISTENCY(pRing,0,__func__,__LINE__,NULL,consistency1);

    do {
      if (0==(uBufSize<<=1)) {
        DMESSAGEV("%s: overflow",__func__);
        goto overflow;
      }
    } while (uBufSize<uSize);

    uSize=uBufSize;
    dwAvailable=uSize-pRing->dwWritten;

    DPRINTF(DEBUG,"  > %s (%d bytes) <\n",__func__,uSize);

    if (NULL==(pRep=YA_MALLOC(uSize))) {
      DMESSAGEV("%s: malloc",__func__);
      goto malloc;
    }

    pMax=pRep+uSize;

    // are we un-wrapped?
    if (pRing->pTail<pRing->pHead) {
      // yes, we are.
      dwOffs1=pRing->pTail-pRing->pRep;
      dwOffs2=pRing->pHead-pRing->pRep;

      pTail=pRep+dwOffs1;
      pHead=pRep+dwOffs2;

      if (0<pRing->dwWritten) {
        if (pTail<pRep||pMax<pTail+pRing->dwWritten) {
          DMESSAGE("re-alloc operation out of range");
          goto range1;
        }

        memcpy(
          pTail,              // _In_       PVOID  Destination,
          pRing->pTail,       // _In_ const VOID   *Source,
          pRing->dwWritten    // _In_       SIZE_T Length
        );
      }
    }
    else if (pRing->pHead<pRing->pTail||0<pRing->dwWritten) {
      // no, we are wrapped.
      dwOffs1=pRing->pHead-pRing->pRep;
      dwOffs2=pRing->pMax-pRing->pTail;

      pHead=pRep+dwOffs1;

      if (pMax<pRep+dwOffs1) {
        DMESSAGE("re-alloc operation out of range");
        goto range2;
      }

      if (0<dwOffs1) {
        memcpy(
          pRep,               // _In_       PVOID  Destination,
          pRing->pRep,        // _In_ const VOID   *Source,
          dwOffs1             // _In_       SIZE_T Length
        );
      }

      pTail=pMax-dwOffs2;

      if (pTail<pRep||pMax<pTail+dwOffs2) {
        DMESSAGE("re-alloc operation out of range");
        goto range3;
      }

      if (0<dwOffs2) {
        memcpy(
          pTail,              // _In_       PVOID  Destination,
          pRing->pTail,       // _In_ const VOID   *Source,
          dwOffs2             // _In_       SIZE_T Length
        );
      }
    }
    else {
      pTail=pRep;
      pHead=pRep;
    }

    YA_FREE(pRing->pRep);
    pRing->pRep=pRep;
    pRing->pMax=pMax;
    pRing->pTail=pTail;
    pRing->pHead=pHead;
    pRing->dwAvailable=dwAvailable;

    RING_CONSISTENCY(pRing,0,__func__, __LINE__,NULL,consistency2);

    return 0;
  }
  else
    return 0;
// cppcheck-suppress unusedLabel
consistency2:
range3:
range2:
range1:
  YA_FREE(pRep);
malloc:
overflow:
// cppcheck-suppress unusedLabel
consistency1:
  return -1;
}

int RingReallocAvailable(Ring *pRing, SIZE_T dwAvailable)
{
  // transform into target units.
  dwAvailable=RingTargetSize(pRing,dwAvailable);

  return RingRealloc(pRing,pRing->dwWritten+dwAvailable);
}
#endif // }

static DWORD RingWriteUnwrapped(Ring *pRing, RingWriteConfig *pc)
{
  if (pRing && pc) {
  if (0<pc->dwSourceSize) {
    if (pRing->pHead<pRing->pRep||pRing->pMax<pRing->pHead+pc->dwTargetSize) {
      DMESSAGE("write operation out of range");
      goto range;
    }

    pRing->Copy(
      pRing->pClient,
      pRing->pHead,     // _In_  PVOID Destination,
      pc->pData,        // _In_  const VOID *Source,
      pc->dwTargetSize  // _In_  SIZE_T Length
    );

    pc->pData+=pc->dwSourceSize;
    pRing->pHead+=pc->dwTargetSize;
  }

  RING_CONSISTENCY(pRing,pc->dwTargetSize,__func__, __LINE__,NULL,consistency);

  return pc->dwSourceSize;
  }
// cppcheck-suppress unusedLabelConfiguration  
consistency:
range:
  return (DWORD)-1;

#if 0 // {
  if (0<dwSize) {
    pRing->Copy(
      pRing->pClient,
      pRing->pHead,     // _In_  PVOID Destination,
      pData,            // _In_  const VOID *Source,
      dwSize            // _In_  SIZE_T Length
    );

    pData+=dwSize>>nShift;
    pRing->pHead+=dwSize;
  }

  RING_CONSISTENCY(pRing,dwSize,__func__, __LINE__,NULL,consistency);

  return dwSize;
consistency:
  return (DWORD)-1;
#endif // }
}

static DWORD RingWriteWrapped(Ring *pRing, RingWriteConfig *pc)
{
  DWORD dwTargetSize;
  DWORD dwSourceSize;

  if (0<(dwTargetSize=pRing->pMax-pRing->pHead)) {
    dwSourceSize=RingSourceSize(pRing,dwTargetSize);

    if (pRing->pHead<pRing->pRep||pRing->pMax<pRing->pHead) {
      DMESSAGE("write operation out of range");
      goto range1;
    }

    pRing->Copy(
      pRing->pClient,
      pRing->pHead,     // _In_  PVOID Destination,
      pc->pData,        // _In_  const VOID *Source,
      dwTargetSize      // _In_  SIZE_T Length
    );

    pc->pData+=dwSourceSize;
    pRing->pHead+=dwTargetSize;
  }
  else
    dwTargetSize=0;

  if (0<(dwTargetSize=pc->dwTargetSize-dwTargetSize)) {
    dwSourceSize=RingSourceSize(pRing,dwTargetSize);

    if (pRing->pMax<pRing->pRep+dwTargetSize) {
      DMESSAGE("write operation out of range");
      goto range2;
    }

    pRing->Copy(
      pRing->pClient,
      pRing->pRep,      // _In_  PVOID Destination,
      pc->pData,        // _In_  const VOID *Source,
      dwTargetSize      // _In_  SIZE_T Length
    );

    pc->pData+=dwSourceSize;
    pRing->pHead=pRing->pRep+dwTargetSize;
  }

  RING_CONSISTENCY(pRing,pc->dwTargetSize,__func__, __LINE__,NULL,consistency);

  return pc->dwSourceSize;
// cppcheck-suppress unusedLabelConfiguration
consistency:
range2:
range1:
  return (DWORD)-1;

#if 0 // {
  DWORD dwOffs1=pRing->pMax-pRing->pHead;
  DWORD dwOffs2=dwSize-dwOffs1;

  if (0<dwOffs1) {
    pRing->Copy(
      pRing->pClient,
      pRing->pHead,     // _In_  PVOID Destination,
      pData,            // _In_  const VOID *Source,
      dwOffs1           // _In_  SIZE_T Length
    );

    pData+=dwOffs1>>nShift;
    pRing->pHead+=dwOffs2;
  }

  if (0<dwOffs2) {
    pRing->Copy(
      pRing->pClient,
      pRing->pRep,      // _In_  PVOID Destination,
      pData,            // _In_  const VOID *Source,
      dwOffs2           // _In_  SIZE_T Length
    );

    pData+=dwOffs2>>nShift;
    pRing->pHead=pRing->pRep+dwOffs2;
  }

  RING_CONSISTENCY(pRing,dwSize,__func__, __LINE__,NULL,consistency);

  return dwSize;
#endif // }
}

DWORD RingWrite(Ring *pRing, LPCSTR pData, SIZE_T dwSize)
{
  RingWriteConfig c={
    pData,                    // LPCSTR pData;
    dwSize,                   // SIZE_T dwSourceSize;
    RingTargetSize(pRing,dwSize),
                              // SIZE_T dwTargetSize;
  };

  RING_CONSISTENCY(pRing,0,__func__, __LINE__,NULL,consistency1);

  if (pRing->dwAvailable<c.dwTargetSize) {
    c.dwTargetSize=pRing->dwAvailable;
    c.dwSourceSize=RingSourceSize(pRing,c.dwTargetSize);
  }

  if (0<c.dwTargetSize) {
    if (pRing->pTail==pRing->pHead) {
      if (0==pRing->dwWritten) {
        if (pRing->pMax<pRing->pHead+c.dwTargetSize) {
          if (RingWriteWrapped(pRing,&c)<0)
            goto wrapped;
        }
        else {
          if (RingWriteUnwrapped(pRing,&c)<0)
            goto unwrapped;
        }
      }
      else {
        DMESSAGEV("%s: overflow: %d",__func__,pRing->dwWritten);
        goto overflow;
      }
    }
    else if (pRing->pTail<pRing->pHead) {
      if (pRing->pMax<pRing->pHead+c.dwTargetSize) {
        if (RingWriteWrapped(pRing,&c)<0)
          goto wrapped;
      }
      else {
        if (RingWriteUnwrapped(pRing,&c)<0)
          goto unwrapped;
      }
    }
    else {
      if (RingWriteUnwrapped(pRing,&c)<0)
        goto wrapped;
    }

	if (pRing) {
    pRing->dwWritten+=c.dwTargetSize;
    pRing->dwAvailable-=c.dwTargetSize;
	}
	else {
		goto consistency1;
	}
  }

  RING_CONSISTENCY(pRing,0,__func__, __LINE__,NULL,consistency2);

  return c.dwSourceSize;
// cppcheck-suppress unusedLabel
consistency2:
overflow:
unwrapped:
wrapped:
// cppcheck-suppress unusedLabel
consistency1:
  return (DWORD)-1;
}

static DWORD RingReadUnwrappedEx(Ring *pRing, RingReadConfig *pc)
{
  if (RING_COPY&pc->uFlags&&0<pc->dwSize) {
    if (pc->pData<pc->pRep||pc->pMax<pc->pData+pc->dwSize) {
      pc->pError->Cleanup(pc->pError);
      DMESSAGE("read operation out of range");
      goto range;
    }

    memcpy(
      pc->pData,        // _In_  PVOID Destination,
      pRing->pTail,     // _In_  const VOID *Source,
      pc->dwSize        // _In_  SIZE_T Length
    );

    pc->pData+=pc->dwSize;
  }

  if (RING_COMMIT&pc->uFlags) {
    pRing->pTail+=pc->dwSize;
    RING_CONSISTENCY(pRing,-pc->dwSize,__func__, __LINE__,pc->pError,
        consistency);
  }

  return pc->dwSize;
// cppcheck-suppress unusedLabel
consistency:
range:
  return (DWORD)-1;
}

static DWORD RingReadWrappedEx(Ring *pRing, RingReadConfig *pc)
{
  DWORD dwOffs1=pRing->pMax-pRing->pTail;
  DWORD dwOffs2=pRing->pHead-pRing->pRep;
  DWORD dwCopySize;

  dwCopySize=pc->dwSize<dwOffs1?pc->dwSize:dwOffs1;

  if (RING_COPY&pc->uFlags) {
    if (pc->pData<pc->pRep||pc->pMax<pc->pData+dwCopySize) {
      pc->pError->Cleanup(pc->pError);
      DMESSAGE("read operation out of range");
      goto range1;
    }

    memcpy(
      pc->pData,          // _In_  PVOID Destination,
      pRing->pTail,       // _In_  const VOID *Source,
      dwCopySize          // _In_  SIZE_T Length
    );

    pc->pData+=dwCopySize;
  }

  if (RING_COMMIT&pc->uFlags) {
    pRing->pTail+=dwCopySize;
    RING_CONSISTENCY(pRing,-dwCopySize,__func__, __LINE__,pc->pError,
        consistency1);
  }

  // is there something left to read?
  if (dwCopySize<pc->dwSize) {
    if (dwOffs2<(dwCopySize=pc->dwSize-dwCopySize)) {
      pc->pError->Cleanup(pc->pError);
      DMESSAGEV("%s: overread",__func__);
      goto overread;
    }

    if (pc->pData<pc->pRep||pc->pMax<pc->pData+dwCopySize) {
      pc->pError->Cleanup(pc->pError);
      DMESSAGE("read operation out of range");
      goto range2;
    }

    if (RING_COPY&pc->uFlags) {
      memcpy(
        pc->pData,        // _In_  PVOID Destination,
        pRing->pRep,      // _In_  const VOID *Source,
        dwCopySize        // _In_  SIZE_T Length
      );

      pc->pData+=dwCopySize;
    }
  
    // we are not wrapped any longer.
    if (RING_COMMIT&pc->uFlags) {
      pRing->pTail=pRing->pRep+dwCopySize;
      RING_CONSISTENCY(pRing,-pc->dwSize,__func__, __LINE__,pc->pError,
          consistency2);
    }
  }

  return pc->dwSize;
// cppcheck-suppress unusedLabel
consistency2:
range2:
overread:
// cppcheck-suppress unusedLabel
consistency1:
range1:
  return (DWORD)-1;
}

DWORD RingReadEx(Ring *pRing, LPSTR pData, SIZE_T dwSize, UINT uFlags,
    RingIOError *pError)
{
  RingReadConfig c={
  	pData,    // LPSTR pRep;
  	pData,    // LPSTR pMax;
    pData,    // LPSTR pData;
    dwSize,   // SIZE_T dwSize;
    uFlags,   // UINT uFlags;
    pError    // RingIOError *pError;
  };

  RING_CONSISTENCY(pRing,0,__func__, __LINE__,pError,consistency1);

  if (pRing->dwWritten<dwSize)
    c.dwSize=dwSize=pRing->dwWritten;

  c.pMax+=dwSize;

  if (0<dwSize) {
    if (pRing->pTail==pRing->pHead) {
      if (0==pRing->dwAvailable) {
        if (pRing->pMax<pRing->pTail+dwSize) {
          if (RingReadWrappedEx(pRing,&c)<0)
            goto wrapped;
        }
        else {
          if (RingReadUnwrappedEx(pRing,&c)<0)
            goto unwrapped;
        }
      }
      else {
        DMESSAGEV("%s: overread",__func__);
        goto overread;
      }
    }
    else if (pRing->pTail<pRing->pHead) {
      if (pRing->pMax<pRing->pTail+dwSize) {
        if (RingReadWrappedEx(pRing,&c)<0)
          goto wrapped;
      }
      else {
        if (RingReadUnwrappedEx(pRing,&c)<0)
          goto unwrapped;
      }
    }
    else {
      if (RingReadWrappedEx(pRing,&c)<0)
        goto wrapped;
    }

    if (RING_COMMIT&uFlags) {
      pRing->dwWritten-=dwSize;
      pRing->dwAvailable+=dwSize;
    }
  }

  RING_CONSISTENCY(pRing,0,__func__, __LINE__,pError,consistency2);

  return dwSize;
// cppcheck-suppress unusedLabel
consistency2:
overread:
unwrapped:
wrapped:
// cppcheck-suppress unusedLabel
consistency1:
  return (DWORD)-1;
}

DWORD RingRead(Ring *pRing, LPSTR pData, SIZE_T dwSize, RingIOError *pError)
{
  return RingReadEx(pRing,pData,dwSize,RING_COPY|RING_COMMIT,pError);
}

void RingCopyMemory(PVOID *Client, PVOID Destination, const VOID *Source,
    SIZE_T Length)
{
  memcpy(
    Destination,              // _In_  PVOID Destination,
    Source,                   // _In_  const VOID *Source,
    Length                    // _In_  SIZE_T Length
  );
}

#if defined (YA_DEBUG) // {
static void RingDumpUnwrapped(const Ring *pRing, const SIZE_T dwSize)
{
  LPCSTR pData=pRing->pTail;
  LPCSTR pMax=pData+dwSize;

  while (pData<pMax)
    DPRINTF(0,"%02X",*pData++);
}

static void RingDumpWrapped(const Ring *pRing, const SIZE_T dwSize)
{
  DWORD dwOffs1=pRing->pMax-pRing->pTail;
  DWORD dwOffs2=pRing->pHead-pRing->pRep;
  DWORD dwCopySize;
  LPCSTR pData;
  LPCSTR pMax;

  dwCopySize=dwSize<dwOffs1?dwSize:dwOffs1;

  pData=pRing->pTail;
  pMax=pData+dwCopySize;

  while (pData<pMax)
    DPRINTF(0,"%02X",*pData++);

  if (dwCopySize<dwSize) {
    if (dwOffs2<(dwCopySize=dwSize-dwCopySize)) {
      DMESSAGEV("%s: overread",__func__);
      goto overread;
    }

    pData=pRing->pRep;
    pMax=pData+dwCopySize;

    while (pData<pMax)
      DPRINTF(0,"%02X",*pData++);
  }
overread:
  ;
}

void RingDump(const Ring *pRing, const SIZE_T dwSize)
{
  if (pRing->dwWritten<dwSize)
    dwSize=pRing->dwWritten;

  if (0<dwSize) {
    if (pRing->pTail==pRing->pHead) {
      if (0==pRing->dwAvailable) {
        if (pRing->pMax<pRing->pTail+dwSize)
          RingDumpWrapped(pRing,dwSize);
        else
          RingDumpUnwrapped(pRing,dwSize);
      }
      else {
        DMESSAGEV("%s: overread",__func__);
        goto overread;
      }
    }
    else if (pRing->pTail<pRing->pHead) {
      if (pRing->pMax<pRing->pTail+dwSize)
        RingDumpWrapped(pRing,dwSize);
      else
        RingDumpUnwrapped(pRing,dwSize);
    }
    else
      RingDumpWrapped(pRing,dwSize);
  }

  DPUTS(0,"\n");
overread:
  ;
}

#endif // }

#if defined (YASAPI_RING_DEBUG) // {
static int RingConsistency(const Ring *pRing, const DWORD dwSize, const char *func,
                                              const int line, RingIOError *pError)
{
  DWORD dwWritten=pRing->dwWritten+dwSize;
  DWORD dwAvailable=pRing->dwAvailable-dwSize;
  DWORD dwBufSize=RingGetSize(pRing);
  DWORD dwOffs1;
  DWORD dwOffs2;
  DWORD dwDiff;

  if (pRing->pTail<=pRing->pHead) {
    dwOffs1=pRing->pTail-pRing->pRep;
    dwOffs2=pRing->pHead-pRing->pRep;
    dwDiff=dwOffs2-dwOffs1;

    if (0==dwDiff&&(dwBufSize==dwWritten||0==dwAvailable))
      ;
    else if (dwWritten==dwDiff)
      ;
#if 0 // {
    else if (0==(dwOffs2-dwOffs1)&&0==dwAvailable)
      ;
#endif // }
    else {
      if (pError)
        pError->Cleanup(pError);

      DMESSAGEV("%s: un-wrapped (%d)",func,line);
      goto unwrapped;
    }
#if 0 // {
    if (dwWritten!=dwOffs2-dwOffs1) {
      if (pError)
        pError->Cleanup(pError);

      DMESSAGEV("%s: %d %d un-wrapped (%d)\n",func,
          dwWritten,dwOffs2-dwOffs1,line);
      goto unwrapped;
    }
#endif // }
  }
  else {
    dwOffs1=pRing->pHead-pRing->pRep;
    dwOffs2=pRing->pMax-pRing->pTail;

    if (dwWritten!=dwOffs1+dwOffs2) {
      if (pError)
        pError->Cleanup(pError);

      DMESSAGEV("%s: wrapped (%d)",func,line);
      goto wrapped;
    }
  }

  return 0;
unwrapped:
wrapped:
  return -1;
}
#endif // }
