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

using namespace Memory;

#include "options.h"
#include "enum.h"
#include "exception.h"

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/system/error_code.hpp>
#include <boost/algorithm/string.hpp>

#include <sstream>
#include <stdexcept>
#include <string>
#include <functional>
using namespace std;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "journal.h"

namespace IMAP {
  namespace Copy {

    State operator++(State &s)
    {
      unsigned i = static_cast<unsigned>(s);
      ++i;
      s = static_cast<State>(i);
      return s;
    }
    State operator+(State s, int b)
    {
      unsigned i = static_cast<unsigned>(s);
      i += b;
      s = static_cast<State>(i);
      return s;
    }
    static const char *const state_map[] = {
      "DISCONNECTED",
      "ESTABLISHED",
      "GOT_INITIAL_CAPABILITIES",
      "LOGGED_IN",
      "GOT_CAPABILITIES",
      "SELECTED_MAILBOX",
      "FETCHING",
      "FETCHED",
      "STORED",
      "EXPUNGED",
      "LOGGING_OUT",
      "LOGGED_OUT",
      "END"
    };
    std::ostream &operator<<(std::ostream &o, State s)
    {
      o << enum_str(state_map, s);
      return o;
    }

    Client::Client(IMAP::Copy::Options &opts,
        std::unique_ptr<Net::Client::Base> &&net_client,
        boost::log::sources::severity_logger<Log::Severity> &lg)
      :
        opts_(opts),
        client_(std::move(net_client)),
        lg_(lg),
        signals_(client_->io_service(), SIGINT, SIGTERM),
        parser_(buffer_proxy_, tag_buffer_, *this),
        login_timer_(client_->io_service()),
        writer_(tags_, std::bind(&Client::to_cmd, this, std::placeholders::_1)),
        maildir_(opts_.maildir),
        tmp_dir_(maildir_.tmp_dir_fd()),
        fetch_timer_(client_->io_service()),
        mailbox_(opts_.mailbox),
        header_decoder_(field_name_, field_body_, [this](){
            string name(field_name_.begin(), field_name_.end());
            string body(field_body_.begin(), field_body_.end());
            fields_.emplace(boost::to_upper_copy(name), body);
        })
    {
      BOOST_LOG_FUNCTION();
      buffer_proxy_.set(&buffer_);
      header_decoder_.set_ending_policy(MIME::Header::Decoder::Ending::LF);
      read_journal();
      do_signal_wait();
      do_resolve();
    }
    Client::~Client()
    {
      try {
        write_journal();
      } catch (...) {
        // don't throw exceptions in destructor ...
      }
    }

    void Client::read_journal()
    {
      if (fs::exists(opts_.journal_file)) {
        Journal journal;
        BOOST_LOG_SEV(lg_, Log::MSG) << "Reading journal " << opts_.journal_file << " ...";
        journal.read(opts_.journal_file);
        uidvalidity_ = journal.uidvalidity_;
        uids_ = journal.uids_;
        mailbox_ = journal.mailbox_;
        task_ = Task::CLEANUP;
        fs::remove(opts_.journal_file);
      }
    }
    void Client::write_journal()
    {
      if (uids_.empty())
        return;
      if (!opts_.del)
        return;
      BOOST_LOG_SEV(lg_, Log::MSG) << "Writing journal " << opts_.journal_file << " ...";
      Journal journal(opts_.mailbox, uidvalidity_, uids_);
      journal.write(opts_.journal_file);
    }

    void Client::to_cmd(vector<char> &x)
    {
      std::swap(x, cmd_);
    }

