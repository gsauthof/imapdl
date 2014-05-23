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
#ifndef UNITTEST_DATA_H
#define  UNITTEST_DATA_H

namespace IMAP {
  namespace Test {
    namespace Dovecot {

    extern const char prologue[];
    extern const char fetch_head[];
    extern const char fetch[];
    extern const char fetch_tail[];
    extern const char epilogue[];

    }
    namespace Cyrus {
      extern const char *response[];
      extern const char message1[];
      extern const char message2[];
    }
    namespace GMAIL {
      extern const char * const received[];
      extern const char message1[];
    }
  }
}

#endif
