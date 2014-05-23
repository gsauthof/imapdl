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
#ifndef SSL_UTIL_H
#define SSL_UTIL_H

#include <boost/asio/ssl.hpp>
#include <ostream>
#include <string>

namespace Net {

  namespace SSL {

    namespace Cipher {

      enum class Class {
        FIRST_,
        FORWARD,
        V2,
        OLD,
        LAST_
      };

      bool operator<(Class a, Class b);
      bool operator>(Class a, Class b);
      Class to_class(unsigned i);
      const char *default_list(Class c);
    }

    namespace Context {
      void set_defaults(boost::asio::ssl::context &context);
    }
  }
}


#endif
