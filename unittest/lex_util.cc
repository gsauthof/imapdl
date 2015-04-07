// Copyright 2015, Georg Sauthoff <mail@georg.so>

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

#include <sstream>
#include <iomanip>

#include <lex_util.h>

using namespace std;

BOOST_AUTO_TEST_SUITE( lex_util )

  BOOST_AUTO_TEST_SUITE( sw )

    BOOST_AUTO_TEST_CASE( basic )
    {
      ostringstream o;
      const char inp[] = "hello world";
      safely_write(o, inp, sizeof(inp)-1);
      BOOST_CHECK_EQUAL(o.str(), "hello world");
    }

    BOOST_AUTO_TEST_CASE( escape )
    {
      ostringstream o;
      const char inp[] = "hello\nworld";
      safely_write(o, inp, sizeof(inp)-1);
      BOOST_CHECK_EQUAL(o.str(), "hello\\x0aworld");
    }

    BOOST_AUTO_TEST_CASE( first )
    {
      ostringstream o;
      const char inp[] = "\rhello world";
      safely_write(o, inp, sizeof(inp)-1);
      BOOST_CHECK_EQUAL(o.str(), "\\x0dhello world");
    }

    BOOST_AUTO_TEST_CASE( last )
    {
      ostringstream o;
      const char inp[] = "hello world\r";
      safely_write(o, inp, sizeof(inp)-1);
      BOOST_CHECK_EQUAL(o.str(), "hello world\\x0d");
    }

    BOOST_AUTO_TEST_CASE( mult )
    {
      ostringstream o;
      const char inp[] = "hello\nworld\r";
      safely_write(o, inp, sizeof(inp)-1);
      BOOST_CHECK_EQUAL(o.str(), "hello\\x0aworld\\x0d");
    }

    BOOST_AUTO_TEST_CASE(fill)
    {
      ostringstream o;
      const char inp[] = "hello\nworld";
      safely_write(o, inp, sizeof(inp)-1);
      o << setw(4) << 33;
      BOOST_CHECK_EQUAL(o.str(), "hello\\x0aworld  33");
    }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(tle)

    BOOST_AUTO_TEST_CASE(basic_throw)
    {
      const char buffer[] = "\"The quick brown fox jumps over the lazy dog\" is an English-language pangram";
      BOOST_CHECK_THROW(throw_lex_error("Message", buffer, buffer+15, buffer+sizeof(buffer)-1), runtime_error);
    }

    BOOST_AUTO_TEST_CASE(msg)
    {
      const char buffer[] = "\"The quick brown fox jumps\r\t\nover the lazy dog\" is an English-language pangram";
      string msg;
      try {
        throw_lex_error("Message", buffer, buffer+15, buffer+sizeof(buffer)-1);
      } catch (const runtime_error &e) {
        msg = e.what();
      }
      const char ref[] = "Message (6e 20 66 6f 78 20 6a 75 6d 70 73  d  9  a 6f 76 65 72 20 74 <=> |n fox jumps\\x0d\\x09\\x0aover t| - 15 bytes before:  22 54 68 65 20 71 75 69 63 6b 20 62 72 6f 77 <=>  |\"The quick brow|)";
      BOOST_CHECK_EQUAL(msg, ref);
    }

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
