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
// TODO localise
static int vmessagea(const char *format, va_list ap)
{
  char buf[256]={0},*wp=buf;

  wp+=vsnprintf(wp,256,format,ap);

  MessageBoxA(
    NULL,                       // _In_opt_  HWND hWnd,
    buf,                        // _In_opt_  LPCTSTR lpText,
    "YASAPI Error Message",     // _In_opt_  LPCTSTR lpCaption,
	MB_SYSTEMMODAL|MB_ICONERROR
                                // _In_      UINT uType
  );

  return wp-buf;
}

static int vmessagew(const wchar_t *format, va_list ap)
{
  wchar_t buf[256]={0},*wp=buf;
  enum { SIZE=(sizeof buf)-(sizeof buf[0]) };

  wp+=vswprintf(wp,SIZE,format,ap);

  MessageBoxW(
    NULL,                       // _In_opt_  HWND hWnd,
    buf,                        // _In_opt_  LPCTSTR lpText,
    L"YASAPI Error Message",    // _In_opt_  LPCTSTR lpCaption,
	MB_SYSTEMMODAL|MB_ICONERROR
                                // _In_      UINT uType
  );

  return wp-buf;
}

void messagea(int force, HRESULT x, HRESULT *y, const char *m, ...)
{
#ifdef _DEBUG
  if ((!force&&x==*y)||force) {
#if ! defined (YA_DEBUG) // {
#if defined (YA_DUMP) // {
    FILE *f;
#endif // }
#endif // }
    va_list ap;
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
#endif
}

void messagew(int force, HRESULT x, HRESULT *y, const wchar_t *m, ...)
{
#ifdef _DEBUG
  if ((!force&&x==*y)||force) {
#if ! defined (YA_DEBUG) // {
#if defined (YA_DUMP) // {
    FILE *f;
#endif // }
#endif // }
    va_list ap;
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
#endif
}

#if defined (YA_DEBUG) // {
void ConsoleOpen(void)
{
  AllocConsole();
  (void)freopen("conin$","r",stdin);
  (void)freopen("conout$","w",stdout);
  (void)freopen("conout$","w",stderr);
}

void ConsoleClose(void)
{
  FreeConsole();
}
#endif // }
