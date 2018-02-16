/*
 * ya_util.c
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

const char *basenamea(const char *s)
{
  const char *p=s+strlen(s);

  while (s<p&&'/'!=p[-1]&&'\\'!=p[-1]&&':'!=p[-1])
    --p;

  return p;
}

const wchar_t *basenamew(const wchar_t *s)
{
  const wchar_t *p=s+wcslen(s);

  while (s<p&&'/'!=p[-1]&&'\\'!=p[-1]&&':'!=p[-1])
    --p;

  return p;
}

wchar_t *yapath(wchar_t *file, HWND hWnd)
{
  char *dir;
  size_t len1,len2;
  wchar_t *path,*wp;

  dir=(char *)SendMessageA(hWnd,WM_WA_IPC,0,IPC_GETINIDIRECTORY);

  if (NULL==dir)
    goto dir;

  len1=MultiByteToWideChar(
    CP_ACP,       // _In_      UINT   CodePage,
    0,            // _In_      DWORD  dwFlags,
    dir,          // _In_      LPCSTR lpMultiByteStr,
    -1,           // _In_      int    cbMultiByte,
    NULL,         // _Out_opt_ LPWSTR lpWideCharStr,
    0             // _In_      int    cchWideChar
  );

  --len1;
  len2=wcslen(file);
  path=(wchar_t *)YA_MALLOC(((len1+1)+(len2+1))*(sizeof *path));

  if (NULL==path)
    goto path;
  
  wp=path;

  wp+=MultiByteToWideChar(
    CP_ACP,       // _In_      UINT   CodePage,
    0,            // _In_      DWORD  dwFlags,
    dir,          // _In_      LPCSTR lpMultiByteStr,
    -1,           // _In_      int    cbMultiByte,
    wp,           // _Out_opt_ LPWSTR lpWideCharStr,
    len1+1        // _In_      int    cchWideChar
  );

  --wp;
  *wp++='\\';

  memcpy(wp,file,len2*(sizeof *wp));
  wp+=len2;
  *wp++=0;

  return path;
// cleanup:
  //YASAPI_FREE(path);
path:
dir:
  return NULL;
}

void yafree(void *p)
{
  YA_FREE(p);
}
