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
#ifndef NET_TCP_CLIENT_H
#define NET_TCP_CLIENT_H

#include <log/log.h>
#include <trace/trace.h>

#include <functional>
#include <vector>
#include <queue>
#include <stack>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace Net {

  namespace Client {
    class Options {
      public:
        unsigned severity      {0};
        unsigned file_severity {0};

        std::string tracefile;
    };
    class Base {
      protected:
        boost::asio::io_service       &io_service_;
        const Options                 &opts_;
        std::vector<char>              input_;
        std::stack<std::vector<char> > write_free_stack_;
        std::queue<std::vector<char> > write_queue_;

        void log_read(size_t size);
        void log_write();
        void log_shutdown();
      protected:
        size_t bytes_read_    {0};
        size_t bytes_written_ {0};

        boost::log::sources::severity_logger<Log::Severity> &lg_;

        Trace::Writer trace_writer_;

        using Resolve_Fn = std::function<void(
            const boost::system::error_code &ec,
            boost::asio::ip::tcp::resolver::iterator iterator
            )>;
        using Connect_Fn = std::function<void(
            const boost::system::error_code &ec
            //,boost::asio::ip::tcp::resolver::iterator iterator
            )>;
        using Handshake_Fn = std::function<void(
            const boost::system::error_code &ec
            )>;
        using Read_Fn = std::function<void(
            const boost::system::error_code &ec,
            size_t length
            )>;
        using Write_Fn = std::function<void(
            const boost::system::error_code &ec,
            size_t length
            )>;
        using Shutdown_Fn = std::function<void(
            const boost::system::error_code &ec
            )>;

        Base(boost::asio::io_service &io_service, const Options &opts,
          boost::log::sources::severity_logger<Log::Severity> &lg
            );

      public:
        virtual ~Base();

        virtual void async_resolve(Resolve_Fn fn) = 0;

        virtual void async_resolve(const boost::asio::ip::tcp::resolver::query &query,
            Resolve_Fn) = 0;
        virtual void async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
            Connect_Fn fn) = 0;
        virtual void async_handshake(Handshake_Fn fn) = 0;

        virtual void async_read_some(Read_Fn fn) = 0;
        virtual void async_write(const char *c, size_t size, Write_Fn fn) = 0;
        virtual void async_write(const std::vector<char> &v, Write_Fn fn) = 0;
        virtual void async_shutdown(Shutdown_Fn fn) = 0;

        virtual void cancel() = 0;

        virtual void close() = 0;
        virtual bool is_open() const = 0;

        boost::asio::io_service &io_service();
        std::vector<char> &input();
        void do_write();
        void push_write(std::vector<char> &v);

        size_t bytes_read() const;
        size_t bytes_written() const;
    };
  }


  namespace TCP {

    namespace Client {

      class Options : public Net::Client::Options {
        public:
          std::string    local_address;
          // bound to any port -> 0
          unsigned short local_port    {0};
          std::string    host;
          std::string    service; // or port

          unsigned       ip            {4};

      };

      class Base : public Net::Client::Base {
        private:
          const Options                 &opts_;
          boost::asio::ip::tcp::socket   socket_;
          boost::asio::ip::tcp::resolver resolver_;

        public:
          void async_resolve(Resolve_Fn fn) override;

          void async_resolve(const boost::asio::ip::tcp::resolver::query &query,
              Resolve_Fn fn) override;
          void async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
              Connect_Fn fn) override;
          void async_handshake(Handshake_Fn fn) override;
          void async_read_some(Read_Fn fn) override;
          void async_write(const char *c, size_t size, Write_Fn fn) override;
          void async_write(const std::vector<char> &v, Write_Fn fn) override;
          void async_shutdown(Shutdown_Fn fn) override;

          void cancel() override;
          void close() override;
          bool is_open() const override;

        public:
          Base(boost::asio::io_service &io_service, const Options &opts,
          boost::log::sources::severity_logger<Log::Severity> &lg
              );
      };

    }

    namespace SSL {

      namespace Client {

        class Options : public Net::TCP::Client::Options {
          public:
            std::string fingerprint;
            std::string cipher;
            unsigned    cipher_preset {1};
            std::string cert_host;
            std::string ca_file;
            std::string ca_path;
            bool        tls1          {true};

            boost::asio::ssl::context &apply(boost::asio::ssl::context &context) const;
        };

        class Base : public Net::Client::Base {
          private:
            const Options             &opts_;
            boost::asio::ssl::context &context_;
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;
            boost::asio::ip::tcp::resolver resolver_;

        public:
            void async_resolve(Resolve_Fn fn) override;

            void async_resolve(const boost::asio::ip::tcp::resolver::query &query,
                Resolve_Fn fn) override;
            void async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
                Connect_Fn fn) override;
            void async_handshake(Handshake_Fn fn) override;
            void async_read_some(Read_Fn fn) override;
            void async_write(const char *c, size_t size, Write_Fn fn) override;
            void async_write(const std::vector<char> &v, Write_Fn fn) override;
            void async_shutdown(Shutdown_Fn fn) override;

            void cancel() override;
            void close() override;
            bool is_open() const override;
          public:
            Base(boost::asio::io_service &io_service,
                boost::asio::ssl::context &context, const Options &opts,
          boost::log::sources::severity_logger<Log::Severity> &lg
                );
        };

      }
    }

  }
}

#endif
