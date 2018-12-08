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
#include <boost/algorithm/string.hpp>
namespace fs = boost::filesystem;

#include <cstring>
#include <fstream>
#include <iterator>
#include <array>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <iostream>

#include <imap/client_parser.h>
#include <imap/imap.h>
#include "data.h"
#include <maildir/maildir.h>
#include <buffer/file.h>

using namespace std;

BOOST_AUTO_TEST_SUITE( imap_client_parser )

  BOOST_AUTO_TEST_SUITE( basic )

    BOOST_AUTO_TEST_CASE( finished )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
"* OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE AUTH=PLAIN] Dovecot ready.\r\n"
;
      const char *begin = response;
      const char *end = response + sizeof(response)-1;
      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Proxy proxy;
        unsigned t {0};
        void imap_untagged_status_end(Status c) override
        {
          ++t;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.proxy, cb.proxy, cb);
      BOOST_CHECK_EQUAL(p.finished(), true);
      BOOST_CHECK_EQUAL(p.in_start(), true);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.t, 1);
      BOOST_CHECK_EQUAL(p.finished(), true);
      BOOST_CHECK_EQUAL(p.in_start(), true);
    }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE( tagged )

    BOOST_AUTO_TEST_CASE( ok )
    {
      using namespace IMAP::Server::Response;
      const char response[] = "A123 OK everything fine\r\n";
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t { 0 };
        void imap_tagged_status_begin() override
        {
          string s(tag_buffer.begin(), tag_buffer.end());
          BOOST_CHECK_EQUAL(s, "A123");
          t+=11;
        }
        void imap_tagged_status_end(Status c) override
        {
          BOOST_CHECK_EQUAL(c, IMAP::Server::Response::Status::OK);
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s, "everything fine");
          t+=12;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.t, 23);
    }

    BOOST_AUTO_TEST_CASE( multiple )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "A123 OK everything fine\r\n"
        "23 NO no way\r\n"
        "x6 BAD who is ...\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

        static const char *tag[] = {
          "A123",
          "23",
          "x6"
        };
        static const char *msg[] = {
          "everything fine",
          "no way",
          "who is ..."
        };
        static const IMAP::Server::Response::Status status[] = {
          IMAP::Server::Response::Status::OK,
          IMAP::Server::Response::Status::NO,
          IMAP::Server::Response::Status::BAD
        };

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t {0};
        unsigned i {0};
        void imap_tagged_status_begin() override
        {
          string s(tag_buffer.begin(), tag_buffer.end());
          BOOST_CHECK_EQUAL(s, tag[i]);
          t+=4;
        }
        void imap_tagged_status_end(Status c) override
        {
          BOOST_CHECK_EQUAL(c, status[i]);
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s, msg[i]);
          t+=64;;
          ++i;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.t, 3*4+3*64);
      BOOST_CHECK_EQUAL(cb.i, 3);
    }

    BOOST_AUTO_TEST_CASE( attribute )
    {
      using namespace IMAP::Server::Response;
      const char response[] = "a002 OK [READ-WRITE] SELECT completed\r\n";
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(3) { }
        void imap_tagged_status_begin() override
        {
          string s(tag_buffer.begin(), tag_buffer.end());
          BOOST_CHECK_EQUAL(s, "a002");
          ++a[0];
        }
        void imap_tagged_status_end(Status c) override
        {
          BOOST_CHECK_EQUAL(c, IMAP::Server::Response::Status::OK);
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s, "SELECT completed");
          ++a[1];
        }
        void imap_status_code(Status_Code c) override
        {
          BOOST_CHECK_EQUAL(c, Status_Code::READ_WRITE);
          ++a[2];
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      for (unsigned i = 0; i<3; ++i)
        BOOST_CHECK_EQUAL(cb.a[i], 1);
    }

  BOOST_AUTO_TEST_SUITE_END();


  BOOST_AUTO_TEST_SUITE( untagged )

    BOOST_AUTO_TEST_CASE( bye )
    {
      using namespace IMAP::Server::Response;
      const char response[] = "* bYe good bye!\r\n";
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t { 0 };
        void imap_untagged_status_begin(Status c) override
        {
          BOOST_CHECK_EQUAL(c, IMAP::Server::Response::Status::BYE);
          t+=2;
        }
        void imap_untagged_status_end(Status c) override
        {
          BOOST_CHECK_EQUAL(c, IMAP::Server::Response::Status::BYE);
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s, "good bye!");
          t+=128;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.t, 130);
    }

    BOOST_AUTO_TEST_CASE( exists )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* 18 EXISTS\r\n"
        "* 23 ReCent\r\n"
        "* 4294967295 EXISTS\r\n"
        ;
      static unsigned number[] = {
        18u,
        4294967295u
      };
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t { 0 };
        unsigned u { 0 };
        unsigned i { 0 };
        void imap_data_exists(unsigned n) override
        {
          BOOST_CHECK_EQUAL(n, number[i]);
          ++t;
          ++i;
        }
        void imap_data_recent(unsigned n) override
        {
          BOOST_CHECK_EQUAL(n, 23);
          ++u;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.t, 2);
      BOOST_CHECK_EQUAL(cb.u, 1);
    }

    BOOST_AUTO_TEST_CASE( number_limit )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* 4294967296 EXISTS\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t { 0 };
        void imap_data_exists(unsigned n) override
        {
          ++t;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      bool caught = false;
      try {
        p.read(begin, end);
      } catch (const exception &e) {
        caught = true;
      }
      BOOST_CHECK_EQUAL(caught, true);
      BOOST_CHECK_EQUAL(cb.t, 0);
    }

    BOOST_AUTO_TEST_CASE( flags )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      static vector<bool> flagged(6);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t { 0 };
        unsigned u { 0 };
        void imap_data_flags_begin() override
        {
          ++t;
        }
        void imap_data_flags_end() override
        {
          ++u;
        }
        void imap_flag(IMAP::Flag flag) override
        {
          flagged[static_cast<unsigned>(flag)] = true;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.t, 1);
      BOOST_CHECK_EQUAL(cb.u, 1);
      for (unsigned i = 1; i<static_cast<unsigned>(IMAP::Flag::RECENT); ++i)
        BOOST_CHECK_EQUAL(flagged[i], true);
      BOOST_CHECK_EQUAL(flagged[static_cast<unsigned>(IMAP::Flag::RECENT)], false);
    }

    BOOST_AUTO_TEST_CASE( attribute )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* OK [UNSEEN 17] Message 17 is the first unseen message\r\n"
        "* OK [UIDNEXT 23] UIDs valid\r\n"
        "* OK [UIDVALIDITY 3857529045] UIDs valid\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      static vector<unsigned> code(static_cast<unsigned>(Status_Code::UNSEEN)+1);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(4) {}
        void imap_status_code(Status_Code c) override
        {
          BOOST_CHECK_EQUAL(a.size(), 4);
          ++a[0];
          ++code[static_cast<unsigned>(c)];
        }
        void imap_status_code_uidnext(uint32_t n) override
        {
          BOOST_CHECK_EQUAL(n, 23);
          ++a[1];
        }
        void imap_status_code_uidvalidity(uint32_t n) override
        {
          BOOST_CHECK_EQUAL(n, 3857529045);
          ++a[2];
        }
        void imap_status_code_unseen(uint32_t n) override
        {
          ++a[3];
          BOOST_CHECK_EQUAL(n, 17);
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.a[0], 3);
      for (unsigned i = 1; i < 4; ++i)
        BOOST_CHECK_EQUAL(cb.a[i], 1);
      BOOST_CHECK_EQUAL(code[static_cast<unsigned>(Status_Code::UIDNEXT)], 1);
      BOOST_CHECK_EQUAL(code[static_cast<unsigned>(Status_Code::UIDVALIDITY)], 1);
      BOOST_CHECK_EQUAL(code[static_cast<unsigned>(Status_Code::UNSEEN)], 1);
      code[static_cast<unsigned>(Status_Code::UIDNEXT)] = 0;
      code[static_cast<unsigned>(Status_Code::UIDVALIDITY)] = 0;
      code[static_cast<unsigned>(Status_Code::UNSEEN)] = 0;
      for (unsigned i = 0; i < static_cast<unsigned>(Status_Code::UNSEEN)+1; ++i)
        BOOST_CHECK_EQUAL(code[static_cast<unsigned>(i)], 0);
    }

    BOOST_AUTO_TEST_CASE( nz_underflow )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* OK [UNSEEN 0] lol\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t { 0 };
        void imap_status_code(Status_Code c) override
        {
          ++t;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      bool caught = false;
      try {
        p.read(begin, end);
      } catch (const exception &e) {
        caught = true;
      }
      BOOST_CHECK_EQUAL(caught, true);
      BOOST_CHECK_EQUAL(cb.t, 0);
    }

    BOOST_AUTO_TEST_CASE( nz_overflow )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* OK [UNSEEN 42949672950] problem?\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned t { 0 };
        void imap_status_code(Status_Code c) override
        {
          ++t;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      bool caught = false;
      try {
        p.read(begin, end);
      } catch (const exception &e) {
        caught = true;
      }
      BOOST_CHECK_EQUAL(caught, true);
      BOOST_CHECK_EQUAL(cb.t, 0);
    }


  BOOST_AUTO_TEST_SUITE_END();

  BOOST_AUTO_TEST_SUITE( fetch )

    BOOST_AUTO_TEST_CASE( flag )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* 12 FETCH (FLAGS (\\Seen))\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(3) {}
        void imap_data_fetch_begin(uint32_t number) override
        {
          BOOST_CHECK_EQUAL(number, 12);
          ++a[0];
        }
        void imap_flag(IMAP::Flag flag) override
        {
          BOOST_CHECK_EQUAL(flag, IMAP::Flag::SEEN);
          ++a[1];
        }
        void imap_data_fetch_end() override
        {
          ++a[2];
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      for (unsigned i = 0; i<3; ++i)
        BOOST_CHECK_EQUAL(cb.a[i], 1);
    }

    BOOST_AUTO_TEST_CASE( uid )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* 241 FETCH (UID 11810 FLAGS (\\Seen))\r\n"
        ;
      const char *begin = response;
      const char *end = begin + sizeof(response)-1;

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        uint32_t number_ = {0};
        CB() {}
        void imap_uid(uint32_t number) override
        {
          number_ = number;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.number_, 11810);
    }

    BOOST_AUTO_TEST_CASE( quote )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* 42 FETCH (RFC822 \"I was \\\"\\\\dreaming\\\\\\\" when I \\\"wrote\\\" this\")\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(2) {}
        void imap_data_fetch_begin(uint32_t number) override
        {
          BOOST_CHECK_EQUAL(number, 42);
          ++a[0];
        }
        void imap_data_fetch_end() override
        {
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s,  "I was \"\\dreaming\\\" when I \"wrote\" this");
          ++a[1];
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      for (unsigned i = 0; i<2; ++i)
        BOOST_CHECK_EQUAL(cb.a[i], 1);
    }

    BOOST_AUTO_TEST_CASE( split_number )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