    void Client::do_signal_wait()
    {
      signals_.async_wait([this]( const boost::system::error_code &ec, int signal_number)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              if (ec.value() == boost::system::errc::operation_canceled) {
              } else {
                THROW_ERROR(ec);
              }
            } else {
              BOOST_LOG_SEV(lg_, Log::ERROR) << "Got signal: " << signal_number;
              if (signaled_) {
                ostringstream o;
                o << "Got a signal (" << signal_number
                  << ") the second time - immediate exit";
                THROW_MSG(o.str());
              } else {
                ++signaled_;
                do_signal_wait();
                do_quit();
              }
            }
          });
    }

    void Client::do_resolve()
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Resolving " << opts_.host << "...";
      client_->async_resolve([this](const boost::system::error_code &ec,
            boost::asio::ip::tcp::resolver::iterator iterator)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              THROW_ERROR(ec);
            } else {
              BOOST_LOG(lg_) << opts_.host << " resolved.";
              do_connect(iterator);
            }
          });
    }

    void Client::do_connect(boost::asio::ip::tcp::resolver::iterator iterator)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Connecting to " << opts_.host << "...";
      client_->async_connect(iterator, [this](const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              THROW_ERROR(ec);
            } else {
              BOOST_LOG(lg_) << opts_.host << " connected.";
              do_handshake();
            }
          });
    }

    void Client::do_handshake()
    {
      BOOST_LOG_FUNCTION();
      if (opts_.use_ssl)
        BOOST_LOG(lg_) << "Cipher list: " << opts_.cipher;
      BOOST_LOG(lg_) << "Shaking hands with " << opts_.host << "...";
      client_->async_handshake([this](const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              THROW_ERROR(ec);
            } else {
              BOOST_LOG(lg_) << "Handshake completed.";
              //state_ = State::ESTABLISHED;
              do_read();
              do_pre_login();
            }
          });
    }

    void Client::print_fetch_stats()
    {
      auto fetch_stop = chrono::steady_clock::now();
      auto d = chrono::duration_cast<chrono::milliseconds>
        (fetch_stop - fetch_start_);
      size_t b = client_->bytes_read() - fetch_bytes_start_;
      double r = (double(b)*1024.0)/(double(d.count())*1000.0);
      BOOST_LOG_SEV(lg_, Log::MSG) << "Fetched " << fetched_messages_
        << " messages (" << b << " bytes) in " << double(d.count())/1000.0
        << " s (@ " << r << " KiB/s)";
    }

    void Client::start_fetch_timer()
    {
      fetch_start_ = chrono::steady_clock::now();
      fetch_bytes_start_ = client_->bytes_read();

      resume_fetch_timer();
    }

    void Client::resume_fetch_timer()
    {
      fetch_timer_.expires_from_now(std::chrono::seconds(1));
      fetch_timer_.async_wait([this](const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              if (ec.value() == boost::asio::error::operation_aborted)
                return;
              THROW_ERROR(ec);
            } else {
              print_fetch_stats();
              resume_fetch_timer();
            }
          });
    }
    void Client::stop_fetch_timer()
    {
      print_fetch_stats();
      fetch_timer_.cancel();
    }

    void Client::async_login_capabilities(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      auto cap_fn = [this, fn](){
        cond_async_capabilities(fn);
      };
      async_login(cap_fn);
    }

    void Client::async_cleanup(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      auto finish_fn = [this, fn](){
        uids_.clear();
        mailbox_ = opts_.mailbox;
        BOOST_LOG_SEV(lg_, Log::MSG) << "Deleting messages from last time ... finished";
        fn();
      };
      auto expunge_fn = [this, finish_fn](){
       async_uid_or_simple_expunge(finish_fn);
      };
      auto store_fn = [this, expunge_fn](){
        async_store(expunge_fn);
      };
      async_select(store_fn);
    }

    void Client::do_cleanup()
    {
      async_cleanup(std::bind(&Client::do_download, this));
    }

    void Client::do_download()
    {
      BOOST_LOG_FUNCTION();
      auto finish_fn = [this](){
        do_quit();
      };
      auto logout_fn = [this, finish_fn](){
        uids_.clear();
        async_logout(finish_fn);
      };
      auto expunge_fn = [this, logout_fn](){
        async_uid_or_simple_expunge(logout_fn);
      };
      auto store_fn = [this, expunge_fn, finish_fn](){
        stop_fetch_timer();
        async_store_or_logout(expunge_fn, finish_fn);
      };
      auto fetch_fn = [this, store_fn, finish_fn](){
        async_fetch_or_logout(store_fn, finish_fn);
      };
      async_select(fetch_fn);
    }

    void Client::do_pre_login()
    {
      login_timer_.expires_from_now(std::chrono::milliseconds(opts_.greeting_wait));
      login_timer_.async_wait([this](
          const boost::system::error_code &ec)
        {
          BOOST_LOG_FUNCTION();
          if (ec && ec.value() != boost::system::errc::operation_canceled) {
            THROW_ERROR(ec);
          } else {
            BOOST_LOG(lg_) << "Point after first possibly occured read";
            //do_capabilities();
            cond_async_capabilities([this](){
                async_login_capabilities(std::bind(&Client::do_task, this));
              });
          }
        });
    }

    void Client::do_task()
    {
      switch (task_) {
        case Task::CLEANUP:
          do_cleanup();
          break;
        case Task::DOWNLOAD:
          do_download();
          break;
        default:
          ;
      }
    }

    void Client::async_capabilities(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.capability(tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Getting CAPABILITIES ..." << " [" << tag << ']';
      do_write();
    }
    void Client::cond_async_capabilities(std::function<void(void)> fn)
    {
      if (capabilities_.empty()) {
        async_capabilities(fn);
      } else {
        BOOST_LOG(lg_) << "not fetching capabilities (already received)";
        fn();
      }
    }

    void Client::async_login(const std::string &username, const std::string &password,
        std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.login(username, password, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Logging in as |" << username << "| [" << tag << "]";
      BOOST_LOG_SEV(lg_, Log::INSANE) << "Password: |" << password << "|";
      do_write();
    }

    void Client::async_login(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      using namespace IMAP::Server::Response;
      if (capabilities_.find(Capability::IMAP4rev1) == capabilities_.end())
        THROW_MSG("Server has not IMAP4rev1 capability");
      if (capabilities_.find(Capability::LOGINDISABLED) != capabilities_.end())
        THROW_MSG("Cannot login because server has LOGINDISABLED");
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "Clearing capabilities";
      capabilities_.clear();
      exists_ = 0;
      recent_ = 0;
      // Don't reset them on login in case the values are loaded from a journal
      //uidvalidity_ = 0;
      //uids_.clear();
      async_login(opts_.username, opts_.password, fn);
    }

    void Client::async_select(const std::string &mailbox, std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.select(mailbox, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Selecting mailbox: |" << mailbox << "|" << " [" << tag << ']';
      do_write();
    }
    void Client::async_select(std::function<void(void)> fn)
    {
      async_select(mailbox_, fn);
    }

    void Client::async_fetch_or_logout(std::function<void(void)> after_fetch,
        std::function<void(void)> after_logout)
    {
      BOOST_LOG_FUNCTION();
      if (exists_) {
        async_fetch(after_fetch);
      } else {
        BOOST_LOG_SEV(lg_, Log::MSG) << "Mailbox " << opts_.mailbox << " is empty.";
        async_logout(after_logout);
      }
    }

    void Client::async_fetch(
            const std::vector<std::pair<uint32_t, uint32_t> > &set,
            const std::vector<IMAP::Client::Fetch_Attribute> &atts,
            std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.fetch(set, atts, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Fetching into " << opts_.maildir
        <<  " ..." << " [" << tag << ']';
      start_fetch_timer();
      do_write();
    }

    void Client::async_fetch(std::function<void(void)> fn)
    {
      vector<pair<uint32_t, uint32_t> > set = {
        {1, numeric_limits<uint32_t>::max()}
      };

      using namespace IMAP::Client;
      vector<Fetch_Attribute> atts;
      atts.emplace_back(Fetch::UID);
      atts.emplace_back(Fetch::FLAGS);
      vector<string> fields;
      fields.emplace_back("date");
      fields.emplace_back("from");
      fields.emplace_back("subject");
      // BODY_PEEK - same as BODY but don't set \seen flag ...
      atts.emplace_back(Fetch::BODY_PEEK,
          IMAP::Section_Attribute(IMAP::Section::HEADER_FIELDS, std::move(fields)));
      atts.emplace_back(Fetch::BODY_PEEK);

      state_ = State::FETCHING;
      async_fetch(set, atts, fn);
    }

    void Client::async_store_or_logout(std::function<void(void)> after_store,
        std::function<void(void)> after_logout)
    {
      BOOST_LOG_FUNCTION();
      if (opts_.del) {
        async_store(after_store);
      } else {
        async_logout(after_logout);
      }
    }

    void Client::async_store(
            const std::vector<std::pair<uint32_t, uint32_t> > &set,
            const std::vector<IMAP::Flag> &flags,
            std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.uid_store(set, flags, tag, IMAP::Client::Store_Mode::REPLACE, true);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Storing DELETED flags ..." << " [" << tag << ']';
      do_write();
    }
    void Client::async_store(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      vector<pair<uint32_t, uint32_t> > set;
      uids_.copy(set);
      vector<IMAP::Flag> flags;
      flags.emplace_back(IMAP::Flag::DELETED);
      async_store(set, flags, fn);
    }

    bool Client::has_uidplus() const
    {
      BOOST_LOG_FUNCTION();
      auto i = capabilities_.find(IMAP::Server::Response::Capability::UIDPLUS);
      BOOST_LOG(lg_) << "Has UIDPLUS capability: " << (i != capabilities_.end());
      return i != capabilities_.end();
    }

    void Client::async_uid_or_simple_expunge(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      if (has_uidplus())
        async_uid_expunge(fn);
      else
        async_expunge(fn);
    }

    void Client::async_uid_expunge(const std::vector<std::pair<uint32_t, uint32_t> > &set,
        std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.uid_expunge(set, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Expunging messages ..." << " [" << tag << ']';
      do_write();
    }
    void Client::async_uid_expunge(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      vector<pair<uint32_t, uint32_t> > set;
      uids_.copy(set);
      async_uid_expunge(set, fn);
    }

    void Client::async_expunge(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.expunge(tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Expunging messages (without UIDPLUS) ..." << " [" << tag << ']';
      do_write();
    }

    void Client::async_logout(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.logout(tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Logging out ..." << " [" << tag << ']';
      //state_ = State::LOGGING_OUT;
      do_write();
    }

    void Client::do_read()
    {
      BOOST_LOG_FUNCTION();
      client_->async_read_some([this](
            const boost::system::error_code &ec,
            size_t size)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              if (          state_ == State::LOGGED_OUT
                  && (   (    ec.category() == boost::asio::error::get_ssl_category()
                           && ec.value()    == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ)
                         )
                      || (    ec.category() == boost::asio::error::get_misc_category()
                           && ec.value()    == boost::asio::error::eof
                         )
                     )
                )
                  {
                //BOOST_LOG_SEV(lg_, Log::DEBUG) << "do_read() -> do_shutdown()";
                //do_shutdown();
              } else {
                BOOST_LOG_SEV(lg_, Log::DEBUG) << "do_read() fail: " << ec.message();
                THROW_ERROR(ec);
              }
            } else {
              parser_.read(client_->input().data(), client_-> input().data() + size);
              if (state_ != State::LOGGED_OUT) // && client_->is_open())
                do_read();
            }
          });
    }

    void Client::do_write()
    {
      client_->push_write(cmd_);
    }

    void Client::do_quit()
    {
      BOOST_LOG_FUNCTION();
      state_ = State::LOGGED_OUT;
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "do_quit()";
      client_->cancel();
      do_shutdown();
    }

    void Client::do_shutdown()
    {
      BOOST_LOG_FUNCTION();
      client_->async_shutdown([this](
            const boost::system::error_code &ec)
          {
            BOOST_LOG_FUNCTION();
            if (ec) {
              if (   ec.category() == boost::asio::error::get_ssl_category()
                  && ec.value()    == ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ)) {
              } else if (   ec.category() == boost::asio::error::get_ssl_category()
                         && ERR_GET_REASON(ec.value())
                              == SSL_R_DECRYPTION_FAILED_OR_BAD_RECORD_MAC) {
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
            client_->close();
            signals_.cancel();
          });
    }


    void Client::imap_status_code_capability_begin()
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "Clearing capabilities";
      capabilities_.clear();
    }
    void Client::imap_capability_begin()
    {
    }
    void Client::imap_capability(IMAP::Server::Response::Capability capability)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Got capability: " << capability;
      capabilities_.insert(capability);
    }
    void Client::imap_status_code_capability_end()
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "finished retrieving capabilties";
      login_timer_.cancel();
    }
    void Client::imap_tagged_status_end(IMAP::Server::Response::Status c)
    {
      BOOST_LOG_FUNCTION();
      string tag(tag_buffer_.begin(), tag_buffer_.end());
      BOOST_LOG(lg_) << "Got status " << c << " for tag " << tag;
      if (c != IMAP::Server::Response::Status::OK) {
        stringstream o;
        o << "Command failed: " << c << " - " << string(buffer_.begin(), buffer_.end());
        THROW_MSG(o.str());
      }
      auto i = tag_to_fn_.find(tag);
      if (i == tag_to_fn_.end()) {
        stringstream o;
        o << "Got unknown tag: " << tag;
        THROW_MSG(o.str());
      }
      tags_.pop(tag);
      auto fn = i->second;
      tag_to_fn_.erase(i);
      fn();
    }
    void Client::imap_data_exists(uint32_t number)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Mailbox " << opts_.mailbox << " contains " << number
        << " messages";
      exists_ = number;
    }
    void Client::imap_data_recent(uint32_t number)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "Mailbox " << opts_.mailbox << " has " << number
        << " RECENT messages";
      recent_ = number;
    }
    void Client::imap_status_code_uidvalidity(uint32_t n)
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG(lg_) << "UIDVALIDITY: " << n;
      if (uidvalidity_ != n) {
        BOOST_LOG_SEV(lg_, Log::DEBUG) << "Replacing UIDVALIDITY "
          << uidvalidity_ << " with " << n;
        uids_.clear();
      }
      uidvalidity_ = n;
    }

    void Client::imap_data_fetch_begin(uint32_t number)
    {
      BOOST_LOG_FUNCTION();
      flags_.clear();
      if (state_ == State::FETCHING) {
        BOOST_LOG(lg_) << "Fetching message: " << number;
        last_uid_ = 0;
        if (opts_.simulate_error == fetched_messages_ + 1) {
          ostringstream o;
          o << "Simulated error after fetched message: " << fetched_messages_;
          THROW_MSG(o.str());
        }
      }
    }
    void Client::imap_data_fetch_end()
    {
      if (!last_uid_)
        THROW_MSG("Did not retrieve any UID");
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "Storing UID: " << last_uid_;
      uids_.push(last_uid_);
    }
    void Client::imap_section_empty()
    {
      full_body_ = true;
    }
    void Client::pp_header()
    {
      if (    static_cast<Log::Severity>(opts_.severity)      < Log::Severity::INFO
           && static_cast<Log::Severity>(opts_.file_severity) < Log::Severity::INFO)
        return;

      if (    static_cast<Log::Severity>(opts_.severity)      >= Log::Severity::DEBUG
           || static_cast<Log::Severity>(opts_.file_severity) >= Log::Severity::DEBUG) 
      {
        string s(buffer_.begin(), buffer_.end());
        BOOST_LOG_SEV(lg_, Log::DEBUG) << "Header: |" << s << "|";
      }

      try {
        header_decoder_.read(buffer_.begin(), buffer_.end());
        header_decoder_.verify_finished();
      } catch (const std::runtime_error &e) {
        BOOST_LOG_SEV(lg_, Log::ERROR) << e.what();
      }
      for (auto &i : fields_) {
        BOOST_LOG(lg_) << setw(10) << left << i.first << ' ' << i.second;
      }
      header_decoder_.clear();
      fields_.clear();
    }
    void Client::imap_body_section_inner()
    {
      if (state_ == State::FETCHING) {
        if (full_body_) {
          string filename;
          maildir_.create_tmp_name(filename);
          Buffer::File f(tmp_dir_, filename);
          file_buffer_ = std::move(f);
          buffer_proxy_.set(&file_buffer_);
        }
      }
    }
    void Client::imap_body_section_end()
    {
      BOOST_LOG_FUNCTION();
      if (state_ == State::FETCHING) {
        if (full_body_) {
          buffer_proxy_.set(&buffer_);
          file_buffer_.close();
          if (flags_.empty()) {
            maildir_.move_to_new();
          } else  {
            BOOST_LOG_SEV(lg_, Log::DEBUG) << "Using maildir flags: " << flags_;
            maildir_.move_to_cur(flags_);
          }
          full_body_ = false;
          ++fetched_messages_;
        } else {
          pp_header();
        }
      }
    }
    void Client::imap_flag(Flag flag)
    {
      switch (flag) {
        case IMAP::Flag::ANSWERED:
          flags_.push_back('R');
          break;
        case IMAP::Flag::SEEN:
          flags_.push_back('S');
          break;
        case IMAP::Flag::FLAGGED:
          flags_.push_back('F');
          break;
        case IMAP::Flag::DRAFT:
          flags_.push_back('D');
          break;
        case IMAP::Flag::RECENT:
        case IMAP::Flag::DELETED:
        case IMAP::Flag::FIRST_:
        case IMAP::Flag::LAST_:
          break;
      }
    }
    void Client::imap_uid(uint32_t number)
    {
      BOOST_LOG_FUNCTION();
      if (state_ == State::FETCHING) {
        BOOST_LOG_SEV(lg_, Log::DEBUG) << "UID: " << number;
        last_uid_ = number;
      }
    }

  }
}
