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
#include "client.h"

#include <exception.h>

#include <boost/log/sources/record_ostream.hpp>

using namespace std;

namespace Net {

  namespace Client {
    Base::Base(boost::asio::io_service &io_service, const Options &opts,
          boost::log::sources::severity_logger<Log::Severity> &lg
        )
      :
        io_service_(io_service),
        opts_(opts),
        input_(4 * 1024),
        lg_(lg),
        trace_writer_(opts_.tracefile)
    {
    }
    Base::~Base()
    {
    }
    boost::asio::io_service &Base::io_service()
    {
      return io_service_;
    }
    std::vector<char> &Base::input()
    {
      return input_;
    }
    void Base::log_read(size_t size)
    {
      bytes_read_ += size;
      trace_writer_.push(Trace::Type::RECEIVED, input_, size);
      if (opts_.severity < Log::DEBUG && opts_.file_severity < Log::DEBUG)
        return;
      BOOST_LOG_SEV(lg_, Log::DEBUG_V) << "Read " << size << " bytes from host";
      string s(input_.data(), size);
      BOOST_LOG_SEV(lg_, Log::DEBUG_V) << "Read |" << s << "|";
    }
    void Base::log_write()
    {
      trace_writer_.push(Trace::Type::SENT, write_queue_.front());
      if (opts_.severity < Log::DEBUG && opts_.file_severity < Log::DEBUG)
        return;
      BOOST_LOG_SEV(lg_, Log::DEBUG_V) << "Schedule " << write_queue_.front().size()
        << " bytes to write to host";
      string s(write_queue_.front().data(), write_queue_.front().size());
      BOOST_LOG_SEV(lg_, Log::DEBUG_V) << "Schedule write |" << s << "|";
    }
    void Base::log_shutdown()
    {
      trace_writer_.push(Trace::Type::DISCONNECT);
    }
    void Base::push_write(std::vector<char> &v)
    {
      bool write_in_progress = !write_queue_.empty();
      if (write_free_stack_.empty()) {
        write_queue_.push(std::move(v));
      } else {
        vector<char> t(std::move(write_free_stack_.top()));
        write_free_stack_.pop();
        std::swap(v, t);
        write_queue_.push(std::move(t));
      }
      if (!write_in_progress)
        do_write();
    }
    void Base::do_write()
    {
      if (write_queue_.empty())
        THROW_LOGIC_MSG("do_write() called with empty queue");

      log_write();
      async_write(write_queue_.front(), [this](
          const boost::system::error_code &ec, size_t size
            )
          {
            if (ec) {
              THROW_ERROR(ec);
            } else {
              bytes_written_ += size;
              BOOST_LOG_SEV(lg_, Log::DEBUG_V) << "Wrote " << size << " bytes.";
              write_free_stack_.push(std::move(write_queue_.front()));
              write_free_stack_.top().clear();
              write_queue_.pop();
            }
          });
    }

    size_t Base::bytes_read() const
    {
      return bytes_read_;
    }
    size_t Base::bytes_written() const
    {
      return bytes_written_;
    }

  }

}
