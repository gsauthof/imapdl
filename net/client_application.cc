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
#include "client_application.h"

#include <net/client.h>

#include <exception.h>

#include <boost/asio/ssl.hpp>
#include <boost/log/sources/record_ostream.hpp>
//#include <boost/log/attributes/named_scope.hpp>

namespace Net {

  namespace Client {

    Application::Application(
            const std::string &host,
            Net::Client::Base &client,
            boost::log::sources::severity_logger< Log::Severity > &lg
            )
      :
        host_(host),
        client_(client),
        lg_(lg)
    {
    }
    void Application::async_start(std::function<void(void)> fn)
    {
      async_resolve(fn);
    }
    void Application::async_finish(std::function<void(void)> fn)
    {
      async_quit(fn);
    }
    void Application::async_resolve(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Resolving " << host_ << "...";
      client_.async_resolve([this, fn](const boost::system::error_code &ec,
            boost::asio::ip::tcp::resolver::iterator iterator)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              THROW_ERROR(ec);
            } else {
              BOOST_LOG(lg_) << host_ << " resolved.";
              async_connect(iterator, fn);
            }
          });
    }

    void Application::async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
        std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Connecting to " << host_ << "...";
      client_.async_connect(iterator, [this, fn](const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              THROW_ERROR(ec);
            } else {
              BOOST_LOG(lg_) << host_ << " connected.";
              async_handshake(fn);
            }
          });
    }

    void Application::async_handshake(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Shaking hands with " << host_ << "...";
      client_.async_handshake([this, fn](const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              THROW_ERROR(ec);
            } else {
              BOOST_LOG(lg_) << "Handshake completed.";
              fn();
            }
          });
    }

    void Application::async_quit(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "async_quit()";
      client_.cancel();
      async_shutdown(fn);
    }

    void Application::async_shutdown(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      client_.async_shutdown([this, fn](
            const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            BOOST_LOG_SEV(lg_, Log::DEBUG) << "shutting down connect to: " << host_;
            if (ec) {
              if (   ec.category() == boost::asio::error::get_ssl_category()
                  && ec.value()    == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ)) {
              } else if (   ec.category() == boost::asio::error::get_ssl_category()
                         && ERR_GET_REASON(ec.value())
                              == SSL_R_DECRYPTION_FAILED_OR_BAD_RECORD_MAC) {
              } else if (ec.category() == boost::asio::error::get_misc_category()
                         && ec.value() == boost::asio::error::eof
                         ) {
                BOOST_LOG_SEV(lg_, Log::DEBUG) << "server " << host_ << " disconnected first";
              } else {
                if (ec.category() == boost::asio::error::get_ssl_category()) {
                  BOOST_LOG_SEV(lg_, Log::ERROR)
                    << "ssl_category: lib " << ERR_GET_LIB(ec.value())
                    << " func " << ERR_GET_FUNC(ec.value())
                    << " reason " << ERR_GET_REASON(ec.value());
                }
                BOOST_LOG_SEV(lg_, Log::DEBUG) << "do_shutdown() fail: " << ec.message();
                THROW_ERROR(ec);
              }
            } else {
            }
            client_.close();
            fn();
          });
    }


  }
}
