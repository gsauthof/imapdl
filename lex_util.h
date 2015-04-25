// Copyright 2014, Georg Sauthoff <mail@georg.so>

/* {{{ GPLv3

    This file is part of imapdl.

    imapdl is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    imapdl is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with imapdl.  If not, see <http://www.gnu.org/licenses/>.

}}} */
#ifndef LEX_UTIL_H
#define LEX_UTIL_H

#include <ostream>
#include <stddef.h>

void safely_write(std::ostream &o, const char *begin, size_t n);

void throw_lex_error(const char *msg, const char *begin, const char *p, const char *pe);

void pp_buffer(std::ostream &o, const char *msg, const char *c, size_t size);

#endif
