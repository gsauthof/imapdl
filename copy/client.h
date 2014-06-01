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
#include <imap/client_parser.h>
#include <imap/client_writer.h>
#include <log/log.h>
#include <maildir/maildir.h>
#include <buffer/buffer.h>
#include <buffer/file.h>
#include <sequence_set.h>
#include <mime/header_decoder.h>

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
    class IMAP_Client : public IMAP::Client::Callback::Null {
      public:
        using Write_Fn = std::function<void(std::vector<char> &v)>;
      private:
        boost::log::sources::severity_logger< Log::Severity > &lg_;
        Write_Fn write_fn_;

        IMAP::Client::Tag    tags_;
        std::vector<char>         cmd_;
        IMAP::Client::Writer writer_;
        std::map<std::string, std::function<void(void)> > tag_to_fn_;

        void to_cmd(vector<char> &x);
        void do_write();

      protected:
        Memory::Buffer::Vector tag_buffer_;
        Memory::Buffer::Vector buffer_;
        // generic imap client functions
        void async_capabilities(std::function<void(void)> fn);
        void async_login(const std::string &username, const std::string &password,
            std::function<void(void)> fn);
        void async_select(const std::string &mailbox, std::function<void(void)> fn);
        void async_fetch(
            const std::vector<std::pair<uint32_t, uint32_t> > &set,
            const std::vector<IMAP::Client::Fetch_Attribute> &atts,
            std::function<void(void)> fn);
        void async_store(
            const std::vector<std::pair<uint32_t, uint32_t> > &set,
            const std::vector<IMAP::Flag> &flags,
            std::function<void(void)> fn);
        void async_uid_expunge(const std::vector<std::pair<uint32_t, uint32_t> > &set,
            std::function<void(void)> fn);
        void async_expunge(std::function<void(void)> fn);
        void async_logout(std::function<void(void)> fn);
      public:
        IMAP_Client(Write_Fn write_fn,
            boost::log::sources::severity_logger< Log::Severity > &lg);
        void imap_tagged_status_end(IMAP::Server::Response::Status c) override;
    };
    class Net_Client_App {
      private:
       const std::string                                      &host_;
        Net::Client::Base                                     &client_;
        boost::log::sources::severity_logger< Log::Severity > &lg_;

        void async_resolve(std::function<void(void)> fn);
        void async_connect(boost::asio::ip::tcp::resolver::iterator iterator,
            std::function<void(void)> fn);
        void async_handshake(std::function<void(void)> fn);

        void async_quit(std::function<void(void)> fn);
        void async_shutdown(std::function<void(void)> fn);
      public:
        Net_Client_App(
            const std::string &host,
            Net::Client::Base &client,
            boost::log::sources::severity_logger< Log::Severity > &lg
            );
        void async_start(std::function<void(void)> fn);
        void async_finish(std::function<void(void)> fn);
    };
    class Fetch_Timer {
      private:
        Net::Client::Base                                     &client_;
        boost::log::sources::severity_logger< Log::Severity > &lg_;
        std::chrono::time_point<std::chrono::steady_clock>           start_;
        boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
        size_t bytes_start_ {0};
        size_t messages_  {0};
      public:
        Fetch_Timer(
            Net::Client::Base &client,
            boost::log::sources::severity_logger< Log::Severity > &lg
            );
        void start();
        void resume();
        void stop();
        void print();
        void increase_messages();
        size_t messages() const;
    };
    class Header_Printer {
      private:
        boost::log::sources::severity_logger< Log::Severity > &lg_;
        const Options                                         &opts_;

        const Memory::Buffer::Vector &buffer_;

        MIME::Header::Decoder header_decoder_;
        Memory::Buffer::Vector field_name_;
        Memory::Buffer::Vector field_body_;
        std::map<std::string, std::string> fields_;
      public:
        Header_Printer(
            const IMAP::Copy::Options &opts,
            const Memory::Buffer::Vector &buffer,
            boost::log::sources::severity_logger< Log::Severity > &lg
            );
        void print();
    };
    class Client : public IMAP_Client {
      private:
        boost::log::sources::severity_logger< Log::Severity > &lg_;
        const Options                                         &opts_;
        Net::Client::Base                                     &client_;
        Net_Client_App app_;
        boost::asio::signal_set                                signals_;
        unsigned                                               signaled_ {0};

        Memory::Buffer::Proxy  buffer_proxy_;
        Memory::Buffer::File   file_buffer_;
        IMAP::Client::Parser   parser_;

        std::unordered_set<IMAP::Server::Response::Capability>       capabilities_;
        boost::asio::basic_waitable_timer<std::chrono::steady_clock> login_timer_;

        Task                         task_         {Task::DOWNLOAD};
        State                        state_        {State::DISCONNECTED };

        unsigned exists_        {0};
        unsigned recent_        {0};
        unsigned uidvalidity_   {0};

        uint32_t     last_uid_  {0};
        Sequence_Set uids_;

        Maildir      maildir_;
        Memory::Dir  tmp_dir_;
        bool         full_body_ {false};
        std::string  flags_;

        std::string mailbox_;

        Fetch_Timer fetch_timer_;

        Header_Printer header_printer_;

        void read_journal();
        void write_journal();

        void do_signal_wait();

        bool has_uidplus() const;

        void do_read();
        void write_command(vector<char> &cmd);

        void do_pre_login();
        void do_quit();

        // specialized dl client functions
        void async_login_capabilities(std::function<void(void)> fn);
        void cond_async_capabilities(std::function<void(void)> fn);
        void async_login(std::function<void(void)> fn);
        void async_select(std::function<void(void)> fn);
        void async_fetch_or_logout(std::function<void(void)> after_fetch,
            std::function<void(void)> after_logout);
        void async_fetch(std::function<void(void)> fn);
        void async_store_or_logout(std::function<void(void)> after_store,
            std::function<void(void)> after_logout);
        void async_store(std::function<void(void)> fn);
        void async_uid_or_simple_expunge(std::function<void(void)> fn);
        void async_uid_expunge(std::function<void(void)> fn);
        void async_cleanup(std::function<void(void)> fn);
        void do_cleanup();
        void do_download();
        void do_task();
      public:
        Client(IMAP::Copy::Options &opts,
            Net::Client::Base &net_client,
            boost::log::sources::severity_logger< Log::Severity > &lg);
        ~Client();

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
