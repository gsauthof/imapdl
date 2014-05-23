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

#include <net/tcp_client.h>
#include <imap/client_lexer.h>
#include <imap/client_writer.h>
#include <log/log.h>
#include <maildir/maildir.h>
#include <buffer/buffer.h>
#include <buffer/file.h>
#include "sequence_set.h"

#include <memory>
#include <unordered_set>
#include <chrono>
#include <map>
#include <ostream>

#include <boost/asio/steady_timer.hpp>

namespace IMAP {
  namespace Copy {
    enum class Task {
      FIRST_,
      CLEANUP,
      DOWNLOAD,
      LAST_
    };
    enum class State {
      FIRST_,
      DISCONNECTED,
      ESTABLISHED,
      GOT_INITIAL_CAPABILITIES,
      LOGGED_IN,
      GOT_CAPABILITIES,
      SELECTED_MAILBOX,
      FETCHING,
      FETCHED,
      STORED,
      EXPUNGED,
      LOGGING_OUT,
      LOGGED_OUT,
      END,
      LAST_
    };
    State operator++(State &s);
    State operator+(State s, int b);
    std::ostream &operator<<(std::ostream &o, State s);
    class Options;
    class Client : public IMAP::Client::Callback::Null {
      private:
        const Options                                         &opts_;
        std::unique_ptr<Net::Client::Base>                     client_;
        boost::log::sources::severity_logger< Log::Severity > &lg_;
        boost::asio::signal_set                                signals_;
        unsigned                                               signaled_ {0};

        Memory::Buffer::Proxy  buffer_proxy_;
        Memory::Buffer::Vector tag_buffer_;
        Memory::Buffer::Vector buffer_;
        Memory::Buffer::File   file_buffer_;
        IMAP::Client::Lexer    lexer_;

        std::unordered_set<IMAP::Server::Response::Capability>       capabilities_;
        boost::asio::basic_waitable_timer<std::chrono::steady_clock> login_timer_;

        IMAP::Client::Tag    tags_;
        vector<char>         cmd_;
        IMAP::Client::Writer writer_;

        Task                         task_         {Task::DOWNLOAD};
        State                        state_        {State::DISCONNECTED };
        std::map<std::string, State> tag_to_state_;

        unsigned exists_        {0};
        unsigned recent_        {0};
        unsigned uidvalidity_   {0};

        uint32_t     last_uid_  {0};
        Sequence_Set uids_;

        Maildir      maildir_;
        Memory::Dir  tmp_dir_;
        bool         full_body_ {false};
        std::string  flags_;

        std::chrono::time_point<std::chrono::steady_clock>           fetch_start_;
        size_t fetch_bytes_start_ {0};
        size_t fetched_messages_  {0};

        boost::asio::basic_waitable_timer<std::chrono::steady_clock> fetch_timer_;

        std::string mailbox_;

        void read_journal();
        void write_journal();

        void to_cmd(vector<char> &x);

        void do_signal_wait();

        void print_fetch_stats();
        void start_fetch_timer();
        void resume_fetch_timer();
        void stop_fetch_timer();

        void command();
        void cleanup_command();
        void download_command();

        bool has_uidplus() const;

        void do_resolve();
        void do_connect(boost::asio::ip::tcp::resolver::iterator iterator);
        void do_handshake();
        void do_read();
        void do_write();
        void do_quit();
        void do_shutdown();

        void do_pre_login();
        void do_capabilities();
        void do_login();
        void do_select();
        void do_fetch_or_logout();
        void do_fetch();
        void do_store_or_logout();
        void do_store();
        void do_uid_or_simple_expunge();
        void do_uid_expunge();
        void do_expunge();
        void do_logout();
      public:
        Client(IMAP::Copy::Options &opts,
            std::unique_ptr<Net::Client::Base> &&net_client,
            boost::log::sources::severity_logger< Log::Severity > &lg);
        ~Client();

        void imap_status_code_capability_begin() override;
        void imap_capability_begin() override;
        void imap_capability(IMAP::Server::Response::Capability capability) override;
        void imap_tagged_status_end(IMAP::Server::Response::Status c) override;
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
