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
#include "log.h"

#include "enum.h"

#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
// for boost::log::expressions::format_named_scope
#include <boost/log/expressions/formatters/named_scope.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>

#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

// needed for attributes::timer()
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iomanip>
#include <iostream>
//using namespace std;


BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", Log::Severity)
BOOST_LOG_ATTRIBUTE_KEYWORD(scope, "Scope", boost::log::attributes::named_scope::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(timeline, "Timeline", boost::log::attributes::timer::value_type)

static const char * const severity_map[] = {
  "FTL",
  "ERR",
  "WRN",
  "MSG",
  "INF",
  "DBG",
  "DDD",
  "INS"
};

namespace Log {

  std::ostream &operator<<(std::ostream &o, Severity s)
  {
    o << enum_str(severity_map, s);
    return o;
  }

  static void setup_console(Severity severity_threshold)
  {
    auto clog = boost::log::add_console_log();
    clog->set_formatter(
        boost::log::expressions::stream
        << std::setw(5) << std::setfill('0')
        << boost::log::expressions::attr< unsigned >("LineID")
        << ' '
        << timeline
        << ' '
        << ": [" << severity
        << "] " << boost::log::expressions::smessage
        );
    clog->set_filter(severity <= severity_threshold);
  }
  void setup_file(Severity severity_threshold, const std::string &filename)
  {
    if (filename.empty())
      return;
    auto flog = boost::log::add_file_log(
      boost::log::keywords::file_name = filename
      //, boost::log::keywords::open_mode = std::ios_base::app | std::ios_base::out
      );
    flog->set_formatter(
        boost::log::expressions::stream
        << std::setw(5) << std::setfill('0')
        << boost::log::expressions::attr< unsigned >("LineID")
        << ' '
        << timeline
        << ' '
        << ": [" << severity
        << "] <"
        << boost::log::expressions::format_named_scope("Scope",
          boost::log::keywords::format = "%n@%f:%l")
        << "> "
        << boost::log::expressions::smessage
        );
    flog->set_filter(severity <= severity_threshold);
  }
  void setup_vanilla_file(Severity severity_threshold, const std::string &filename)
  {
    if (filename.empty())
      return;
    auto flog = boost::log::add_file_log(boost::log::keywords::file_name = filename);
    flog->set_formatter(boost::log::expressions::stream
        << "[" << severity << "] " << boost::log::expressions::smessage);
    flog->set_filter(severity <= severity_threshold);
  }

  boost::log::sources::severity_logger< Severity > 
    create(Severity sev, Severity file_severity,
        const std::string &logfile)
    {
      BOOST_LOG_SCOPED_THREAD_ATTR("Timeline", boost::log::attributes::timer());
      boost::log::core::get()->add_global_attribute("Scope",
          boost::log::attributes::named_scope());
      boost::log::add_common_attributes();
      boost::log::sources::severity_logger< Severity > lg(boost::log::keywords::severity = INFO);
      lg.add_attribute("Timeline", boost::log::attributes::timer());
      setup_console(sev);
      setup_file(file_severity, logfile);
      return std::move(lg);
    }

}
