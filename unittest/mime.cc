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
#include <boost/algorithm/hex.hpp>

#include <mime/base64_decoder.h>
#include <mime/q_decoder.h>
#include <mime/header_decoder.h>

#include <string>
#include <vector>
using namespace std;
using namespace Memory;

BOOST_AUTO_TEST_SUITE(mime)

  BOOST_AUTO_TEST_SUITE(base64)

    BOOST_AUTO_TEST_SUITE(decoder)

      BOOST_AUTO_TEST_CASE(basic)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = "aGVsbG8gd29ybGQK";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "hello world\n");
      }
      BOOST_AUTO_TEST_CASE(shor)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v, "?=");

        const char inp[] = "/GJl?=";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "\xfc""be");
      }
      BOOST_AUTO_TEST_CASE(pad1)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = "aGVsbG8gd29ybGQ=";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "hello world");
      }
      BOOST_AUTO_TEST_CASE(pad2)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = "Zm9vYg==";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "foob");
      }
      BOOST_AUTO_TEST_CASE(trailing)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v, "?=");
        const char inp[] = "ZUJheS1SZWNobnVuZyB2b20gRnJlaXRhZywgMzEuIEF1Z3VzdCAyMDEy?=";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "eBay-Rechnung vom Freitag, 31. August 2012");
        BOOST_CHECK(d.next() == inp+sizeof(inp)-3);
      }
      BOOST_AUTO_TEST_CASE(multiple)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v, "?=");
        const char inp[] = "Tm/Dq2w=?=";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Noël");
        BOOST_CHECK(d.next() == inp+sizeof(inp)-3);
        const char inp2[] = "S8O2dGhl?=";
        d.read(inp2, inp2 + sizeof(inp2) - 1);
        string t(v.begin(), v.end());
        BOOST_CHECK_EQUAL(t, "NoëlKöthe");
        BOOST_CHECK(d.next() == inp2+sizeof(inp2)-3);
      }
      BOOST_AUTO_TEST_CASE(ascii)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] =
          "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4"
          "OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3Bx"
          "cnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmq"
          "q6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj"
          "5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==";
        d.read(inp, inp + sizeof(inp) - 1);
        BOOST_CHECK_EQUAL(v.size(), 256);
        uint8_t i = 0;
        for (auto x : v) {
          BOOST_CHECK_EQUAL(uint8_t(x), i);
          ++i;
        }
      }

      BOOST_AUTO_TEST_CASE(empty)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = " ";
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }

      BOOST_AUTO_TEST_CASE(invalid)
      {
        using namespace MIME::Base64;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = "ZUJheS1SZWNobnVuZyB2b20_RnJlaXRhZywgMzEuIEF1Z3VzdCAyMDEy";
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }

    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(q)

    BOOST_AUTO_TEST_SUITE(decoder)

      BOOST_AUTO_TEST_CASE(basic)
      {
        using namespace MIME::Q;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = "Newsletter:_Neue_Verd=E4chtige_in_Neonazi-Mordserie_-_";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Newsletter: Neue Verd\xE4""chtige in Neonazi-Mordserie - ");
      }


      BOOST_AUTO_TEST_CASE(lowercase)
      {
        using namespace MIME::Q;
        Buffer::Vector v;
        Decoder d(v, "?=");
        const char inp[] = "Reiser=fccktritts=2d=20und=20Reiseabbruchschutz=3a=20116=20Tarife=20im=20Test";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Reiser\xfc""cktritts\x2d und Reiseabbruchschutz\x3a 116 Tarife im Test");
      }

      BOOST_AUTO_TEST_CASE(trailing)
      {
        using namespace MIME::Q;
        Buffer::Vector v;
        Decoder d(v, "?=");
        const char inp[] = "Reiser=fccktritts=2d=20und=20Reiseabbruchschutz=3a=20116=20Tarife=20im=20Test?=";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Reiser\xfc""cktritts\x2d und Reiseabbruchschutz\x3a 116 Tarife im Test");
        BOOST_CHECK(d.next() == inp+sizeof(inp)-3);
      }

      BOOST_AUTO_TEST_CASE(multiple)
      {
        using namespace MIME::Q;
        Buffer::Vector v;
        Decoder d(v, "?=");
        const char inp[] = "Best=E4tigung=20";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Best\xe4tigung ");
        //BOOST_CHECK(d.next() == inp+sizeof(inp)-3);
        const char inp2[] = "=20f=FCr=20?=";
        d.read(inp2, inp2 + sizeof(inp2) - 1);
        string t(v.begin(), v.end());
        BOOST_CHECK_EQUAL(t, "Best\xe4tigung  f\xfcr ");
        //BOOST_CHECK(d.next() == inp2+sizeof(inp2)-3);
      }

      BOOST_AUTO_TEST_CASE(empty)
      {
        using namespace MIME::Q;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = " ";
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }

      BOOST_AUTO_TEST_CASE(invalid)
      {
        using namespace MIME::Q;
        Buffer::Vector v;
        Decoder d(v);
        const char inp[] = "Best=E4ti gung=20";
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }

    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(header)

    BOOST_AUTO_TEST_SUITE(decoder)

      BOOST_AUTO_TEST_CASE(basic)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject: =?utf-8?Q?HelloWorld?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "HelloWorld");
      }
      BOOST_AUTO_TEST_CASE(charset)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Vector w;
        Buffer::Vector charset;
        Buffer::Proxy p;
        Decoder d(p, v, charset, w, [](){});
        const char inp[] = "Subject: =?utf-8?Q?HelloWorld?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "HelloWorld");
        string t(charset.begin(), charset.end());
        BOOST_CHECK_EQUAL(t, "utf-8");
      }
      BOOST_AUTO_TEST_CASE(field)
      {
        using namespace MIME::Header;
        Buffer::Vector v, w;
        Buffer::Vector charset;
        Buffer::Vector field;
        Buffer::Proxy p;
        Decoder d(field, v, charset, w, [](){});
        const char inp[] = "Subject: =?utf-8?Q?HelloWorld?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "HelloWorld");
        string t(charset.begin(), charset.end());
        BOOST_CHECK_EQUAL(t, "utf-8");
        string u(field.begin(), field.end());
        BOOST_CHECK_EQUAL(u, "Subject");
      }
      BOOST_AUTO_TEST_CASE(language)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Vector charset;
        Buffer::Vector field;
        Buffer::Vector lang;
        Decoder d(field, v, charset, lang, [](){});
        const char inp[] = "Subject: =?utf-8*ru?Q?HelloWorld?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "HelloWorld");
        string t(charset.begin(), charset.end());
        BOOST_CHECK_EQUAL(t, "utf-8");
        string u(field.begin(), field.end());
        BOOST_CHECK_EQUAL(u, "Subject");
        string w(lang.begin(), lang.end());
        BOOST_CHECK_EQUAL(w, "ru");
      }
      BOOST_AUTO_TEST_CASE(mixed)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject: foo bar =?utf-8?Q?HelloWorld?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "foo bar HelloWorld");
      }
      BOOST_AUTO_TEST_CASE(unfold)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject: foo \r\n bar =?utf-8?Q?HelloWorld?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "foo  bar HelloWorld");
      }
      BOOST_AUTO_TEST_CASE(header)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        string s;
        Decoder d(p, v, [&v,&s](){s.insert(s.end(), v.begin(), v.end());});
        const char inp[] =
