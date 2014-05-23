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
#ifndef ENUM_H
#define ENUM_H

#include <typeinfo>
#include <stdexcept>

template <size_t e_map_bytes, typename Enum>
const char *enum_strP(const char * const *e_map, Enum c)
{
  if (!(c > Enum::FIRST_ && c < Enum::LAST_))
    throw std::out_of_range(std::string(typeid(c).name()) + " out of range");
  static_assert(e_map_bytes/sizeof(const char*) ==
      static_cast<unsigned>(Enum::LAST_) - 1,
      "command map has not enough/too many entries");
  unsigned i = static_cast<unsigned>(c)-1;
  return e_map[i];
}

#define enum_str(M, E) enum_strP<sizeof(M)>(M, E)

#endif
