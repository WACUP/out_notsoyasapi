/*
 * ya_control.c
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
#include <commctrl.h>
#define WA_UTILS_SIMPLE
#include <../loader/loader/utils.h>

#define CONTROL_TYPE_MAGIC "control_type_magic"

extern LPWSTR GetLangString(const UINT id);

///////////////////////////////////////////////////////////////////////////////
// How to Create a Tooltip for a Control
// https://msdn.microsoft.com/en-us/library/windows/desktop/hh298368%28v=vs.85%29.aspx
HWND ControlCreateToolTip(HWND hDlg/*, HINSTANCE hInstance*/)
{
  HWND hwndTip = CreateWindowExW(
    WS_EX_NOPARENTNOTIFY | WS_EX_TOPMOST,  // _In_     DWORD     dwExStyle,
    TOOLTIPS_CLASS,     // _In_opt_ LPCTSTR   lpClassName,
    NULL,               // _In_opt_ LPCTSTR   lpWindowName,
    WS_POPUP|TTS_ALWAYSTIP/*|TTS_BALLOON*/,
                        // _In_     DWORD     dwStyle,
    0,                  // _In_     int       x,
    0,                  // _In_     int       y,
    0,                  // _In_     int       nWidth,
    0,                  // _In_     int       nHeight,
    hDlg,               // _In_opt_ HWND      hWndParent,
    NULL,               // _In_opt_ HMENU     hMenu,
	// TODO do we need this?
    NULL/*hInstance*/,          // _In_opt_ HINSTANCE hInstance,
    NULL                // _In_opt_ LPVOID    lpParam
  );
  // ensure that it can be seen
  SetWindowPos(hwndTip,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  return hwndTip;
}

void ControlAddToolTip(HWND hDlg, HWND hWnd, int idc, UINT idText/*PWSTR pszText*/)
{
  TOOLINFOW ti={0};

  /*if (pszText&&*pszText)*/ {
    ti.cbSize=sizeof ti;
    ti.hwnd=hDlg;
    ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS;
    ti.uId=(UINT_PTR)GetDlgItem(hDlg,idc);
    ti.lpszText=(LPWSTR)GetLangString(idText)/*pszText*/;
    SendMessage(hWnd,TTM_ADDTOOLW,0,(LPARAM)&ti);
	SendMessage(hWnd,TTM_SETMAXTIPWIDTH,0,(LPARAM)450);
	SendMessage(hWnd,TTM_SETWINDOWTHEME,0,(LPARAM)L"");
  }
}

///////////////////////////////////////////////////////////////////////////////
static void CheckBoxTypeInit(const Control *pControl, HWND hDlg, HWND hWnd)
{
  const ControlCheckBoxConfig *config=pControl->config;

  ControlAddToolTip(hDlg,hWnd,config->idc,config->help);
}

static void CheckBoxTypeSet(const Control *pControl, HWND hDlg,
    const void *pData)
{
  const ControlCheckBoxConfig *config=pControl->config;
  int val=VALUE_INT(pData,config->offset);

  CheckDlgButton(
    hDlg,                 // _In_  HWND hDlg,
    config->idc,          // _In_  int nIDButton,
    val==config->on       // _In_  UINT uCheck
  );
}

static void CheckBoxTypeGet(const Control *pControl, HWND hDlg,
    void *pData)
{
  const ControlCheckBoxConfig *config=pControl->config;
  UINT uChecked;

  uChecked=IsDlgButtonChecked(
    hDlg,                 // _In_  HWND hDlg,
    config->idc           // _In_  int nIDButton
  );

  VALUE_INT(pData,config->offset)=uChecked?config->on:config->off;
}

static void CheckBoxTypeSync(const Control *pControl, HWND hDlg)
{
}

const ControlType gcCheckBoxType={
  CONTROL_TYPE_MAGIC,
  "CheckBox",
  CheckBoxTypeInit,
  CheckBoxTypeSet,
  CheckBoxTypeGet,
  CheckBoxTypeSync
};

///////////////////////////////////////////////////////////////////////////////
static void RadioButtonTypeInit(const Control *pControl, HWND hDlg, HWND hWnd)
{
  const ControlRadioButtonConfig *config=pControl->config;

  while (config->idc) {
    ControlAddToolTip(hDlg,hWnd,config->idc,config->help);
    ++config;
  }
}

static void RadioButtonTypeSet(const Control *pControl, HWND hDlg,
    const void *pData)
{
  const ControlRadioButtonConfig *config=pControl->config;
  int val=VALUE_INT(pData,config->offset);

  while (config->idc) {
    CheckDlgButton(
      hDlg,             // _In_  HWND hDlg,
      config->idc,      // _In_  int nIDButton,
      val==config->val  // _In_  UINT uCheck
    );

    ++config;
  }
}

static void RadioButtonTypeGet(const Control *pControl, HWND hDlg,
    void *pData)
{
  const ControlRadioButtonConfig *config=pControl->config;

  while (config->idc) {
    UINT uChecked=IsDlgButtonChecked(
      hDlg,             // _In_  HWND hDlg,
      config->idc       // _In_  int nIDButton
    );

    if (uChecked) {
      VALUE_INT(pData,config->offset)=config->val;
      break;
    }

    ++config;
  }
}

static void RadioButtonTypeSync(const Control *pControl, HWND hDlg)
{
}

const ControlType gcRadioButtonType={
  CONTROL_TYPE_MAGIC,
  "RadioButton",
  RadioButtonTypeInit,
  RadioButtonTypeSet,
  RadioButtonTypeGet,
  RadioButtonTypeSync
};

///////////////////////////////////////////////////////////////////////////////
static void SliderTypeInit(const Control *pControl, HWND hDlg, HWND hWnd)
{
  const ControlSliderConfig *config=pControl->config;
  int idcSlider=config->idcSlider;
  LPARAM lMaxRange=config->lMaxRange;

  ControlAddToolTip(hDlg,hWnd,idcSlider,config->help);

  if (0<lMaxRange) {
    SendMessage(
      GetDlgItem(hDlg,idcSlider), // _In_  HWND hWnd,
      TBM_SETRANGE,               // _In_  UINT Msg,
      FALSE,                      // _In_  WPARAM wParam (redraw flag),
      MAKELONG(0,lMaxRange)       // _In_  LPARAM lParam
    );
  }
}

static void SliderTypeSetStatic(const Control *pControl, HWND hDlg,
    double x)
{
  enum { SIZE=128 };
  const ControlSliderConfig *config=pControl->config;

  if (config->format) {
    wchar_t buf[SIZE]={0};
    PrintfCch(buf,SIZE,GetLangString(config->format),x);
    SetDlgItemTextW(hDlg,config->idcStatic,buf);
  }
}

static void SliderTypeSet(const Control *pControl, HWND hDlg,
    const void *pData)
{
  const ControlSliderConfig *config=pControl->config;
  double x=VALUE_DOUBLE(pData,config->offset);
  HWND hWndCtl=GetDlgItem(hDlg,config->idcSlider);
  LRESULT lMin=SendMessage(hWndCtl,TBM_GETRANGEMIN,0,0);
  LRESULT lMax=SendMessage(hWndCtl,TBM_GETRANGEMAX,0,0);
  LPARAM lPos=(LPARAM)((double)lMin
      +(x-config->min)/(config->max-config->min)*(lMax-lMin)
      +0.5);

  SendMessage(hWndCtl,TBM_SETPOS,TRUE,lPos);
  SliderTypeSetStatic(pControl,hDlg,x);
}

static double SliderTypeGetSlider(const Control *pControl, HWND hDlg)
{
  const ControlSliderConfig *config=pControl->config;
  double min=config->min;
  double max=config->max;
  HWND hWndCtl=GetDlgItem(hDlg,config->idcSlider);
  LRESULT lMin=SendMessage(hWndCtl,TBM_GETRANGEMIN,0,0);
  LRESULT lMax=SendMessage(hWndCtl,TBM_GETRANGEMAX,0,0);
  LRESULT lPos=SendMessage(hWndCtl,TBM_GETPOS,0,0);

  return min+(max-min)*(lPos-lMin)/(lMax-lMin);
}

static void SliderTypeGet(const Control *pControl, HWND hDlg,
    void *pData)
{
  const ControlSliderConfig *config=pControl->config;
  VALUE_DOUBLE(pData,config->offset)=SliderTypeGetSlider(pControl,hDlg);
}

static void SliderTypeSync(const Control *pControl, HWND hDlg)
{
  SliderTypeSetStatic(pControl,hDlg,SliderTypeGetSlider(pControl,hDlg));
}

const ControlType gcSliderType={
  CONTROL_TYPE_MAGIC,
  "Slider",
  SliderTypeInit,
  SliderTypeSet,
  SliderTypeGet,
  SliderTypeSync
};

///////////////////////////////////////////////////////////////////////////////
static void ComboBoxTypeInit(const Control *pControl, HWND hDlg, HWND hWnd)
{
  const ControlComboBoxConfig *config=pControl->config;
  const ControlComboBoxList *list=config->list;
  HWND hWndCtl=GetDlgItem(hDlg,config->idc);

  ControlAddToolTip(hDlg,hWnd,config->idc,config->help);

  while (list->label) {
    SendMessage(hWndCtl,CB_ADDSTRING,0,(LPARAM)GetLangString(list->label));
    ++list;
  }
}

static void ComboBoxTypeSet(const Control *pControl, HWND hDlg,
    const void *pData)
{
  const ControlComboBoxConfig *config=pControl->config;
  const ControlComboBoxList *list=config->list;
  int val=VALUE_INT(pData,config->offset);
  HWND hWndCtl=GetDlgItem(hDlg,config->idc);
  WPARAM wParam=0;

  while (list->label) {
    if (val==list->val) {
      SendMessage(hWndCtl,CB_SETCURSEL,wParam,0);
      break;
    }

    ++wParam;
    ++list;
  }
}

static void ComboBoxTypeGet(const Control *pControl, HWND hDlg,
    void *pData)
{
  const ControlComboBoxConfig *config=pControl->config;
  const ControlComboBoxList *list=config->list;
  HWND hWndCtl=GetDlgItem(hDlg,config->idc);
  LRESULT lCurSel=SendMessage(hWndCtl,CB_GETCURSEL,0,0);

  VALUE_INT(pData,config->offset)=list[lCurSel].val;
}

static void ComboBoxTypeSync(const Control *pControl, HWND hDlg)
{
}

const ControlType gcComboBoxType={
  CONTROL_TYPE_MAGIC,
  "ComboBox",
  ComboBoxTypeInit,
  ComboBoxTypeSet,
  ComboBoxTypeGet,
  ComboBoxTypeSync
};

///////////////////////////////////////////////////////////////////////////////
static void SliderCascadeSetStatic(HWND hDlg, int idc, UINT/*const wchar_t **/format,
    double x)
{
  if (format) {
    enum { SIZE=128 };
    wchar_t buf[SIZE]={0};
    PrintfCch(buf,SIZE,GetLangString(format),x);
    SetDlgItemTextW(hDlg,idc,buf);
  }
}

static LONG SliderCascadeGetHumbleWidth(HWND hWnd)
{
#if 0 // {
  RECT rc;

  SendMessage(hWnd,TBM_GETTHUMBRECT,0,(LPARAM)&rc);

  return rc.right-rc.left;
#else // } {
  return 27;
#endif // }
}

static LONG SliderCascadeGetSliderWidth(HWND hWnd)
{
  RECT rc={0};

  GetWindowRect(hWnd,&rc);

  return rc.right-rc.left-SliderCascadeGetHumbleWidth(hWnd);
}

static void SliderCascadeSetSliderWidth(HWND hDlg, int idc, int idcParent)
{
  HWND hWndParent=GetDlgItem(hDlg,idcParent);
  LRESULT lRangeMinParent=SendMessage(hWndParent,TBM_GETRANGEMIN,0,0);
  LRESULT lRangeMaxParent=SendMessage(hWndParent,TBM_GETRANGEMAX,0,0);
  LRESULT lPosParent=SendMessage(hWndParent,TBM_GETPOS,0,0);
  HWND hWnd=GetDlgItem(hDlg,idc);
  LONG lWidth;
  RECT rc={0};

  lWidth=MulDiv(SliderCascadeGetSliderWidth(hWndParent),
                (int)(lPosParent-lRangeMinParent),
                (int)(lRangeMaxParent-lRangeMinParent));
  lWidth+=SliderCascadeGetHumbleWidth(hWnd);

  GetWindowRect(hWnd,&rc);
  MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rc,2);

  MoveWindow(
    hWnd,                 // _In_  HWND hWnd,
    rc.left,              // _In_  int X,
    rc.top,               // _In_  int Y,
    lWidth,               // _In_  int nWidth,
    rc.bottom-rc.top,     // _In_  int nHeight,
    TRUE                  // _In_  BOOL bRepaint
  );
}

