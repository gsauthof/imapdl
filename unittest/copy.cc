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

class Client_Frontend {
  private:
    IMAP::Copy::Options opts;
    boost::log::sources::severity_logger<Log::Severity> lg;
    boost::asio::io_service io_service;
    boost::asio::ssl::context context;
    unique_ptr<Net::Client::Base> net_client;
    IMAP::Copy::Client client;
  public:
    Client_Frontend(int argc, char **argv, bool use_ssl)
      :
        opts(argc, argv),
        lg(Log::create(
                static_cast<Log::Severity>(opts.severity),
                static_cast<Log::Severity>(opts.file_severity),
                opts.logfile)),
        context(boost::asio::ssl::context::sslv23),
        net_client(use_ssl?
            (static_cast<Net::Client::Base*>(new Net::TCP::SSL::Client::Base(io_service, context, opts, lg)))
            :
            (static_cast<Net::Client::Base*>(new Net::TCP::Client::Base(io_service, opts, lg)))
            ),

        client(opts, *net_client, lg)

        {
        }
    void run()
    {
      io_service.run();
    }
};

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

    Client_Frontend client(argc, argv, use_ssl);
    client.run();

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

  Client_Frontend client(argc, argv, use_ssl);
  BOOST_CHECK_THROW(client.run(), std::runtime_error);

  replay_server.join();
  BOOST_CHECK_EQUAL(rc, 23);
}


static void test_partial_1(bool use_ssl)
{
  string maildir{"tmp/cp/partial"};
  string journal{"tmp/fake.journal"};
  fs::remove_all(maildir);
  fs::remove(journal);
  int rc = 0;
  thread replay_server{Replay_Server{rc, "part1.trace", "ut_partial1_server.log", use_ssl, 5}};

  this_thread::sleep_for(chrono::seconds{1});

  string prefix(ut_prefix());
  prefix += '/';
  string configfile{prefix+"cp.conf"};
  char cconfigfile[128] = {0};
  strncpy(cconfigfile, configfile.c_str(), sizeof(cconfigfile)-1);
  char *argv[] = {
    (char*)"imapcp",
    (char*)"--account", (char*)"fake",
    (char*)"--log", (char*)"ut_partial1.log", (char*)"--log_v",
    (char*)"--maildir", (char*)"tmp/cp/partial",
    (char*)"-v6",
    (char*)"--gwait", (char*)"400",
    (char*)"--config", cconfigfile,
    (char*)"--ssl", (char*)(use_ssl?"yes":"no"),
    (char*)"--journal", (char*)journal.c_str(),
    0
  };
  int argc = sizeof(argv)/sizeof(char*)-1;
  {
    Client_Frontend client(argc, argv, true);
    BOOST_CHECK_THROW(client.run(), std::runtime_error);
  }

  replay_server.join();
  BOOST_CHECK_EQUAL(rc, 23);

  set<string> sums;
  fs::directory_iterator begin(maildir + "/new");
  fs::directory_iterator end;
  for (auto i = begin; i != end; ++i) {
    string t{(*i).path().generic_string()};
    string sum{sha256_sum(t)};
    sums.insert(sum);
  }
  array<const char*, 2> ref = {{
    "3ed2b804502b2b81f7aa37494401d21fd4314121c52174c68ebf739b7e57abd5",
      "a7f858db4ee1035f87b9df1ecbde7bb9d27da2b8aa044380d5706c3b5784039d"
  }};
  BOOST_CHECK_EQUAL_COLLECTIONS(sums.begin(), sums.end(), ref.begin(), ref.end());

  BOOST_CHECK_EQUAL(fs::exists(journal), true);
}

static void test_partial_2(bool use_ssl)
{
  string maildir{"tmp/cp/partial"};
  string journal{"tmp/fake.journal"};
  BOOST_CHECK_EQUAL(fs::exists(journal), true);
  int rc = 0;
  thread replay_server{Replay_Server{rc, "part2.trace", "ut_partial2_server.log", use_ssl}};

  this_thread::sleep_for(chrono::seconds{1});

  string prefix(ut_prefix());
  prefix += '/';
  string configfile{prefix+"cp.conf"};
  char cconfigfile[128] = {0};
  strncpy(cconfigfile, configfile.c_str(), sizeof(cconfigfile)-1);
  char *argv[] = {
    (char*)"imapcp",
    (char*)"--account", (char*)"fake",
    (char*)"--log", (char*)"ut_partial2.log", (char*)"--log_v",
    (char*)"--maildir", (char*)"tmp/cp/partial",
    (char*)"-v6",
    (char*)"--gwait", (char*)"400",
    (char*)"--config", cconfigfile,
    (char*)"--ssl", (char*)(use_ssl?"yes":"no"),
    (char*)"--journal", (char*)journal.c_str(),
    0
  };
  int argc = sizeof(argv)/sizeof(char*)-1;
  {
    Client_Frontend client(argc, argv, true);
    client.run();
  }

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
    "3ed2b804502b2b81f7aa37494401d21fd4314121c52174c68ebf739b7e57abd5",
      "4c473d113c4b7a7efb33797380e6c230055571b528f5f1392ffb176e3de88a5b",
      "a7f858db4ee1035f87b9df1ecbde7bb9d27da2b8aa044380d5706c3b5784039d"
  }};
  BOOST_CHECK_EQUAL_COLLECTIONS(sums.begin(), sums.end(), ref.begin(), ref.end());

  BOOST_CHECK_EQUAL(fs::exists(journal), false);
}

