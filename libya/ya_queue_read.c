/*
 * ya_queue_read.c
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
  ReadGetRequest,
#if defined (YA_DEBUG) // {
  QueueNopAvailable,
  QueueIncAvailable,
#endif // }
};
