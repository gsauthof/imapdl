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
#ifndef CSTRING_H
#define CSTRING_H

#include <functional>
#include <cstring>

namespace std
{
    template<> struct hash<const char*>
    {
      using argument_type = const char*;
      using result_type   = size_t;

      size_t operator()(argument_type s) const
      {
        size_t r = std::hash<std::string>()(string(s));
        return r;
      }
    };
    template<> struct equal_to<const char*>
    {
      using first_argument_type  = const char*;
      using second_argument_type = const char*;
      using result_type          = bool;
      bool operator()(first_argument_type lhs, second_argument_type rhs) const
      {
        return !std::strcmp(lhs, rhs);
      }
    };
}

#endif
