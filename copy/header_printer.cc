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
#include "header_printer.h"
#include "options.h"

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/algorithm/string.hpp>

#include <iomanip>

using namespace std;

namespace IMAP {
  namespace Copy {

    Header_Printer::Header_Printer(
            const IMAP::Copy::Options &opts,
            const Memory::Buffer::Vector &buffer,
            boost::log::sources::severity_logger< Log::Severity > &lg
        )
      :
        lg_(lg),
        opts_(opts),
        buffer_(buffer),
        header_decoder_(field_name_, field_body_, [this](){
            string name(field_name_.begin(), field_name_.end());
            string body(field_body_.begin(), field_body_.end());
            fields_.emplace(boost::to_upper_copy(name), body);
        })
    {
      header_decoder_.set_ending_policy(MIME::Header::Decoder::Ending::LF);
    }

    void Header_Printer::print()
    {
      if (    opts_.task != Task::FETCH_HEADER
           && static_cast<Log::Severity>(opts_.severity)      < Log::Severity::INFO
           && static_cast<Log::Severity>(opts_.file_severity) < Log::Severity::INFO)
        return;

      if (    static_cast<Log::Severity>(opts_.severity)      >= Log::Severity::DEBUG
           || static_cast<Log::Severity>(opts_.file_severity) >= Log::Severity::DEBUG) 
      {
        string s(buffer_.begin(), buffer_.end());
        BOOST_LOG_SEV(lg_, Log::DEBUG) << "Header: |" << s << "|";
      }

      try {
        header_decoder_.read(buffer_.begin(), buffer_.end());
        header_decoder_.verify_finished();
      } catch (const std::runtime_error &e) {
        BOOST_LOG_SEV(lg_, Log::ERROR) << e.what();
      }
      for (auto &i : fields_) {
        BOOST_LOG_SEV(lg_, opts_.task == Task::FETCH_HEADER ? Log::MSG : Log::INFO)
          << setw(10) << left << i.first << ' ' << i.second;
      }
      header_decoder_.clear();
      fields_.clear();
    }


  }
}