"* 12 FETCH (BODY[HEADER] {11}\r\n"
"abcdefg\r\n"
"\r\n"
")\r\n"
"a004 OK FETCH completed\r\n"
        ;
        ;
      const char *begin = response;
      const char *end = begin + sizeof(response)-1;

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Proxy  proxy;
        Memory::Buffer::Vector tag_buffer;
        CB() {}
        void imap_body_section_begin() override
        {
        }
        void imap_body_section_inner() override
        {
          proxy.set(&buffer);
        }
        void imap_body_section_end() override
        {
          proxy.set(nullptr);
        }
      };
      CB cb;
      char small[128] = {0};
      IMAP::Client::Parser p(cb.proxy, cb.tag_buffer, cb);
      memcpy(small, begin, 27);
      p.read(small, small+27);
      memcpy(small, begin+27, end-(begin+27));
      p.read(small, small + (end-(begin+27)));
      string s(cb.buffer.begin(), cb.buffer.end());
      BOOST_CHECK_EQUAL(s, "abcdefg\n\n");
    }

    BOOST_AUTO_TEST_CASE( inline_cr )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
"* 12 FETCH (BODY[HEADER] {15}\r\n"
"hello\rworld\r\n"
"\r\n"
")\r\n"
"a004 OK FETCH completed\r\n"
        ;
        ;
      const char *begin = response;
      const char *end = begin + sizeof(response)-1;

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Proxy  proxy;
        Memory::Buffer::Vector tag_buffer;
        CB() {}
        void imap_body_section_begin() override
        {
        }
        void imap_body_section_inner() override
        {
          proxy.set(&buffer);
        }
        void imap_body_section_end() override
        {
          proxy.set(nullptr);
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.proxy, cb.tag_buffer, cb);
      p.read(begin, end);
      string s(cb.buffer.begin(), cb.buffer.end());
      const char ref[] = "hello\rworld\n\n";
      BOOST_CHECK_EQUAL(s, ref);
    }

    BOOST_AUTO_TEST_CASE( header )
    {
      static const char filename[] = "tmp/fetch_header";
      fs::create_directory("tmp");
      fs::remove(filename);
      BOOST_CHECK_EQUAL(fs::exists(filename), false);
      using namespace IMAP::Server::Response;
      const char response[] =
"* 12 FETCH (BODY[HEADER] {342}\r\n"
"Date: Wed, 17 Jul 1996 02:23:25 -0700 (PDT)\r\n"
"From: Terry Gray <gray@cac.washington.edu>\r\n"
"Subject: IMAP4rev1 WG mtg summary and minutes\r\n"
"To: imap@cac.washington.edu\r\n"
"cc: minutes@CNRI.Reston.VA.US, John Klensin <KLENSIN@MIT.EDU>\r\n"
"Message-Id: <B27397-0100000@cac.washington.edu>\r\n"
"MIME-Version: 1.0\r\n"
"Content-Type: TEXT/PLAIN; CHARSET=US-ASCII\r\n"
"\r\n"
")\r\n"
"a004 OK FETCH completed\r\n"
        ;
      const char header[] =
"Date: Wed, 17 Jul 1996 02:23:25 -0700 (PDT)\n"
"From: Terry Gray <gray@cac.washington.edu>\n"
"Subject: IMAP4rev1 WG mtg summary and minutes\n"
"To: imap@cac.washington.edu\n"
"cc: minutes@CNRI.Reston.VA.US, John Klensin <KLENSIN@MIT.EDU>\n"
"Message-Id: <B27397-0100000@cac.washington.edu>\n"
"MIME-Version: 1.0\n"
"Content-Type: TEXT/PLAIN; CHARSET=US-ASCII\n"
"\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::File   file;
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Proxy  proxy;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(6) {}
        void imap_body_section_begin() override
        {
          ++a[0];
        }
        void imap_body_section_inner() override
        {
          ++a[1];
          file.open("tmp", "fetch_header");
          proxy.set(&file);
        }
        void imap_body_section_end() override
        {
          ++a[2];
          proxy.set(nullptr);
          file.close();
        }
        void imap_section_header() override
        {
          ++a[3];
        }
        void imap_tagged_status_begin() override
        {
          ++a[4];
          proxy.set(&buffer);
          string s(tag_buffer.begin(), tag_buffer.end());
          BOOST_CHECK_EQUAL(s, "a004");
        }
        void imap_tagged_status_end(Status c) override
        {
          ++a[5];
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(c, Status::OK);
          BOOST_CHECK_EQUAL(s, "FETCH completed");
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.proxy, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(fs::exists(filename), true);
      for (unsigned i = 0; i<6; ++i) {
        //cerr << "i => " << i << '\n';
        BOOST_CHECK_EQUAL(cb.a[i], 1);
      }
      ifstream f(filename, ifstream::in | ifstream::binary);
      array<char, 1024> b = {{0}};
      f.read(b.data(), b.size()-1);
      BOOST_CHECK_EQUAL(b.data(), header);
    }

    BOOST_AUTO_TEST_CASE( header_raw )
    {
      static const char filename[] = "tmp/fetch_header";
      fs::create_directory("tmp");
      fs::remove(filename);
      BOOST_CHECK_EQUAL(fs::exists(filename), false);
      using namespace IMAP::Server::Response;
      const char response[] =
"* 12 FETCH (BODY[HEADER] {342}\r\n"
"Date: Wed, 17 Jul 1996 02:23:25 -0700 (PDT)\r\n"
"From: Terry Gray <gray@cac.washington.edu>\r\n"
"Subject: IMAP4rev1 WG mtg summary and minutes\r\n"
"To: imap@cac.washington.edu\r\n"
"cc: minutes@CNRI.Reston.VA.US, John Klensin <KLENSIN@MIT.EDU>\r\n"
"Message-Id: <B27397-0100000@cac.washington.edu>\r\n"
"MIME-Version: 1.0\r\n"
"Content-Type: TEXT/PLAIN; CHARSET=US-ASCII\r\n"
"\r\n"
")\r\n"
"a004 OK FETCH completed\r\n"
        ;
      const char header[] =
"Date: Wed, 17 Jul 1996 02:23:25 -0700 (PDT)\r\n"
"From: Terry Gray <gray@cac.washington.edu>\r\n"
"Subject: IMAP4rev1 WG mtg summary and minutes\r\n"
"To: imap@cac.washington.edu\r\n"
"cc: minutes@CNRI.Reston.VA.US, John Klensin <KLENSIN@MIT.EDU>\r\n"
"Message-Id: <B27397-0100000@cac.washington.edu>\r\n"
"MIME-Version: 1.0\r\n"
"Content-Type: TEXT/PLAIN; CHARSET=US-ASCII\r\n"
"\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::File   file;
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Proxy  proxy;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(6) {}
        void imap_body_section_begin() override
        {
          ++a[0];
        }
        void imap_body_section_inner() override
        {
          ++a[1];
          file.open("tmp", "fetch_header");
          proxy.set(&file);
        }
        void imap_body_section_end() override
        {
          ++a[2];
          proxy.set(nullptr);
          file.close();
        }
        void imap_section_header() override
        {
          ++a[3];
        }
        void imap_tagged_status_begin() override
        {
          ++a[4];
          proxy.set(&buffer);
          string s(tag_buffer.begin(), tag_buffer.end());
          BOOST_CHECK_EQUAL(s, "a004");
        }
        void imap_tagged_status_end(Status c) override
        {
          ++a[5];
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(c, Status::OK);
          BOOST_CHECK_EQUAL(s, "FETCH completed");
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.proxy, cb.tag_buffer, cb);
      p.set_convert_crlf(false);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(fs::exists(filename), true);
      for (unsigned i = 0; i<6; ++i) {
        //cerr << "i => " << i << '\n';
        BOOST_CHECK_EQUAL(cb.a[i], 1);
      }
      ifstream f(filename, ifstream::in | ifstream::binary);
      array<char, 1024> b = {{0}};
      f.read(b.data(), b.size()-1);
      BOOST_CHECK_EQUAL(b.data(), header);
    }

    BOOST_AUTO_TEST_CASE( body )
    {
      static const char filepath[] = "tmp/fetch_body";
      static const char filename[] = "fetch_body";
      static const char dir[] = "tmp";
      fs::create_directory("tmp");
      fs::remove(filepath);
      BOOST_CHECK_EQUAL(fs::exists(filepath), false);
      using namespace IMAP::Server::Response;

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::File   file;
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Proxy  proxy;
        Memory::Buffer::Vector tag_buffer;
        bool fetch_active {false};
        vector<unsigned> a;
        CB() : a(6) {}
        void imap_data_fetch_begin(uint32_t number) override
        {
          ++a[0];
        }
        void imap_data_fetch_end() override
        {
          ++a[1];
        }
        void imap_body_section_inner() override
        {
          ++a[2];
          file.open(dir, filename);
          proxy.set(&file);
        }
        void imap_body_section_end() override
        {
          ++a[3];
          proxy.set(nullptr);
          file.close();
        }
        void imap_tagged_status_begin() override
        {
          string t(tag_buffer.begin(), tag_buffer.end());
          if (t == "a4") {
            ++a[4];
            fetch_active = true;
            proxy.set(&buffer);
          }

        }
        void imap_tagged_status_end(Status c) override
        {
          if (fetch_active) {
            ++a[5];
            fetch_active = false;
            string t(tag_buffer.begin(), tag_buffer.end());
            BOOST_CHECK_EQUAL(t, "a4");
            BOOST_CHECK_EQUAL(c, Status::OK);
            string s(buffer.begin(), buffer.end());
            BOOST_CHECK_EQUAL(s, "Fetch completed.");
            proxy.set(nullptr);
          }
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.proxy, cb.tag_buffer, cb);
      using namespace IMAP::Test::Dovecot;
      p.read(prologue, prologue + strlen(prologue));
      p.read(fetch_head,
          fetch_head + strlen(fetch_head));
      pair<const char*, const char*> pair(IMAP::Test::Dovecot::fetch,
          IMAP::Test::Dovecot::fetch + strlen(IMAP::Test::Dovecot::fetch));
      const char *x = pair.first;
      for (; x+23<pair.second; x+=23)
        p.read(x, x+23);
      p.read(x, pair.second);

      p.read(fetch_tail,
          fetch_tail + strlen(fetch_tail));
      p.read(epilogue, epilogue + strlen(epilogue));
      for (unsigned i = 0; i<2; ++i)
        BOOST_CHECK_EQUAL(cb.a[i], 74);
      for (unsigned i = 2; i<6; ++i)
        BOOST_CHECK_EQUAL(cb.a[i], 1);
      ifstream f(filepath, ifstream::in | ifstream::binary);
      array<char, 20 * 1024> b = {{0}};
      f.read(b.data(), b.size()-1);
      string ref(IMAP::Test::Dovecot::fetch);
      boost::replace_all(ref, "\r", "");
      BOOST_CHECK_EQUAL(b.data(), ref);
    }

    BOOST_AUTO_TEST_CASE( body_cyrus )
    {
      static const char filepath[] = "tmp/mdir_cyrus";
      fs::create_directory("tmp");
      fs::remove_all(filepath);
      BOOST_CHECK_EQUAL(fs::exists(filepath), false);
      using namespace IMAP::Server::Response;
      using namespace Memory;

      struct CB : public IMAP::Client::Callback::Null {
        Buffer::File   file;
        Buffer::Vector buffer;
        Buffer::Proxy  proxy;
        Buffer::Vector tag_buffer;
        bool fetch_active {false};
        string next_tag_ok;
        vector<unsigned> a;
        Maildir mdir;
        Memory::Dir tmp_dir;
        bool saw_tag_ok {false};
        bool full_body_ {false};
        CB() : a(6), mdir(filepath), tmp_dir(mdir.tmp_dir_fd()) {}
        void imap_data_fetch_begin(uint32_t number) override
        {
          ++a[0];
          fetch_active = true;
          BOOST_CHECK(number == 210 || number == 234);
        }
        void imap_data_fetch_end() override
        {
          ++a[1];
          fetch_active = false;
          next_tag_ok = "a4";
        }
        void imap_section_empty() override
        {
          ++a[2];
          full_body_ = true;
        }
        void imap_body_section_inner() override
        {
          ++a[3];
          if (full_body_) {
            string filename;
            mdir.create_tmp_name(filename);
            Buffer::File f(tmp_dir, filename);
            file = std::move(f);
            proxy.set(&file);
          }
        }
        void imap_body_section_end() override
        {
          ++a[4];
          if (full_body_) {
            proxy.set(nullptr);
            mdir.move_to_new();
            file.close();
          }
          full_body_ = false;
        }
        void imap_tagged_status_end(Status c) override
        {
           ++a[5];
          string t(tag_buffer.begin(), tag_buffer.end());
          if (!next_tag_ok.empty() && t == next_tag_ok) {
            next_tag_ok.clear();
            saw_tag_ok = true;
            BOOST_CHECK_EQUAL(c, Status::OK);
          }
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.proxy, cb.tag_buffer, cb);
      for (const char **resp = IMAP::Test::Cyrus::response; *resp; ++resp) {
        p.read(*resp, *resp + strlen(*resp));
      }
      BOOST_CHECK_EQUAL(cb.saw_tag_ok, true);
      for (unsigned i = 0; i<3; ++i) {
        BOOST_CHECK_EQUAL(cb.a[i], 2);
      }
      for (unsigned i = 3; i<6; ++i)
        BOOST_CHECK_EQUAL(cb.a[i], 4);
      {
        string p(filepath);
        p += "/new";
        set<fs::path> s;
        fs::directory_iterator begin(p);
        fs::directory_iterator end;
        size_t count = 0;
        for (auto i = begin; i!= end; ++i) {
          ++count;
          s.insert(*i);
        }
        BOOST_CHECK_EQUAL(count, 2);
        BOOST_REQUIRE_EQUAL(s.size(), 2);
        {
          auto x = s.begin();
          BOOST_CHECK_EQUAL(fs::file_size(*x), 41729);
          ++x;
          BOOST_CHECK_EQUAL(fs::file_size(*x), 47195);
        }

        {
          using namespace IMAP::Test::Cyrus;
          auto x = s.begin();
          {
            ifstream f((*x).generic_string(), ifstream::in | ifstream::binary);
            noskipws(f);
            istream_iterator<char> b(f), e;
            BOOST_CHECK_EQUAL_COLLECTIONS(b, e, message1, message1+strlen(message1));
          }
          ++x;
          {
            ifstream f((*x).generic_string(), ifstream::in | ifstream::binary);
            noskipws(f);
            istream_iterator<char> b(f), e;
            BOOST_CHECK_EQUAL_COLLECTIONS(b, e, message2, message2+strlen(message2));
          }
        }
      }
    }

    BOOST_AUTO_TEST_CASE( body_gmail )
    {
      static const char filepath[] = "tmp/mdir_gmail";
      fs::create_directory("tmp");
      fs::remove_all(filepath);
      BOOST_CHECK_EQUAL(fs::exists(filepath), false);
      using namespace IMAP::Server::Response;
      using namespace Memory;

      struct CB : public IMAP::Client::Callback::Null {
        Buffer::File   file;
        Buffer::Vector buffer;
        Buffer::Proxy  proxy;
        Buffer::Vector tag_buffer;
        bool fetch_active {false};
        string next_tag_ok;
        vector<unsigned> a;
        Maildir mdir;
        Memory::Dir tmp_dir;
        bool saw_tag_ok {false};
        bool full_body_ {false};
        CB() : a(6), mdir(filepath), tmp_dir(mdir.tmp_dir_fd()) {}
        void imap_data_fetch_begin(uint32_t number) override
        {
          ++a[0];
          fetch_active = true;
        }
        void imap_data_fetch_end() override
        {
          ++a[1];
          fetch_active = false;
        }
        void imap_section_empty() override
        {
          ++a[2];
          full_body_ = true;
        }
        void imap_body_section_inner() override
        {
          ++a[3];
          if (full_body_) {
            string filename;
            mdir.create_tmp_name(filename);
            Buffer::File f(tmp_dir, filename);
            file = std::move(f);
            proxy.set(&file);
          }
        }
        void imap_body_section_end() override
        {
          ++a[4];
          if (full_body_) {
            proxy.set(nullptr);
            mdir.move_to_new();
            file.close();
          }
          full_body_ = false;
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.proxy, cb.tag_buffer, cb);
      for (const char*const*resp = IMAP::Test::GMAIL::received; *resp; ++resp) {
        p.read(*resp, *resp + strlen(*resp));
      }
      {
        string p(filepath);
        p += "/new";
        set<fs::path> s;
        fs::directory_iterator begin(p);
        fs::directory_iterator end;
        size_t count = 0;
        for (auto i = begin; i!= end; ++i) {
          ++count;
          s.insert(*i);
        }
        BOOST_CHECK_EQUAL(count, 1);
        BOOST_REQUIRE_EQUAL(s.size(), 1);
        {
          auto x = s.begin();
          BOOST_REQUIRE_EQUAL(fs::file_size(*x), 3107);
        }

        {
          using namespace IMAP::Test::GMAIL;
          auto x = s.begin();
          {
            ifstream f((*x).generic_string(), ifstream::in | ifstream::binary);
            noskipws(f);
            istream_iterator<char> b(f), e;
            BOOST_CHECK_EQUAL_COLLECTIONS(b, e, message1, message1+strlen(message1));
          }
        }
      }
    }

  BOOST_AUTO_TEST_SUITE_END();


  BOOST_AUTO_TEST_SUITE( capability )

    BOOST_AUTO_TEST_CASE( null )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
"* OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE AUTH=PLAIN] Dovecot ready.\r\n"
"a0 OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE SORT SORT=DISPLAY THREAD=REFERENCES THREAD=REFS MULTIAPPEND UNSELECT IDLE CHILDREN NAMESPACE UIDPLUS LIST-EXTENDED I18NLEVEL=1 CONDSTORE QRESYNC ESEARCH ESORT SEARCHRES WITHIN CONTEXT=SEARCH] Logged in\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(2) {}
        void imap_tagged_status_end(Status c) override
        {
          ++a[0];
          BOOST_CHECK_EQUAL(c, Status::OK);
          string tag(tag_buffer.begin(), tag_buffer.end());
          BOOST_CHECK_EQUAL(tag, "a0");
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s, "Logged in");
        }
        void imap_untagged_status_end(Status c) override
        {
          ++a[1];
          BOOST_CHECK_EQUAL(c, Status::OK);
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s, "Dovecot ready.");
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      for (unsigned i = 0; i<2; ++i) {
        //cerr << i << '\n';
        BOOST_CHECK_EQUAL(cb.a[i], 1);
      }
    }

    BOOST_AUTO_TEST_CASE( present )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
"* OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE AUTH=PLAIN] Dovecot ready.\r\n"
"a0 OK [CAPABILITY IMAP4rev1 LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE SORT SORT=DISPLAY THREAD=REFERENCES THREAD=REFS MULTIAPPEND UNSELECT IDLE CHILDREN NAMESPACE UIDPLUS LIST-EXTENDED I18NLEVEL=1 CONDSTORE QRESYNC ESEARCH ESORT SEARCHRES WITHIN CONTEXT=SEARCH] Logged in\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        vector<unsigned> a;
        CB() : a(5) {}
        unordered_set<Capability> c_set;
        unordered_set<string> s_set;
        void imap_status_code_capability_begin()
        {
          ++a[0];
        }
        void imap_status_code_capability_end()
        {
          vector<const char*> s_ref;
          vector<Capability> c_ref;
          unsigned n = 0;
          unsigned m = 0;
          if (a[1] == 0) {
            n = 7;
            m = 8;
            const vector<const char*> s =
            {
"IMAP4rev1",
"LITERAL+",
"SASL-IR",
"LOGIN-REFERRALS",
"ID",
"ENABLE",
"AUTH=PLAIN"
            };
            s_ref = std::move(s);
            const vector<Capability> c =
            {
              Capability::IMAP4rev1,
              Capability::LITERAL_plus_,
              Capability::SASL_IR,
              Capability::LOGIN_REFERRALS,
              Capability::ID,
              Capability::ENABLE,
              Capability::AUTH_eq_PLAIN,
              Capability::AUTH_eq_
            };
            c_ref = std::move(c);
          } else {
            n = 25;
            m = 22;
            const vector<const char*> s =
            {
              "IMAP4rev1",
              "LITERAL+",
              "SASL-IR",
              "LOGIN-REFERRALS",
              "ID",
              "ENABLE",
              "SORT",
              "SORT=DISPLAY",
              "THREAD=REFERENCES",
              "THREAD=REFS",
              "MULTIAPPEND",
              "UNSELECT",
              "IDLE",
              "CHILDREN",
              "NAMESPACE",
              "UIDPLUS",
              "LIST-EXTENDED",
              "I18NLEVEL=1",
              "CONDSTORE",
              "QRESYNC",
              "ESEARCH",
              "ESORT",
              "SEARCHRES",
              "WITHIN",
              "CONTEXT=SEARCH",
            };
            s_ref = std::move(s);
            const vector<Capability> c =
            {
              Capability::IMAP4rev1,
              Capability::LITERAL_plus_,
              Capability::SASL_IR,
              Capability::LOGIN_REFERRALS,
              Capability::ID,
              Capability::ENABLE,
              Capability::SORT,
              Capability::SORT_eq_DISPLAY,
              Capability::MULTIAPPEND,
              Capability::UNSELECT,
              Capability::IDLE,
              Capability::CHILDREN,
              Capability::NAMESPACE,
              Capability::UIDPLUS,
              Capability::I18NLEVEL_eq_1,
              Capability::CONDSTORE,
              Capability::QRESYNC,
              Capability::ESEARCH,
              Capability::ESORT,
              Capability::SEARCHRES,
              Capability::WITHIN,
              Capability::CONTEXT_eq_SEARCH
            };
            c_ref = std::move(c);
          }
          BOOST_CHECK_EQUAL(s_set.size(), n);
          BOOST_CHECK_EQUAL(c_set.size(), m);
          for (auto x : s_ref) {
            if (s_set.find(x) == s_set.end())
              cout << "missing: " << x << '\n';
            BOOST_CHECK(s_set.find(x) != s_set.end());
          }
          for (auto x : c_ref) {
            if (c_set.find(x) == c_set.end())
              cout << "missing: " << x << '\n';
            BOOST_CHECK(c_set.find(x) != c_set.end());
          }
          s_set.clear();
          c_set.clear();
          ++a[1];
        }
        void imap_capability_begin()
        {
          ++a[2];
        }
        void imap_capability(Capability capability)
        {
          ++a[3];
          BOOST_CHECK(c_set.find(capability) == c_set.end());
          c_set.insert(capability);
        }
        void imap_capability_end()
        {
          ++a[4];
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK(s_set.find(s) == s_set.end());
          s_set.insert(s);
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      const array<unsigned, 5> a_ref = {{
        2, 2, 32, 30,32
      }};
      for (unsigned i = 0; i<5; ++i) {
        //cerr << i << '\n';
        BOOST_CHECK_EQUAL(cb.a[i], a_ref[i]);
      }
    }

    BOOST_AUTO_TEST_CASE( mandatory )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
"* OK [CAPABILITY LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE AUTH=PLAIN] Dovecot ready.\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      bool caught = false;
      try {
        p.read(begin, end);
      } catch (const exception&) {
        caught = true;
      }
      BOOST_CHECK_EQUAL(caught, true);
    }

    BOOST_AUTO_TEST_CASE( mandatory_tagged )
    {
      using namespace IMAP::Server::Response;
      const char response[] =
"a0 OK [CAPABILITY LITERAL+ SASL-IR LOGIN-REFERRALS ID ENABLE SORT SORT=DISPLAY THREAD=REFERENCES THREAD=REFS MULTIAPPEND UNSELECT IDLE CHILDREN NAMESPACE UIDPLUS LIST-EXTENDED I18NLEVEL=1 CONDSTORE QRESYNC ESEARCH ESORT SEARCHRES WITHIN CONTEXT=SEARCH] Logged in\r\n"
        ;
      const char *begin = response;
      const char *end = begin + strlen(begin);

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      bool caught = false;
      try {
        p.read(begin, end);
      } catch (const exception&) {
        caught = true;
      }
      BOOST_CHECK_EQUAL(caught, true);
    }

  BOOST_AUTO_TEST_SUITE_END();

  BOOST_AUTO_TEST_SUITE( list )

    BOOST_AUTO_TEST_CASE(basic)
    {
      using namespace IMAP::Server::Response;
      const char response[] =
        "* LIST (\\Noselect) \"/\" ~/Mail/foo\r\n"
        ;
      const char *begin = response;
      const char *end = begin + sizeof(response)-1;

      struct CB : public IMAP::Client::Callback::Null {
        Memory::Buffer::Vector buffer;
        Memory::Buffer::Vector tag_buffer;
        unsigned list_begin {0};
        unsigned list_end {0};
        unsigned list_sflag {0};
        unsigned list_oflag {0};
        unsigned quoted_char {0};
        unsigned list_mailbox {0};
        void imap_list_begin() override
        {
          ++list_begin;
        }
        void imap_list_end() override
        {
          ++list_end;
        }
        void imap_list_sflag(SFlag flag) override
        {
          ++list_sflag;
          BOOST_CHECK_EQUAL(flag, SFlag::NOSELECT);
        }
        void imap_list_oflag(OFlag oflag) override
        {
          ++list_oflag;
        }
        void imap_quoted_char(char c) override
        {
          ++quoted_char;
          BOOST_CHECK_EQUAL(c, '/');
        }
        void imap_list_mailbox() override
        {
          ++list_mailbox;
          string s(buffer.begin(), buffer.end());
          BOOST_CHECK_EQUAL(s, "~/Mail/foo");
        }
      };
      CB cb;
      IMAP::Client::Parser p(cb.buffer, cb.tag_buffer, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(cb.list_begin, 1);
      BOOST_CHECK_EQUAL(cb.list_end, 1);
      BOOST_CHECK_EQUAL(cb.list_sflag, 1);
      BOOST_CHECK_EQUAL(cb.list_oflag, 0);
      BOOST_CHECK_EQUAL(cb.quoted_char, 1);
      BOOST_CHECK_EQUAL(cb.list_mailbox, 1);
    }

  BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE_END();
