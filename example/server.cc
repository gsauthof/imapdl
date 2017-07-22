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
#include "server.h"
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <utility>
using namespace std;

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <net/ssl_util.h>
using namespace Net::SSL;
#include <trace/trace.h>

#include <lex_util.h>

namespace Server {

  namespace OPT {
    static const char HELP_S[]        = "help,h";
    static const char HELP[]          = "help";
    static const char CIPHER[]        = "cipher";
    static const char CIPHER_PRESET[] = "cipher_preset";
    static const char USE_SSL[]       = "ssl";
    static const char TRACEFILE[]     = "trace";
    static const char REPLAYFILE[]    = "replay";
    static const char LIMIT[]         = "limit";

    static const char PORT[]          = "port";
    static const char DHPARAM[]       = "dhparam";
    static const char CERTIFICATE[]   = "cert";
    static const char KEY[]           = "key";
  }

  Options::Options(ostream &out)
    :
      out_(out),
      cipher(Cipher::default_list(Cipher::Class::FORWARD))
  {
  }

  Options::Options(int argc, char **argv, ostream &out)
    :
      out_(out)
  {
    po::options_description general_group("Options");
    general_group.add_options()
      (OPT::HELP_S, "this help screen")
      (OPT::USE_SSL,
       po::value<bool>(&use_ssl)
       ->default_value(false, "false")
       ->implicit_value(true, "true")->value_name("bool"),
       "use SSL/TLS")
      (OPT::KEY, po::value<string>(&key)->default_value("server.key"),
       "private key file")
      (OPT::CERTIFICATE, po::value<string>(&cert)->default_value("server.crt"),
       "public key file")
      (OPT::DHPARAM, po::value<string>(&dhparam)->default_value("dh768.pem"),
       "dh param file")
      (OPT::CIPHER, po::value<string>(&cipher)->default_value(""),
       "openssl cipher list - default: only ones with forward secrecy")
      (OPT::CIPHER_PRESET, po::value<unsigned>(&cipher_preset)->default_value(1),
       "cipher list presets: 1: forward secrecy, 2: TLSv1.2, 3: old, "
       "smaller is better")
      (OPT::TRACEFILE, po::value<string>(&tracefile)->default_value(""),
       "trace file for capturing send/received messages")
      (OPT::REPLAYFILE, po::value<string>(&replayfile)->default_value(""),
       "replay a previously recorded tracefile")
      (OPT::LIMIT, po::value<unsigned>(&limit)->default_value(0),
       "time limit for replay session in seconds - 0 means unlimited")
      ;
    po::options_description hidden_group;
    hidden_group.add_options()
      (OPT::PORT, po::value<unsigned>(&port), "port")
      ;

    po::options_description visible_group;
    visible_group.add(general_group);
    po::options_description all;
    all.add(visible_group);
    all.add(hidden_group);

    po::positional_options_description pdesc;
    pdesc.add(OPT::PORT, 1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
        .options(all)
        .positional(pdesc)
        .run(), vm);

    if (vm.count(OPT::HELP)) {
      out_ << "call: " << *argv << " OPTION* PORT\n"
        << visible_group << "\n";
      exit(0);
    }

    po::notify(vm);

    fix();
  }
  void Options::fix()
  {
    if (cipher.empty())
      cipher = Cipher::default_list(Cipher::to_class(cipher_preset));
    use_replay = !replayfile.empty();
  }


  class session : public std::enable_shared_from_this<session> {
    private:
      ostream &out_;
      Main &parent_;
      const Options &opts_;

      tcp::socket socket_;
      ssl_socket ssl_socket_;
      boost::asio::signal_set signals_;
      std::array<char, 1024> data_ = { "S: " };
      boost::asio::streambuf buf_;
      unsigned signaled_ {0};

      queue<vector<char> > write_queue_;
      vector<char> expected_data_;
      ifstream replayfile_;
      unique_ptr<boost::archive::text_iarchive> iarchive_;
      // asio::steady_timer timer_;
      // workaround: boost autodetection of std::chrono
      // does not seem to work in all boost versions
      asio::basic_waitable_timer<std::chrono::steady_clock> timer_;


      void start_replay();
      void do_close();
      void do_replay();
      void do_read_line();
      void do_read();

      void do_write(std::size_t length);
      void do_write();
      void do_last_write(std::size_t length);
      void do_last_write_close();
      void wait_for_disconnect();

      void do_signal_wait();
    public:
      session(asio::io_service& io_service, asio::ssl::context& context,
          const Options &opts,
          Main &parent
          );
      session(
          tcp::socket &&socket,
          asio::io_service& io_service, asio::ssl::context& context,
          const Options &opts,
          Main &parent);
      ~session();

      void start();

      ssl_socket::lowest_layer_type &socket();
  };


