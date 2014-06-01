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
#ifndef COPY_FETCH_TIMER_H
#define COPY_FETCH_TIMER_H

#include <log/log.h>
#include <net/client.h>

#include <chrono>

#include <boost/asio/steady_timer.hpp>

namespace IMAP {
  namespace Copy {

    class Fetch_Timer {
      private:
        Net::Client::Base                                           &client_;
        boost::log::sources::severity_logger<Log::Severity>         &lg_;
        std::chrono::time_point<std::chrono::steady_clock>           start_;
        boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
        size_t bytes_start_ {0};
        size_t messages_  {0};
      public:
        Fetch_Timer(
            Net::Client::Base &client,
            boost::log::sources::severity_logger<Log::Severity> &lg
            );
        void start();
        void resume();
        void stop();
        void print();
        void increase_messages();
        size_t messages() const;
    };

  }
}

#endif
