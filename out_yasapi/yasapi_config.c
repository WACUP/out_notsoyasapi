/*
 * yasapi_config.c
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
#if ! defined (PKEY_Device_FriendlyName) // {
#include <FunctionDiscoveryKeys_devpkey.h>
#endif // }
#include <resource.h>
#define WA_UTILS_SIMPLE
#include <loader/loader/utils.h>

///////////////////////////////////////////////////////////////////////////////
typedef enum _Pages Pages;
typedef enum _PageControl PageControl;
typedef struct _PageVMT PageVMT;
typedef struct _PageTemplate PageTemplate;
typedef struct _Page Page;
typedef struct _Config Config;
typedef struct _ConfigDevice ConfigDevice;

enum _Pages {
  PAGE_COMMON,
  PAGE_DEVICE,
  PAGE_BUFFERS,
  NUM_PAGES
};

enum _PageControl {
  PAGE_DEBUG=PAGE_COMMON,
  PAGE_SHARE_MODE=PAGE_DEVICE,
  PAGE_PULL=PAGE_DEVICE,
#if defined (YASAPI_GAPLESS) // {
  PAGE_GAPLESS=PAGE_COMMON,
  PAGE_GAPLESS_OFFSET=PAGE_DEVICE,
#endif // }
#if defined (YASAPI_BALANCE) // {
  PAGE_BALANCE=PAGE_DEVICE,
#endif // }
  PAGE_SRC=PAGE_DEVICE
};

///////////////////////////////////////////////////////////////////////////////
struct _PageVMT {
  const char *name;
  DLGPROC lpDialogFunc;
  void (*Set)(Page *pPage, Config *pConfig);
  void (*Get)(Page *pPage, Config *pConfig);
  void (*Reset)(Page *pPage, Config *pConfig);
};

struct _PageTemplate {
  UINT label;
  UINT idd;
  PageVMT *(*GetVMT)(void);
};

static PageVMT *GetCommonVMT(void);
static PageVMT *GetDeviceVMT(void);
static PageVMT *GetBuffersVMT(void);

static PageTemplate gaTemplates[]={
  { IDS_COMMON_OPTIONS,IDD_PAGE_COMMON,GetCommonVMT },
  { IDS_DEVICE_OPTIONS,IDD_PAGE_DEVICE,GetDeviceVMT },
  { IDS_BUFFER_OPTIONS,IDD_PAGE_BUFFERS,GetBuffersVMT }
};

///////////////////////////////////////////////////////////////////////////////
struct _Page {
  PageVMT *vmt;
  Config *pConfig;
  HWND hDlg;
};

///////////////////////////////////////////////////////////////////////////////
struct _Config {
  Player *pPlayer;
  //HMODULE hModule;
  Options options;
  IMMDeviceEnumerator *pEnumerator;
  IMMDeviceCollection *pCollection;
  UINT nDevices;
  HWND hDlg;
  HWND hTabCtlPage;
  Page aPages[NUM_PAGES];
};

///////////////////////////////////////////////////////////////////////////////
static void SetShareMode(HWND hDlg, int eShareMode)
{
	SendDlgItemMessage(hDlg,IDC_COMBOBOX_SHAREMODE,CB_SETCURSEL,eShareMode,0);
}

static void SetPull(HWND hDlg, int bPull)
{
	SendDlgItemMessage(hDlg,IDC_COMBOBOX_PULL,CB_SETCURSEL,bPull,0);
}

static int GetShareMode(HWND hDlg)
{
  return SendDlgItemMessage(hDlg,IDC_COMBOBOX_SHAREMODE,CB_GETCURSEL,0,0);
}

static int GetPull(HWND hDlg)
{
  return SendDlgItemMessage(hDlg,IDC_COMBOBOX_PULL,CB_GETCURSEL,0,0);
}

static int GetAutoConvertPCM(HWND hDlg)
{
  return IsDlgButtonChecked(hDlg,IDC_CHECKBOX_AUTOCONVERT_PCM);
}

#if defined (YASAPI_GAPLESS) // {
static int GetGapless(HWND hDlg)
{
  return IsDlgButtonChecked(hDlg,IDC_CHECKBOX_GAPLESS);
}
#endif // }

#if defined (YASAPI_BALANCE) // {
static int GetBalance(HWND hDlg)
{
  return IsDlgButtonChecked(hDlg,IDC_CHECKBOX_BALANCE);
}
#endif // }

////////////////
static void SyncDevicePeriod(HWND hDlg, int eShareMode, int bPull)
{
  BOOL bShare=YASAPI_SHAREMODE_SHARE==eShareMode;
  BOOL bDevicePeriod=!(bShare&&bPull);

  EnableControl(hDlg,IDC_STATIC_LABEL_DEVICE_PERIOD,bDevicePeriod);
  EnableControl(hDlg,IDC_COMBOBOX_DEVICE_PERIOD,bDevicePeriod);
  EnableControl(hDlg,IDC_SLIDER_SHARE_SIZE,bDevicePeriod);
  EnableControl(hDlg,IDC_STATIC_SHARE_SIZE,bDevicePeriod);
}

static void SyncSRC(HWND hDlg, int eShareMode, int bAutoConvertPCM)
{
  if (YASAPI_SHAREMODE_EXCLUSIVE==eShareMode) {
    EnableControl(hDlg,IDC_CHECKBOX_AUTOCONVERT_PCM,FALSE);
	EnableControl(hDlg,IDC_CHECKBOX_SRC_DEFAULT_QUALITY,FALSE);
  }
  else {
    EnableControl(hDlg,IDC_CHECKBOX_AUTOCONVERT_PCM,TRUE);
	EnableControl(hDlg,IDC_CHECKBOX_SRC_DEFAULT_QUALITY,bAutoConvertPCM);
  }
}

#if 0 // {
#if defined (YASAPI_GAPLESS) // {
static void SyncGapless(Config *pConfig)
{
  HWND hDlg=pConfig->aPages[PAGE_GAPLESS_OFFSET].hDlg;
  int bGapless=GetGapless(pConfig->aPages[PAGE_GAPLESS].hDlg);

  EnableControl(hDlg,IDC_RADIOBUTTON_GAPLESS_OFFSET_POSITION,bGapless);
  EnableControl(hDlg,IDC_RADIOBUTTON_GAPLESS_OFFSET_TIME,bGapless);
}
#endif // }
#endif // }

#if defined (YASAPI_BALANCE) // {
static void SyncBalance(HWND hDlg, int bPull, int bBalance)
{

  if (bPull) {
    EnableControl(hDlg,IDC_CHECKBOX_BALANCE,FALSE);
	EnableControl(hDlg,IDC_SLIDER_BALANCE_START,FALSE);
	EnableControl(hDlg,IDC_STATIC_BALANCE_START,FALSE);
  }
  else {
    EnableControl(hDlg,IDC_CHECKBOX_BALANCE,TRUE);
	EnableControl(hDlg,IDC_SLIDER_BALANCE_START,bBalance);
	EnableControl(hDlg,IDC_STATIC_BALANCE_START,bBalance);
  }
}
#endif // }

////////////////
static void ConfigSetGlobalShareMode(Config *pConfig, int eShareMode)
{
  SetShareMode(pConfig->hDlg,eShareMode);
}

static void DeviceSetShareMode(Config *pConfig, int eShareMode)
{
  SetShareMode(pConfig->aPages[PAGE_SHARE_MODE].hDlg,eShareMode);
}

static void ConfigSetGlobalPull(Config *pConfig, int bPull)
{
  SetPull(pConfig->hDlg,bPull);
}

static void DeviceSetPull(Config *pConfig, int bPull)
{
  SetPull(pConfig->aPages[PAGE_PULL].hDlg,bPull);
}

static int ConfigGetShareMode(Config *pConfig)
{
  return GetShareMode(pConfig->aPages[PAGE_SHARE_MODE].hDlg);
}

static int ConfigGetPull(Config *pConfig)
{
  return GetPull(pConfig->aPages[PAGE_PULL].hDlg);
}

static int ConfigGetAutoConvertPCM(Config *pConfig)
{
  return GetAutoConvertPCM(pConfig->aPages[PAGE_SRC].hDlg);
}

#if defined (YASAPI_BALANCE) // {
static int ConfigGetBalance(Config *pConfig)
{
  return GetBalance(pConfig->aPages[PAGE_BALANCE].hDlg);
}
#endif // }

////////////////
static void ConfigSyncDevicePeriod(Config *pConfig, int eShareMode, int bPull)
{
  SyncDevicePeriod(pConfig->aPages[PAGE_BUFFERS].hDlg,eShareMode,bPull);
}

static void ConfigSyncSRC(Config *pConfig, int eShareMode, int bAutoConvertPCM)
{
  SyncSRC(pConfig->aPages[PAGE_SRC].hDlg,eShareMode,bAutoConvertPCM);
}

#if defined (YASAPI_BALANCE) // {
static void ConfigSyncBalance(Config *pConfig, int bPull, int bBalance)
{
  SyncBalance(pConfig->aPages[PAGE_BALANCE].hDlg,bPull,bBalance);
}
#endif // }

////////////////
static void ConfigOnDevicePeriod(Config *pConfig)
{
  int eShareMode=ConfigGetShareMode(pConfig);
  int bPull=ConfigGetPull(pConfig);

  ConfigSyncDevicePeriod(pConfig,eShareMode,bPull);
}

static void ConfigOnSRC(Config *pConfig)
{
  int eShareMode=ConfigGetShareMode(pConfig);
  int bAutoConvertPCM=ConfigGetAutoConvertPCM(pConfig);

  ConfigSyncSRC(pConfig,eShareMode,bAutoConvertPCM);
}

#if defined (YASAPI_BALANCE) // {
static void ConfigOnBalance(Config *pConfig)
{
  int bPull=ConfigGetPull(pConfig);
  int bBalance=ConfigGetBalance(pConfig);

  ConfigSyncBalance(pConfig,bPull,bBalance);
}
#endif // }

static void ConfigEnableShareMode(Config *pConfig, int eShareMode,
    int bPull, int bAutoConvertPCM)
{
  ConfigSyncSRC(pConfig,eShareMode,bAutoConvertPCM);
  ConfigSyncDevicePeriod(pConfig,eShareMode,bPull);
}

static void ConfigOnShareMode(Config *pConfig)
{
  int eShareMode=ConfigGetShareMode(pConfig);
  int bPull=ConfigGetPull(pConfig);
  int bAutoConvertPCM=ConfigGetAutoConvertPCM(pConfig);

  ConfigEnableShareMode(pConfig,eShareMode,bPull,bAutoConvertPCM);
}

#if defined (YASAPI_BALANCE) // {
static void ConfigEnablePull(Config *pConfig, int bPull, int bBalance,
    int eShareMode)
#else // } {
static void ConfigEnablePull(Config *pConfig, int bPull, int eShareMode)
#endif // }
{
#if defined (YASAPI_BALANCE) // {
  ConfigSyncBalance(pConfig,bPull,bBalance);
#endif // }
  ConfigSyncDevicePeriod(pConfig,eShareMode,bPull);
}

static void ConfigOnPull(Config *pConfig)
{
  int bPull=ConfigGetPull(pConfig);
#if defined (YASAPI_BALANCE) // {
  int bBalance=ConfigGetBalance(pConfig);
#endif // }
  int eShareMode=ConfigGetShareMode(pConfig);

#if defined (YASAPI_BALANCE) // {
  ConfigEnablePull(pConfig,bPull,bBalance,eShareMode);
#else // } {
  ConfigEnablePull(pConfig,bPull,eShareMode);
#endif // }
}

///////////////////////////////////////////////////////////////////////////////
struct _ConfigDevice {
  int nDevice;
  IMMDevice *pDevice;
  LPWSTR pstrId;
  IPropertyStore *pProperties;
  PROPVARIANT vName;
};

static ConfigDevice *ConfigDeviceNew(Config *pConfig, int nDevice)
{
  ConfigDevice *pConfigDevice = 0;
  IMMDevice *pDevice = 0;
  LPWSTR pstrId = 0;
  IPropertyStore *pProperties = 0;
  HRESULT hr;

  if (NULL==(pConfigDevice=malloc(sizeof *pConfigDevice)))
    goto malloc;

  pConfigDevice->nDevice=nDevice;

  if (0==nDevice) {
    /////////////////////////////////////////////////////////////////////////
	IMMDeviceEnumerator *pEnumerator = pConfig->pEnumerator;
    hr=pEnumerator->lpVtbl->GetDefaultAudioEndpoint(pEnumerator,
      eRender,        // [in]  EDataFlow dataFlow,
      eMultimedia,    // [in]  ERole     role,
      &pDevice        // [out] IMMDevice **ppDevice
    );

    if (FAILED(hr)) {
      DERROR(E_POINTER,hr,device);
      DERROR(E_INVALIDARG,hr,device);
      DERROR(E_NOTFOUND,hr,device);
      DERROR(E_OUTOFMEMORY,hr,device);
      DUNKNOWN(hr);
      DMESSAGE("getting the default device");
      goto device;
    }

    DPUTS(3,"  got the default device.\n");
    pConfigDevice->pDevice=pDevice;
  }
  else {
    /////////////////////////////////////////////////////////////////////////
	IMMDeviceCollection *pCollection = pConfig->pCollection;
	hr = pCollection->lpVtbl->Item(pCollection,
		nDevice - 1,      // [in]  UINT      nDevice,
		&pDevice        // [out] IMMDevice **ppDevice
	);

    if (FAILED(hr)) {
      DERROR(E_POINTER,hr,device);
      DERROR(E_INVALIDARG,hr,device);
      DUNKNOWN(hr);
      DMESSAGEV("getting the %d. device",nDevice);
      goto device;
    }

    DPRINTF(3,"  got the %d. device.\n",nDevice);
    pConfigDevice->pDevice=pDevice;
  }

  /////////////////////////////////////////////////////////////////////////////
  hr=pDevice->lpVtbl->GetId(pDevice,
    &pstrId         // [out] LPWSTR *ppstrId
  );

  if (FAILED(hr)) {
    DERROR(E_OUTOFMEMORY,hr,id);
    DERROR(E_POINTER,hr,id);
    DUNKNOWN(hr);
    DMESSAGEV("getting the %d. device's id",nDevice);
    goto id;
  }

  DWPRINTF(3,L"  got the %d. device's id: %s.\n",nDevice,pstrId);
  pConfigDevice->pstrId=pstrId;

  /////////////////////////////////////////////////////////////////////////////
  hr=pDevice->lpVtbl->OpenPropertyStore(pDevice,
    STGM_READ,      // [in]  DWORD          stgmAccess,
    &pProperties    // [out] IPropertyStore **ppProperties
  );

  if (FAILED(hr)) {
    DERROR(E_INVALIDARG,hr,properties);
    DERROR(E_POINTER,hr,properties);
    DERROR(E_OUTOFMEMORY,hr,properties);
    DMESSAGEV("getting the %d. device's property store",nDevice);
    goto properties;
  }

  DPRINTF(3,"  got the %d. device's property store\n",nDevice);
  pConfigDevice->pProperties=pProperties;

  /////////////////////////////////////////////////////////////////////////////
  hr=pProperties->lpVtbl->GetValue(pProperties,
    &PKEY_Device_FriendlyName,    // [in]  REFPROPERTYKEY key,
    &pConfigDevice->vName         // [out] PROPVARIANT    *pv
  );

  if (FAILED(hr)) {
    DERROR(E_OUTOFMEMORY,hr,name);
    DERROR(E_POINTER,hr,name);
    DUNKNOWN(hr);
    DMESSAGEV("getting the %d. device's name",nDevice);
    goto name;
  }

  DWPRINTF(3,L"  got the %d. device's name: %s.\n",nDevice,
      pConfigDevice->vName.pwszVal);

  return pConfigDevice;
//cleanup:
  //PropVariantClear(&pConfigDevice->vName);
name:
  pProperties->lpVtbl->Release(pProperties);
properties:
  pDevice->lpVtbl->Release(pDevice);
id:
  CoTaskMemFree(pstrId);
device:
  free(pConfigDevice);
malloc:
  return NULL;
}

static void ConfigDeviceDelete(ConfigDevice *pConfigDevice)
{
  IMMDevice *pDevice=pConfigDevice->pDevice;
  LPWSTR pstrId=pConfigDevice->pstrId;
  IPropertyStore *pProperties=pConfigDevice->pProperties;

  PropVariantClear(&pConfigDevice->vName);
  pProperties->lpVtbl->Release(pProperties);
  pDevice->lpVtbl->Release(pDevice);
  CoTaskMemFree(pstrId);
  free(pConfigDevice);
}

// set progress bar.
static void ConfigUpdateProgress(HWND hWnd, WORD wParam)
{
  LRESULT lMin=SendMessage(hWnd,PBM_GETRANGE,TRUE,0);
  LRESULT lMax=SendMessage(hWnd,PBM_GETRANGE,FALSE,0);
  WORD wPos=lMin+MulDiv(lMax-lMin,wParam,USHRT_MAX);

  SendMessage(hWnd,PBM_SETPOS,wPos,0);
}

static void ConfigSyncVisualization(HWND hDlg, Config *pConfig)
{
  Player *pPlayer=pConfig->pPlayer;
  BOOL bVisualization=IsDlgButtonChecked(hDlg,IDC_CHECKBOX_VISUALIZATION);
  HWND hWndRing=GetDlgItem(pConfig->hDlg,IDC_PROGRESSBAR_RING);
  HWND hWndShared=GetDlgItem(pConfig->hDlg,IDC_PROGRESSBAR_SHARED);

  if (!bVisualization) {
    // clear progress bars.
    ConfigUpdateProgress(hWndRing,0);
    ConfigUpdateProgress(hWndShared,0);
  }

  EnableControl(pConfig->hDlg,IDC_STATIC_RING,bVisualization);
  EnableWindow(hWndRing,bVisualization);
  EnableControl(pConfig->hDlg,IDC_STATIC_SHARED,bVisualization);
  EnableWindow(hWndShared,bVisualization);

  pConfig->options.common.bVisualization=bVisualization;
  pPlayer->options.common.bVisualization=bVisualization;
}

#if 0 // {
#if defined (YA_DEBUG) // {
static LRESULT ConfigEnableDebug(HWND hDlg, LRESULT nDebug)
{
  int bFile=IsDlgButtonChecked(hDlg,IDC_CHECKBOX_FILE);

  EnableControl(hDlg,IDC_CHECKBOX_FILE,nDebug);
  EnableControl(hDlg,IDC_STATIC_LABEL_SLEEP,!bFile&&nDebug);
  EnableControl(hDlg,IDC_COMBOBOX_SLEEP,!bFile&&nDebug);

  return nDebug;
}

static int64_t PlayerWriteDebug(Player *pPlayer, Request *pRequest)
{
  return TraceSwitch(&trace);
}

static void ConfigSyncDebug(Config *pConfig, HWND hDlg)
{
  Player *pPlayer=pConfig->pPlayer;

  ControlsGet(gcaTraceControls,hDlg,&trace);
  ConfigEnableDebug(hDlg,trace.nDebug);
  PLAYER_SEND(pPlayer,PlayerWriteDebug);
}
#endif // }
#endif // }

///////////////////////////////////////////////////////////////////////////////
#if defined (YASAPI_CHECK_UNDERFOW) // {
static void CommonSyncCheckUndeflow(HWND hDlg)
{
  int bGapless=IsDlgButtonChecked(hDlg,IDC_CHECKBOX_GAPLESS);
  int bDisconnect=IsDlgButtonChecked(hDlg,IDC_CHECKBOX_DISCONNECT);

  if (!bGapless) {
    EnableControl(hDlg,IDC_CHECKBOX_DISCONNECT,FALSE);
	EnableControl(hDlg,IDC_COMBOBOX_CHECK_UNDERFLOW,FALSE);
  }
  else {
    EnableControl(hDlg,IDC_CHECKBOX_DISCONNECT,TRUE);
	EnableControl(hDlg,IDC_COMBOBOX_CHECK_UNDERFLOW,bDisconnect);
  }
}
#endif // }

static INT_PTR CALLBACK CommonProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
  Config *pConfig=(Config *)GetWindowLongPtrW(hDlg,GWLP_USERDATA);

  switch (uMsg) {
  case WM_INITDIALOG:
    SetWindowLongPtrW(hDlg,GWLP_USERDATA,lParam);
    pConfig=(Config *)lParam;
#if 0 // {
#if defined (YA_DEBUG) // {
    ControlsInit(gcaTraceControls,hDlg,pConfig->hModule);
    ControlsSet(gcaTraceControls,hDlg,&trace);
#endif // }
#endif // }
#if defined (YA_DEBUG) // {
    TraceControlsInit(hDlg,pConfig->hModule);
#else // } {
    TraceControlsInit(hDlg);
#endif // }
    ControlsInit(gcaCommonControls,hDlg/*,pConfig->hModule*/);
