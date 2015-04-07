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
#include "lex_util.h"

#include <boost/io/ios_state.hpp>

#include <stdexcept>
#include <sstream>
#include <iomanip>

using namespace std;


// replacement for boost::algorithm::hex because
// it does not allow for delimiting every byte with a blank
static void hex(const char *begin, const char *end, ostream &o)
{
  boost::io::ios_flags_saver  ifs(o);
  for (const char *i = begin; i!=end; ++i)
    o << setw(3) << hex << unsigned((unsigned char)*i);
}

static bool is_nice_char(char c)
{
  return c != '\\' && c >= ' ' && c <= '~';
}

void safely_write(ostream &o, const char *begin, size_t n)
{
  const char *end = begin + n;

  const char *last = begin;
  const char *i = begin;
  for (; i<end; ++i) {
    if (is_nice_char(*i))
      continue;
    o.write(last, i-last);
    last = i+1;
    o << "\\x";
    boost::io::ios_flags_saver ifs(o);
    boost::io::ios_fill_saver ifis(o);
    o << setw(2) << setfill('0') << hex << unsigned((unsigned char)*i);
  }
  o.write(last, i-last);
}

void throw_lex_error(const char *msg, const char *begin, const char *p, const char *pe)
{
  ostringstream o;
  o << msg;
  o << " (";
  size_t n = pe-p;
  size_t e = min(n,size_t(20));
  hex(p, p + e, o);
  o << " <=> |";
  safely_write(o, p, e);
  o << "|";
  if (p>begin) {
    size_t d = min(size_t(p-begin), size_t(20));
    o << " - " << d << " bytes before: ";
    hex(p - d, p, o);
    o << " <=>  |";
    safely_write(o, p-d, d);
    o << '|';
  }
  o << ')';
  throw runtime_error(o.str());
}

void pp_buffer(ostream &o, const char *msg, const char *c, size_t size)
{
  o << msg;
  hex(c, c+size, o);
  o << '\n';
  o.write(c, size);
}
