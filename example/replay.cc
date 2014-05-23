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
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <trace/trace.h>



static void c_print_server(boost::archive::text_iarchive &iarchive, ostream &o)
{
  o << "const char * const received[] = {\n\n";

  for (;;) {
    Trace::Record r;
    iarchive >> r;

    switch (r.type) {
      case Trace::Type::SENT:
        o << "// " << r.message << '\n';
        break;
      case Trace::Type::RECEIVED:
        // o << "R\"(" << r.message << ")\",\n";
        // GCC eats '\r' in raw string literals when they come before '\n'
        // thus workaround:
        {
          o << R"(")";
          for (auto c : r.message) {
            switch (c) {
              case '\n':
                o << R"(\n")" << "\n\"";
                break;
              case '\r':
                o << R"(\r)";
                break;
              case '\t':
                o << R"(\t)";
                break;
              case '\\':
                o << R"(\\)";
                break;
              case '"':
                o << R"(\")";
                break;
              default:
                o << c;
            }
          }
          o << R"(",)" << "\n\n";
        }
        break;
      case Trace::Type::END_OF_FILE:
        o << "0\n\n };\n";
        return;
      default:
        break;

    }
  }
}


int main(int argc, char **argv)
{
  if (argc < 2) {
    cout << "Call: " << *argv << " LOGFILE\n";
    return 1;
  }

  string logfile(argv[1]);
  ifstream f(logfile, ifstream::in | ifstream::out);
  f.exceptions(ifstream::badbit | ifstream::failbit );
  boost::archive::text_iarchive iarchive(f);

  if (argc > 2 && argv[2] == string("--cs")) {
    c_print_server(iarchive, cout);
    return 0;
  }

  for (;;) {
    Trace::Record r;
    iarchive >> r;
    cout << r;
    if (r.type == Trace::Type::END_OF_FILE)
      break;
  }

  return 0;
}
