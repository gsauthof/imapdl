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
#include "journal.h"
//#include "client.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/tracking.hpp>

#include <fstream>
using namespace std;

#include "sequence_set.h"

namespace boost {
  namespace serialization {

    template<class Archive>
      void serialize(Archive & a, IMAP::Copy::Journal &d, const unsigned int /* version */)
      {
        a & d.mailbox_;
        a & d.uidvalidity_;
        a & d.uids_;
      }

  }
}
BOOST_CLASS_TRACKING(IMAP::Copy::Journal, boost::serialization::track_never)

namespace IMAP {
  namespace Copy {

    Journal::Journal(const string &mailbox, uint32_t uidvalidity, const Sequence_Set &set)
      :
        mailbox_(mailbox),
        uidvalidity_(uidvalidity)
    {
      set.copy(uids_);
    }
    Journal::Journal()
    {
    }

    void Journal::read(const std::string &filename)
    {
      ifstream f;
      f.exceptions(ifstream::failbit | ifstream::badbit );
      f.open(filename, ifstream::in | ifstream::binary);
      // when Journal::write() was called from inside a destructor,
      // the written file doesn't end in a newline
      // and in that case the boost deserialization triggers an EOF bit
      // that also yields a failbit ...
      // and we don't want to transform that into an exception that isn't caught inside
      // a boost detructor chain ... and thus would terminate the program ...
      f.exceptions(ifstream::badbit);
      boost::archive::text_iarchive a(f);
      a >> *this;
    }
    void Journal::write(const std::string &filename)
    {
      ofstream f;
      f.exceptions(ofstream::failbit | ofstream::badbit );
      f.open(filename, ofstream::out | ofstream::binary);
      boost::archive::text_oarchive a(f);
      a << *this;
    }


  }
}
