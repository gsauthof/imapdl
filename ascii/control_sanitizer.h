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
#ifndef ASCI_CONTROL_SANITIZER_H
#define ASCI_CONTROL_SANITIZER_H

#include <buffer/buffer.h>

namespace ASCII {
  namespace Control {

    class Sanitizer {
      private:
        int cs {0};
        Memory::Buffer::Base &u_buffer_;
        bool seen_ctl_ {false};
      public:
        Sanitizer(Memory::Buffer::Base &dst);
        void read(const char *begin, const char *end);
        bool seen_ctl() const;
    };
  }
}

#endif
