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
#include "trace.h"
#include <algorithm>
#include <memory>
#include <fstream>
#include <chrono>
#include <utility>
#include <stdexcept>
#include <boost/archive/text_oarchive.hpp>

using namespace std;

namespace Trace {

  Record::Record()
  {
  }
  Record::Record(Type type, size_t timestamp, const char *b, size_t size)
    :
      type(type),
      timestamp(timestamp),
      message(b, size)
  {
  }
  Record::Record(Type type)
    : type(type)
  {
  }
  Record::Record(Type type, size_t timestamp)
    :
      type(type),
      timestamp(timestamp)
  {
  }
  std::ostream &Record::print(std::ostream &o) const
  {
    switch (type) {
      case Type::SENT:
        o << "->";
        break;
      case Type::RECEIVED:
        o << "<-";
        break;
      case Type::DISCONNECT:
        o << "->||";
        break;
      case Type::END_OF_FILE:
        o << "{|}";
        break;
    }
    o << ' ' << timestamp << " |" << message << "|\n";
    return o;
  }
  std::ostream &operator<<(std::ostream &o, const Record &r)
  {
    return r.print(o);
  }

  class Writer_Priv {
    private:
      ofstream file_;
      std::chrono::time_point<std::chrono::steady_clock> start_;
    public:
      unique_ptr<boost::archive::text_oarchive>          oarchive_;

    public:
      Writer_Priv(const string &filename);
      size_t elapsed();
  };
  Writer_Priv::Writer_Priv(const string &filename)
  {
    file_.exceptions(ofstream::failbit | ofstream::badbit );
    file_.open(filename, ofstream::out | ofstream::binary);
    unique_ptr<boost::archive::text_oarchive> a(new boost::archive::text_oarchive(file_));
    oarchive_ = std::move(a);
    start_ = chrono::steady_clock::now();
  }
  size_t Writer_Priv::elapsed()
  {
    auto s = chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - start_).count();
    start_ = chrono::steady_clock::now();
    return s;
  }

  Writer::Writer()
  {
  }
  Writer::Writer(const std::string &filename)
  {
    if (!filename.empty())
      start(filename);
  }
  Writer::~Writer()
  {
    try {
      finish();
    } catch (...) {
      // don't throw exceptions from destructor ...
    }
  }
  void Writer::start(const std::string &filename)
  {
    if (d)
      throw logic_error("Trace Writer already started");
    d = unique_ptr<Writer_Priv>(new Writer_Priv(filename));
  }
  void Writer::push(Type type)
  {
    if (!d)
      return;
    Trace::Record r(type, d->elapsed());
    *d->oarchive_ << r;
  }
  void Writer::push(Type type, const std::vector<char> &v, size_t size)
  {
    if (!d)
      return;
    Trace::Record r(type, d->elapsed(), v.data(), std::min(v.size(), size));
    *d->oarchive_ << r;
  }
  void Writer::finish()
  {
    if (!d)
      return;
    Trace::Record r(Trace::Type::END_OF_FILE);
    *d->oarchive_ << r;
  }

}
