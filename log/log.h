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
#ifndef LOG_H
#define LOG_H

#include <ostream>

#include <boost/log/sources/severity_logger.hpp>

namespace Log {

  enum Severity {
    FIRST_,
    FATAL,
    ERROR,
    WARN,
    MSG,
    INFO,
    DEBUG,
    DEBUG_V,
    INSANE,
    LAST_
  };
  std::ostream &operator<<(std::ostream &o, Severity s);

  boost::log::sources::severity_logger< Severity > 
    create(Severity severity, Severity file_severity,
        const std::string &logfile = std::string());
  void setup_file(Severity severity_threshold, const std::string &filename);
}

#endif