static double SliderCascadeGetSlider(HWND hDlg, int idc, LONG *plWidthParent,
    double min, double max)
{
  HWND hWnd=GetDlgItem(hDlg,idc);
  LRESULT lRangeMin=SendMessage(hWnd,TBM_GETRANGEMIN,0,0);
  LRESULT lRangeMax=SendMessage(hWnd,TBM_GETRANGEMAX,0,0);
  LRESULT lPos=SendMessage(hWnd,TBM_GETPOS,0,0);
  LONG lWidth=SliderCascadeGetSliderWidth(hWnd);
  double x=(max-min)*(lPos-lRangeMin)/(lRangeMax-lRangeMin);

  if (*plWidthParent<0)
    *plWidthParent=lWidth;
  else
    x*=(double)lWidth/(*plWidthParent);

  x+=min;

  return x;
}

void SliderCascadeSetSlider(HWND hDlg, int idc, LONG *plWidthRoot,
    double min, double max, double x)
{
  HWND hWnd=GetDlgItem(hDlg,idc);
  LRESULT lRangeMin=SendMessage(hWnd,TBM_GETRANGEMIN,0,0);
  LRESULT lRangeMax=SendMessage(hWnd,TBM_GETRANGEMAX,0,0);
  LONG lWidth=SliderCascadeGetSliderWidth(hWnd);
  LPARAM lPos;

  if (*plWidthRoot<0) {
    *plWidthRoot=lWidth;
    lPos=(LPARAM)(lRangeMin+(x-min)/(max-min)*(lRangeMax-lRangeMin)+0.5);
  }
  else {
    double q=(double)(*plWidthRoot)/lWidth;
    lPos=(LPARAM)(lRangeMin+(q*(x-min)/(max-min)*(lRangeMax-lRangeMin)+0.5));
  }

  SendMessage(hWnd,TBM_SETPOS,TRUE,lPos);
}

