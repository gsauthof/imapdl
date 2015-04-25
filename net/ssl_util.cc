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
#include "ssl_util.h"

#include <boost/asio/ssl.hpp>

#include <stdexcept>

using namespace std;
namespace asio = boost::asio;

namespace Net {

  namespace SSL {

    // test it via:
    //     openssl ciphers -v  'TLSv1.2+HIGH:!aNULL:!eNULL@STRENGTH'
    // and
    //
    //     openssl s_client -connect host:post -cipher ...
    namespace Cipher {

      bool operator<(Class a, Class b)
      {
        return static_cast<unsigned>(a) < static_cast<unsigned>(b);
      }
      bool operator>(Class a, Class b)
      {
        return static_cast<unsigned>(a) > static_cast<unsigned>(b);
      }
      Class to_class(unsigned i)
      {
        if (!(i > static_cast<unsigned>(Class::FIRST_) &&
              i < static_cast<unsigned>(Class::LAST_)))
          throw range_error("to_class() range error");
        return static_cast<Class>(i);
      }

      static const char * const default_list_array[] = {
        // only ones with forward secrecy
        // (cf. http://www.postfix.org/FORWARD_SECRECY_README.html)
        "TLSv1.2+HIGH+AECDH:"
          "TLSv1.2+HIGH+ECDHE:"
          "TLSv1.2+HIGH+ADH:"
          "TLSv1.2+HIGH+EDH:"
          "TLSv1.2+HIGH+DHE:"
          "!aNULL:!eNULL",
        // only recent stuff
        "TLSv1.2+HIGH:!aNULL:!eNULL@STRENGTH",
        // for old servers (e.g. Ubuntu 10.04)
        "TLSv1+HIGH:!aNULL:!eNULL",
        0
      };

      const char *default_list(Class c)
      {
        if (!(c>Class::FIRST_ && c<Class::LAST_))
          throw out_of_range("Cipher::Class out of range");
        return default_list_array[static_cast<unsigned>(c)-1];
      }
    }

    namespace Context {

      void set_defaults(boost::asio::ssl::context &context)
      {
        context.set_options(
            asio::ssl::context::default_workarounds
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::no_sslv3
            // only allow tls1_1, tls1_2 ...
            | asio::ssl::context::no_tlsv1
            | asio::ssl::context::single_dh_use);
      }
    }



  }

}


