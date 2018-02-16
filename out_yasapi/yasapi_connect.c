/*
 * yasapi_connect.c
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

int ConnectionGetPosition(Connection *pConnect, UINT64 *pu64Position)
{
  IAudioClock *pClock=pConnect->pClock;
  HRESULT hr;

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClock) {
    DMESSAGE("null pointer");
    goto null;
  }

  // get position /////////////////////////////////////////////////////////////
  hr=pClock->lpVtbl->GetPosition(pClock,
    pu64Position,         // [out] UINT64 *pu64Position,
    NULL                  // [out] UINT64 *pu64QPCPosition
  );

  if (FAILED(hr)) {
    if (hr==AUDCLNT_E_DEVICE_INVALIDATED&&ConnectionSetInvalid(pConnect,1)) {
      DWARNINGV("invalid device on getting position in %s",__func__);
      goto invalid;
    }

    DERROR(E_POINTER,hr);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr);
    DUNKNOWN(hr);
    DMESSAGE("getting position");
    goto position;
  }

invalid:
  return 0;
position:
null:
  return -1;
}

int ConnectionGetFrequency(Connection *pConnect, UINT64 *pu64Frequency)
{
  IAudioClock *pClock=pConnect->pClock;
  HRESULT hr;

  if (ConnectionIsInvalid(pConnect)) {
    DWARNINGV("invalid connection in %s",__func__);
    goto invalid;
  }

  if (!pConnect->pClock) {
    DMESSAGE("null pointer");
    goto null;
  }

  // get frequency ////////////////////////////////////////////////////////////
  hr=pClock->lpVtbl->GetFrequency(pClock,
    pu64Frequency         // [out] UINT64 *pu64Frequency
  );

  if (FAILED(hr)) {
    if (hr==AUDCLNT_E_DEVICE_INVALIDATED&&ConnectionSetInvalid(pConnect,1)) {
      DWARNINGV("invalid device on getting frequency in %s",__func__);
      goto invalid;
    }

    DERROR(E_POINTER,hr);
    DERROR(AUDCLNT_E_DEVICE_INVALIDATED,hr);
    DERROR(AUDCLNT_E_SERVICE_NOT_RUNNING,hr);
    DUNKNOWN(hr);
    DMESSAGE("getting frequency");
    goto frequency;
  }
  else if (*pu64Frequency==0) {
    DMESSAGE("zero frequency");
    goto zero;
  }
invalid:
  return 0;
zero:
frequency:
null:
  return -1;
}

int ConnectionIsInvalid(Connection *pConnect)
{
#if defined (YASAPI_NOTIFY) // {
  return pConnect->bInvalid;
#else // } {
  return 0;
#endif // }
}

int ConnectionSetInvalid(Connection *pConnect, int bInvalid)
{
#if defined (YASAPI_NOTIFY) // {
  return pConnect->bInvalid=bInvalid;
#else // } {
  return 0;
#endif // }
}