////////////////
static void SliderCascadeTypeInit(const Control *pControl, HWND hDlg,
    HWND hWnd)
{
  const ControlSliderCascadeConfig *config=pControl->config;
  const ControlSliderCascadeList *list=config->list;
  LONG lMaxRange=config->lMaxRange;
  int idcSlider;

  while (0<(idcSlider=list->idcSlider)) {
    if (0l<lMaxRange) {
      SendMessage(
        GetDlgItem(hDlg,idcSlider), // _In_  HWND hWnd,
        TBM_SETRANGE,               // _In_  UINT Msg,
        FALSE,                      // _In_  WPARAM wParam (redraw flag),
        MAKELONG(0,lMaxRange)       // _In_  LPARAM lParam
      );
    }

    ControlAddToolTip(hDlg,hWnd,idcSlider,list->help);
    ++list;
  }
}

static void SliderCascadeTypeSet(const Control *pControl, HWND hDlg,
    const void *pData)
{
  const ControlSliderCascadeConfig *config=pControl->config;
  const ControlSliderCascadeList *list=config->list;
  double min=config->min;
  double max=config->max;
  int idcSliderParent=0;
  LONG lWidthParent=-1;
  int idcSlider;

  while (0<(idcSlider=list->idcSlider)) {
    double x;
    if (idcSliderParent)
      SliderCascadeSetSliderWidth(hDlg,idcSlider,idcSliderParent);

    x=VALUE_DOUBLE(pData,list->offset);
    SliderCascadeSetSlider(hDlg,idcSlider,&lWidthParent,min,max,x);
    SliderCascadeSetStatic(hDlg,list->idcStatic,list->format,x);
    idcSliderParent=idcSlider;
    ++list;
  }
}

