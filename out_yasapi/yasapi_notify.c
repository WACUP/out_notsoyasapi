/*
 * yasapi_notification.c
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

#if defined (YASAPI_NOTIFY) // {
#define YASAPI_NOTIFY_DEBUG   0

typedef struct _PlayerNotificationClient PlayerNotificationClient;

struct _PlayerNotificationClient {
  CONST_VTBL IMMNotificationClientVtbl *lpVtbl;
  LONG lRefCount;
  Player *pPlayer;
};

static HRESULT STDMETHODCALLTYPE PlayerNotifyQueryInterface( 
            IMMNotificationClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
static ULONG STDMETHODCALLTYPE PlayerNotifyAddRef( 
            IMMNotificationClient * This);
static ULONG STDMETHODCALLTYPE PlayerNotifyRelease( 
            IMMNotificationClient * This);
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDeviceStateChanged( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId,
            /* [annotation][in] */ 
            __in  DWORD dwNewState);
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDeviceAdded( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId);
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDeviceRemoved( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId);
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDefaultDeviceChanged( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  EDataFlow flow,
            /* [annotation][in] */ 
            __in  ERole role,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDefaultDeviceId);
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnPropertyValueChanged( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId,
            /* [annotation][in] */ 
            __in  const PROPERTYKEY key);

static IMMNotificationClientVtbl gPlayerNotificationClientVtbl={
  PlayerNotifyQueryInterface, 
  PlayerNotifyAddRef, 
  PlayerNotifyRelease, 
  PlayerNotifyOnDeviceStateChanged, 
  PlayerNotifyOnDeviceAdded, 
  PlayerNotifyOnDeviceRemoved, 
  PlayerNotifyOnDefaultDeviceChanged, 
  PlayerNotifyOnPropertyValueChanged
};

static HRESULT STDMETHODCALLTYPE PlayerNotifyQueryInterface( 
            IMMNotificationClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject)
{
  return E_NOINTERFACE;
}
        
static ULONG STDMETHODCALLTYPE PlayerNotifyAddRef( 
            IMMNotificationClient * This)
{
  PlayerNotificationClient *pNotify=(PlayerNotificationClient *)This;

  DPRINTF(YASAPI_NOTIFY_DEBUG,"  >> %s <<\n",__func__);

  return InterlockedIncrement(&pNotify->lRefCount);
};
        