#if ! defined (YASAPI_GAPLESS) // {
	EnableControl(hDlg,IDC_STATIC_LABEL_GAPLESS,FALSE);
	EnableControl(hDlg,IDC_CHECKBOX_GAPLESS,FALSE);
	EnableControl(hDlg,IDC_CHECKBOX_DISCONNECT,FALSE);
#if defined (YASAPI_CHECK_UNDERFLOW) // {
	EnableControl(hDlg,IDC_COMBOBOX_CHECK_UNDERFLOW,FALSE);
#else // } {
#endif // }
#endif // }
#if ! defined (YASAPI_SURROUND) // {
	EnableControl(hDlg,IDC_STATIC_LABEL_SURROUND,FALSE);
	EnableControl(hDlg,IDC_CHECKBOX_SURROUND,FALSE);
#endif // }
#if 0 // {
#if ! defined (YA_DEBUG) // {
	EnableControl(hDlg,IDC_STATIC_LABEL_DEBUG,FALSE);
	SendDlgItemMessage(hDlg,IDC_COMBOBOX_DEBUG,
        CB_ADDSTRING,0,(LPARAM)L"Off");
	SendDlgItemMessage(hDlg,IDC_COMBOBOX_DEBUG,
        CB_SETCURSEL,0,0);
	EnableControl(hDlg,IDC_COMBOBOX_DEBUG,FALSE);

	EnableControl(hDlg,IDC_STATIC_LABEL_SLEEP,FALSE);
	EnableControl(hDlg,IDC_CHECKBOX_FILE,FALSE);
	SendDlgItemMessage(hDlg,IDC_COMBOBOX_SLEEP,
        CB_ADDSTRING,0,(LPARAM)L"0 sec");
	SendDlgItemMessage(hDlg,IDC_COMBOBOX_SLEEP,
        CB_SETCURSEL,0,0);
	EnableControl(hDlg,IDC_COMBOBOX_SLEEP,FALSE);
