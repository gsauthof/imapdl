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
#ifndef EXAMPLE_SERVER_H
#define EXAMPLE_SERVER_H

#include <iostream>
#include <ostream>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>

namespace Server {

  using namespace std;
  using boost::asio::ip::tcp;
  using ssl_socket = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
  namespace asio = boost::asio;

  class Options {
    private:
      ostream &out_;
      void fix();

    public:
      unsigned port {6666};
      string cipher;
      unsigned cipher_preset {1};
      bool use_ssl {false};
      string dhparam;
      string cert;
      string key;

      bool use_replay {false};
      string tracefile;
      string replayfile;
      unsigned limit {0};


      Options(ostream &out = cout);
      Options(int argc, char **argv, ostream &out = cout);

  };

  class Main {
    private:
      std::ostream &out_;
      const Options &opts_;
      boost::asio::io_service &io_service_;
      tcp::acceptor acceptor_;
      tcp::socket socket_;
      boost::asio::ssl::context context_;
      boost::asio::signal_set signals_;
      asio::basic_waitable_timer<std::chrono::steady_clock> limit_timer_;

      void do_ssl_accept();
      void do_accept();

    public:
      Main(boost::asio::io_service& io_service, const Options &opts,
          std::ostream &out = std::cout);

      void cancel_limit();

      std::ostream &out() const { return out_; }

  };

}

#endif
