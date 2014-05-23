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
#include "tcp_client.h"

#include "ssl_util.h"
#include "ssl_verification.h"
#include "exception.h"

#include <boost/log/sources/record_ostream.hpp>

using namespace std;
namespace asio = boost::asio;

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
      if (opts_.severity < Log::DEBUG)
        return;
      BOOST_LOG_SEV(lg_, Log::DEBUG_V) << "Read " << size << " bytes from host";
      string s(input_.data(), size);
      BOOST_LOG_SEV(lg_, Log::DEBUG_V) << "Read |" << s << "|";
    }
    void Base::log_write()
    {
      trace_writer_.push(Trace::Type::SENT, write_queue_.front());
      if (opts_.severity < Log::DEBUG)
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

  namespace TCP {

    namespace Client {

      Base::Base(boost::asio::io_service &io_service, const Options &opts,
          boost::log::sources::severity_logger<Log::Severity> &lg
          )
        :
          Net::Client::Base(io_service, opts, lg),
          opts_(opts),
          socket_(io_service),
          resolver_(io_service)
      {
      }

      void Base::async_resolve(Resolve_Fn fn)
      {
        asio::ip::tcp::resolver::query query(opts_.host, opts_.service);
        async_resolve(query, fn);
      }
      void Base::async_resolve(const boost::asio::ip::tcp::resolver::query &query,
          Resolve_Fn fn)
      {
        resolver_.async_resolve(query, fn);
      }
      void Base::async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
          Connect_Fn fn)
      {
        if (!opts_.local_address.empty()) {
          if (opts_.ip == 4)
            socket_.open(asio::ip::tcp::v4());
          else
            socket_.open(asio::ip::tcp::v6());

          asio::ip::tcp::endpoint local_endpoint(
              asio::ip::address::from_string(opts_.local_address),
              opts_.local_port
              );
          socket_.bind(local_endpoint);
        }
        socket_.async_connect(iterator->endpoint(), fn);
      }
      void Base::async_handshake(Handshake_Fn fn)
      {
        boost::system::error_code ec;
        fn(ec);
      }
      void Base::async_read_some(Read_Fn fn)
      {
        socket_.async_read_some(asio::buffer(input_), [this, fn](
            const boost::system::error_code &ec,
            size_t size)
          {
            if (!ec)
              log_read(size);
            fn(ec, size);
          });
      }
      void Base::async_write(const char *c, size_t size, Write_Fn fn)
      {
        asio::async_write(socket_, asio::buffer(c, size), fn);
      }
      void Base::async_write(const std::vector<char> &v, Write_Fn fn)
      {
        asio::async_write(socket_, asio::buffer(v), fn);
      }
      void Base::async_shutdown(Shutdown_Fn fn)
      {
        log_shutdown();
        boost::system::error_code ec;
        fn(ec);
      }
      void Base::cancel()
      {
        socket_.cancel();
      }
      void Base::close()
      {
        socket_.close();
      }
      bool Base::is_open() const
      {
        return socket_.is_open();
      }

    }

    namespace SSL {

      namespace Client {

        boost::asio::ssl::context &Options::apply(boost::asio::ssl::context &context)
          const
        {
          using namespace Net::SSL;

          Context::set_defaults(context);
          if (tls1)
            context.clear_options(asio::ssl::context::no_tlsv1);
          if (fingerprint.empty())
            context.load_verify_file(ca_file);
          if (!ca_path.empty())
            context.add_verify_path(ca_path);
          return context;
        }

        Base::Base(boost::asio::io_service &io_service,
            boost::asio::ssl::context &context, const Options &opts,
          boost::log::sources::severity_logger<Log::Severity> &lg
            )
          :
            Net::Client::Base(io_service, opts, lg),
            opts_(opts),
            context_(opts_.apply(context)),
            stream_(io_service, context_),
            resolver_(io_service)
        {
          using namespace Net::SSL;
          stream_.set_verify_mode(asio::ssl::verify_peer);
          stream_.set_verify_callback(
              Verification(lg_, opts_.cert_host, opts_.fingerprint));
          if (!opts_.cipher.empty())
            SSL_set_cipher_list(stream_.native_handle(), opts_.cipher.c_str());
        }

        void Base::async_resolve(Resolve_Fn fn)
        {
          asio::ip::tcp::resolver::query query(opts_.host, opts_.service);
          async_resolve(query, fn);
        }
        void Base::async_resolve(const boost::asio::ip::tcp::resolver::query &query,
            Resolve_Fn fn)
        {
          resolver_.async_resolve(query, fn);
        }
        void Base::async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
            Connect_Fn fn)
        {
          if (!opts_.local_address.empty()) {
            if (opts_.ip == 4)
              stream_.lowest_layer().open(asio::ip::tcp::v4());
            else
              stream_.lowest_layer().open(asio::ip::tcp::v6());

            asio::ip::tcp::endpoint local_endpoint(
                asio::ip::address::from_string(opts_.local_address),
                opts_.local_port
                );
            stream_.lowest_layer().bind(local_endpoint);
          }
          stream_.lowest_layer().async_connect(iterator->endpoint(), fn);
        }
        void Base::async_handshake(Handshake_Fn fn)
        {
          stream_.async_handshake(asio::ssl::stream_base::client, fn);
        }
        void Base::async_read_some(Read_Fn fn)
        {
          stream_.async_read_some(asio::buffer(input_), [this, fn](
            const boost::system::error_code &ec,
            size_t size)
          {
            if (!ec)
              log_read(size);
            fn(ec, size);
          });
        }
        void Base::async_write(const char *c, size_t size, Write_Fn fn)
        {
          asio::async_write(stream_, asio::buffer(c, size), fn);
        }
        void Base::async_write(const std::vector<char> &v, Write_Fn fn)
        {
          asio::async_write(stream_, asio::buffer(v), fn);
        }
        void Base::async_shutdown(Shutdown_Fn fn)
        {
          log_shutdown();
          stream_.async_shutdown(fn);
        }
        void Base::cancel()
        {
          stream_.lowest_layer().cancel();
        }
        void Base::close()
        {
          stream_.lowest_layer().close();
        }
        bool Base::is_open() const
        {
          return stream_.lowest_layer().is_open();
        }

      }
    }
  }

}
