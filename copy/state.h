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
#ifndef COPY_STATE_H
#define COPY_STATE_H

#include <ostream>

namespace IMAP {
  namespace Copy {

    enum class Task : unsigned {
      FIRST_,
      CLEANUP,
      DOWNLOAD,
      LAST_
    };
    enum class State : unsigned {
      FIRST_,
      DISCONNECTED,
      ESTABLISHED,
      GOT_INITIAL_CAPABILITIES,
      LOGGED_IN,
      GOT_CAPABILITIES,
      SELECTED_MAILBOX,
      FETCHING,
      FETCHED,
      STORED,
      EXPUNGED,
      LOGGING_OUT,
      LOGGED_OUT,
      END,
      LAST_
    };
    State operator++(State &s);
    State operator+(State s, int b);
    std::ostream &operator<<(std::ostream &o, State s);

  }
}

#endif
