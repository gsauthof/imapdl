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
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


#include <example/client.h>
#include <example/server.h>
#include <net/ssl_util.h>
using namespace Net::SSL;


#include <ixxx/ixxx.h>
using namespace ixxx;

#include <iostream>
using namespace std;

static string ut_prefix()
{
  string prefix("../unittest");
  try {
    prefix = ansi::getenv("UT_PREFIX");
  } catch (const ixxx::runtime_error &) {
  }
  return prefix;
}

static void basic_server_in_child()
{
  try {
    string prefix(ut_prefix());
    prefix += '/';
    Server::Options opts;
    opts.limit = 119;
    opts.use_ssl = true;
    opts.key = prefix + "server.key";
    opts.cert =  prefix + "server.crt";
    opts.dhparam = prefix + "dh2048.pem";
    opts.replayfile = prefix + "simple.log";
    // XXX remove from that class
    opts.use_replay = true;
    opts.port = 6666;
    boost::asio::io_service io_service;
    fs::create_directory("tmp");
    const char filename[] = "tmp/replay_server.log";
    fs::remove(filename);
    ofstream f(filename);
    Server::Main s(io_service, opts, f);
    io_service.run();
  } catch (const exception &e) {
    cerr << "excpetion in child: " << e.what() << '\n';
    _exit(23);
  }
  _exit(0);
}


BOOST_AUTO_TEST_SUITE( replay )

  BOOST_AUTO_TEST_CASE( basic )
  {
    int id = posix::fork();
    if (id) {
      timespec ts = {.tv_sec = 1};
      posix::nanosleep(&ts, nullptr);

      string prefix(ut_prefix());
      prefix += '/';
      Client::Options opts;
      opts.limit = 120;
      opts.replayfile = prefix + "simple.log";
      opts.ca_file = prefix + "server.crt";
      opts.fingerprint = "ED77CA3CE8B917C3F081FEC35C316E17E7879D35";
      opts.host = "localhost";
      opts.service = "6666";
      opts.use_ssl = true;

      boost::asio::io_service io_service;

      boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);
      Context::set_defaults(context);
      context.load_verify_file(opts.ca_file);
      if (!opts.ca_path.empty())
        context.add_verify_path(opts.ca_path);

      Client::Main c(io_service, context, opts);
      io_service.run();

      siginfo_t info = {0};

      posix::waitid(P_PID, id, &info, WEXITED);
      BOOST_CHECK_EQUAL(info.si_code, CLD_EXITED);
      BOOST_CHECK_EQUAL(info.si_status, 0);
    } else {
      // 2 rules when forking in in the boost unit test framework (UTF):
      // - don't use BOOST_*EQUAL etc. macros in the child
      // - exit with _exit() from the child - otherwise (with exit()) UTF handler
      //   are called in the child, or remaining tests are
      //   executed in the child as well
      basic_server_in_child();
    }
    
  }

BOOST_AUTO_TEST_SUITE_END()