static void SliderCascadeTypeGet(const Control *pControl, HWND hDlg,
    void *pData)
{
  const ControlSliderCascadeConfig *config=pControl->config;
  const ControlSliderCascadeList *list=config->list;
  double min=config->min;
  double max=config->max;
  LONG lWidthParent=-1;
  int idc;

  while (0<(idc=list->idcSlider)) {
    VALUE_DOUBLE(pData,list->offset)=SliderCascadeGetSlider(hDlg,idc,
        &lWidthParent,min,max);
    ++list;
  }
}

static void SliderCascadeTypeSync(const Control *pControl, HWND hDlg)
{
  const ControlSliderCascadeConfig *config=pControl->config;
  const ControlSliderCascadeList *list=config->list;
  double min=config->min;
  double max=config->max;
  int idcSliderParent=0;
  LONG lWidthParent=-1;
  int idcSlider;

  while (0<(idcSlider=list->idcSlider)) {
    double x;
    x=SliderCascadeGetSlider(hDlg,idcSlider,&lWidthParent,min,max);

    if (idcSliderParent)
      SliderCascadeSetSliderWidth(hDlg,idcSlider,idcSliderParent);

    SliderCascadeSetSlider(hDlg,idcSlider,&lWidthParent,min,max,x);
    SliderCascadeSetStatic(hDlg,list->idcStatic,list->format,x);
    idcSliderParent=idcSlider;
    ++list;
  }
}

