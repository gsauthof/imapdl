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
#ifndef SSL_VERIFICATION_H
#define SSL_VERIFICATION_H

#include <log/log.h>
#include <boost/asio/ssl.hpp>

namespace Net {

  namespace SSL {

  class Verification {
    private:
      boost::log::sources::severity_logger<Log::Severity> &lg_;
      boost::asio::ssl::rfc2818_verification default_;

      unsigned pos_ = {0};
      bool result_ = {false};
      std::string fingerprint_;
    public:
      Verification(
          boost::log::sources::severity_logger<Log::Severity> &lg,
          const std::string &hostname,
          const std::string &fingerprint = std::string());
      bool operator()(bool preverified, boost::asio::ssl::verify_context & ctx);
  };
  }
}


#endif
