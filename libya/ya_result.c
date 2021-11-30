/*
 * ya_result.c
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
int ResultCreate(Result *pResult)
{
  /////////////////////////////////////////////////////////////////////////////
  pResult->prPrev=NULL;

  /////////////////////////////////////////////////////////////////////////////
  pResult->hEvent=CreateEvent(
    0,        // _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
    FALSE,    // _In_     BOOL                  bManualReset,
    FALSE,    // _In_     BOOL                  bInitialState,
    NULL      // _In_opt_ LPCTSTR               lpName
  );

  if (NULL==pResult->hEvent) {
    DMESSAGE("creating result event");
    goto event;
  }
  
  DPUTS(2,"    result event created\n");

  /////////////////////////////////////////////////////////////////////////////
  return 0;
// cleanup:
  //CloseHandle(pResult->hEvent);
event:
  return -1;
}

void ResultDestroy(Result *pResult)
{
  DPUTS(2,"    destroying result event\n");
  if (pResult->hEvent) {
  CloseHandle(pResult->hEvent);
    pResult->hEvent = NULL;
  }
}
