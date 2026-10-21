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

/*const wchar_t *basenamew(const wchar_t *s)
{
  const wchar_t *p=s+wcslen(s);

  while (s<p&&'/'!=p[-1]&&'\\'!=p[-1]&&':'!=p[-1])
    --p;

  return p;
}*/

const wchar_t *yapath(const wchar_t *file)
{
    static const wchar_t* path;
    if (!path)
{
        wchar_t temp[MAX_PATH];
        path = CombinePathDup(temp, GetPaths()->settings_sub_dir, file);
    }
    return path;
}