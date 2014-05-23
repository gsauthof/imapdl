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

#include <botan/pipe.h>
#include <botan/basefilt.h>
#include <botan/filters.h>
#include <botan/data_snk.h>
using namespace Botan;

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
  if (argc < 2) {
    cerr << "Call: " << *argv << " FILE\n";
    return 1;
  }
  ifstream in(argv[1], ios::binary);
  ostringstream out;

  Pipe pipe(new Chain(new Hash_Filter("SHA-256"),
                      new Hex_Encoder(Hex_Encoder::Lowercase)),
            new DataSink_Stream(out));
  pipe.start_msg();
  in >> pipe;
  pipe.end_msg();

  cout << out.str() << '\n';
  return 0;
}
