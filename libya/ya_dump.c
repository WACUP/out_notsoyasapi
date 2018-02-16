/*
 * ya_dump.c
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
#if defined (YA_DUMP) // {

///////////////////////////////////////////////////////////////////////////////
Dump dump;

int DumpCreate(Dump *pDump, const char *label, const wchar_t *path)
{
  pDump->label=label;
  pDump->path=path;

  return 0;
}

void DumpDestroy(Dump *pDump)
{
}

FILE *DumpOpenFile(Dump *pDump)
{
  if (pDump->path) {
    wchar_t filepath[MAX_PATH]={0};
    wchar_t *pp;
    size_t len;
    SYSTEMTIME time={0};
    FILE *f;

    wcscpy(filepath,pDump->path);
    len=wcslen(filepath)-4;
    pp=filepath+len;

    GetSystemTime(&time);

    StringCchPrintf(pp,
      (sizeof filepath)-len,
      L"-message"
      L"-%04d"            // wYear
      L"-%02d"            // wMonth
      L"-%02d"            // wDay
      L"-%02d"            // wHour
      L"-%02d"            // wMinute
      L"-%02d"            // wSecond
      L"-%04d"            // wMilliseconds
      L".txt",
      time.wYear,
      time.wMonth,
      time.wDay,
      time.wHour,
      time.wMinute,
      time.wSecond,
      time.wMilliseconds
    );

    if (NULL==(f=_wfopen(filepath,L"w")))
      return NULL;
    else {
      fseek(f,0,SEEK_END);
      fputs(pDump->label,f);
      fputc('\n',f);

      return f;
    }
  }
  else
    return NULL;
}
#endif // }