#endif // }
#endif // }
    return TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDC_CHECKBOX_VISUALIZATION:
      if (BN_CLICKED==HIWORD(wParam)) {
        ConfigSyncVisualization(hDlg,pConfig);
        return TRUE;
	    }

      break;
#if 0 // {
#if defined (YASAPI_GAPLESS) // {
    case IDC_CHECKBOX_GAPLESS:
#if defined (YASAPI_CHECK_UNDERFLOW) // {
      CommonSyncCheckUndeflow(hDlg);
#endif // }
      SyncGapless(pConfig);
      return TRUE;
    case IDC_CHECKBOX_DISCONNECT:
#if defined (YASAPI_CHECK_UNDERFLOW) // {
      CommonSyncCheckUndeflow(hDlg);
#endif // }
      return TRUE;
#endif // }
#endif // }
#if defined (YA_DEBUG) // {
    case IDC_COMBOBOX_DEBUG:
    case IDC_CHECKBOX_FILE:
    case IDC_COMBOBOX_SLEEP:
      //ConfigSyncDebug(pConfig,hDlg);
      PlayerStubSyncDebug(pPlayer->base.pStub,pPlayer->stamp,hDlg);
      return TRUE;
#endif // }
    default:
      break; 
	  }

    break;
  case WM_HSCROLL:
#if 0 // {
#if defined (YA_DEBUG) // {
    //ControlsSync(gcaTraceControls,hDlg);
    PlayerSyncDebug(pConfig->pPlayer,hDlg);
#endif // }
#endif // }
    ControlsSync(gcaCommonControls,hDlg);
    break;
  default:
    break;
  }

  return FALSE;
}

static void CommonSet(Page *pPage, Config *pConfig)
{
#if defined (YA_DEBUG) // {
  ControlsSet(gcaTraceControls,pPage->hDlg,&trace);
#endif // }
  ControlsSet(gcaCommonControls,pPage->hDlg,&pConfig->options.common);
}

static void CommonGet(Page *pPage, Config *pConfig)
{
#if defined (YA_DEBUG) // {
  ControlsGet(gcaTraceControls,pPage->hDlg,&trace);
#endif // }
  ControlsGet(gcaCommonControls,pPage->hDlg,&pConfig->options.common);
}

static void CommonReset(Page *pPage, Config *pConfig)
{
#if defined (YA_DEBUG) // {
  ControlsSet(gcaTraceControls,pPage->hDlg,TraceDefault());
#endif // }
  ControlsSet(gcaCommonControls,pPage->hDlg,OptionsCommonDefault());
}

static PageVMT *GetCommonVMT(void)
{
  static PageVMT vmt;

  if (NULL==vmt.name) {
    vmt.name="Common";
    vmt.lpDialogFunc=CommonProc;
    vmt.Set=CommonSet;
    vmt.Get=CommonGet;
    vmt.Reset=CommonReset;
  }

  return &vmt;
}

