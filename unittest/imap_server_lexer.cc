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

#include <imap/server_lexer.h>
#include <imap/imap.h>

using namespace std;

using namespace Memory;

BOOST_AUTO_TEST_SUITE( imap_server_parser )

  BOOST_AUTO_TEST_SUITE( basic )

    BOOST_AUTO_TEST_CASE( login )
    {
      const char inp[] =
        "a1 nOOp\r\n"
        "a2 CAPABILITY\r\n"
        "a3 login juser geheimvery\r\n"
        "a4 examine INBOX\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
    }

    BOOST_AUTO_TEST_CASE( login_fail )
    {
      const char inp[] =
        "a1 nOOp\r\n"
        "a2 CAPABILITY\r\n"
        "a3 login juser geheimvery\r\n"
        "a4 examine INBOX\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      struct CB : public Callback::Null {
          bool imapd_login(const Memory::Buffer::Base &userid,
              const Memory::Buffer::Base &password) override
          {
            return false;
          }
      };
      CB cb;
      Parser p(proxy, proxy, cb);
      BOOST_CHECK_THROW(p.read(begin, end), std::runtime_error);
    }

    BOOST_AUTO_TEST_CASE( bad_tag )
    {
      const char inp[] =
        "a1 nOOp\r\n"
        "* capability\r\n"
        "a3 nOOp\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      BOOST_CHECK_THROW(p.read(begin, end), std::runtime_error);
    }

    BOOST_AUTO_TEST_CASE( mini_session )
    {
      const char inp[] =
        "a1 login juser secrectvery\r\n"
        "a2 examine INBOX\r\n"
        //"a3 fetch 1:* (uid body[header.fields (subject)])\r\n"
        "a4 select INBOX\r\n"
        "a5 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(p.finished(), true);
    }

    BOOST_AUTO_TEST_CASE( select_session )
    {
      const char inp[] =
        "a1 login juser secrectvery\r\n"
        "a2 examine INBOX\r\n"
        "a3 fetch 1:* (uid body[header.fields (subject)])\r\n"
        "a31 fetch 10 (uid flags body[header.fields (date from to subject message-id)] body[])\r\n"
        "a4 select inbox\r\n"
        "a41 select INBOX\r\n"
        "a5 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(p.finished(), true);
    }

    BOOST_AUTO_TEST_CASE( search )
    {
      const char inp[] =
        "a1 login juser secrectvery\r\n"
        "a2 status \"inbox\" (messages)\r\n"
        "a3 examine INBOX\r\n"
        "a4 search on 1-Mar-2014\r\n"
        "a5 fetch 73 (uid flags body[])\r\n"
        "a6 fetch 1:* (uid flags)\r\n"
        "a7 fetch 210,234 (uid flags body[header.fields (date from to subject message-id)] body[])\r\n"
        "a5 uid search all\r\n"
        "a8 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(p.finished(), true);
    }

    BOOST_AUTO_TEST_CASE( store )
    {
      const char inp[] =
        "a1 login juser secrectvery\r\n"
        "a3 select INBOX\r\n"
        "A003 STORE 2:4 +FLAGS (\\Deleted)\r\n"
        "a8 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(p.finished(), true);
    }

    BOOST_AUTO_TEST_CASE( id )
    {
      const char inp[] =
        "a023 ID (\"name\" \"sodr\" \"version\" \"19.34\" \"vendor\""
          " \"Pink Floyd Music Limited\")\r\n"
        "a042 ID NIL\r\n"
        "a8 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(p.finished(), true);
    }

    BOOST_AUTO_TEST_CASE( id_fail )
    {
      const char inp[] =
        "a2 id ()\r\n"
        "a8 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      BOOST_CHECK_THROW(p.read(begin, end), std::runtime_error);
    }

    BOOST_AUTO_TEST_CASE( uidplus )
    {
      const char inp[] =
        "a1 login juser secrectvery\r\n"
        "a3 select INBOX\r\n"
        "A003 UID EXPUNGE 3000:3002\r\n"
        "a8 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(p.finished(), true);
    }

    BOOST_AUTO_TEST_CASE( close_fail )
    {
      const char inp[] =
        "a1 login juser secrectvery\r\n"
        "a3 select INBOX\r\n"
        "a3 close\r\n"
        "A003 STORE 2:4 +FLAGS (\\Deleted)\r\n"
        "a8 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      BOOST_CHECK_THROW(p.read(begin, end), std::runtime_error);
    }

    BOOST_AUTO_TEST_CASE( close )
    {
      const char inp[] =
        "a1 login juser secrectvery\r\n"
        "a3 select INBOX\r\n"
        "A003 STORE 2:4 +FLAGS (\\Deleted)\r\n"
        "a3 close\r\n"
        "a8 logout\r\n"
        ;
      const char *begin = inp;
      const char *end   = inp + sizeof(inp)-1;
      using namespace IMAP::Server;
      Buffer::Proxy proxy;
      Callback::Null cb;
      Parser p(proxy, proxy, cb);
      p.read(begin, end);
      BOOST_CHECK_EQUAL(p.finished(), true);
    }

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
