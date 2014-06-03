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

#include <imap/imap.h>
#include <imap/client_writer.h>

#include <iostream>
#include <sstream>
using namespace std;



BOOST_AUTO_TEST_SUITE( imap_client_writer )

  BOOST_AUTO_TEST_SUITE( enumeration )

    BOOST_AUTO_TEST_CASE( tostr )
    {
      BOOST_CHECK_EQUAL(
          IMAP::Client::command_str(IMAP::Client::Command::EXAMINE), "EXAMINE");
      BOOST_CHECK_EQUAL(
          IMAP::Client::command_str(IMAP::Client::Command::LOGIN), "LOGIN");
      ostringstream o;
      o << IMAP::Client::Command::EXAMINE;
      BOOST_CHECK_EQUAL(o.str(), "EXAMINE");
    }

    BOOST_AUTO_TEST_CASE( except )
    {
      ostringstream o;
      BOOST_CHECK_THROW(o << static_cast<IMAP::Server::Response::Status_Code>(23),
          std::out_of_range);
    }

  BOOST_AUTO_TEST_SUITE_END()
  BOOST_AUTO_TEST_SUITE( tag )

    BOOST_AUTO_TEST_CASE( basic )
    {
      IMAP::Client::Tag tag;
      ostringstream o;
      for (unsigned i = 0; i<5; ++i) {
        string t;
        tag.next(t, static_cast<IMAP::Client::Command>(i+1));
        o << t << '\n';
      }
      BOOST_CHECK_EQUAL(o.str(), "A000\nA001\nA002\nA003\nA004\n");
    }

    BOOST_AUTO_TEST_CASE( active_throw )
    {
      IMAP::Client::Tag tag;

      string t;
      tag.next(t, IMAP::Client::Command::NOOP);
      BOOST_CHECK_THROW(tag.next(t, IMAP::Client::Command::NOOP),
          std::logic_error);
    }

    BOOST_AUTO_TEST_CASE( pop )
    {
      IMAP::Client::Tag tag;
      string t;
      tag.next(t, IMAP::Client::Command::NOOP);
      BOOST_CHECK_EQUAL(t, "A000");
      tag.pop(t);
      tag.next(t, IMAP::Client::Command::NOOP);
      BOOST_CHECK_EQUAL(t, "A001");
      tag.pop(t);
    }
    BOOST_AUTO_TEST_CASE( pop_throw )
    {
      IMAP::Client::Tag tag;
      string t;
      tag.next(t, IMAP::Client::Command::NOOP);
      BOOST_CHECK_EQUAL(t, "A000");
      tag.pop(t);
      BOOST_CHECK_THROW(tag.pop(t), std::logic_error);
    }
    BOOST_AUTO_TEST_CASE( pop_throw2 )
    {
      IMAP::Client::Tag tag;
      string t;
      tag.next(t, IMAP::Client::Command::NOOP);
      BOOST_CHECK_EQUAL(t, "A000");
      t = "A001";
      BOOST_CHECK_THROW(tag.pop(t), std::logic_error);

    }

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE( writer )

    BOOST_AUTO_TEST_CASE( basic )
    {
      vector<char> v;
      using namespace IMAP::Client;
      Tag tag;
      Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
      string t;
      writer.capability(t);
      BOOST_CHECK_EQUAL(t, "A000");
      v.push_back('\0');
      BOOST_CHECK_EQUAL(v.data(), "A000 CAPABILITY\r\n");
    }
    BOOST_AUTO_TEST_CASE( two )
    {
      vector<char> v;
      using namespace IMAP::Client;
      Tag tag;
      Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
      string t;
      writer.noop(t);
      BOOST_CHECK_EQUAL(t, "A000");
      v.push_back('\0');
      BOOST_CHECK_EQUAL(v.data(), "A000 NOOP\r\n");
      writer.logout(t);
      BOOST_CHECK_EQUAL(t, "A001");
      v.push_back('\0');
      BOOST_CHECK_EQUAL(v.data(), "A001 LOGOUT\r\n");
    }

    BOOST_AUTO_TEST_CASE( login )
    {
      vector<char> v;
      using namespace IMAP::Client;
      Tag tag;
      Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
      string t;
      writer.login("juser", "secretvery", t);
      BOOST_CHECK_EQUAL(t, "A000");
      v.push_back('\0');
      BOOST_CHECK_EQUAL(v.data(), "A000 LOGIN juser secretvery\r\n");
    }
    BOOST_AUTO_TEST_CASE( examine )
    {
      vector<char> v;
      using namespace IMAP::Client;
      Tag tag;
      Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
      string t;
      writer.login("juser", "secretvery", t);
      writer.examine("INBOX", t);
      BOOST_CHECK_EQUAL(t, "A001");
      v.push_back('\0');
      BOOST_CHECK_EQUAL(v.data(), "A001 EXAMINE INBOX\r\n");
      writer.select("other", t);
      BOOST_CHECK_EQUAL(t, "A002");
      v.push_back('\0');
      BOOST_CHECK_EQUAL(v.data(), "A002 SELECT other\r\n");
    }

    BOOST_AUTO_TEST_SUITE( wrong_state )

      BOOST_AUTO_TEST_CASE( throw_selected )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        BOOST_CHECK_THROW(writer.uid_expunge(set, t), std::runtime_error);
      }
      BOOST_AUTO_TEST_CASE( throw_login )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        BOOST_CHECK_THROW(writer.expunge(t), std::runtime_error);
      }
      BOOST_AUTO_TEST_CASE( throw_login_double )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        BOOST_CHECK_THROW(writer.login("juser", "secretvery", t), std::logic_error);
      }
      BOOST_AUTO_TEST_CASE( throw_read_only )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.examine("INBOX", t);
        BOOST_CHECK_THROW(writer.expunge(t), std::runtime_error);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        BOOST_CHECK_THROW(writer.uid_expunge(set, t), std::runtime_error);
      }
    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE( uid_expunge )

      BOOST_AUTO_TEST_CASE( single )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        writer.uid_expunge(set, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID EXPUNGE 1\r\n");
      }
      BOOST_AUTO_TEST_CASE( range )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 2);
        writer.uid_expunge(set, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID EXPUNGE 1:2\r\n");
      }
      BOOST_AUTO_TEST_CASE( reverse )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(4, 2);
        writer.uid_expunge(set, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID EXPUNGE 4:2\r\n");
      }
      BOOST_AUTO_TEST_CASE( multiple )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set = { {2,4}, {6,6}, {4,2} };
        writer.uid_expunge(set, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID EXPUNGE 2:4,6,4:2\r\n");
      }
      BOOST_AUTO_TEST_CASE( star )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set = {
          {2,numeric_limits<uint32_t>::max()},
          {numeric_limits<uint32_t>::max(),numeric_limits<uint32_t>::max()},
          {numeric_limits<uint32_t>::max(),2} };
        writer.uid_expunge(set, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID EXPUNGE 2:*,*,*:2\r\n");
      }
      BOOST_AUTO_TEST_CASE( throw_single )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.examine("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(0, 0);
        BOOST_CHECK_THROW(writer.uid_expunge(set, t), std::logic_error);
      }
      BOOST_AUTO_TEST_CASE( throw_range )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.examine("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(0, 1);
        BOOST_CHECK_THROW(writer.uid_expunge(set, t), std::logic_error);
      }
      BOOST_AUTO_TEST_CASE( throw_empty)
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.examine("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        BOOST_CHECK_THROW(writer.uid_expunge(set, t), std::logic_error);
      }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE( fetch )

      BOOST_AUTO_TEST_CASE( throw_non_selected )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        vector<Fetch_Attribute> atts;
        atts.emplace_back(Fetch::BODY);
        BOOST_CHECK_EQUAL(atts.size(), 1);
        BOOST_CHECK_THROW(writer.fetch(set, atts, t), std::runtime_error);
      }

      BOOST_AUTO_TEST_CASE( body )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        vector<Fetch_Attribute> atts;
        atts.emplace_back(Fetch::BODY);
        BOOST_CHECK_EQUAL(atts.size(), 1);
        writer.fetch(set, atts, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 FETCH 1 BODY[]\r\n");
      }

      BOOST_AUTO_TEST_CASE( multi )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        vector<Fetch_Attribute> atts;
        atts.emplace_back(Fetch::UID);
        vector<string> fields;
        fields.emplace_back("date");
        fields.emplace_back("from");
        fields.emplace_back("subject");
        atts.emplace_back(Fetch::BODY,
            IMAP::Section_Attribute(IMAP::Section::HEADER_FIELDS, std::move(fields)));
        atts.emplace_back(Fetch::BODY);
        BOOST_CHECK_EQUAL(atts.size(), 3);
        writer.fetch(set, atts, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(),"A002 FETCH 1 "
            "(UID BODY[HEADER.FIELDS (date from subject)] BODY[])\r\n");
      }
      BOOST_AUTO_TEST_CASE( empty_atts )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        vector<Fetch_Attribute> atts;
        BOOST_CHECK_THROW(writer.fetch(set, atts, t), std::logic_error);
      }
      BOOST_AUTO_TEST_CASE( empty_names )
      {
        vector<string> fields;
        BOOST_CHECK_THROW(IMAP::Section_Attribute section(IMAP::Section::HEADER_FIELDS, fields), std::logic_error);
      }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE( store )

      BOOST_AUTO_TEST_CASE( simple )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        vector<IMAP::Flag> flags;
        flags.emplace_back(IMAP::Flag::DELETED);
        writer.uid_store(set, flags, t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID STORE 1 FLAGS \\DELETED\r\n");
      }

      BOOST_AUTO_TEST_CASE( add_minus )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        vector<IMAP::Flag> flags;
        flags.emplace_back(IMAP::Flag::DELETED);
        writer.uid_store(set, flags, t, Store_Mode::ADD);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID STORE 1 +FLAGS \\DELETED\r\n");
        tag.pop(t);
        writer.uid_store(set, flags, t, Store_Mode::REMOVE);
        BOOST_CHECK_EQUAL(t, "A003");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A003 UID STORE 1 -FLAGS \\DELETED\r\n");
      }

      BOOST_AUTO_TEST_CASE( silent )
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.select("INBOX", t);
        vector<pair<uint32_t, uint32_t> > set;
        set.emplace_back(1, 1);
        vector<IMAP::Flag> flags;
        flags.emplace_back(IMAP::Flag::FLAGGED);
        flags.emplace_back(IMAP::Flag::ANSWERED);
        writer.uid_store(set, flags, t, Store_Mode::REPLACE, true);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 UID STORE 1 FLAGS.SILENT \\FLAGGED \\ANSWERED\r\n");
      }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE(list)

      BOOST_AUTO_TEST_CASE(basic)
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        writer.list("", "", t);
        BOOST_CHECK_EQUAL(t, "A001");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A001 LIST {0}\r\n {0}\r\n\r\n");
      }
      BOOST_AUTO_TEST_CASE(two)
      {
        vector<char> v;
        using namespace IMAP::Client;
        Tag tag;
        Writer writer(tag, [&v](vector<char> &x){ swap(v, x);});
        string t;
        writer.login("juser", "secretvery", t);
        tag.pop(t);
        writer.list("", "", t);
        BOOST_CHECK_EQUAL(t, "A001");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A001 LIST {0}\r\n {0}\r\n\r\n");
        tag.pop(t);
        writer.list("~/Mail/", "%", t);
        BOOST_CHECK_EQUAL(t, "A002");
        v.push_back('\0');
        BOOST_CHECK_EQUAL(v.data(), "A002 LIST {7}\r\n~/Mail/ {1}\r\n%\r\n");
      }

    BOOST_AUTO_TEST_SUITE_END()


  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
