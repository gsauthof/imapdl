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
#include "client.h"
#include "options.h"
#include <log/log.h>
#include <net/ssl_util.h>

using namespace IMAP::Copy;

#include <exception>
#include <iostream>
#include <memory>
using namespace std;

#include <boost/log/sources/record_ostream.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/support/exception.hpp>

int main(int argc, char **argv)
{
  try {
    Options opts(argc, argv);
    // no std::move() because return value is an r-value
    boost::log::sources::severity_logger<Log::Severity> lg(Log::create(
            static_cast<Log::Severity>(opts.severity),
            static_cast<Log::Severity>(opts.file_severity),
            opts.logfile));

    try {
      BOOST_LOG(lg) << "Startup.";
      BOOST_LOG(lg) << "Username: |" << opts.username << "|";
      BOOST_LOG_SEV(lg, Log::INSANE) << "Password: |" << opts.password << "|";
      BOOST_LOG(lg) << "Parsing options ... done";

      boost::asio::io_service io_service;
      boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);

      unique_ptr<Net::Client::Base> net_client;
      if (opts.use_ssl) {
        unique_ptr<Net::Client::Base> c(
            new Net::TCP::SSL::Client::Base(io_service, context, opts, lg));
        net_client = std::move(c);
      } else {
        unique_ptr<Net::Client::Base> c(
            new Net::TCP::Client::Base(io_service, opts, lg));
        net_client = std::move(c);
      }
      IMAP::Copy::Client client(opts, *net_client, lg);

      io_service.run();
    } catch (const exception &e) {
      BOOST_LOG_SEV(lg, Log::ERROR) << e.what();

      auto tfu = boost::get_error_info<boost::throw_function>(e);
      auto tfi = boost::get_error_info<boost::throw_file>(e);
      auto tl  = boost::get_error_info<boost::throw_line>(e);
      BOOST_LOG_SEV(lg, Log::DEBUG) << "in "
        << (tfu?*tfu:"") << " (" << (tfi?*tfi:"") << ':' << (tl?*tl:0) << ")"
        ;
      auto si = boost::get_error_info<boost::log::current_scope_info>(e);
      if (si)
        BOOST_LOG_SEV(lg, Log::DEBUG) << "Scope stack: " << *si;

      //BOOST_LOG_SEV(lg, Log::ERROR) << boost::diagnostic_information(e);

      return 1;
    }
  } catch (const exception &e) {
    cerr << "Error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
