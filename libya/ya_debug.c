/*
 * ya_debug.c
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

static int vmessagea(const char *format, va_list ap)
{
  char buf[256],*wp=buf;
  int code;

  wp+=vsprintf(wp,format,ap);
  wp+=sprintf(wp,"Do you want to exit Winamp?");

  code=MessageBoxA(
    NULL,                       // _In_opt_  HWND hWnd,
    buf,                        // _In_opt_  LPCTSTR lpText,
    "YASAPI Error Message",     // _In_opt_  LPCTSTR lpCaption,
	MB_SYSTEMMODAL|MB_YESNO|MB_ICONERROR|MB_DEFBUTTON2
                                // _In_      UINT uType
  );

  if (IDYES==code)
    exit(1);

  return wp-buf;
}

static int vmessagew(const wchar_t *format, va_list ap)
{
  wchar_t buf[256],*wp=buf;
  enum { SIZE=(sizeof buf)-(sizeof buf[0]) };
  int code;

  wp+=vswprintf(wp,SIZE,format,ap);
  wp+=swprintf(wp,SIZE-(wp-buf),L"Do you want to exit Winamp?");

  code=MessageBoxW(
    NULL,                       // _In_opt_  HWND hWnd,
    buf,                        // _In_opt_  LPCTSTR lpText,
    L"YASAPI Error Message",    // _In_opt_  LPCTSTR lpCaption,
    MB_SYSTEMMODAL|MB_YESNO|MB_ICONERROR
                                // _In_      UINT uType
  );

  if (IDYES==code)
    exit(1);

  return wp-buf;
}

int messagea(int force, HRESULT x, HRESULT *y, const char *m, ...)
{
  va_list ap;
#if ! defined (YA_DEBUG) // {
#if defined (YA_DUMP) // {
  FILE *f;
#endif // }
#endif // }

  if ((!force&&x==*y)||force) {
    va_start(ap,m);
#if defined (YA_DEBUG) // {
    TRACE_CLOSE<trace.tag?vtprintf(&trace,m,ap):vmessagea(m,ap);
#else // } {
#if defined (YA_DUMP) // {
    if (NULL!=(f=DumpOpenFile(&dump))) {
      vfprintf(f,m,ap);
      fputc('\n',f);
      fclose(f);
    }
#endif // }
    vmessagea(m,ap);
#endif // }
    va_end(ap);

    if (y)
      *y=0;
  }

  return 0;
}

int messagew(int force, HRESULT x, HRESULT *y, const wchar_t *m, ...)
{
  va_list ap;
#if ! defined (YA_DEBUG) // {
#if defined (YA_DUMP) // {
  FILE *f;
#endif // }
#endif // }

  if ((!force&&x==*y)||force) {
    va_start(ap,m);
#if defined (YA_DEBUG) // {
    TRACE_CLOSE<trace.tag?vtwprintf(&trace,m,ap):vmessagew(m,ap);
#else // } {
#if defined (YA_DUMP) // {
    if (NULL!=(f=DumpOpenFile(&dump))) {
      vfwprintf(f,m,ap);
      fputc('\n',f);
      fclose(f);
    }
#endif // }
    vmessagew(m,ap);
#endif // }
    va_end(ap);

    if (y)
      *y=0;
  }

  return 0;
}

#if defined (YA_DEBUG) // {
void ConsoleOpen(void)
{
  AllocConsole();
  freopen("conin$","r",stdin);
  freopen("conout$","w",stdout);
  freopen("conout$","w",stderr);
}

void ConsoleClose(void)
{
  FreeConsole();
}
#endif // }