///////////////////////////////////////////////////////////////////////////////
static INT_PTR CALLBACK DeviceProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
  Config *pConfig=(Config *)GetWindowLongPtrW(hDlg,GWLP_USERDATA);

  switch (uMsg) {
  case WM_INITDIALOG:
    pConfig=(Config *)lParam;
    SetWindowLongPtrW(hDlg,GWLP_USERDATA,lParam);
    ControlsInit(gcaDeviceControls,hDlg/*,pConfig->hModule*/);

#if ! defined (YASAPI_FORCE24BIT) // {
	ShowControl(hDlg,IDC_STATIC_LABEL_FORCE24BIT,FALSE);
	ShowControl(hDlg,IDC_CHECKBOX_FORMAT_FORCE24BIT,FALSE);
#endif // }
#if ! defined (YASAPI_GAPLESS) // {
	ShowControl(hDlg,IDC_STATIC_LABEL_GAPLESS_OFFSET,FALSE);
	ShowControl(hDlg,IDC_RADIOBUTTON_GAPLESS_OFFSET_POSITION,FALSE);
	ShowControl(hDlg,IDC_RADIOBUTTON_GAPLESS_OFFSET_TIME,FALSE);
#endif // }
#if ! defined (YASAPI_BALANCE) // {
#if 0 // {
	ShowControl(hDlg,IDC_CHECKBOX_BALANCE,FALSE);
	ShowControl(hDlg,IDC_SLIDER_BALANCE_START,FALSE);
	ShowControl(hDlg,IDC_STATIC_BALANCE_START,FALSE);
#endif // }
#endif // }
    return TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDC_CHECKBOX_AUTOCONVERT_PCM:
    case IDC_CHECKBOX_SRC_DEFAULT_QUALITY:
      ConfigOnSRC(pConfig);
      return TRUE;
#if defined (YASAPI_BALANCE) // {
    case IDC_CHECKBOX_BALANCE:
      ConfigOnBalance(pConfig);
      return TRUE;
#endif // }
    case IDC_COMBOBOX_SHAREMODE:
      ConfigSetGlobalShareMode(pConfig,GetShareMode(hDlg));
      ConfigOnShareMode(pConfig);
      return TRUE;
    case IDC_COMBOBOX_PULL:
      ConfigSetGlobalPull(pConfig,GetPull(hDlg));
      ConfigOnPull(pConfig);
      return TRUE;
    default:
      break;
    }

    break;
  case WM_HSCROLL:
    ControlsSync(gcaDeviceControls,hDlg);
    break;
  default:
    break;
  }

  return FALSE;
}

static void DeviceSet(Page *pPage, Config *pConfig)
{
  ControlsSet(gcaDeviceControls,pPage->hDlg,&pConfig->options.device);
}

static void DeviceGet(Page *pPage, Config *pConfig)
{
  ControlsGet(gcaDeviceControls,pPage->hDlg,&pConfig->options.device);
}

static void DeviceReset(Page *pPage, Config *pConfig)
{
  ControlsSet(gcaDeviceControls,pPage->hDlg,OptionsDeviceDefault());
}

static PageVMT *GetDeviceVMT(void)
{
  static PageVMT vmt;

  if (NULL==vmt.name) {
    vmt.name="Device";
    vmt.lpDialogFunc=DeviceProc;
    vmt.Set=DeviceSet;
    vmt.Get=DeviceGet;
    vmt.Reset=DeviceReset;
  }

  return &vmt;
}

///////////////////////////////////////////////////////////////////////////////
static INT_PTR CALLBACK BuffersProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
  Config *pConfig=(Config *)GetWindowLongPtrW(hDlg,GWLP_USERDATA);

  switch (uMsg) {
  case WM_INITDIALOG:
    pConfig=(Config *)lParam;
    SetWindowLongPtrW(hDlg,GWLP_USERDATA,lParam);
    ControlsInit(gcaBuffersControls,hDlg/*,pConfig->hModule*/);
    return TRUE;
  case WM_HSCROLL:
    ControlsSync(gcaBuffersControls,hDlg);
    break;
  default:
    break;
  }

  return FALSE;
}

static void BuffersSet(Page *pPage, Config *pConfig)
{
  ControlsSet(gcaBuffersControls,pPage->hDlg,&pConfig->options.device);
}

static void BuffersGet(Page *pPage, Config *pConfig)
{
  ControlsGet(gcaBuffersControls,pPage->hDlg,&pConfig->options.device);
}

static void BuffersReset(Page *pPage, Config *pConfig)
{
  ControlsSet(gcaBuffersControls,pPage->hDlg,OptionsDeviceDefault());
}

static PageVMT *GetBuffersVMT(void)
{
  static PageVMT vmt;

  if (NULL==vmt.name) {
    vmt.name="Buffers";
    vmt.lpDialogFunc=BuffersProc;
    vmt.Set=BuffersSet;
    vmt.Get=BuffersGet;
    vmt.Reset=BuffersReset;
  }

  return &vmt;
}

///////////////////////////////////////////////////////////////////////////////
static void ConfigOnSelChangeComboBox(HWND hDlg,Config *pConfig,
    HWND hComboBox);

static void ConfigOnSelChangeTabCtrl(HWND hDlg, Config *pConfig, HWND hWndTab)
{
  int nCurPage=TabCtrl_GetCurSel(hWndTab);
  Page *pPage=pConfig->aPages+nCurPage;

  if (pConfig->hTabCtlPage) {
    ShowWindow(pConfig->hTabCtlPage,FALSE);
    pConfig->hTabCtlPage=NULL;
  }

  pConfig->hTabCtlPage=pPage->hDlg;
  ShowWindow(pConfig->hTabCtlPage,TRUE);
  pConfig->options.common.nPage=nCurPage;
}

#ifndef WACUP_BUILD
static int ResizeComboBoxDropDown(HWND hParent, HWND hComboBox, const wchar_t *str, int width)
{
	SIZE size = {0};
	HDC hdc = GetDC(hComboBox);

	// get and select parent dialog's font so that it'll calculate things correctly
	HFONT font = (HFONT)SendMessage(hParent, WM_GETFONT, 0, 0),
		  oldfont = (HFONT)SelectObject(hdc, font);

	GetTextExtentPoint32(hdc, str, lstrlen(str) + 1, &size);

	if (size.cx > width)
	{
		SendMessage(hComboBox, CB_SETDROPPEDWIDTH, size.cx, 0);
	}

	SelectObject(hdc, oldfont);
	ReleaseDC(hComboBox, hdc);
	return size.cx;
}
#else
__declspec(dllimport) int _cdecl ResizeComboBoxDropDown(HWND hwndDlg, UINT id, const wchar_t *str, int width);
#endif

