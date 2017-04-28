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
#ifndef COPY_HEADER_PRINTER_H
#define COPY_HEADER_PRINTER_H

#include <log/log.h>
#include <buffer/buffer.h>
#include <mime/header_decoder.h>

#include <map>
#include <string>

namespace IMAP {
  namespace Copy {

    class Options;
    class Header_Printer {
      private:
        boost::log::sources::severity_logger<Log::Severity> &lg_;
        const Options                                       &opts_;

        const Memory::Buffer::Vector &buffer_;

        MIME::Header::Decoder  header_decoder_;
        Memory::Buffer::Vector field_name_;
        Memory::Buffer::Vector field_body_;
        std::map<std::string, std::string> fields_;
        std::string line_;

        void pretty_print();
      public:
        Header_Printer(
            const IMAP::Copy::Options &opts,
            const Memory::Buffer::Vector &buffer,
            boost::log::sources::severity_logger<Log::Severity> &lg
            );
        void print();
    };

  }
}

#endif
