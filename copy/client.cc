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

#include "state.h"
#include "options.h"
#include <exception.h>

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/system/error_code.hpp>

#include <sstream>
#include <stdexcept>
#include <string>
#include <functional>
using namespace std;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include "journal.h"

#include <boost/asio/yield.hpp>

namespace IMAP {
  namespace Copy {

    Client::Client(IMAP::Copy::Options &opts,
        Net::Client::Base &net_client,
        boost::log::sources::severity_logger<Log::Severity> &lg)
      :
        IMAP::Client::Base(std::bind(&Client::write_command, this, std::placeholders::_1), lg),
        lg_(lg),
        opts_(opts),
        client_(net_client),
        app_(opts_.host, client_, lg_),
        signals_(client_.io_service(), SIGINT, SIGTERM),
        login_timer_(client_.io_service()),
        parser_(buffer_proxy_, tag_buffer_, *this),
        maildir_(opts_.maildir),
        tmp_dir_(maildir_.tmp_dir_fd()),
        mailbox_(opts_.mailbox),
        fetch_timer_(client_, lg_),
        header_printer_(opts_, buffer_, lg_)
    {
      BOOST_LOG_FUNCTION();
      buffer_proxy_.set(&buffer_);
      read_journal();
      do_signal_wait();
      app_.async_start([this](){
            //state_ = State::ESTABLISHED;
            do_read();
            do_pre_login();
          });
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
        need_cleanup_ = true;
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

    // Boost ASIO stackless coroutine in combination
    // with classical std::bind()s to supply the completion handler
    void Client::do_download()
    {
      BOOST_LOG_FUNCTION();
      reenter (download_coroutine_) {
        yield async_select(bind(&Client::do_download, this));
        if (exists_) {
          BOOST_LOG(lg_) << "Fetching into " << opts_.maildir << " ...";
          fetch_timer_.start();
          yield async_fetch(bind(&Client::do_download, this));
          fetch_timer_.stop();
          if (opts_.del) {
            yield async_store(bind(&Client::do_download, this));
            yield async_uid_or_simple_expunge(bind(&Client::do_download, this));
          }
        } else {
          BOOST_LOG_SEV(lg_, Log::MSG) << "Mailbox " << opts_.mailbox
            << " is empty.";
        }
        uids_.clear();
        yield async_logout(bind(&Client::do_download, this));
        do_quit();
      }
    }

    // Boost ASIO stackless coroutine and as variation:
    // completion-handler is specified as C++11 lambda
    // (less characters to type than using std::bind() ...)
    void Client::do_fetch_header()
    {
      BOOST_LOG_FUNCTION();
      reenter (fetch_header_coroutine_) {
        yield async_select      ([this](){do_fetch_header();});
        yield async_fetch_header([this](){do_fetch_header();});
        uids_.clear();
        yield async_logout      ([this](){do_fetch_header();});
        do_quit();
      }
    }

    // Another variant: instead of a coroutine use only C++ lambdas
    //
    // Overhead (with gcc 4.8 on Fedora 20 x86-64) of different variants:
    //
    // lambda with captured this pointer                   :  8 byte
    // lambda with captured this pointer and another lambda: 16 byte
    // lambda with captured this pointer and two lambdas   : 24 byte
    // std::bind() of this                                 : 24 byte
    // std::function() of 8 byte lambda                    : 32 byte
    // std::function() of 24 byte lambda                   : 32 byte
    // std::function() of 24 byte std::bind()              : 32 byte
    //
    // coroutine object -> sizeof(int)                     :  4 byte
    void Client::do_list()
    {
      BOOST_LOG_FUNCTION();
      auto finish_fn = [this](){
        do_quit();
      };
      auto logout_fn = [this, finish_fn](){
        uids_.clear();
        async_logout(finish_fn);
      };
      auto list_fn = [this, logout_fn](){
        async_list(logout_fn);
      };
      async_select(list_fn);
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
            cond_async_capabilities([this](){
                async_login_capabilities(std::bind(&Client::do_post_login, this));
              });
          }
        });
    }
    void Client::do_post_login()
    {
      if (need_cleanup_)
        async_cleanup(std::bind(&Client::do_task, this));
      else
        do_task();
    }

    void Client::do_task()
    {
      switch (opts_.task) {
        case Task::DOWNLOAD:
          do_download();
          break;
        case Task::FETCH_HEADER:
          do_fetch_header();
          break;
        case Task::LIST:
          do_list();
          break;
        default:
          ;
      }
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
      IMAP::Client::Base::async_login(opts_.username, opts_.password, fn);
    }

    void Client::async_select(std::function<void(void)> fn)
    {
      IMAP::Client::Base::async_select(mailbox_, fn);
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
      IMAP::Client::Base::async_fetch(set, atts, fn);
    }

    void Client::async_fetch_header(std::function<void(void)> fn)
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

      state_ = State::FETCHING;
      IMAP::Client::Base::async_fetch(set, atts, fn);
    }
    void Client::async_list(std::function<void(void)> fn)
    {
      IMAP::Client::Base::async_list(opts_.list_reference, opts_.list_mailbox, fn);
    }

    void Client::async_store(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      vector<pair<uint32_t, uint32_t> > set;
      uids_.copy(set);
      vector<IMAP::Flag> flags;
      flags.emplace_back(IMAP::Flag::DELETED);
      IMAP::Client::Base::async_store(set, flags, fn);
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

    void Client::async_uid_expunge(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      vector<pair<uint32_t, uint32_t> > set;
      uids_.copy(set);
      IMAP::Client::Base::async_uid_expunge(set, fn);
    }


    void Client::do_read()
    {
      BOOST_LOG_FUNCTION();
      client_.async_read_some([this](
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
              parser_.read(client_.input().data(), client_. input().data() + size);
              if (state_ != State::LOGGED_OUT) // && client_.is_open())
                do_read();
            }
          });
    }

    void Client::write_command(vector<char> &cmd)
    {
      client_.push_write(cmd);
    }

    void Client::do_quit()
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "do_quit()";
      state_ = State::LOGGED_OUT;
      app_.async_finish([this](){
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
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "Got capability: " << capability;
      capabilities_.insert(capability);
    }
    void Client::imap_status_code_capability_end()
    {
      BOOST_LOG_FUNCTION();
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "finished retrieving capabilties";
      login_timer_.cancel();
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
        if (opts_.simulate_error == fetch_timer_.messages() + 1) {
          ostringstream o;
          o << "Simulated error after fetched message: " << fetch_timer_.messages();
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
      if (state_ == State::FETCHING) {
        if (opts_.task == Task::FETCH_HEADER)
          THROW_MSG("server sends body during header only fetch");
        full_body_ = true;
      }
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
          fetch_timer_.increase_messages();
        } else {
          header_printer_.print();
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

    void Client::imap_list_begin()
    {
      oflags_.clear();
    }
    void Client::imap_list_mailbox()
    {
      using namespace IMAP::Server::Response;
      if (buffer_.empty()) {
        BOOST_LOG_SEV(lg_, Log::MSG) << "NIL-Mailbox";
        return;
      }
      string m(buffer_.begin(), buffer_.end());
      char x = ' ';
      if (oflags_.find(OFlag::HASCHILDREN) != oflags_.end())
        x = '+';
      else if (oflags_.find(OFlag::HASNOCHILDREN) != oflags_.end())
          x = '|';
      BOOST_LOG_SEV(lg_, Log::MSG) << "Mailbox: -" << x << ' ' << m;
    }
    void Client::imap_list_oflag(IMAP::Server::Response::OFlag o)
    {
      oflags_.insert(o);
    }

  }
}
