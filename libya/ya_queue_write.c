/*
 * ya_queue_write.c
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
static HANDLE WriteGetSemaphoreDown(const Queue *pQueue)
{
  return pQueue->hAvailable;
}

static HANDLE WriteGetSemaphoreUp(const Queue *pQueue)
{
  return pQueue->hWritten;
}

static BOOL WriteIsAlertable(const Queue *pQueue)
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
  WriteGetRequest,
#if defined (YA_DEBUG) // {
  QueueDecAvailable,
  QueueNopAvailable,
#endif // }
};
