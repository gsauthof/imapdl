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
#ifndef NET_CLIENT_APPLICATION_H
#define NET_CLIENT_APPLICATION_H

#include <string>
#include <functional>

#include <boost/asio/ip/tcp.hpp>

#include <log/log.h>

namespace Net { namespace Client { class Base; } }

namespace Net {

  namespace Client {

    class Application {
      private:
        const std::string                                     &host_;
        Net::Client::Base                                     &client_;
        boost::log::sources::severity_logger< Log::Severity > &lg_;

        void async_resolve(std::function<void(void)> fn);
        void async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
            std::function<void(void)> fn);
        void async_handshake(std::function<void(void)> fn);

        void async_quit(std::function<void(void)> fn);
        void async_shutdown(std::function<void(void)> fn);
      public:
        Application(
            const std::string &host,
            Net::Client::Base &client,
            boost::log::sources::severity_logger<Log::Severity> &lg
            );
        void async_start (std::function<void(void)> fn);
        void async_finish(std::function<void(void)> fn);
    };

  }
}

#endif