const ControlType gcSliderCascadeType={
  CONTROL_TYPE_MAGIC,
  "SliderCascade",
  SliderCascadeTypeInit,
  SliderCascadeTypeSet,
  SliderCascadeTypeGet,
  SliderCascadeTypeSync,
};

///////////////////////////////////////////////////////////////////////////////
HWND ControlsInit(const Control *pControl, HWND hDlg/*, HINSTANCE hInstance*/)
{
  HWND hWndTip;

  if (NULL==(hWndTip=ControlCreateToolTip(hDlg/*,hInstance*/)))
    goto hwnd;

  while (pControl->type) {
    pControl->type->Init(pControl,hDlg,hWndTip);
    ++pControl;
  }

hwnd:
  return hWndTip;
}

void ControlsSet(const Control *pControl, HWND hDlg, const void *pData)
{
  while (pControl->type) {
    pControl->type->Set(pControl,hDlg,pData);
    ++pControl;
  }
}

void ControlsGet(const Control *pControl, HWND hDlg, void *pData)
{
  while (pControl->type) {
    if (strcmp(pControl->type->magic,CONTROL_TYPE_MAGIC))
      ;
    else
      pControl->type->Get(pControl,hDlg,pData);

    ++pControl;
  }
}

void ControlsSync(const Control *pControl, HWND hDlg)
{
  while (pControl->type) {
    pControl->type->Sync(pControl,hDlg);
    ++pControl;
  }
}
