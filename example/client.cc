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
#include <memory>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <iterator>
#include <cstring>
#include <sstream>
#include <inttypes.h>
using namespace std;
#include <boost/lexical_cast.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>




#include <boost/program_options.hpp>
namespace po = boost::program_options;


#include <ixxx/ixxx.h>
using namespace ixxx;

#include <trace/trace.h>

#include <net/ssl_util.h>
using namespace Net::SSL;

#include <lex_util.h>

namespace Client {

  namespace OPT {

    static const char HELP_S[]        = "help,h";
    static const char HELP[]          = "help";
    static const char FINGERPRINT[]   = "fp";
    static const char CIPHER[]        = "cipher";
    static const char CIPHER_PRESET[] = "cipher_preset";
    static const char USE_SSL[]       = "ssl";
    static const char CA_FILE[]       = "ca";
    static const char CA_PATH[]       = "ca_path";
    static const char HOST[]          = "host";
    static const char CERT_HOST[]     = "cert_host";
    static const char SERVICE[]       = "service";
    static const char TRACEFILE[]     = "trace";
    static const char REPLAYFILE[]    = "replay";
    static const char LIMIT[]         = "limit";
  }


  Options::Options()
    : 
      cipher(Cipher::default_list(Cipher::Class::FORWARD))
  {
  }

  Options::Options(int argc, char **argv)
  {
    po::options_description general_group("Options");
    general_group.add_options()
      (OPT::HELP_S, "this help screen")
      (OPT::USE_SSL,
       po::value<bool>(&use_ssl)
       ->default_value(false, "false")
       ->implicit_value(true, "true")->value_name("bool"),
       "use SSL/TLS")
      (OPT::FINGERPRINT, po::value<string>(&fingerprint)->default_value(""),
       "verify certificate using a known fingerprint (instead of a CA)")
      (OPT::CA_FILE, po::value<string>(&ca_file)->default_value("server.crt"),
       "file containing CA/server certificate")
      (OPT::CA_PATH, po::value<string>(&ca_path)->default_value(""),
       "directory contained hashed CA certs")
      (OPT::CERT_HOST, po::value<string>(&cert_host)->default_value(""),
       "hostname used for certificate checking - "
       "if not set the connecting hostname is used")
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
      (OPT::HOST, po::value<string>(&host)->required(), "remote host")
      (OPT::SERVICE, po::value<string>(&service)->required(), "service name or port")
      ;

    po::options_description visible_group;
    visible_group.add(general_group);
    po::options_description all;
    all.add(visible_group);
    all.add(hidden_group);

    po::positional_options_description pdesc;
    pdesc.add(OPT::HOST, 1);
    pdesc.add(OPT::SERVICE, 1);
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
        .options(all)
        .positional(pdesc)
        .run(), vm);

    if (vm.count(OPT::HELP)) {
      cout << "call: " << *argv << " OPTION* HOST PORT\n"
        << visible_group << "\n";
      exit(0);
    }
    po::notify(vm);

