/*
 * yasapi_util.c
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

// https://en.wikipedia.org/wiki/Greatest_common_divisor#Binary_method
int gcd(int m, int n)
{
  int k=0;

  while (0==(m&1)&&0==(n&1)) {
    m>>=1;
    n>>=1;
    ++k;
  }

  while (m!=n) {
    if (0==(m&1))
      m>>=1;
    else if (0==(n&1))
      n>>=1;
    else if (m>n) {
      m-=n;
      m>>=1;
    }
    else {
      n-=m;
      n>>=1;
    }
  }

  return m*(1<<k);
}
