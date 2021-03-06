/* Copyright (C) 1992, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "swab.h"

// we use this because it works with overlapping buffers (undefined in POSIX)
static inline void swab_private (const void *bfrom, void *bto, ssize_t n)
{
  const char *from = (const char *) bfrom;
  char *to = (char *) bto;

  n &= ~((ssize_t) 1);
  while (n > 1)
    {
      const char b0 = from[--n], b1 = from[--n];
      to[n] = b0;
      to[n + 1] = b1;
    }
}

#ifndef HAVE_SWAB
void swab (const void *bfrom, void *bto, ssize_t n)
{
	swab_private(bfrom, bto, n);
}
#endif

void swabs (void *buf, ssize_t n)
{
	swab_private(buf, buf, n);
}
