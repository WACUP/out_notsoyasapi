/*
 * yasapi_plugin_device.c
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

int PlayerDeviceCreate(PlayerDevice *pPlayerDevice, LPCWSTR pcstrId,
    IMMDeviceEnumerator *pEnumerator)
{
  LPWSTR pstrId=NULL;
  IMMDevice *pDevice=NULL;
  HRESULT hr;

  DPRINTF(0,"  > %s <\n",__func__);

  if (!(pcstrId > (LPWSTR)65536)||!pcstrId[0]) {
    // for some WINE based setups this may not be working
    // so we'll abort instead of continuing & crashing...
    if (!pEnumerator) {
      goto device;
    }
    ///////////////////////////////////////////////////////////////////////////
    hr=pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator,
      eRender,              // [in]  EDataFlow dataFlow,
      eMultimedia,          // [in]  ERole     role,
      &pDevice              // [out] IMMDevice **ppDevice
    );

    if (FAILED(hr)) {
      DERROR(E_POINTER,hr,device);
      DERROR(E_INVALIDARG,hr,device);
      DERROR(E_NOTFOUND,hr,device);
      DERROR(E_OUTOFMEMORY,hr,device);
      DUNKNOWN(hr);
      DMESSAGE("getting the default audio device");
      goto device;
    }

    DPRINTF(0,"  got the the default audio device\n");
    pPlayerDevice->pDevice=pDevice;

    ///////////////////////////////////////////////////////////////////////////
    hr=pDevice->lpVtbl->GetId(pDevice,
      &pstrId               // [out] LPWSTR *ppstrId
    );

    if (FAILED(hr)) {
      DERROR(E_OUTOFMEMORY,hr,id);
      DERROR(E_POINTER,hr,id);
      DUNKNOWN(hr);
      DMESSAGE("getting endpoint ID string");
      goto id;
    }
  
    DPUTS(0,"  got the endpoint ID string\n");

    if (pPlayerDevice->szId) {
      MemFreeCOM(pPlayerDevice->szId);
    }
    pPlayerDevice->szId=pstrId;
  }
  else {
    const size_t uLen = (wcslen(pcstrId) + 1);
    pPlayerDevice->pDevice=NULL;
    if (pPlayerDevice->szId) {
      MemFreeCOM(pPlayerDevice->szId);
    }
    pPlayerDevice->szId=CoTaskMemAlloc(uLen * 2);
    if (pPlayerDevice->szId) {
      StringCchCopy(pPlayerDevice->szId, uLen, pcstrId);
    }
  }

  return 0;
id:
  pDevice->lpVtbl->Release(pDevice);
device:
  return -1;
}

int PlayerDeviceDestroy(PlayerDevice *pPlayerDevice)
{
  IMMDevice *pDevice=pPlayerDevice->pDevice;

  DPRINTF(0,"  > %s <\n",__func__);

  if (pDevice) {
    DPUTS(0,"  destroying device\n");
    if (!pDevice->lpVtbl->Release(pDevice))
    {
      pDevice = NULL;
    }
  }

  memset(pPlayerDevice, 0, sizeof * pPlayerDevice);

  return 0;
}

int PlayerDeviceGet(PlayerDevice *pPlayerDevice,
    IMMDeviceEnumerator *pEnumerator)
{
  IMMDevice *pDevice=NULL;
  HRESULT hr;

  DPRINTF(0,"  > %s <\n",__func__);

  if(!pPlayerDevice->pDevice) {
    // for some WINE based setups this may not be working
    // so we'll abort instead of continuing & crashing...
    if (!pEnumerator) {
      goto device;
    }
    ///////////////////////////////////////////////////////////////////////////
    hr=pEnumerator->lpVtbl->GetDevice(pEnumerator,
      pPlayerDevice->szId,  // [in]  LPCWSTR   pwstrId,
      &pDevice              // [out] IMMDevice **ppDevice
    );

    if (FAILED(hr)) {
      DERROR(E_POINTER,hr,device);
      DERROR(E_NOTFOUND,hr,device);
      DERROR(E_OUTOFMEMORY,hr,device);
      DUNKNOWN(hr);
      DWMESSAGEV(L"getting the audio device %s",pPlayerDevice->szId);
      goto device;
    }

    DPRINTF(0,"  got the the audio device\n");
    pPlayerDevice->pDevice=pDevice;
  }

  return 0;
device:
  return -1;
}

int PlayerDeviceCreateV(Player *pPlayer, Request *pRequest)
{
  return PlayerDeviceCreate(&pPlayer->device,pPlayer->options.common.szId,
                            pPlayer->run.pEnumerator);
}

int PlayerDeviceDestroyV(Player *pPlayer, Request *pRequest)
{
  return PlayerDeviceDestroy(&pPlayer->device);
}
