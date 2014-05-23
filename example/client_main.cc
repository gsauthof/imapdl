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

#include <net/ssl_util.h>

#include <iostream>
#include <exception>

using namespace Net::SSL;

int main(int argc, char **argv)
{
  try {
    Client::Options opts(argc, argv);

    boost::asio::io_service io_service;

    boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);
    Context::set_defaults(context);
    if (opts.fingerprint.empty())
      context.load_verify_file(opts.ca_file);
    if (!opts.ca_path.empty())
      context.add_verify_path(opts.ca_path);

    Client::Main c(io_service, context, opts);
    io_service.run();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
