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
#ifndef COPY_CLIENT_H
#define COPY_CLIENT_H

#include <boost/asio/signal_set.hpp>

#include <copy/state.h>
#include <copy/fetch_timer.h>
#include <copy/header_printer.h>

#include <net/tcp_client.h>
#include <net/client_application.h>
#include <imap/client_parser.h>
#include <imap/client_writer.h>
#include <imap/client_base.h>
#include <log/log.h>
#include <maildir/maildir.h>
#include <buffer/buffer.h>
#include <buffer/file.h>
#include <sequence_set.h>

#include <string>
#include <unordered_set>
#include <chrono>
#include <map>
#include <functional>

#include <boost/asio/steady_timer.hpp>

namespace IMAP {
  namespace Copy {
    class Options;
    class Client : public IMAP::Client::Base {
      private:
        boost::log::sources::severity_logger<Log::Severity> &lg_;
        const Options          &opts_;
        Net::Client::Base      &client_;
        Net::Client::Application app_;
        boost::asio::signal_set signals_;
        unsigned                signaled_ {0};
        boost::asio::basic_waitable_timer<std::chrono::steady_clock> login_timer_;

        Memory::Buffer::Proxy   buffer_proxy_;
        Memory::Buffer::File    file_buffer_;
        IMAP::Client::Parser    parser_;

        bool          need_cleanup_ {false};
        State         state_       {State::DISCONNECTED };

        unsigned      exists_      {0};
        unsigned      recent_      {0};
        unsigned      uidvalidity_ {0};
        uint32_t      last_uid_    {0};
        Sequence_Set  uids_;
        std::unordered_set<IMAP::Server::Response::Capability> capabilities_;
        Maildir       maildir_;
        Memory::Dir   tmp_dir_;
        bool          full_body_   {false};
        std::string   flags_;
        std::string   mailbox_;

        Fetch_Timer    fetch_timer_;
        Header_Printer header_printer_;

        void read_journal();
        void write_journal();

        void do_signal_wait();

        void do_read();
        void write_command(vector<char> &cmd);

        bool has_uidplus() const;

        // specialized download client functions
        void do_pre_login();
        void do_post_login();
        void async_login_capabilities(std::function<void(void)> fn);
        void cond_async_capabilities(std::function<void(void)> fn);
        void async_login(std::function<void(void)> fn);
        void async_select(std::function<void(void)> fn);
        void async_fetch_or_logout(std::function<void(void)> after_fetch,
            std::function<void(void)> after_logout);
        void async_fetch_header(std::function<void(void)> fn);
        void async_fetch(std::function<void(void)> fn);
        void async_store_or_logout(std::function<void(void)> after_store,
            std::function<void(void)> after_logout);
        void async_store(std::function<void(void)> fn);
        void async_uid_or_simple_expunge(std::function<void(void)> fn);
        void async_uid_expunge(std::function<void(void)> fn);
        void async_cleanup(std::function<void(void)> fn);
        void do_fetch_header();
        void do_download();
        void do_task();
        void do_quit();
      public:
        Client(IMAP::Copy::Options &opts,
            Net::Client::Base &net_client,
            boost::log::sources::severity_logger< Log::Severity > &lg);
        ~Client();

      protected:
        void imap_status_code_capability_begin() override;
        void imap_capability_begin() override;
        void imap_capability(IMAP::Server::Response::Capability capability) override;
        void imap_status_code_capability_end() override;
        void imap_data_exists(uint32_t number) override;
        void imap_data_recent(uint32_t number) override;
        void imap_status_code_uidvalidity(uint32_t n) override;

        void imap_data_fetch_begin(uint32_t number) override;
        void imap_data_fetch_end() override;
        void imap_section_empty() override;
        void imap_body_section_inner() override;
        void imap_body_section_end() override;
        void imap_flag(Flag flag) override;
        void imap_uid(uint32_t number) override;


    };
  }
}

#endif