static void ConfigInitComboBox(HWND hDlg, Config *pConfig, int idc)
{
  Options *pOptions=&pConfig->pPlayer->options;
  HWND hComboBox=GetDlgItem(hDlg,idc);
  UINT uLen,cDevice,nDevice=0,labelLen,comboWidth=0;
  ConfigDevice *pConfigDevice;
  wchar_t *pwszLabel;

  for (cDevice=0;cDevice<1+pConfig->nDevices;++cDevice) {
    if (NULL==(pConfigDevice=ConfigDeviceNew(pConfig,cDevice)))
      goto config;

    if (!cDevice) {
      uLen=0;
      uLen+=wcslen(L"Default Device -- ");
      uLen+=wcslen(pConfigDevice->vName.pwszVal);

      if (NULL==(pwszLabel=malloc((uLen+1)*(sizeof *pwszLabel))))
        goto label;

      lstrcpyn(pwszLabel,L"Default Device -- ",uLen);
	  labelLen = wcslen(pwszLabel);
	  uLen-=labelLen;
	  lstrcpyn(pwszLabel+labelLen,pConfigDevice->vName.pwszVal,uLen);
    }
    else
      pwszLabel=pConfigDevice->vName.pwszVal;

    SendMessage(hComboBox,CB_ADDSTRING,0,(LPARAM)pwszLabel);
#ifndef WACUP_BUILD
	comboWidth = ResizeComboBoxDropDown(hDlg,hComboBox,pwszLabel,comboWidth);
#else
	comboWidth = ResizeComboBoxDropDown(hDlg,idc,pwszLabel,comboWidth);
#endif

    if (!cDevice)
      free(pwszLabel);

    SendMessage(hComboBox,CB_SETITEMDATA,cDevice,(LPARAM)pConfigDevice);

    if (0==cDevice&&0==pOptions->common.szId[0])
      nDevice=cDevice+1;
    else if (0==wcscmp(pConfigDevice->pstrId,pOptions->common.szId))
      nDevice=cDevice+1;
  }

  SendMessage(hComboBox,CB_SETCURSEL,nDevice?nDevice-1:0,0);
//cleanup:
label:
config:
  return;
}

static int ConfigInitPage(HWND hDlg, HWND hWndTab, Config *pConfig, int nPage)
{
  int code=-1;
  PageTemplate *pTemplate=gaTemplates+nPage;
  Page *pPage=pConfig->aPages+nPage;
  TCITEM tie = {0};
  RECT rc;

  pPage->vmt=pTemplate->GetVMT();

  tie.mask=TCIF_TEXT|TCIF_IMAGE;
  tie.iImage=-1;
  tie.pszText=(LPWSTR)GetLangString(pTemplate->label);
  SendMessage(hWndTab,TCM_INSERTITEM,nPage,(LPARAM)&tie);

  pPage->hDlg=(HWND)WACreateDialogParam(
    pTemplate->idd,   // _In_     LPCTSTR   lpTemplateName,
    hDlg,             // _In_opt_ HWND      hWndParent,
    pPage->vmt->lpDialogFunc,
                      // _In_opt_ DLGPROC   lpDialogFunc,
    (LPARAM)pConfig   // _In_     LPARAM    dwInitParam
  );

  if (NULL==pPage->hDlg) {
    DMESSAGE("creating child dialog");
    goto child;
  }

  GetClientRect(pPage->hDlg,&rc);
  TabCtrl_AdjustRect(hWndTab,FALSE,&rc);

  MapWindowPoints(
    hWndTab,          // _In_    HWND    hWndFrom,
    hDlg,             // _In_    HWND    hWndTo,
    (LPPOINT)&rc,     // _Inout_ LPPOINT lpPoints,
    2                 // _In_    UINT    cPoints
  );

  SetWindowPos(pPage->hDlg,hWndTab/*HWND_TOP*/,rc.left,rc.top,0,0,
      SWP_NOSIZE|SWP_HIDEWINDOW|SWP_NOACTIVATE);
  code=0;
child:
  return code;
}

static int ConfigInitTabControl(HWND hDlg, Config *pConfig, int idc)
{
  int code=-1;
  HWND hWndTab;
  int nPageCreate;

  pConfig->hTabCtlPage=NULL;

  // Get the tabcontrol handle.
  if (NULL==(hWndTab=GetDlgItem(hDlg,idc))) {
    DMESSAGE("getting tabcontrol handle");
    goto item;
  }

  // Initialize pages.
  for (nPageCreate=0;nPageCreate<NUM_PAGES;++nPageCreate) {
    if (ConfigInitPage(hDlg,hWndTab,pConfig,nPageCreate)<0) {
      DMESSAGE("initializing page");
      goto param;
    }

	UXThemeFunc((WPARAM)pConfig->aPages[nPageCreate].hDlg);
  }
  BringWindowToTop(hWndTab);
  SendMessage(hWndTab,TCM_SETCURSEL,pConfig->options.common.nPage,0);
  ConfigOnSelChangeTabCtrl(hDlg,pConfig,hWndTab);
  code=0;
param:
item:
  return code;
}

static INT_PTR ConfigOnInit(HWND hDlg, Config *pConfig)
{
#if defined (YASAPI_ABOUT)
  Options *pOptions=&pConfig->pPlayer->options;
#endif
  HWND hWndTip;

#if defined (YASAPI_ABOUT) // {
  if (0<=pOptions->common.nConfigX&&0<=pOptions->common.nConfigY) {
    SetWindowPos(
      hDlg,     // _In_     HWND hWnd,
      NULL,     // _In_opt_ HWND hWndInsertAfter,
      pOptions->common.nConfigX,
                // _In_     int  X,
      pOptions->common.nConfigY,
                // _In_     int  Y,
      0,        // _In_     int  cx,
      0,        // _In_     int  cy,
      SWP_NOOWNERZORDER|SWP_NOSIZE
                //_In_     UINT uFlags
    );
  }
#endif // }

  hWndTip=ControlsInit(gcaCoreDeviceControls,hDlg/*,pConfig->hModule*/);
  ControlAddToolTip(hDlg,hWndTip,IDC_COMBOBOX_DEVICE,IDS_DEVICE_FOR_OUTPUT);
  ConfigInitTabControl(hDlg,pConfig,IDC_TABCONTROL);
  ConfigInitComboBox(hDlg,pConfig,IDC_COMBOBOX_DEVICE);
  ConfigOnSelChangeComboBox(hDlg,pConfig,GetDlgItem(hDlg,IDC_COMBOBOX_DEVICE));
  pConfig->pPlayer->hDlgConfig=hDlg;
  return TRUE;
}

void ConfigSet(Config *pConfig, LPWSTR pstrId, wchar_t *pwszLabel)
{
  int cPage;
  Page *pPage;
  int nLen;

  nLen=(sizeof pConfig->options.common.szId);
  nLen/=(sizeof pConfig->options.common.szId[0]);
  nLen-=1;
  lstrcpyn(pConfig->options.common.szId,pstrId,nLen);
  pConfig->options.common.szId[nLen]=0;

  ControlsSet(gcaCoreDeviceControls,pConfig->hDlg,&pConfig->options.device);

  for (cPage=0,pPage=pConfig->aPages;cPage<NUM_PAGES;++cPage,++pPage) {
    pPage->vmt->Set(pPage,pConfig);
  }
}

