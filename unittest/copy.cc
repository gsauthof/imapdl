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
#include <boost/log/sources/record_ostream.hpp>

#include <copy/client.h>
#include <copy/options.h>
#include <example/server.h>
#include <net/ssl_util.h>
using namespace Net::SSL;


#include <ixxx/ixxx.h>
using namespace ixxx;

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <memory>
#include <stdexcept>
using namespace std;

#include <botan/pipe.h>
#include <botan/basefilt.h>
#include <botan/filters.h>
#include <botan/data_snk.h>
using namespace Botan;

static string ut_prefix()
{
  string prefix("../unittest");
  try {
    prefix = ansi::getenv("UT_PREFIX");
  } catch (ixxx::runtime_error) {
  }
  return prefix;
}

static string sha256_sum(const string &filename)
{
  ifstream in(filename, ios::binary);
  ostringstream out;

  Pipe pipe(new Chain(new Hash_Filter("SHA-256"),
        new Hex_Encoder(Hex_Encoder::Lowercase)),
        new DataSink_Stream(out));
  pipe.start_msg();
  in >> pipe;
  pipe.end_msg();

  return out.str();
}


class Replay_Server {
  private:
    int *rc_ = {nullptr};
    Server::Options opts_;
    string filename_;
    // workaround gcc/gnu stl not supporting move semantics for iostreams, yet
    // (as of gcc 4.8)
    unique_ptr<ofstream> out_;
  public:
    Replay_Server(int &rc, const string &filename,
        const string &log_filename,
        bool use_ssl = true, unsigned limit = 119, unsigned port = 6666);

    void operator()();
};
Replay_Server::Replay_Server(int &rc, const string &filename,
    const string &log_filename,
    bool use_ssl, unsigned limit, unsigned port)
  :
    rc_(&rc),
    filename_(filename),
    out_(new ofstream(log_filename))
{
  string prefix(ut_prefix());
  prefix += '/';
  opts_.limit = limit;
  opts_.use_ssl = use_ssl;
  opts_.key = prefix + "server.key";
  opts_.cert =  prefix + "server.crt";
  opts_.dhparam = prefix + "dh512.pem";
  opts_.replayfile = prefix + filename_;

  opts_.use_replay = true;
  opts_.port = port;
}
void Replay_Server::operator()()
{
  try {
    boost::asio::io_service io_service;
    Server::Main s(io_service, opts_, *out_);
    io_service.run();
  } catch (const exception &e) {
    *out_ << "exception in child: " << e.what() << '\n';
    *rc_ = 23;
    out_->flush();
  }
}

static void test_basic(bool use_ssl)
{
    string maildir{"tmp/cp/basicmd"};
    fs::remove_all(maildir);
    int rc = 0;
    thread replay_server{Replay_Server{rc, "cp_basic.trace", "ut_cp_server.log", use_ssl}};

    this_thread::sleep_for(chrono::seconds{1});

    string prefix(ut_prefix());
    prefix += '/';
    string configfile{prefix+"cp.conf"};
    char cconfigfile[128] = {0};
    strncpy(cconfigfile, configfile.c_str(), sizeof(cconfigfile)-1);
    char *argv[] = {
      (char*)"imapcp",
      (char*)"--account", (char*)"fake",
      (char*)"--log", (char*)"ut_cp.log", (char*)"--log_v",
      (char*)"--maildir", (char*)"tmp/cp/basicmd",
      (char*)"-v6",
      (char*)"--gwait", (char*)"400",
      (char*)"--config", cconfigfile,
      (char*)"--ssl", (char*)(use_ssl?"yes":"no"),
      0
    };
    int argc = sizeof(argv)/sizeof(char*)-1;

    IMAP::Copy::Options opts{argc, argv};
    boost::log::sources::severity_logger<Log::Severity> lg(std::move(Log::create(
            static_cast<Log::Severity>(opts.severity),
            static_cast<Log::Severity>(opts.file_severity),
            opts.logfile)));

    boost::asio::io_service io_service;
    boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);

      unique_ptr<Net::Client::Base> net_client;
      if (use_ssl) {
        unique_ptr<Net::Client::Base> c(
            new Net::TCP::SSL::Client::Base(io_service, context, opts, lg));
        net_client = std::move(c);
      } else {
        unique_ptr<Net::Client::Base> c(
            new Net::TCP::Client::Base(io_service, opts, lg));
        net_client = std::move(c);
      }

    IMAP::Copy::Client client(opts, std::move(net_client), lg);
    io_service.run();

    replay_server.join();
    BOOST_CHECK_EQUAL(rc, 0);

    set<string> sums;
    fs::directory_iterator begin(maildir + "/new");
    fs::directory_iterator end;
    for (auto i = begin; i != end; ++i) {
      string t{(*i).path().generic_string()};
      string sum{sha256_sum(t)};
      sums.insert(sum);
    }
    array<const char*, 3> ref = {{
      "6a8c8af376177fad7261d487fac2f5ebfa977820420470841335f6cbe9cb0bfa",
      "a456fb5e0073393d887806c852d775b9eb8276c6a0d7ee3b3345bfe1a5e1658a",
      "cb48864719e554c91fbf77849d06b8c8b23107eabe932d05cd332ce21f868d5b"
    }};
    BOOST_CHECK_EQUAL_COLLECTIONS(sums.begin(), sums.end(), ref.begin(), ref.end());
}