    fix();
  }

  void Options::fix()
  {
    if (cert_host.empty())
      cert_host = host;
    if (cipher.empty())
      cipher = Cipher::default_list(Cipher::to_class(cipher_preset));
  }

  class Verification {
    private:
      asio::ssl::rfc2818_verification default_;

      unsigned pos_ = {0};
      bool result_ = {false};
      string fingerprint_;
    public:
      Verification(const string &hostname,
          const string &fingerprint = string())
        :
          default_(hostname),
          fingerprint_(boost::to_upper_copy<std::string>(fingerprint))
      {
      }
      bool operator()(bool preverified, asio::ssl::verify_context & ctx)
      {
        ++pos_;

        bool r = default_(preverified, ctx);

        if (!r) {
          X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
          (void)cert;

          auto digest_type = EVP_sha1();
          {
            array<unsigned char, 128> buffer = {{0}};
            unsigned size = buffer.size();
            int r = X509_digest(cert, digest_type, buffer.data(), &size);
            (void)r;
            ostringstream fp;
            //for (unsigned i = 0; i < size; ++i)
            //  fp << setw(2) << hex << unsigned(buffer[i]);
            boost::algorithm::hex(buffer.data(), buffer.data() + size,
                ostream_iterator<char>(fp));
            cout << "sha1 fingerprint: |" << fp.str() << "|\n";
            if (!fingerprint_.empty()) {
              if (pos_ == 1) {
                result_ = fingerprint_ == fp.str();
                if (result_) {
                  cout << "Fingerprint matches. Authentication finished.\n";
                } else
                  cout << "Given fingerprint |" << fingerprint_ << "| does not"
                    " match the one of the certificate: |" << fp.str() << "|\n";
              }
              return result_;
            }
          }
          {
            array<char, 128> char_buffer = {{0}};
            X509_NAME_oneline(X509_get_subject_name(cert),
                char_buffer.data(), char_buffer.size());
            cout << "Subject: " << char_buffer.data() << '\n';
          }

        }

        return r;
      }
  };



  asio::ssl::stream<tcp::socket>::lowest_layer_type &Main::socket()
  {
    if (use_ssl_)
      return ssl_socket_.lowest_layer();
    else
      return socket_;
  }

  Main::Main(asio::io_service &io_service, asio::ssl::context &context,
      const Options &opts)
      :
        opts_(opts),
        resolver_(io_service),
        socket_(io_service),
        use_ssl_(opts.use_ssl),
        context_(context),
        ssl_socket_(io_service, context_),
        data_(4 * 1024),
        in_(io_service),
        inp_(4 * 1024),
        signals_(io_service, SIGINT, SIGTERM),
        use_log_(!opts.tracefile.empty()),
        use_replay_(!opts.replayfile.empty()),
        timer_(io_service),
        limit_timer_(io_service)
    {
      if (use_log_) {
        tracefile_.exceptions(ofstream::failbit | ofstream::badbit );
        tracefile_.open(opts.tracefile, ofstream::out | ofstream::binary);
        unique_ptr<boost::archive::text_oarchive> a(
            new boost::archive::text_oarchive(tracefile_));
        oarchive_ = std::move(a);
      }
      if (!use_replay_) {
        in_.assign(posix::dup(STDIN_FILENO));
      }

      if (use_ssl_) {
        ssl_socket_.set_verify_mode(asio::ssl::verify_peer);
        ssl_socket_.set_verify_callback(
            Verification(opts.cert_host, opts.fingerprint));

        cout << "Using cipher list: " << opts.cipher << '\n';
        SSL_set_cipher_list(ssl_socket_.native_handle(), opts.cipher.c_str());
      }

      do_signal_wait();
      do_keyboard_read();

      // Start an asynchronous resolve to translate the server and service names
      // into a list of endpoints.
      tcp::resolver::query query(opts.host, opts.service);
      resolver_.async_resolve(query,
          [this](
              const boost::system::error_code& ec,
              tcp::resolver::iterator iterator
            ){
            if (!ec) {
              start_ = chrono::steady_clock::now();
              if (use_ssl_)
                do_ssl_connect(iterator);
              else
                do_connect(iterator);
            }
            else
            {
              std::cout << "resolve error: " << ec.message() << "\n";
            }
          });
    }
    Main::~Main()
    {
      if (use_log_) {
        Trace::Record r(Trace::Type::END_OF_FILE);
        *oarchive_ << r;
      }
      cout << "Client destructed\n";
    }
    void Main::do_signal_wait()
    {
      // the handler is called once for one async_wait call
      // thus, the loop logic in case write blocks
      // and there is a 2nd signal delivered ...
      signals_.async_wait([this](
            const boost::system::error_code &ec, int signal_number)
          {
            if (!ec) {
              cout << "Got signal: " << signal_number << '\n' << endl;
              if (signaled_) {
                socket().close();
                in_.close();
                signals_.cancel();
              } else {
                ++signaled_;
                do_signal_wait();
                do_last_write_close();
              }
            }
          });
    }
    void Main::do_last_write_close()
    {
      const char bye[] = "good bye!\r\n";

      auto f = [this](const boost::system::error_code &ec, std::size_t /*length*/)
          {
            if (use_ssl_) {
              // this yields a 'short read' 'error' in the do_read() callback
              // on which it calls do_shutdown()
              //ssl_socket_.lowest_layer().shutdown(tcp::socket::shutdown_receive);

              // alternatively: triggers a operation_aborted error in the
              // do_read() callback
              ssl_socket_.lowest_layer().cancel();
            } else {
              socket().close();
              in_.close();
              signals_.cancel();
            }
          };

      if (use_ssl_)
        boost::asio::async_write(ssl_socket_,
            boost::asio::buffer(bye, sizeof(bye)-1), f);
      else
        boost::asio::async_write(socket_,
            boost::asio::buffer(bye, sizeof(bye)-1), f);
    }
    void Main::do_keyboard_read()
    {
      if (use_replay_)
        return;
      in_.async_read_some(asio::buffer(inp_.data(), inp_.size()-1),
          [this](const boost::system::error_code &ec, size_t length)
            {
              if (!ec) {
                pp_buffer(cout, "Read from stdin: ", inp_.data(), length);
                bool write_in_progress = !write_queue_.empty();
                vector<char> v(inp_.data(), inp_.data() + length + 1);
                v[length ? length-1 : 0] = '\r';
                v[length ? length   : 1] = '\n';

                write_queue_.push(std::move(v));
                if (!write_in_progress)
                  do_write();

                do_keyboard_read();
              } else {
                cout << "std-in error or eof: " << ec.message() << '\n';
                socket().close();
                signals_.cancel();
              }
          });
    }
    void Main::do_write()
    {
      if (write_queue_.empty())
        throw logic_error("do_write() called with empty queue");

      auto f=[this](const boost::system::error_code &ec, std::size_t /*length*/)
          {
            if (!ec) {
              write_queue_.pop();
              if (!write_queue_.empty())
                do_write();
            }
          }
          ;

      if (use_log_) {
        Trace::Record r(Trace::Type::SENT,
            std::chrono::duration_cast<chrono::milliseconds>(
              (chrono::steady_clock::now()-start_)).count(),
            write_queue_.front().data(),
                              write_queue_.front().size());
        start_ = chrono::steady_clock::now();
        *oarchive_ << r;
      }

      if (use_ssl_)
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
    void Main::do_ssl_connect(tcp::resolver::iterator iterator)
    {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      asio::async_connect(ssl_socket_.lowest_layer(), iterator,
          [this](
              const boost::system::error_code& ec,
              tcp::resolver::iterator iterator
            ){
            if (!ec) {
              do_handshake();
            } else {
              std::cout << "ssl connect error: " << ec.message() << "\n";
              signals_.cancel();
              in_.close();
            }
          });
    }
    void Main::do_handshake()
    {
      ssl_socket_.async_handshake(asio::ssl::stream_base::client,
          [this](
              const boost::system::error_code& ec
            ){
              if (use_replay_)
                start_replay();
              else
                do_read();
            if (!ec) {
            } else {
              std::cout << "ssl handshake failed: " << ec.message() << '\n';
              signals_.cancel();
              in_.close();
              ssl_socket_.lowest_layer().close();
            }
          });
    }

    void Main::do_connect(tcp::resolver::iterator iterator)
    {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      asio::async_connect(socket_, iterator,
          [this](
              const boost::system::error_code& ec,
              tcp::resolver::iterator iterator
            ){
            if (!ec) {
              if (use_replay_)
                start_replay();
              else
                do_read();
            } else {
              std::cout << "connect error: " << ec.message() << "\n";
              signals_.cancel();
              in_.close();
            }
          });
    }

    void Main::start_replay()
    {
      replayfile_.open(opts_.replayfile, ifstream::in | ifstream::binary);
      replayfile_.exceptions(ifstream::failbit | ifstream::badbit);
      unique_ptr<boost::archive::text_iarchive> a(
          new boost::archive::text_iarchive(replayfile_));
      iarchive_ = std::move(a);

      if (opts_.limit) {
        limit_timer_.expires_from_now(std::chrono::seconds(opts_.limit));
        limit_timer_.async_wait([this](const boost::system::error_code &ec)
            {
              if (ec) {
                cout << "limit_timer error: " << ec.message() << '\n';
              if (ec.value() == asio::error::operation_aborted) {
                cout << "limit_timer aborted\n";
                return;
              }
              }

              ostringstream o;
              o << "Replay takes too long - limit was: " << opts_.limit << " s";
              throw std::runtime_error(o.str());
            }
            );
      }

      do_replay();
    }

    void Main::do_replay()
    {
      Trace::Record r;
      *iarchive_ >> r;
      timer_.expires_from_now(std::chrono::milliseconds(r.timestamp));
      switch (r.type) {
        case Trace::Type::SENT:
          {
            timer_.async_wait([this, r](const boost::system::error_code &ec)
                {
                  vector<char> v(r.message.data(), r.message.data() + r.message.size());
                  bool write_in_progress = !write_queue_.empty();
                  write_queue_.push(std::move(v));
                  if (!write_in_progress)
                    do_write();
                  do_replay();
                }
                );
          }
          break;
        case Trace::Type::RECEIVED:
          {
            timer_.async_wait([this, r](const boost::system::error_code &ec)
                {
                  vector<char> v(r.message.data(), r.message.data() + r.message.size());
                  expected_data_ = std::move(v);
                  do_read();
                }
                );
          }
          break;
        case Trace::Type::DISCONNECT:
          do_close();
          break;
        case Trace::Type::END_OF_FILE:
          break;
      }
    }

    void Main::do_close()
    {
      if (use_ssl_) {
        // this yields a 'short read' 'error' in the do_read() callback
        // on which it calls do_shutdown()
        //ssl_socket_.lowest_layer().shutdown(tcp::socket::shutdown_receive);

        // alternatively: triggers a operation_aborted error in the
        // do_read() callback
        ssl_socket_.lowest_layer().cancel();
      } else {
        socket().close();
        in_.close();
        signals_.cancel();
      }
    }

    void Main::do_read()
    {
      auto f =   [this](const boost::system::error_code &ec, size_t length)
          {
            if (!ec)
            {
              pp_buffer(cout, "Read some: ", data_.data(), length);

              if (use_log_) {
                Trace::Record r(Trace::Type::RECEIVED,
                    std::chrono::duration_cast<chrono::milliseconds>(
                      (chrono::steady_clock::now()-start_)).count(),
                    data_.data(), length);
                start_ = chrono::steady_clock::now();
                *oarchive_ << r;
              }

              if (use_replay_) {
                if (    length != expected_data_.size()
                    || !equal(data_.data(), data_.data() + length,
                            expected_data_.data()) ) {
                  ostringstream o;
                  o << "Received string |";
                  o.write(data_.data(), length);
                  o << "| does not match saved one |";
                  o.write(expected_data_.data(), expected_data_.size());
                  o << "|";
                  throw std::runtime_error(o.str());
                }
              }

              const char bye_string[] = "bye\r\n";
              if (!memcmp(data_.data(), bye_string, sizeof(bye_string)-1)) {
                cout << "exiting because server send bye\n";
                do_shutdown();
                return;
              }

              if (use_replay_)
                do_replay();
              else
                do_read();
            } else {
              cout << "Read error: " << ec.message() << '\n';
              // either close notifier received from server
              // or shutdown(receive) was calles on lowest_layer
              if
#if BOOST_VERSION >= 106300
  (    ec.category() == boost::asio::ssl::error::get_stream_category()
    && ec.value() == boost::asio::ssl::error::stream_errors::stream_truncated)
#else
  (    ec.category() == boost::asio::error::get_ssl_category()
    && ec.value() == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ) )
#endif
              {
                cout << "(not really an error)\n";
                do_shutdown();
              // cancel was called on the lowest layer socket
              // to initiate a proper tls shutdown
              } else if (ec.value() == asio::error::operation_aborted) {
                cout << "(not really an error)\n";
                do_shutdown();
              }
            }
          }
      ;
      if (use_ssl_)
        ssl_socket_.async_read_some(asio::buffer(data_.data(), data_.size()-1), f);
      else
        socket_.async_read_some(asio::buffer(data_.data(), data_.size()-1), f);
    }

    void Main::do_shutdown()
    {
      if (!use_ssl_)
        return;

      cout << "async shutdown\n";
            //ssl_socket_.lowest_layer().close();

      ssl_socket_.async_shutdown([this](const boost::system::error_code &ec)
          {
            cout << "Doing shutdown ...\n";
            if (use_log_) {
              Trace::Record r(Trace::Type::DISCONNECT,
                  std::chrono::duration_cast<chrono::milliseconds>(
                    (chrono::steady_clock::now()-start_)).count());
              start_ = chrono::steady_clock::now();
              *oarchive_ << r;
            }
            if (ec) {
              cout << "SSL shutdown error: " << ec.message() << '\n';

              if
#if BOOST_VERSION >= 106300
  (    ec.category() == boost::asio::ssl::error::get_stream_category()
    && ec.value() == boost::asio::ssl::error::stream_errors::stream_truncated)
#else
  (    ec.category() == boost::asio::error::get_ssl_category()
    && ec.value() == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ) )
#endif
              {
                cout << "(not really an error)\n";
              }
            }
            in_.close();
            signals_.cancel();
            timer_.cancel();
            limit_timer_.cancel();

            ssl_socket_.lowest_layer().close();
          }
          );
    }


  };



