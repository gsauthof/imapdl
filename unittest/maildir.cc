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

#include <maildir/maildir.h>
#include <ixxx/ixxx.h>
using namespace ixxx;

using namespace std;

#ifdef STL_REGEX_IS_FIXED
  #include <regex>
  namespace re = std;
#else
  // std::regex is broken e.g. on Fedora 19, gcc 4.8.2, clang
  #include <boost/regex.hpp>
  namespace re = boost;
#endif

namespace {
  void touch(const string &path)
  {
    int fd = posix::open(path, O_CREAT | O_WRONLY, 0666);
    posix::close(fd);
  }
}

BOOST_AUTO_TEST_SUITE( maildir )

  BOOST_AUTO_TEST_CASE( create )
  {
    const char path[] = "tmp/mdir";
    fs::create_directory("tmp");
    fs::remove_all(path);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
    Maildir m(path);
    BOOST_CHECK_EQUAL(fs::exists(path), true);
    for (unsigned i = 0; i<3; ++i) {
      string f(m.create_tmp_name());
      //cout << "tmp: " << f << '\n';
      touch(f);
      m.move_to_new();
    }
    for (unsigned i = 0; i<10; ++i) {
      string f(m.create_tmp_name());
      touch(f);
      m.move_to_cur();
    }
    {
      fs::directory_iterator begin(path);
      fs::directory_iterator end;
      size_t sub_count  = distance(begin, end);
      BOOST_CHECK_EQUAL(sub_count, 3);
    }
    const array<const char*, 3> sub = { { "cur", "new", "tmp" } };
    const array<size_t, 3> count = {{ 10, 3, 0 }};
    for (unsigned i = 0; i<sub.size(); ++i) {
      string p(path);
      p += '/';
      p += sub[i];
      fs::directory_iterator begin(p);
      fs::directory_iterator end;
      size_t sub_count  = distance(begin, end);
      if (sub_count != count[i])
        cout << "info: " << sub[i] << '\n';
      BOOST_CHECK_EQUAL(sub_count, count[i]);
    }
  }
  BOOST_AUTO_TEST_CASE( except )
  {
    const char path[] = "tmp/mdirexcept";
    fs::create_directory("tmp");
    fs::remove_all(path);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
    bool caught = false;
    try {
      Maildir m(path, false);
    } catch (std::runtime_error) {
      caught = true;
    }
    BOOST_CHECK_EQUAL(caught, true);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
  }
  BOOST_AUTO_TEST_CASE( flags )
  {
    const char path[] = "tmp/mdirflags";
    fs::create_directory("tmp");
    fs::remove_all(path);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
    Maildir m(path);
    string f(m.create_tmp_name());
    touch(f);
    m.move_to_cur("SSF");
    string p(path);
    p += '/';
    p += "cur";
    fs::directory_iterator begin(p);
    fs::directory_iterator end;
    BOOST_REQUIRE(begin != end);
    string name((*begin).path().filename().generic_string());
    auto x = name.crbegin();
    BOOST_CHECK_EQUAL(*x, 'S');
    ++x;
    BOOST_CHECK_EQUAL(*x, 'F');
    ++x;
    BOOST_CHECK_EQUAL(*x, ',');
    ++x;
    BOOST_CHECK_EQUAL(*x, '2');
    ++x;
    BOOST_CHECK_EQUAL(*x, ':');
  }

  BOOST_AUTO_TEST_CASE( flags_except )
  {
    const char path[] = "tmp/mdirflagsexcept";
    fs::create_directory("tmp");
    fs::remove_all(path);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
    Maildir m(path);
    string f(m.create_tmp_name());
    touch(f);
    bool caught = false;
    try {
      m.move_to_cur("XYZ");
    } catch (std::runtime_error) {
      caught = true;
    }
    BOOST_CHECK_EQUAL(caught, true);
  }
  BOOST_AUTO_TEST_CASE( wrong_order )
  {
    const char path[] = "tmp/mdirwrong";
    fs::create_directory("tmp");
    fs::remove_all(path);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
    Maildir m(path);
    bool caught = false;
    try {
      m.move_to_new();
    } catch (std::runtime_error) {
      caught = true;
    }
    BOOST_CHECK_EQUAL(caught, true);
  }
  BOOST_AUTO_TEST_CASE( wrong_order2 )
  {
    const char path[] = "tmp/mdirwrong2";
    fs::create_directory("tmp");
    fs::remove_all(path);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
    Maildir m(path);
    string f(m.create_tmp_name());
    touch(f);
    m.move_to_new();
    bool caught = false;
    try {
      m.move_to_new();
    } catch (std::runtime_error) {
      caught = true;
    }
    BOOST_CHECK_EQUAL(caught, true);
  }

  BOOST_AUTO_TEST_CASE( regex )
  {
    // testing against non-conforming C++11 std::regex versions
    // (e.g. Fedora 19, gcc 4.8.2, clang)
    re::regex pat(R"(([0-9].*))", re::regex::extended);
    BOOST_CHECK_EQUAL(re::regex_match("12312.foo.bar", pat), true);
  }

  BOOST_AUTO_TEST_CASE( pattern )
  {
    const char path[] = "tmp/mdirpat";
    fs::create_directory("tmp");
    fs::remove_all(path);
    BOOST_CHECK_EQUAL(fs::exists(path), false);
    Maildir m(path);
    BOOST_CHECK_EQUAL(fs::exists(path), true);
    for (unsigned i = 0; i<23; ++i) {
      string f(m.create_tmp_name());
      touch(f);
      m.move_to_cur();
    }
    string p(path);
    p += "/cur" ;
    fs::directory_iterator begin(p);
    fs::directory_iterator end;
    size_t i = 0;
    const char x[] = R"(^[0-9]+\.P[0-9]+Q[0-9]+R[0-9a-fA-F]+\.[A-Za-z0-9._-]+:2,[DFPRST]*$)";
    // cout << "pattern: |" << x << "|\n";
    // should work with std::regex as well
    // but as least on Fedora 19 (gcc 4.8) std::regex is broken
    // throws pattern errors and incorrectly mis-matches
    re::regex pat( x, re::regex::extended);
    for (auto it = begin; it != end; ++it, ++i) {
      string t((*it).path().filename().generic_string());
      //cout << "> " << t << '\n';
      bool b = re::regex_match(t, pat);
      if (!b)
        cout << "info: mismatch: |" << t << "| pattern: |" << x << "|\n";
      BOOST_CHECK_EQUAL(b, true);
    }
    BOOST_CHECK_EQUAL(i, 23);
  }


BOOST_AUTO_TEST_SUITE_END()