void ConfigGet(Config *pConfig)
{
  Player *pPlayer=pConfig->pPlayer;
  int cPage;
  Page *pPage;
  //RECT rc;

  for (cPage=0,pPage=pConfig->aPages;cPage<NUM_PAGES;++cPage,++pPage)
    pPage->vmt->Get(pPage,pConfig);

  //GetWindowRect(pConfig->hDlg,&rc);
#if defined (YASAPI_ABOUT) // {
  pConfig->options.common.nConfigX=rc.left;
  pConfig->options.common.nConfigY=rc.top;
#else // } {
  /*pConfig->options.common.nPosX=rc.left;
  pConfig->options.common.nPosY=rc.top;*/
#endif // }

  pPlayer->options.common=pConfig->options.common;
  pPlayer->options.device=pConfig->options.device;
}

void ConfigReset(Config *pConfig)
{
	if (pConfig != NULL)
	{
		int cPage;
		Page *pPage;

		ControlsSet(gcaCoreDeviceControls, pConfig->hDlg, OptionsDeviceDefault());

		for (cPage = 0, pPage = pConfig->aPages; cPage < NUM_PAGES; ++cPage, ++pPage)
		{
			pPage->vmt->Reset(pPage, pConfig);
		}
	}
}

static int PlayerWriteConfig(Player *pPlayer, Request *pRequest)
{
  ConfigDevice *pConfigDevice=va_arg((pRequest)->ap,ConfigDevice *);

  OptionsDeviceSave(&pPlayer->options.device,pConfigDevice->pstrId,
      pPlayer->options.path);
  OptionsCommonSave(&pPlayer->options.common,0,pPlayer->options.path);
#if defined (YASAP_DEBUG) // {
  TraceSwitch(&trace);
#endif // }

  return 0;
}

static int PlayerWriteConfigSpecific(Player *pPlayer, Request *pRequest)
{
  OptionsCommonSave(&pPlayer->options.common,1,pPlayer->options.path);
#if defined (YASAP_DEBUG) // {
  TraceSwitch(&trace);
#endif // }

  return 0;
}

void ConfigSave(Config *pConfig)
{
	if (pConfig != NULL) {
		Player *pPlayer = pConfig->pPlayer;
		HWND hComboBox = GetDlgItem(pConfig->hDlg, IDC_COMBOBOX_DEVICE);
		DWORD cDevice = SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
		if (cDevice != CB_ERR) {
		ConfigDevice *pConfigDevice = (ConfigDevice *)SendMessage(hComboBox,
			CB_GETITEMDATA, cDevice, 0);

			if (pConfigDevice && (pConfigDevice != CB_ERR)) {
			// should be called before the lock in order to not dead-lock this window.
			ConfigGet(pConfig);
			PLAYER_SEND(pPlayer, PlayerWriteConfig, pConfigDevice);
		}
	}
}
}

void ConfigSaveSpecific(Config *pConfig)
{
	if (pConfig)
	{
		Player *pPlayer = pConfig->pPlayer;
		/*HWND hComboBox=GetDlgItem(pConfig->hDlg,IDC_COMBOBOX_DEVICE);
		DWORD cDevice=SendMessage(hComboBox,CB_GETCURSEL,0,0);
		ConfigDevice *pConfigDevice=(ConfigDevice *)SendMessage(hComboBox,
			CB_GETITEMDATA,cDevice,0);*/

			//if (pConfigDevice) {
			  // should be called before the lock in order to not dead-lock this window.
		ConfigGet(pConfig);
		PLAYER_SEND(pPlayer, PlayerWriteConfigSpecific);
		//}
	}
}

static void ConfigEndDialog(HWND hDlg, Config *pConfig, INT_PTR nResult)
{
	if (pConfig != NULL)
	{
		HWND hComboBox = GetDlgItem(hDlg, IDC_COMBOBOX_DEVICE);
		int nDevices = 1 + pConfig->nDevices;

		pConfig->pPlayer->hDlgConfig = NULL;

		while (0 < nDevices) {
			ConfigDevice *pConfigDevice
				= (ConfigDevice *)SendMessage(hComboBox, CB_GETITEMDATA, --nDevices, 0);

			if (pConfigDevice) {
				ConfigDeviceDelete(pConfigDevice);
				SendMessage(hComboBox, CB_SETITEMDATA, nDevices, (LPARAM)NULL);
			}
		}
	}
	EndDialog(hDlg,nResult);
}

static void ConfigOnSelChangeComboBox(HWND hDlg,Config *pConfig,
    HWND hComboBox)
{
  Player *pPlayer=pConfig->pPlayer;
  DWORD cDevice=SendMessage(hComboBox,CB_GETCURSEL,0,0);
  if (cDevice == CB_ERR)
    goto label;

  ConfigDevice *pConfigDevice=(ConfigDevice *)SendMessage(hComboBox,
      CB_GETITEMDATA,cDevice,0);
  if (pConfigDevice == CB_ERR)
    goto label;

  DWORD nLen=SendMessage(hComboBox,CB_GETLBTEXTLEN,cDevice,0);
  wchar_t *pwszLabel;

  if (!pConfigDevice) {
    DMESSAGE("getting device");
    goto device;
  }

  if (NULL==(pwszLabel=malloc((nLen+1)*(sizeof *pwszLabel))))
    goto label;

  SendMessage(hComboBox,CB_GETLBTEXT,cDevice,(LPARAM)pwszLabel);
  pwszLabel[nLen]=0;

  OptionsDeviceLoad(&pConfig->options.device,L"    ",pConfigDevice->pstrId,
      pPlayer->options.path);
  DPUTS(0,"  device options loaded\n");
  ConfigSet(pConfig,cDevice?pConfigDevice->pstrId:L"",pwszLabel);

#if defined (YA_DEBUG) // {
  //ConfigEnableDebug(pConfig->aPages[PAGE_DEBUG].hDlg,trace.nDebug);
  TraceEnableDebug(pConfig->aPages[PAGE_DEBUG].hDlg);
#endif // }
  ConfigOnShareMode(pConfig);
  ConfigOnPull(pConfig);
#if 0 // {
#if defined (YASAPI_GAPLESS) // {
  SyncGapless(pConfig);
#if defined (YASAPI_CHECK_UNDERFLOW) // {
  CommonSyncCheckUndeflow(pConfig->aPages[PAGE_GAPLESS].hDlg);
#endif // }
#endif // }
#endif // }

  free(pwszLabel);
label:
device:
  ;
}