static void test_logindisabled()
{
  bool use_ssl = false;
  int rc = 0;
  thread replay_server{Replay_Server{rc, "logindisabled.trace", "ut_logindisabled_server.log", use_ssl, 10}};

  this_thread::sleep_for(chrono::seconds{1});

  string prefix(ut_prefix());
  prefix += '/';
  string configfile{prefix+"cp.conf"};
  char cconfigfile[128] = {0};
  strncpy(cconfigfile, configfile.c_str(), sizeof(cconfigfile)-1);
  char *argv[] = {
    (char*)"imapcp",
    (char*)"--account", (char*)"fake",
    (char*)"--log", (char*)"ut_logindisabled.log", (char*)"--log_v",
    (char*)"--maildir", (char*)"tmp/cp/basicmd",
    (char*)"-v6",
    (char*)"--gwait", (char*)"400",
    (char*)"--config", cconfigfile,
    (char*)"--ssl", (char*)(use_ssl?"yes":"no"),
    0
  };
  int argc = sizeof(argv)/sizeof(char*)-1;

  IMAP::Copy::Options opts{argc, argv};
  boost::log::sources::severity_logger<Log::Severity> lg(std::move(Log::create(
          static_cast<Log::Severity>(opts.severity),
          static_cast<Log::Severity>(opts.file_severity),
          opts.logfile)));

  boost::asio::io_service io_service;
  boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);

  unique_ptr<Net::Client::Base> net_client;
  if (use_ssl) {
    unique_ptr<Net::Client::Base> c(
        new Net::TCP::SSL::Client::Base(io_service, context, opts, lg));
    net_client = std::move(c);
  } else {
    unique_ptr<Net::Client::Base> c(
        new Net::TCP::Client::Base(io_service, opts, lg));
    net_client = std::move(c);
  }

  IMAP::Copy::Client client(opts, std::move(net_client), lg);
  BOOST_CHECK_THROW(io_service.run(), std::runtime_error);

  replay_server.join();
  BOOST_CHECK_EQUAL(rc, 23);
}

struct Log_Fixture {
  boost::log::sources::severity_logger<Log::Severity> lg;
  Log_Fixture()
    :
      lg(std::move(Log::create(
              static_cast<Log::Severity>(5),
              static_cast<Log::Severity>(7),
              "ut_cp.log")))
  {
    BOOST_LOG(lg) <<  "setup fixture" ;
  }
  ~Log_Fixture() {
    BOOST_LOG(lg) <<  "teardown fixture" ;
    boost::log::core::get()->remove_all_sinks();
  }

};

//BOOST_GLOBAL_FIXTURE(Log_Fixture);


BOOST_AUTO_TEST_SUITE( imapcp )
//BOOST_FIXTURE_TEST_SUITE( imapcp, Log_Fixture )

  BOOST_AUTO_TEST_CASE( basic )
  {
    //test_basic(lg, true);
    boost::log::core::get()->remove_all_sinks();
    test_basic(true);
  }

  BOOST_AUTO_TEST_CASE( basic_no_ssl )
  {
    //test_basic(lg, false);
    boost::log::core::get()->remove_all_sinks();
    test_basic(false);
  }

  BOOST_AUTO_TEST_CASE( logindisabled )
  {
    boost::log::core::get()->remove_all_sinks();
    test_logindisabled();
  }

BOOST_AUTO_TEST_SUITE_END()