"Date: Mon, 5 May 2014 15:03:14 +0200 (CEST)\n"
"From: PureMessage Admin <postmaster@uni-bielefeld.de>\n"
"Subject: =?UTF-8?Q?SpamMailsinQuarant?= =?UTF-8?Q?neseit?= Apr 16 15:00\n"
;
        const char ref[] =
"Mon, 5 May 2014 15:03:14 +0200 (CEST)"
"PureMessage Admin <postmaster@uni-bielefeld.de>"
"SpamMailsinQuarantneseit Apr 16 15:00"
;
        d.read(inp, inp + sizeof(inp) - 1);
        char e{0}; d.read(&e, &e+1);
        BOOST_CHECK_EQUAL(s, ref);
      }
      BOOST_AUTO_TEST_CASE(header_split)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Vector field;
        vector<string> vs;
        vector<string> fields;
        Decoder d(field, v, [&v,&field, &vs, &fields](){vs.emplace_back(v.begin(), v.end());fields.emplace_back(field.begin(), field.end());v.clear();field.clear();});
        const char inp[] =
"Date: Mon, 5 May 2014 15:03:14 +0200 (CEST)\n"
"From: PureMessage Admin <postmaster@uni-bielefeld.de>\n"
"Subject: =?UTF-8?Q?SpamMailsinQuarant?= =?UTF-8?Q?neseit?= Apr 16 15:00\n"
;
        const array<const char *,3> ref_vs = {{
"Mon, 5 May 2014 15:03:14 +0200 (CEST)",
"PureMessage Admin <postmaster@uni-bielefeld.de>",
"SpamMailsinQuarantneseit Apr 16 15:00"
        }}
