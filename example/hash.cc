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

#include "config.h"
#if defined(IMAPDL_USE_BOTAN)
  #include <botan/pipe.h>
  #include <botan/basefilt.h>
  #include <botan/filters.h>
  #include <botan/data_snk.h>
#elif defined(IMAPDL_USE_CRYPTOPP)
  #include <cryptopp/files.h>
  #include <cryptopp/filters.h>
  #include <cryptopp/hex.h>
  #include <cryptopp/sha.h>
#endif

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
#if defined(IMAPDL_USE_BOTAN)
  using namespace Botan;
  ifstream in(argv[1], ios::binary);
  ostringstream out;

  Pipe pipe(new Chain(new Hash_Filter("SHA-256"),
                      new Hex_Encoder(Hex_Encoder::Lowercase)),
            new DataSink_Stream(out));
  pipe.start_msg();
  in >> pipe;
  pipe.end_msg();

  cout << out.str() << '\n';
#elif defined(IMAPDL_USE_CRYPTOPP)
  using namespace CryptoPP;
  string out;
  CryptoPP::SHA256 sha1;
  FileSource source(argv[1], true,
      new HashFilter(sha1,
        new HexEncoder(new StringSink(out), false, 0, ""),
        false)
      );
  cout << out << '\n';
#endif

  return 0;
}
