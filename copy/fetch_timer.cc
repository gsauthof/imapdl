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
#include "fetch_timer.h"

#include <exception.h>

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/named_scope.hpp>

using namespace std;

namespace IMAP {
  namespace Copy {

    Fetch_Timer::Fetch_Timer(
            Net::Client::Base &client,
            boost::log::sources::severity_logger< Log::Severity > &lg
        )
      :
        client_(client),
        lg_(lg),
        timer_(client_.io_service())
    {
    }

    void Fetch_Timer::print()
    {
      auto fetch_stop = chrono::steady_clock::now();
      auto d = chrono::duration_cast<chrono::milliseconds>
        (fetch_stop - start_);
      size_t b = client_.bytes_read() - bytes_start_;
      double r = (double(b)*1024.0)/(double(d.count())*1000.0);
      BOOST_LOG_SEV(lg_, Log::MSG) << "Fetched " << messages_
        << " messages (" << b << " bytes) in " << double(d.count())/1000.0
        << " s (@ " << r << " KiB/s)";
    }

    void Fetch_Timer::start()
    {
      start_ = chrono::steady_clock::now();
      bytes_start_ = client_.bytes_read();

      resume();
    }

    void Fetch_Timer::resume()
    {
      timer_.expires_from_now(std::chrono::seconds(1));
      timer_.async_wait([this](const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              if (ec.value() == boost::asio::error::operation_aborted)
                return;
              THROW_ERROR(ec);
            } else {
              print();
              resume();
            }
          });
    }
    void Fetch_Timer::stop()
    {
      print();
      timer_.cancel();
    }
    void Fetch_Timer::increase_messages()
    {
      ++messages_;
    }
    size_t Fetch_Timer::messages() const
    {
      return messages_;
    }

  }
}
