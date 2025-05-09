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
#include <../loader/loader/paths.h>

const wchar_t *basenamew(const wchar_t *s)
{
  const wchar_t *p=s+wcslen(s);

  while (s<p&&'/'!=p[-1]&&'\\'!=p[-1]&&':'!=p[-1])
    --p;

  return p;
}

wchar_t *yapath(wchar_t *file)
{
  const winamp_paths* paths = GetPaths();
  size_t len1,len2;
  wchar_t *path;
  const wchar_t *dir=paths->settings_sub_dir;

  if (NULL==dir)
    goto dir;

  len1=wcslen(dir);
  len2=wcslen(file);
  path=(wchar_t *)YA_MALLOC(((len1+1)+(len2+1))*(sizeof *path));

  if (NULL==path)
    goto path;
  
  return (wchar_t *)CombinePath(path,paths->settings_sub_dir,file);
// cleanup:
  //YASAPI_FREE(path);
path:
dir:
  return NULL;
}