;
        const array<const char *,3> ref_fields = {{
          "Date",
          "From",
          "Subject"
        }};
        d.read(inp, inp + sizeof(inp)-1 );
        // invokes callback
        d.read(inp + sizeof(inp)-1, inp+sizeof(inp) );
        BOOST_CHECK_EQUAL_COLLECTIONS(fields.begin(), fields.end(), ref_fields.begin(), ref_fields.end());
        BOOST_CHECK_EQUAL_COLLECTIONS(vs.begin(), vs.end(), ref_vs.begin(), ref_vs.end());
      }

      BOOST_AUTO_TEST_CASE(base64)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject: =?utf-8?b?SGVsbG8gV29ybGQhIA==?= foo\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Hello World!  foo");
      }

      BOOST_AUTO_TEST_CASE(q)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject: =?UTF-8?q?D=C3=A9tails=20du=20compte=20sur=20Wikip=C3=A9dia?=";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Détails du compte sur Wikipédia");
      }
      BOOST_AUTO_TEST_CASE(qb)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject: Re: Bestellung =?iso-8859-1?B?/GJl?= =?iso-8859-1?Q?r?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Re: Bestellung über");
      }
      BOOST_AUTO_TEST_CASE(leading_space)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject:  Re: Bestellung =?iso-8859-1?B?/GJl?= =?iso-8859-1?Q?r?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, " Re: Bestellung über");
      }
      BOOST_AUTO_TEST_CASE(leading_space2)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        const char inp[] = "Subject:    Re: Bestellung =?iso-8859-1?B?/GJl?= =?iso-8859-1?Q?r?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "   Re: Bestellung über");
      }
      BOOST_AUTO_TEST_CASE(multws)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        // example from RFC2047
        const char inp[] = "Subject: =?ISO-8859-1?Q?a?=  =?ISO-8859-1?Q?b?=\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "ab");
      }
      BOOST_AUTO_TEST_CASE(multwsfold)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        // example from RFC2047
        const char inp[] = "Subject: =?ISO-8859-1?Q?a?=\r\n   =?ISO-8859-1?Q?b?=\r\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "ab");
      }
      BOOST_AUTO_TEST_CASE(charset_conv)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        // example from RFC2047
        const char inp[] = "From: Nathaniel Borenstein <nsb@thumper.bellcore.com>\r\n"
            "    (=?iso-8859-8?b?7eXs+SDv4SDp7Oj08A==?=)\r\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Nathaniel Borenstein <nsb@thumper.bellcore.com>    (םולש ןב ילטפנ)");
      }
      BOOST_AUTO_TEST_CASE(charset_latin1)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        // example from RFC2047
        const char inp[] = "From: =?ISO-8859-1?Q?Patrik_F=E4ltstr=F6m?= <paf@nada.kth.se>\r\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Patrik Fältström <paf@nada.kth.se>");
      }

      BOOST_AUTO_TEST_CASE(enforce_crlf)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::CRLF);
        // example from RFC2047
        const char inp[] = "From: =?ISO-8859-1?Q?Patrik_F=E4ltstr=F6m?= <paf@nada.kth.se>\r\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Patrik Fältström <paf@nada.kth.se>");
      }
      BOOST_AUTO_TEST_CASE(enforce_crlf_compl)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::CRLF);
        // example from RFC2047
        const char inp[] = "From: =?ISO-8859-1?Q?Patrik_F=E4ltstr=F6m?= <paf@nada.kth.se>\n";
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }
      BOOST_AUTO_TEST_CASE(enforce_lf)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::LF);
        // example from RFC2047
        const char inp[] = "From: =?ISO-8859-1?Q?Patrik_F=E4ltstr=F6m?= <paf@nada.kth.se>\n";
        d.read(inp, inp + sizeof(inp) - 1);
        string s(v.begin(), v.end());
        BOOST_CHECK_EQUAL(s, "Patrik Fältström <paf@nada.kth.se>");
      }
      BOOST_AUTO_TEST_CASE(enforce_lf_compl)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::LF);
        // example from RFC2047
        const char inp[] = "From: =?ISO-8859-1?Q?Patrik_F=E4ltstr=F6m?= <paf@nada.kth.se>\r\n";
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }
      BOOST_AUTO_TEST_CASE(complete_body)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        string s;
        Decoder d(p, v, [&v, &s](){s.insert(s.end(), v.begin(), v.end());});
        d.set_ending_policy(Decoder::Ending::LF);
        // example from RFC2047
        const char inp[] =
          "Subject: =?iso-8859-1?Q?DWD_-=3E_Pollenflug-Gefahrenindex_Hessen_-_?=\n"
          "From: \"Deutscher Wetterdienst - Pollenflug-Gefahrenindex\" <DWD-NewsletterAdmin_PV@newsletter.dwd.de>\n"
          "Date: Sun, 25 May 2014 09:01:25 +0000 (UTC)\n"
          "\n"
          ;
        const char ref[] = "DWD -> Pollenflug-Gefahrenindex Hessen - \"Deutscher Wetterdienst - Pollenflug-Gefahrenindex\" <DWD-NewsletterAdmin_PV@newsletter.dwd.de>Sun, 25 May 2014 09:01:25 +0000 (UTC)"
          ;
        d.read(inp, inp + sizeof(inp) - 1);
        BOOST_CHECK_EQUAL(s, ref);
      }
      BOOST_AUTO_TEST_CASE(clear)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        string s;
        Decoder d(p, v, [&v, &s](){s.insert(s.end(), v.begin(), v.end());});
        d.set_ending_policy(Decoder::Ending::LF);
        // example from RFC2047
        const char inp1[] =
          "Subject: a\n"
          "From: b\n"
          "Date: c\n"
          "\n"
          ;
        const char inp2[] =
          "Subject: d\n"
          "From: e\n"
          "Date: f\n"
          "\n"
          ;
        d.read(inp1, inp1 + sizeof(inp1) - 1);
        BOOST_CHECK_EQUAL(s, "abc");
        d.clear();
        v.clear();
        s.clear();
        d.read(inp2, inp2 + sizeof(inp2) - 1);
        BOOST_CHECK_EQUAL(s, "def");
      }
      BOOST_AUTO_TEST_CASE(clear_compl)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        string s;
        Decoder d(p, v, [&v, &s](){s.insert(s.end(), v.begin(), v.end());});
        d.set_ending_policy(Decoder::Ending::LF);
        // example from RFC2047
        const char inp1[] =
          "Subject: a\n"
          "From: b\n"
          "Date: c\n"
          "\n"
          ;
        const char inp2[] =
          "Subject: d\n"
          "From: e\n"
          "Date: f\n"
          "\n"
          ;
        d.read(inp1, inp1 + sizeof(inp1) - 1);
        BOOST_CHECK_EQUAL(s, "abc");
        //d.clear();
        v.clear();
        BOOST_CHECK_THROW(d.read(inp2, inp2 + sizeof(inp2) - 1), std::runtime_error);
      }
      BOOST_AUTO_TEST_CASE(escape)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::LF);
        const char inp[] =
          "Subject: =?utf-8?q?=1b]0;Foo_Bar=07?=\n"
          "\n"
          ;
        // output without escaping of control characters
        //const char ref[] = "\x1b]0;Foo Bar\x07";
        const char ref[] = "\\x1B]0;Foo Bar\\x07";
        ostringstream o, r;
        boost::algorithm::hex(ref, ref+sizeof(ref)-1, ostream_iterator<char>(r));
        d.read(inp, inp + sizeof(inp) - 1);
        boost::algorithm::hex(v.begin(), v.end(), ostream_iterator<char>(o));
        BOOST_CHECK_EQUAL(o.str(), r.str());
      }

      BOOST_AUTO_TEST_CASE(empty_q)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::LF);
        const char inp[] =
          "Subject: =?utf-8?q?""?=\n"
          "\n"
          ;
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }

      BOOST_AUTO_TEST_CASE(empty_b)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::LF);
        const char inp[] =
          "Subject: =?utf-8?b?""?=\n"
          "\n"
          ;
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }

      BOOST_AUTO_TEST_CASE(empty_charset)
      {
        using namespace MIME::Header;
        Buffer::Vector v;
        Buffer::Proxy p;
        Decoder d(p, v, [](){});
        d.set_ending_policy(Decoder::Ending::LF);
        const char inp[] =
          "Subject: =?""?q?foo_bar?=\n"
          "\n"
          ;
        BOOST_CHECK_THROW(d.read(inp, inp + sizeof(inp) - 1), std::runtime_error);
      }

    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