  session::session(
      asio::io_service& io_service, asio::ssl::context& context,
      const Options &opts, Main &parent)
    :
      out_(parent.out()),
      parent_(parent),
      opts_(opts),
      socket_(io_service),
      ssl_socket_(io_service, context),
      signals_(io_service, SIGINT, SIGTERM),
      timer_(io_service)
  {
    out_ << "Using cipher list: " << opts.cipher << '\n';
    SSL_set_cipher_list(ssl_socket_.native_handle(), opts_.cipher.c_str());
    out_ << "Session started (" << this << ")\n";
  }
  session::~session()
  {
    out_ << "Session terminated (" << this << ")\n";
    parent_.cancel_limit();
  }
  session::session(
      tcp::socket &&socket,
      asio::io_service& io_service, asio::ssl::context& context,
      const Options &opts,
      Main &parent
      )
    :
      out_(parent.out()),
      parent_(parent),
      opts_(opts),
      socket_(std::move(socket)),
      ssl_socket_(io_service, context),
      signals_(io_service, SIGINT, SIGTERM),
      timer_(io_service)
  {
    out_ << "Session started (" << this << ")\n";
  }
  void session::start()
  {

    auto self(shared_from_this());

    do_signal_wait();

    if (opts_.use_ssl) {
      ssl_socket_.async_handshake(boost::asio::ssl::stream_base::server,
          [this, self](const boost::system::error_code &ec)
          {
            if (!ec) {
              if (opts_.use_replay)
                start_replay();
              else
                do_read();
            } else {
              out_ << "handshake error: " << ec.message() << '\n';
              signals_.cancel();
            }
          });
    } else {
      if (opts_.use_replay)
        start_replay();
      else
        do_read();
    }
  }

  void session::start_replay()
  {
    auto self(shared_from_this());

    replayfile_.open(opts_.replayfile, ifstream::in | ifstream::binary);
    replayfile_.exceptions(ifstream::failbit | ifstream::badbit);
    unique_ptr<boost::archive::text_iarchive> a(
        new boost::archive::text_iarchive(replayfile_));
    iarchive_ = std::move(a);


    do_replay();
  }

  void session::do_replay()
  {
    auto self(shared_from_this());

    Trace::Record r;
    *iarchive_ >> r;
    out_ << "Replay expires in: " << r.timestamp << '\n';
    timer_.expires_from_now(std::chrono::milliseconds(r.timestamp));
    switch (r.type) {
      case Trace::Type::RECEIVED:
        {
          timer_.async_wait([this, self, r](const boost::system::error_code &ec)
              {
                if (!ec) {
                  out_ << "do_replay: RECEIVED\n";
                  vector<char> v(r.message.data(), r.message.data() + r.message.size());
                  bool write_in_progress = !write_queue_.empty();
                  write_queue_.push(std::move(v));
                  if (!write_in_progress)
                    do_write();
                  do_replay();
                } else {
                  out_ << "timer error 1: " << ec.message() << '\n';
                }
              }
              );
        }
        break;
      case Trace::Type::SENT:
        {
          out_ << "do_replay: SENT\n";
          timer_.async_wait([this, self, r](const boost::system::error_code &ec)
              {
                if (!ec) {
                  vector<char> v(r.message.data(), r.message.data() + r.message.size());
                  expected_data_ = std::move(v);
                  do_read();
                  // wrong place to call do_replay() because do_read()
                  // is non blocking, i.e. most likely immediately returns
                  // do_replay();
                } else {
                  out_ << "timer error 2: " << ec.message() << '\n';
                }
              }
              );
        }
        break;
      case Trace::Type::DISCONNECT:
        wait_for_disconnect();
        break;
      case Trace::Type::END_OF_FILE:
        out_ << "do_replay: EOF\n";
        //do_close();
        break;
    }
  }

