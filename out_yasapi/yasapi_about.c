/*
 * yasapi_about.c
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
#include <resource.h>

#if defined (YASAPI_ABOUT) // {
static INT_PTR CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
  Player *pPlayer=(Player *)GetWindowLongPtr(hDlg,GWLP_USERDATA);
  Options *pOptions=pPlayer?&pPlayer->options:NULL;
  RECT rc;
  wchar_t buf[YA_PROPERTY_SIZE];

  switch (uMsg) {
  case WM_INITDIALOG:
    SetWindowLongPtr(hDlg,GWLP_USERDATA,lParam);
    pPlayer=(Player *)lParam;
    pOptions=&pPlayer->options;
    SetWindowTextA(hDlg,"About " PI_LABEL);

    if (0<=pOptions->common.nAboutX&&0<=pOptions->common.nAboutY) {
      SetWindowPos(
        hDlg,                             // _In_     HWND hWnd,
        NULL,                             // _In_opt_ HWND hWndInsertAfter,
        pOptions->common.nAboutX,         // _In_     int  X,
        pOptions->common.nAboutY,         // _In_     int  Y,
        0,                                // _In_     int  cx,
        0,                                // _In_     int  cy,
        SWP_NOOWNERZORDER|SWP_NOSIZE
                                          //_In_     UINT uFlags
      );
    }

    // https://social.msdn.microsoft.com/Forums/vstudio/en-US/b4079221-0a82-4ec9-b85e-d59394a34584/how-to-add-a-html-link-in-a-microsoft-resource-rc-file?forum=vcgeneral
#if 0 // {
    SetWindowLongPtr(
      GetDlgItem(hDlg,IDC_STATIC_COPY),   // _In_ HWND hWnd,
      GWL_STYLE,                          // _In_ int  nIndex,
      (LONG)WC_LINK                       // _In_ LONG dwNewLong
    );
#endif // }

#if 0 // {
    SetWindowTextW(
      GetDlgItem(hDlg,IDC_STATIC_COPY),   // _In_     HWND    hWnd,
      L"Copyright \xA9 2015-2016 Peter Belkner."
      L"\r\n\r\n"
      L"For further details refer to <a href=\"http://out-yasapi.sourceforge.net/\">http://out-yasapi.sourceforge.net/</a>."
                                          // _In_opt_ LPCTSTR lpString
    );
#else // } {
//fprintf(stderr,"%s: %p\n",__func__,GetDlgItem(hDlg,IDC_STATIC_COPY));
#if 1 // {
#if 0 // {
    SetWindowTextW(
      GetDlgItem(hDlg,IDC_STATIC_COPY),   // _In_     HWND    hWnd,
      L"Copyright \xA9 2015-2016 Peter Belkner."
      L"\r\n\r\n"
      L"For further details refer to \"http://out-yasapi.sourceforge.net/\"."
                                          // _In_opt_ LPCTSTR lpString
    );
#endif // }
#else // } {
    SetWindowTextW(
      GetDlgItem(hDlg,IDC_STATIC_COPY),   // _In_     HWND    hWnd,
      L"Copyright \xA9 2015-2016 Peter Belkner."
      L"\r\n\r\n"
      L"For further details refer to"
      L" <a href=\"http://out-yasapi.sourceforge.net/\">http://out-yasapi.sourceforge.net/</a>."
                                          // _In_opt_ LPCTSTR lpString
    );
#endif // }
#endif // }

    SetWindowTextW(
      GetDlgItem(hDlg,IDC_STATIC_GPL),    // _In_     HWND    hWnd,
      L"\"Yet Another (WA)SAPI Output Plugin for Winamp YASAPI\""
      L" (out_yasapi) is free software: you can redistribute it and/or modify"
      L" it under the terms of the GNU General Public License as published by"
      L" the Free Software Foundation, either version 3 of the License, or"
      L" (at your option) any later version."
      L"\r\n\r\n"
      L"\"Yet Another (WA)SAPI Output Plugin for Winamp YASAPI\""
      L" (out_yasapi) is distributed in the hope that it will be useful,"
      L" but WITHOUT ANY WARRANTY; without even the implied warranty of"
      L" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
      L" GNU General Public License for more details."
      L"\r\n\r\n"
      L"You should have received a copy of the GNU General Public License"
      L" along with"
      L" \"Yet Another (WA)SAPI Output Plugin for Winamp YASAPI\""
      L" (out_yasapi).  If not, see <http://www.gnu.org/licenses/>."
    );

    return TRUE;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDOK:
    case IDCANCEL:
      GetWindowRect(hDlg,&rc);
      pPlayer->options.common.nAboutX=rc.left;
      swprintf(buf,YA_PROPERTY_SIZE,L"%d",rc.left);

      WritePrivateProfileStringW(
        YA_GROUP_COMMON,                  // _In_  LPCTSTR lpAppName,
        YA_PROPERTY_ABOUTX,               // _In_  LPCTSTR lpKeyName,
        buf,                              // _In_  LPCTSTR lpString,
        pOptions->path                    // _In_  LPCTSTR lpFileName
      );

      pPlayer->options.common.nAboutY=rc.top;
      swprintf(buf,YA_PROPERTY_SIZE,L"%d",rc.top);

      WritePrivateProfileStringW(
        YA_GROUP_COMMON,                  // _In_  LPCTSTR lpAppName,
        YA_PROPERTY_ABOUTY,               // _In_  LPCTSTR lpKeyName,
        buf,                              // _In_  LPCTSTR lpString,
        pOptions->path                    // _In_  LPCTSTR lpFileName
      );

      EndDialog(hDlg,TRUE);
      return TRUE;
    default:
      break;
    }

    break;
  default:
    break;
  }

  return FALSE;
}

void AboutDialog(Player *pPlayer, HINSTANCE hInstance, HWND hWndParent)
{
  LPCWSTR lpTemplate=MAKEINTRESOURCEW(IDD_ABOUT);

  DialogBoxParamW(
    hInstance,                  // _In_opt_ HINSTANCE hInstance,
    lpTemplate,                 // _In_     LPCTSTR   lpTemplate,
    hWndParent,                 // _In_opt_ HWND      hWndParent,
    AboutProc,                  // _In_opt_ DLGPROC   lpDialogFunc
    (LPARAM)pPlayer             // _In_      LPARAM dwInitParam
  );
}
#else // } {
#ifdef WACUP_BUILD
#define WA_UTILS_SIMPLE
#include "../../loader/loader/utils.h"
#endif
void AboutDialog(HWND hWndParent)
{
	wchar_t message[4096] = {0};
#ifdef WACUP_BUILD
	LPWSTR text = GetTextResource(IDR_ABOUT_GZ);
#else
	LPWSTR text = GetTextResource(IDR_ABOUT_TEXT);
#endif

	StringCchPrintf(message, ARRAYSIZE(message), text, TEXT(PLUGIN_VERSION),
					TEXT(YASAPI_VERSION), TEXT("Darren Owen aka DrO"),
					TEXT("2016-2018"), TEXT(__DATE__));
	MessageBoxW(hWndParent, message, (LPWSTR)GetLangString(IDS_ABOUT_TITLE), MB_OK);

	if (text)
	{
		free(text);
	}
}
#endif // }
