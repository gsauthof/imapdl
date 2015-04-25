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
#ifndef TRACE_H
#define TRACE_H

#include <string>
#include <vector>
#include <ostream>
#include <memory>
#include <limits>
#include <stddef.h>

// needed when serializing std::vector ...
//#include <boost/serialization/vector.hpp>

#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>

namespace Trace {
  enum class Type {
    SENT,
    RECEIVED,
    DISCONNECT,
    END_OF_FILE
  };

  struct Record {
    Type        type      {Type::END_OF_FILE};
    size_t      timestamp {0};
    // don't using a vector here because then the content is not
    // human readable stored in the text archive anymore ...
    //std::vector<char> message;
    std::string message;

    Record();
    Record(Type type, size_t timestamp, const char *b, size_t size);
    Record(Type type);
    Record(Type type, size_t timestamp);
/*
    template<class Archive>
      void serialize(Archive & ar, const unsigned int version)
      {
      }
*/
    template<class Archive>
      void save(Archive & ar, const unsigned int version) const
      {
        ar & type;
        ar & timestamp;
        ar & message;
      }
    template<class Archive>
      void load(Archive & ar, const unsigned int version)
      {
        ar & type;
        ar & timestamp;
        // in version 0 timestamps used to be saved as seconds (and unsigned) ...
        if (version == 0)
          timestamp *= 1000;
        ar & message;
      }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    std::ostream &print(std::ostream &o) const;
  };
  std::ostream &operator<<(std::ostream &o, const Record &r);

  class Writer_Priv;
  class Writer {
    private:
      std::unique_ptr<Writer_Priv> d;
    public:
      Writer();
      ~Writer();
      Writer(const std::string &filename);
      void start(const std::string &filename);
      void push(Type type);
      void push(Type type, const std::vector<char> &v,
          size_t size = std::numeric_limits<size_t>::max());
      void finish();
  };
}
BOOST_CLASS_VERSION(Trace::Record, 1)

#endif