INT_PTR CALLBACK ConfigProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  Config *pConfig=(Config *)GetWindowLongPtr(hDlg,GWLP_USERDATA);

  switch (uMsg) {
  case WM_INITDIALOG:
  {
	  DPRINTF(0, "%s\n", __func__);

	  // incase the user only goes to the
	  // config, this ensure we've setup
	  // correctly otherwise all crashes
	  winampGetOutModeChange(OUT_SET);

	  extern Player player;
	  ConfigDialog(&player, pConfig, hDlg);
	  return TRUE;
  }
  case WM_DESTROY:
  {
	  // with the change to the config being
	  // a native prefs page it is necessary
	  // to release the pCollection here as
	  // the old method was a blocking dialog
	  if (pConfig != NULL)
	  {
		  ConfigSave(pConfig);
		  ConfigEndDialog(hDlg, pConfig, TRUE);

		  if (pConfig->pCollection != NULL)
		  {
			  pConfig->pCollection->lpVtbl->Release(pConfig->pCollection);
			  pConfig->pCollection = NULL;
		  }

		  free(pConfig);
	  }
	  break;
  }
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDC_BUTTON_DEFAULT:
      ConfigReset(pConfig);
      return TRUE;
    case IDC_COMBOBOX_SHAREMODE:
      DeviceSetShareMode(pConfig,GetShareMode(hDlg));
      ConfigOnShareMode(pConfig);
      return TRUE;
    case IDC_COMBOBOX_PULL:
      DeviceSetPull(pConfig,GetPull(hDlg));
      ConfigOnPull(pConfig);
      return TRUE;
    case IDC_COMBOBOX_DEVICE:
      switch (HIWORD(wParam)) {
      case CBN_SELCHANGE:
        ConfigOnSelChangeComboBox(hDlg,pConfig,(HWND)lParam);
        return TRUE;
      default:
        break;
      }

      break;
    default:
      break;
    }

    break;
  case WM_NOTIFY:
    switch (((NMHDR *)lParam)->idFrom) {
    case IDC_TABCONTROL:
      switch (((NMHDR *)lParam)->code) {
      case TCN_SELCHANGE:
        ConfigOnSelChangeTabCtrl(hDlg,pConfig,((NMHDR *)lParam)->hwndFrom);
        return TRUE;
      default:
        break;
      }

      break;
    default:
      break;
    }

    break;
  case WM_CONFIG_UPDATE:
    if (IsDlgButtonChecked(pConfig->aPages[PAGE_COMMON].hDlg,
        IDC_CHECKBOX_VISUALIZATION)) {
      ConfigUpdateProgress(GetDlgItem(hDlg,IDC_PROGRESSBAR_RING),
          LOWORD(lParam));
      ConfigUpdateProgress(GetDlgItem(hDlg,IDC_PROGRESSBAR_SHARED),
          HIWORD(lParam));
      return TRUE;
    }

    break;
  default:
    break;
  }
  DialogLayout(hDlg, uMsg, lParam, MODE_GENERIC);
  return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
void ConfigDialog(Player *pPlayer, Config *pConfig, HWND hWndParent)
{
  IMMDeviceEnumerator *pEnumerator=pPlayer->run.pEnumerator;
  IMMDeviceCollection *pCollection = 0;
  int nNumPages = (sizeof gaTemplates) / (sizeof *gaTemplates);
  UINT nDevices = 0;
  HRESULT hr = -1;

  if (nNumPages!=NUM_PAGES) {
    MessageBoxA(
      NULL,                     // _In_opt_ HWND    hWnd,
      "Inconsistent number of pages.",
                                // _In_opt_ LPCTSTR lpText,
      "YASAPI Error Message",   // _In_opt_ LPCTSTR lpCaption,
      MB_SYSTEMMODAL|MB_OK|MB_ICONERROR
                                // _In_     UINT    uType
    );

    goto pages;
  }

  /////////////////////////////////////////////////////////////////////////////
  pConfig = calloc(1, sizeof(Config));
  pConfig->pPlayer=pPlayer;
  //config.hModule=hInstance;
  pConfig->options.common=pPlayer->options.common;
  pConfig->pEnumerator=pPlayer->run.pEnumerator;

  // create a device enumerator ///////////////////////////////////////////////
  if (!pEnumerator) {
	  hr=CoCreateInstance(
		&CLSID_MMDeviceEnumerator,  // _In_   REFCLSID rclsid,
		NULL,                       // _In_   LPUNKNOWN pUnkOuter,
		CLSCTX_ALL,                 // _In_   DWORD dwClsContext,
		&IID_IMMDeviceEnumerator,   // _In_   REFIID riid,
		(void**)&pEnumerator        // _Out_  LPVOID *ppv
	  );

	  if (FAILED(hr)) {
		DERROR(REGDB_E_CLASSNOTREG,hr,enumerator);
		DERROR(CLASS_E_NOAGGREGATION,hr,enumerator);
		DERROR(E_NOINTERFACE,hr,enumerator);
		DERROR(E_POINTER,hr,enumerator);
		DMESSAGE("creating device enumerator");
		goto enumerator;
	  }
	  else {
		  pConfig->pEnumerator=pPlayer->run.pEnumerator=pEnumerator;
	  }
  }

  /////////////////////////////////////////////////////////////////////////////
  if (pEnumerator) {
	  hr=pEnumerator->lpVtbl->EnumAudioEndpoints(pEnumerator,
		eRender,              // [in]  EDataFlow           dataFlow,
		DEVICE_STATE_ACTIVE,  // [in]  DWORD               dwStateMask,
		&pCollection          // [out] IMMDeviceCollection **ppDevices
	  );
  }

  if (FAILED(hr)) {
    DERROR(E_POINTER,hr,collection);
    DERROR(E_INVALIDARG,hr,collection);
    DERROR(E_OUTOFMEMORY,hr,collection);
    DUNKNOWN(hr);
    DMESSAGE("getting the device collection");
    goto collection;
  }

  DPUTS(0,"  got the device collection\n");
  pConfig->pCollection=pCollection;

  /////////////////////////////////////////////////////////////////////////////
  hr=pCollection->lpVtbl->GetCount(pCollection,
    &nDevices           // [out] UINT *pcDevices
  );

  if (FAILED(hr)) {
    DERRORC(E_POINTER,hr);
    DUNKNOWN(hr);
    DMESSAGE("getting the device count");
    goto count;
  }

  if (0==nDevices) {
    DMESSAGE("no devices found");
    goto count;
  }

  DPRINTF(0,"  got the device count: %d\n",nDevices);
  pConfig->nDevices=nDevices;

  pConfig->hDlg = hWndParent;
  SetWindowLongPtr(hWndParent, GWLP_USERDATA, (LONG_PTR)pConfig);
  ConfigOnInit(hWndParent, pConfig);

  /////////////////////////////////////////////////////////////////////////////
//cleanup:
count:
collection:
enumerator:
pages:
  return;
}