static void test_fetch_header()
{
  bool use_ssl = false;
  int rc = 0;
  thread replay_server{Replay_Server{rc, "fetch_header.trace", "ut_header_server.log", use_ssl, 10}};

  this_thread::sleep_for(chrono::seconds{1});

  string prefix(ut_prefix());
  prefix += '/';
  string configfile{prefix+"cp.conf"};
  char cconfigfile[128] = {0};
  strncpy(cconfigfile, configfile.c_str(), sizeof(cconfigfile)-1);
  char *argv[] = {
    (char*)"imapcp",
    (char*)"--account", (char*)"fake",
    (char*)"--log", (char*)"ut_fetch_header.log", (char*)"--log_v",
    (char*)"--maildir", (char*)"tmp/cp/basicmd",
    (char*)"-v6",
    (char*)"--gwait", (char*)"400",
    (char*)"--config", cconfigfile,
    (char*)"--ssl", (char*)(use_ssl?"yes":"no"),
    (char*)"--header",
    0
  };
  int argc = sizeof(argv)/sizeof(char*)-1;

  const char filename[] = "tmp/fetch_header.log";
  fs::create_directory("tmp");
  fs::remove(filename);
  {
    Client_Frontend client(argc, argv, use_ssl);
    Log::setup_vanilla_file(Log::MSG, filename);
    client.run();
  }
  boost::log::core::get()->remove_all_sinks();

  replay_server.join();
  BOOST_CHECK_EQUAL(rc, 0);

  const char ref[] =
R"([MSG] DATE       Mon, 2 Jun 2014 23:09:55 +0200
[MSG] FROM       Georg Sauthoff <mail@georg.so>
[MSG] SUBJECT    test1
[MSG] DATE       Mon, 2 Jun 2014 23:10:13 +0200
[MSG] FROM       Georg Sauthoff <mail@georg.so>
[MSG] SUBJECT    test2
[MSG] DATE       Mon, 2 Jun 2014 23:10:25 +0200
[MSG] FROM       Georg Sauthoff <mail@georg.so>
[MSG] SUBJECT    test3
)";
  std::array<char, 512> buffer = {{0}};
  {
    ifstream f(filename, ifstream::in | ifstream::binary);
    f.read(buffer.data(), buffer.size()-1);
  }
  BOOST_CHECK_EQUAL(buffer.data(), ref);
}

static void test_list()
{
  bool use_ssl = false;
  int rc = 0;
  fs::create_directory("tmp");
  thread replay_server{Replay_Server{rc, "list.trace", "tmp/ut_list_server.log", use_ssl, 10}};

  this_thread::sleep_for(chrono::seconds{1});

  string prefix(ut_prefix());
  prefix += '/';
  string configfile{prefix+"cp.conf"};
  char cconfigfile[128] = {0};
  strncpy(cconfigfile, configfile.c_str(), sizeof(cconfigfile)-1);
  char *argv[] = {
    (char*)"imapcp",
    (char*)"--account", (char*)"fake",
    (char*)"--log", (char*)"tmp/ut_list.log", (char*)"--log_v",
    (char*)"--maildir", (char*)"tmp/cp/basicmd",
    (char*)"-v6",
    (char*)"--gwait", (char*)"400",
    (char*)"--config", cconfigfile,
    (char*)"--ssl", (char*)(use_ssl?"yes":"no"),
    (char*)"--list",
    0
  };
  int argc = sizeof(argv)/sizeof(char*)-1;

  const char filename[] = "tmp/list.log";
  fs::remove(filename);
  try {
    Client_Frontend client(argc, argv, use_ssl);
    Log::setup_vanilla_file(Log::MSG, filename);
    client.run();
  } catch (const exception &e) {
    cerr << "Client frontend failed with: " << e.what() << '\n';
  }
  boost::log::core::get()->remove_all_sinks();

  replay_server.join();
  BOOST_CHECK_EQUAL(rc, 0);

  const char ref[] =
R"([MSG] Mailbox: -  INBOX
[MSG] Mailbox: -| Drafts
[MSG] Mailbox: -| Sent
[MSG] Mailbox: -| Trash
[MSG] Mailbox: -| junk
)";
  std::array<char, 512> buffer = {{0}};
  {
    ifstream f(filename, ifstream::in | ifstream::binary);
    f.read(buffer.data(), buffer.size()-1);
  }
  BOOST_CHECK_EQUAL(buffer.data(), ref);
}

struct Log_Fixture {
  boost::log::sources::severity_logger<Log::Severity> lg;
  Log_Fixture()
    :
      lg(Log::create(
              static_cast<Log::Severity>(5),
              static_cast<Log::Severity>(7),
              "ut_cp.log"))
  {
    BOOST_LOG(lg) <<  "setup fixture" ;
  }
  ~Log_Fixture() {
    BOOST_LOG(lg) <<  "teardown fixture" ;
    boost::log::core::get()->remove_all_sinks();
  }

};

//BOOST_GLOBAL_FIXTURE(Log_Fixture);


BOOST_AUTO_TEST_SUITE( copy )
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

  BOOST_AUTO_TEST_CASE(partial)
  {
    boost::log::core::get()->remove_all_sinks();
    test_partial_1(true);
    boost::log::core::get()->remove_all_sinks();
    test_partial_2(true);
  }
  BOOST_AUTO_TEST_CASE(fetch_header)
  {
    boost::log::core::get()->remove_all_sinks();
    test_fetch_header();
  }
  BOOST_AUTO_TEST_CASE(list)
  {
    boost::log::core::get()->remove_all_sinks();
    test_list();
  }

BOOST_AUTO_TEST_SUITE_END()
