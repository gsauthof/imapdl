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
#ifndef EXAMPLE_CLIENT_H
#define EXAMPLE_CLIENT_H

#include <string>
#include <fstream>
#include <chrono>
#include <array>
#include <queue>
#include <vector>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>

namespace Client {

  using namespace std;
  using boost::asio::ip::tcp;
  namespace asio = boost::asio;

  class Options {
    public:
      string host;
      string service; // or port
      string fingerprint;
      string cipher;
      unsigned cipher_preset {1};
      string cert_host;
      string ca_file;
      string ca_path;
      bool use_ssl {false};

      string tracefile;
      string replayfile;
      unsigned limit {0};

      Options();
      Options(int argc, char **argv);

    private:
      void fix();
  };

  class Main {
    private:
      const Options &opts_;
      chrono::time_point<std::chrono::steady_clock> start_;

      tcp::resolver resolver_;
      tcp::socket socket_;

      bool use_ssl_ { false };
      asio::ssl::context &context_;
      asio::ssl::stream<tcp::socket> ssl_socket_;

      vector<char> data_;

      asio::posix::stream_descriptor in_;
      vector<char> inp_;

      queue<vector<char> > write_queue_;

      boost::asio::signal_set signals_;
      unsigned signaled_ {0};

      bool use_log_ { false };
      ofstream tracefile_;
      unique_ptr<boost::archive::text_oarchive> oarchive_;

      bool use_replay_ { false };
      vector<char> expected_data_;
      ifstream replayfile_;
      unique_ptr<boost::archive::text_iarchive> iarchive_;
      // asio::steady_timer timer_;
      // workaround: boost autodetection of std::chrono
      // does not seem to work in all boost versions
      asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
      asio::basic_waitable_timer<std::chrono::steady_clock> limit_timer_;

      asio::ssl::stream<tcp::socket>::lowest_layer_type &socket();
      void do_signal_wait();
      void do_last_write_close();
      void do_keyboard_read();
      void do_write();
      void do_ssl_connect(tcp::resolver::iterator iterator);
      void do_handshake();
      void do_connect(tcp::resolver::iterator iterator);
      void start_replay();
      void do_replay();
      void do_close();
      void do_read();
      void do_shutdown();
    public:
      Main(asio::io_service &io_service,
          asio::ssl::context &context,
          const Options &opts
          );
      ~Main();
  };

}

#endif
