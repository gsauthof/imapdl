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
#include "state.h"
#include <enum.h>

namespace IMAP {
  namespace Copy {

    State operator++(State &s)
    {
      unsigned i = static_cast<unsigned>(s);
      ++i;
      s = static_cast<State>(i);
      return s;
    }
    State operator+(State s, int b)
    {
      unsigned i = static_cast<unsigned>(s);
      i += b;
      s = static_cast<State>(i);
      return s;
    }
    static const char *const state_map[] = {
      "DISCONNECTED",
      "ESTABLISHED",
      "GOT_INITIAL_CAPABILITIES",
      "LOGGED_IN",
      "GOT_CAPABILITIES",
      "SELECTED_MAILBOX",
      "FETCHING",
      "FETCHED",
      "STORED",
      "EXPUNGED",
      "LOGGING_OUT",
      "LOGGED_OUT",
      "END"
    };
    std::ostream &operator<<(std::ostream &o, State s)
    {
      o << enum_str(state_map, s);
      return o;
    }

  }
}