static ULONG STDMETHODCALLTYPE PlayerNotifyRelease( 
            IMMNotificationClient * This)
{
  PlayerNotificationClient *pNotify=(PlayerNotificationClient *)This;

  LONG lRefCount=InterlockedDecrement(&pNotify->lRefCount);

  DPRINTF(YASAPI_NOTIFY_DEBUG,"  >> %s <<\n",__func__);

  if (0==lRefCount)
    YA_FREE(pNotify);

  return lRefCount;
}
        
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDeviceStateChanged( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId,
            /* [annotation][in] */ 
            __in  DWORD dwNewState)
{
#if defined (YA_DEBUG) // {
  wchar_t  *pszState=L"?????";
#endif // }

#if defined (YA_DEBUG) // {
  switch (dwNewState) {
  case DEVICE_STATE_ACTIVE:
    pszState=L"ACTIVE";
    break;
  case DEVICE_STATE_DISABLED:
    pszState=L"DISABLED";
    break;
  case DEVICE_STATE_NOTPRESENT:
    pszState=L"NOTPRESENT";
    break;
  case DEVICE_STATE_UNPLUGGED:
    pszState=L"UNPLUGGED";
    break;
  default:
    break;
  }

  DWPRINTF(YASAPI_NOTIFY_DEBUG,L"  >> %s (%s) <<\n",__wfunc__,pszState);
  DWPRINTF(YASAPI_NOTIFY_DEBUG,L"  id: %s\n",
      pwstrDeviceId?pwstrDeviceId:L"NULL");
#endif // }

  return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDeviceAdded( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId)
{
  DPRINTF(YASAPI_NOTIFY_DEBUG,"  >> %s <<\n",__func__);

  return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDeviceRemoved( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId)
{
  DPRINTF(YASAPI_NOTIFY_DEBUG,"  >> %s <<\n",__func__);

  return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnDefaultDeviceChanged( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  EDataFlow flow,
            /* [annotation][in] */ 
            __in  ERole role,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDefaultDeviceId)
{
  PlayerNotificationClient *pNotify=(PlayerNotificationClient *)This;
  Player *pPlayer=pNotify->pPlayer;
#if defined (YA_DEBUG) // {
  wchar_t *pszFlow=L"?????";
  wchar_t *pszRole=L"?????";

  switch (flow) {
  case eRender:
    pszFlow=L"eRender";
    break;
  case eCapture:
    pszFlow=L"eCapture";
    break;
  default:
    break;
  }

  switch (role) {
  case eConsole:
    pszRole=L"eConsole";
    break;
  case eMultimedia:
    pszRole=L"eMultimedia";
    break;
  case eCommunications:
    pszRole=L"eCommunications";
    break;
  default:
    break;
  }

  DWPRINTF(0,L"  >> %s (%s, %s) <<\n",__wfunc__,pszFlow,pszRole);
  DWPRINTF(YASAPI_NOTIFY_DEBUG,L"  id: %s\n",
      pwstrDefaultDeviceId?pwstrDefaultDeviceId:L"NULL");
#endif // }

  if (eRender==flow&&eCommunications==role) {
    if (NULL==pPlayer->device.szId) {
      DPUTS(0,"  GOING TO CLOSE\n");
      PLAYER_SEND(pPlayer,PlayerClose);
    }
    else if (0!=wcscmp(pPlayer->device.szId,pwstrDefaultDeviceId)) {
      DPUTS(0,"  GOING TO MIGRATE\n");
      PLAYER_SEND(pPlayer,PlayerMigrate,pwstrDefaultDeviceId);
    }
  }

  return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE PlayerNotifyOnPropertyValueChanged( 
            IMMNotificationClient * This,
            /* [annotation][in] */ 
            __in  LPCWSTR pwstrDeviceId,
            /* [annotation][in] */ 
            __in  const PROPERTYKEY key)
{
  DPRINTF(YASAPI_NOTIFY_DEBUG,"  >> %s <<\n",__func__);

  return S_OK;
}

int PlayerAddNotify(Player *pPlayer)
{
  IMMDeviceEnumerator *pEnumerator=pPlayer->run.pEnumerator;
  PlayerNotificationClient *pNotify;
  HRESULT hr;

  DPRINTF(0,"  >> %s <<\n",__func__);

  if (NULL==(pNotify=YA_MALLOC(sizeof *pNotify))) {
    DMESSAGE("allocating notification client\n");
    goto malloc;
  }

  ZeroMemory(pNotify,sizeof *pNotify);
  pNotify->lpVtbl=&gPlayerNotificationClientVtbl;
  pNotify->lRefCount=1;
  pNotify->pPlayer=pPlayer;

  hr=pEnumerator->lpVtbl->RegisterEndpointNotificationCallback(pEnumerator,
    (IMMNotificationClient *)pNotify  // [in] IMMNotificationClient *pNotify
  );

  if (FAILED(hr)) {
    DERROR(E_POINTER,hr);
    DERROR(E_OUTOFMEMORY,hr);
    DUNKNOWN(hr);
    DMESSAGEV("adding notifiy");
    goto add;
  }

  pPlayer->run.pNotify=(IMMNotificationClient *)pNotify;
  DPUTS(0,"  notify added\n");

  return 0;
add:
  YA_FREE(pNotify);
malloc:
  return -1;
}

void PlayerRemoveNotify(Player *pPlayer)
{
  IMMDeviceEnumerator *pEnumerator=pPlayer->run.pEnumerator;
  IMMNotificationClient *pNotify=pPlayer->run.pNotify;

  DPRINTF(0,"  >> %s <<\n",__func__);

  pEnumerator->lpVtbl->UnregisterEndpointNotificationCallback(pEnumerator,
    (IMMNotificationClient *)pNotify  // [in] IMMNotificationClient *pNotify
  );

  pNotify->lpVtbl->Release(pNotify);
}
#endif // }
