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

#include "../sequence_set.h"

#include <iostream>
#include <sstream>
using namespace std;



BOOST_AUTO_TEST_SUITE( sequence_set )

  BOOST_AUTO_TEST_CASE( basic )
  {
    Sequence_Set set;
    BOOST_CHECK_EQUAL(set.size(), 0);
    for (uint32_t i = 1; i<32; ++i)
      set.push(i);
    BOOST_CHECK_EQUAL(set.size(), 1);
    vector<pair<uint32_t, uint32_t> > v;
    set.copy(v);
    BOOST_CHECK_EQUAL(v.size(), 1);
    BOOST_CHECK_EQUAL(v[0].first, 1);
    BOOST_CHECK_EQUAL(v[0].second, 31);
    set.clear();
    BOOST_CHECK_EQUAL(set.size(), 0);
    set.copy(v);
    BOOST_CHECK_EQUAL(v.size(), 0);
  }

  BOOST_AUTO_TEST_CASE( two )
  {
    Sequence_Set set;
    for (uint32_t i = 3; i<12; ++i)
      set.push(i);
    BOOST_CHECK_EQUAL(set.size(), 1);
    for (uint32_t i = 13; i<15; ++i)
      set.push(i);
    BOOST_CHECK_EQUAL(set.size(), 2);
    vector<pair<uint32_t, uint32_t> > v;
    set.copy(v);
    BOOST_CHECK_EQUAL(v.size(), 2);
    BOOST_CHECK_EQUAL(v[0].first, 3);
    BOOST_CHECK_EQUAL(v[0].second, 11);
    BOOST_CHECK_EQUAL(v[1].first, 13);
    BOOST_CHECK_EQUAL(v[1].second, 14);
    set.push(12);
    BOOST_CHECK_EQUAL(set.size(), 1);
    set.copy(v);
    BOOST_CHECK_EQUAL(v.size(), 1);
    BOOST_CHECK_EQUAL(v[0].first, 3);
    BOOST_CHECK_EQUAL(v[0].second, 14);
  }


BOOST_AUTO_TEST_SUITE_END()