  void session::wait_for_disconnect()
  {
    out_ << "replay: wait for disconnect\n";
    auto f =   [this](const boost::system::error_code &ec, size_t length)
          {
            out_ << "replay: ";
            if (ec) {
              if
#if BOOST_VERSION >= 106300
  (    ec.category() == boost::asio::ssl::error::get_stream_category()
    && ec.value() == boost::asio::ssl::error::stream_errors::stream_truncated)
#else
  (    ec.category() == boost::asio::error::get_ssl_category()
    && ec.value() == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ) )
#endif
              {
                out_ << "got SSL notfied\n";
                signals_.cancel();
                return;
              }
              if (ec.value() == asio::error::operation_aborted) {
                out_ << "got operation aborted\n";
                signals_.cancel();
                return;
              }
              if (ec.value() == asio::error::eof) {
                out_ << "got eof\n";
                signals_.cancel();
                return;
              }
              out_ << "other error: " << ec.message() << '\n';
            } else {
              out_ << "No error, read: " << length << '\n';
            }
            throw runtime_error("expected disconnect from client");
          };
      if (opts_.use_ssl)
        ssl_socket_.async_read_some(asio::buffer(data_.data()+3, data_.size()-3), f);
      else
        socket_.async_read_some(asio::buffer(data_.data()+3, data_.size()-3), f);
  }

  void session::do_close()
  {
    if (opts_.use_ssl) {
      ssl_socket_.async_shutdown([this](const boost::system::error_code &ec)
          {
            out_ << "Doing shutdown ...\n";
            if (ec) {
              out_ << "SSL shutdown error: " << ec.message() << '\n';
              if
#if BOOST_VERSION >= 106300
  (    ec.category() == boost::asio::ssl::error::get_stream_category()
    && ec.value() == boost::asio::ssl::error::stream_errors::stream_truncated)
#else
  (    ec.category() == boost::asio::error::get_ssl_category()
    && ec.value() == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ) )
#endif
              {
                out_ << "(not really an error)\n";
              }
            }
            signals_.cancel();
            // destructor does this for us ...
            //ssl_socket_.lowest_layer().close();
          }
          );
    }

  }

  void session::do_signal_wait()
  {
    // don't add self to the capture list because otherwise
    // the smartpointer does not destroy the object on return
    // from do_read
    // -> alternatively explicitly cancel the pending handler
    //    via do_last_write
    // we assume that destructor of signals_ destroys pending signal
    // handler
    auto self(shared_from_this());
    signals_.async_wait([this,self](
          const boost::system::error_code &ec,
          int signal_number
          )
        {
          if (!ec) {
            out_ << "Got signal (in session): " << signal_number << '\n' << endl;
            if (signaled_) {
              socket().close();
              signals_.cancel();
            } else {
              ++signaled_;
              do_signal_wait();
              do_last_write_close();
            }
          }
        });
  }

  ssl_socket::lowest_layer_type &session::socket()
  {
    return ssl_socket_.lowest_layer();
  }

  void session::do_read()
  {
    auto self(shared_from_this());

    auto f = [this, self](const boost::system::error_code &ec, std::size_t length)
    {
      if (!ec) {
        pp_buffer(out_, "Read some: ", data_.data(), length);

        if (opts_.use_replay) {
          if (    length != expected_data_.size()
              || !equal(data_.data()+3, data_.data() + 3 + length,
                      expected_data_.data()) ) {
            ostringstream o;
            o << "Received string |";
            o.write(data_.data()+3, length);
            o << "| does not match saved one |";
            o.write(expected_data_.data(), expected_data_.size());
            o << "|";
            throw std::runtime_error(o.str());
          }
        }


        const char exit_string[] = "exit\r\n";
        if (!memcmp(data_.data()+3, exit_string, sizeof(exit_string)-1)) {
          strcpy(data_.data()+3, "exit -> GOOD BYE!\r\n");
          do_last_write(strlen(data_.data()));
          return;
        }
        if (opts_.use_replay) {
          // right place because socket is in non-blocking mode ...
          do_replay();
        } else {
          do_write(length+3);
        }
      } else {
        out_ << "read error: " << ec.message() << '\n';
      }
    };
    if (opts_.use_ssl) {
      ssl_socket_.async_read_some(asio::buffer(data_.data() + 3, data_.size()-3),
          f);
    } else {
      socket_.async_read_some(asio::buffer(data_.data() + 3, data_.size()-3), f);
    }
  }

  void session::do_read_line()
  {
    auto self(shared_from_this());
    // not needed if we can guarantee that the sender
    // always make sure that packet payload ends in "\r\n" and the
    // underlying protocol has large enough packets
    boost::asio::async_read_until(ssl_socket_, buf_, "\r\n",
        [this, self](const boost::system::error_code &ec, std::size_t length)
        {
          if (!ec) {
            buf_.sgetn(data_.data()+3, std::min(data_.size()-1, length));
            pp_buffer(out_, "Read until: ", data_.data(), length);
            const char exit_string[] = "exit\r\n";
            if (!memcmp(data_.data()+3, exit_string, sizeof(exit_string)-1)) {
              strcpy(data_.data()+3, "exit -> GOOD BYE!\r\n");
              do_last_write(strlen(data_.data()));
              return;
            }
            do_write(length+3);
          }
        });
  }

  void session::do_write(std::size_t length)
  {
    auto self(shared_from_this());

    auto f = 
      [this, self](const boost::system::error_code &ec, std::size_t /*length*/)
      {
        if (!ec) {
          do_read();
        }
      };
    if (opts_.use_ssl)
      asio::async_write(ssl_socket_, asio::buffer(data_.data(), length),f);
    else
      asio::async_write(socket_, asio::buffer(data_.data(), length),f);
  }

  void session::do_write()
  {
    auto self(shared_from_this());

    if (write_queue_.empty())
      throw logic_error("do_write() called with empty queue");

    auto f = [this, self](const boost::system::error_code &ec, std::size_t /*length*/)
        {
          if (!ec) {
            write_queue_.pop();
            if (!write_queue_.empty())
              do_write();
          }
        }
        ;

    if (opts_.use_ssl)
      boost::asio::async_write(ssl_socket_,
          boost::asio::buffer(write_queue_.front().data(),
                              write_queue_.front().size()),
          f);
    else
      boost::asio::async_write(socket_,
          boost::asio::buffer(write_queue_.front().data(),
                              write_queue_.front().size()),
          f);
  }


  void session::do_last_write(std::size_t length)
  {
    auto self(shared_from_this());

    auto f =
      [this, self](const boost::system::error_code &ec, std::size_t /*length*/)
      {
        // such that smartpointer gets decremented
        signals_.cancel();
      };

    if (opts_.use_ssl)
      asio::async_write(ssl_socket_, asio::buffer(data_.data(), length), f);
    else
      asio::async_write(socket_, asio::buffer(data_.data(), length), f);
  }

  static const char goodbye_msg_[] = "server is shutting down";

  void session::do_last_write_close()
  {
    auto self(shared_from_this());

    auto f =
      [this, self](const boost::system::error_code &ec, std::size_t /*length*/)
      {
        // unconditionally end this session ...
        //if (!ec)
        if (opts_.use_ssl)
          socket().close();
        else
          socket_.cancel();
        signals_.cancel();
      };
    if (opts_.use_ssl)
      asio::async_write(ssl_socket_,
          asio::buffer(goodbye_msg_, sizeof(goodbye_msg_)-1), f);
    else
      asio::async_write(socket_,
          asio::buffer(goodbye_msg_, sizeof(goodbye_msg_)-1), f);
  }



  Main::Main(boost::asio::io_service& io_service, const Options &opts,
      ostream &out)
    :
      out_(out),
      opts_(opts),
      io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v6(), opts.port)),
      socket_(io_service),
      context_(boost::asio::ssl::context::sslv23),
      signals_(io_service, SIGINT, SIGTERM),
      limit_timer_(io_service)
  {
    Context::set_defaults(context_);

    //context_.set_password_callback(boost::bind(&Main::get_password, this));
    context_.use_certificate_chain_file(opts.cert);
    context_.use_private_key_file(opts.key, boost::asio::ssl::context::pem);
    context_.use_tmp_dh_file(opts.dhparam);

    signals_.async_wait([this](
          const boost::system::error_code &ec,
          int signal_number
          )
        {
          if (!ec) {
            out_ << "Got signal: " << signal_number
              << " size = " << sizeof(boost::asio::signal_set) << '\n' << endl;
            acceptor_.close();
          } else {
            out_ << "signals error: " << ec.message() << '\n';
          }
        });

    if (opts_.limit) {
      limit_timer_.expires_from_now(std::chrono::seconds(opts_.limit));
      limit_timer_.async_wait([this](const boost::system::error_code &ec)
          {
            if (ec) {
              if (ec.value() == asio::error::operation_aborted) {
                out_ << "limit_timer aborted\n";
                return;
              }
              out_ << "limit_timer error: " << ec.message() << '\n';
            }

            ostringstream o;
            o << "Replay takes too long - limit was: " << opts_.limit << " s";
            throw std::runtime_error(o.str());
          }
          );
    }

    if (opts_.use_ssl)
      do_ssl_accept();
    else
      do_accept();
  }

  void Main::do_ssl_accept()
  {
    // note that there is always one session object ready to take over
    // on start, immediately a new object is created,
    // waiting for its start ...
    auto new_session = std::make_shared<session>(io_service_, context_, opts_,
        *this);
    acceptor_.async_accept(new_session->socket(),
        [this, new_session](const boost::system::error_code &ec)
        {
          if (!ec) {
            new_session->start();

            if (opts_.use_replay) {
              out_ << "replay: just one shot during replay\n";
              signals_.cancel();
            } else {
              do_ssl_accept();
            }
          } else {
            out_ << "accept ERROR: " << ec.message() << '\n';
          }
        });
  }
  void Main::do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](const boost::system::error_code &ec)
        {
          if (!ec) {
            std::make_shared<session>(std::move(socket_),
              io_service_, context_, opts_, *this
              )->start();

            if (opts_.use_replay) {
              out_ << "replay: just one shot during replay\n";
              signals_.cancel();
            } else {
              do_accept();
            }
          } else {
            out_ << "accept error: " << ec.message() << '\n';
          }
        });
  }
  void Main::cancel_limit()
  {
    limit_timer_.cancel();
  }

}

