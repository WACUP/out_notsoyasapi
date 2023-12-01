/*
 * yasapi_time.c
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

void TimeSegmentReset(TimeSegment *pSegment, const UINT64 u64Frequency)
{
  DPRINTF(0,"  > %s <\n",__func__);

  if (u64Frequency)
    pSegment->u64Position=0ll;
  else
    pSegment->xSeconds=0.0;
}

void TimeSegmentAdd(TimeSegment *pOffset, const TimeSegment *pCurrent,
                                          const UINT64 u64Frequency)
{
  DPRINTF(0,"  > %s <\n",__func__);

  if (u64Frequency)
    pOffset->u64Position+=pCurrent->u64Position;
  else
    pOffset->xSeconds+=pCurrent->xSeconds;
}

void TimeSegmentSub(TimeSegment *pOffset, const TimeSegment *pCurrent,
                                          const UINT64 u64Frequency)
{
  if (u64Frequency)
    pOffset->u64Position-=pCurrent->u64Position;
  else
    pOffset->xSeconds-=pCurrent->xSeconds;
}

int TimeSegmentGet(TimeSegment *pSegment, UINT64 u64Frequency,
    Connection *pConnect)
{
  enum { DEBUG=3 };
  UINT64 u64Position;

  DPRINTF(DEBUG,"  > %s <\n",__func__);

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClock) {
    DMESSAGE("null pointer");
    goto null;
  }

  if (ConnectionGetPosition(pConnect,&u64Position)<0) {
    DMESSAGE("getting position");
    goto position;
  }

  if (u64Frequency)
    pSegment->u64Position=u64Position;
  else {
    if (ConnectionGetFrequency(pConnect,&u64Frequency)<0) {
      DMESSAGE("getting frequency");
      goto frequency;
    }

    pSegment->xSeconds=(double)u64Position/u64Frequency;
  }

  return 0;
frequency:
position:
null:
invalid:
  return -1;
}

int TimeReset(Time *pTime, TimeTag eTimeTag, Connection *pConnect)
{
  enum { DEBUG=0 };

  DPRINTF(DEBUG,"  > %s <\n",__func__);

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClient) {
    DMESSAGE("null pointer");
    goto null;
  }

  switch (eTimeTag) {
  case TIME_POSITION:
    if (ConnectionGetFrequency(pConnect,&pTime->u64Frequency)<0) {
      DMESSAGE("getting frequency");
      goto frequency;
    }

    break;
  case TIME_TIME:
    pTime->u64Frequency=0ull;
    break;
  default:
    DMESSAGEV("tag mismatch: %d\n",eTimeTag);
    goto tag;
  }

  TimeSegmentReset(&pTime->gapless,pTime->u64Frequency);
  TimeSegmentReset(&pTime->pause,pTime->u64Frequency);
  TimeSegmentReset(&pTime->current,pTime->u64Frequency);

  return 0;
tag:
frequency:
null:
invalid:
  return -1;
}

void TimeFlush(Time *pTime)
{
  DPRINTF(0,"  > %s <\n",__func__);

  TimeSegmentReset(&pTime->pause,pTime->u64Frequency);
  TimeSegmentReset(&pTime->current,pTime->u64Frequency);
}

int TimePause(Time *pTime, Connection *pConnect)
{
  DPRINTF(0,"  > %s <\n",__func__);

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClient) {
    DMESSAGE("null pointer");
    goto null;
  }

  //TimeSegmentReset(&pTime->current,pTime->u64Frequency);

  if (TimeSegmentGet(&pTime->current,pTime->u64Frequency,pConnect)<0) {
    DMESSAGE("getting time");
    goto time;
  }

  TimeSegmentReset(&pTime->gapless,pTime->u64Frequency);
  pTime->pause=pTime->current;

  return 0;
time:
null:
invalid:
  return -1;
}

int TimeGapless(Time *pTime, Connection *pConnect)
{
  DPRINTF(0,"  > %s <\n",__func__);

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClient) {
    DMESSAGE("null pointer");
    goto null;
  }

  TimeSegmentReset(&pTime->pause,pTime->u64Frequency);

  if (TimeSegmentGet(&pTime->gapless,pTime->u64Frequency,pConnect)<0) {
    DMESSAGE("getting time");
    goto time;
  }

  pTime->current=pTime->gapless;

  return 0;
time:
null:
invalid:
  return -1;
}

#if defined (YASAPI_CONVERT_TIME) // {
int TimeMigrate(Time *pTime, TimeTag eTimeTag, Connection *pConnect)
#else // } {
int TimeMigrate(Time *pTime, Connection *pConnect)
#endif // }
{
  TimeSegment *pGapless=&pTime->gapless;
  TimeSegment *pPause=&pTime->pause;
  TimeSegment *pCurrent=&pTime->current;

  DPRINTF(0,"  > %s <\n",__func__);

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClient) {
    DMESSAGE("null pointer");
    goto null;
  }

#if defined (YASAPI_CONVERT_TIME) // {
  if (pTime->u64Frequency) {
    if (TIME_TIME==eTimeTag) {
      // cppcheck-suppress overlappingWriteUnion
      pGapless->xSeconds=(double)pGapless->u64Position/pTime->u64Frequency;
      // cppcheck-suppress overlappingWriteUnion
      pPause->xSeconds=(double)pPause->u64Position/pTime->u64Frequency;
      // cppcheck-suppress overlappingWriteUnion
      pCurrent->xSeconds=(double)pCurrent->u64Position/pTime->u64Frequency;
      pTime->u64Frequency=0ull;
    }
  }
  else {
    if (TIME_TIME==eTimeTag) {
      if (ConnectionGetFrequency(pConnect,&pTime->u64Frequency)<0) {
        DMESSAGE("getting frequency");
        goto frequency;
      }

      // cppcheck-suppress overlappingWriteUnion
      pGapless->u64Position=pGapless->xSeconds*pTime->u64Frequency+0.5;
      // cppcheck-suppress overlappingWriteUnion
      pPause->u64Position=pPause->xSeconds*pTime->u64Frequency+0.5;
      // cppcheck-suppress overlappingWriteUnion
      pCurrent->u64Position=pCurrent->xSeconds*pTime->u64Frequency+0.5;
    }
  }
#endif // }

  if (pTime->u64Frequency) {
    // cppcheck-suppress overlappingWriteUnion
    pGapless->xSeconds=(double)pGapless->u64Position/pTime->u64Frequency;
	// cppcheck-suppress overlappingWriteUnion
    pPause->xSeconds=(double)pPause->u64Position/pTime->u64Frequency;
	// cppcheck-suppress overlappingWriteUnion
    pCurrent->xSeconds=(double)pCurrent->u64Position/pTime->u64Frequency;
  
    if (ConnectionGetFrequency(pConnect,&pTime->u64Frequency)<0) {
      DMESSAGE("getting frequency");
      goto frequency;
    }

    // cppcheck-suppress overlappingWriteUnion
    pGapless->u64Position=(UINT64)(pGapless->xSeconds*pTime->u64Frequency+0.5);
    // cppcheck-suppress overlappingWriteUnion
    pPause->u64Position=(UINT64)(pPause->xSeconds*pTime->u64Frequency+0.5);
    // cppcheck-suppress overlappingWriteUnion
    pCurrent->u64Position=(UINT64)(pCurrent->xSeconds*pTime->u64Frequency+0.5);
  }

  //difference=*pCurrent;
  //TimeSegmentSub(&difference,pPause,pTime->u64Frequency);
  TimeSegmentAdd(pPause,pCurrent,pTime->u64Frequency);
  TimeSegmentReset(pCurrent,pTime->u64Frequency);

  return 0;
frequency:
null:
invalid:
  return -1;
}

int TimeGetMS(Time *pTime, Connection *pConnect, double *pms)
{
  enum { DEBUG=3 };
  TimeSegment *pCurrent=&pTime->current;
  const TimeSegment *pPause=&pTime->pause;
  const TimeSegment *pGapless=&pTime->gapless;

  if (!pConnect) {
    DMESSAGE("null pointer connect");
    goto connect;
  }

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClient) {
    DMESSAGE("null pointer client");
    goto client;
  }

  //TimeSegmentReset(pCurrent,pTime->u64Frequency);

  if (TimeSegmentGet(pCurrent,pTime->u64Frequency,pConnect)<0) {
    DMESSAGE("getting current time");
    goto time;
  }

  if (pTime->u64Frequency) {
    *pms=1000.0*(pCurrent->u64Position+pPause->u64Position
        -pGapless->u64Position)/pTime->u64Frequency;
  }
  else
    *pms=1000.0*(pCurrent->xSeconds+pPause->xSeconds
        -pGapless->xSeconds);

  DPRINTF(DEBUG,"  %s: %.0f\n",__func__,*pms+0.5);

  return 0;
time:
client:
invalid:
connect:
  return -1;
}
