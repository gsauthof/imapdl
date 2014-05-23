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
#ifndef SEQUENCE_SET_H
#define SEQUENCE_SET_H

#include <vector>
#include <inttypes.h>
#include <stddef.h>

class Sequence_Set_Priv;
class Sequence_Set {

  private:
    Sequence_Set_Priv *d {nullptr};
    Sequence_Set(const Sequence_Set&) =delete;
    Sequence_Set &operator=(const Sequence_Set&) =delete;
  public:
    Sequence_Set();
    ~Sequence_Set();
    void   push(uint32_t id);
    void   copy(std::vector<std::pair<uint32_t, uint32_t> > &v) const;
    size_t size() const;
    void   clear();
};

#endif
