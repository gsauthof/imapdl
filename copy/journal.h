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
#ifndef IMAP_COPY_SERIALIZE_H
#define IMAP_COPY_SERIALIZE_H

#include <string>
#include <vector>
#include <utility>
#include <stdint.h>

class Sequence_Set;

namespace IMAP {
  namespace Copy {

    struct Journal {
      std::string mailbox_;
      uint32_t uidvalidity_ {0};
      std::vector<std::pair<uint32_t, uint32_t> > uids_;

      Journal();
      Journal(const std::string &mailbox, uint32_t uidvalidity, const Sequence_Set &set);
      void read(const std::string &filename);
      void write(const std::string &filename);
    };

  }
}

#endif
