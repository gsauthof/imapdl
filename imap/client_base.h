#ifndef IMAP_CLIENT_H
#define IMAP_CLIENT_H

#include <imap/client_writer.h>
#include <imap/client_parser.h>

#include <log/log.h>
#include <buffer/buffer.h>

#include <string>
#include <map>
#include <functional>

namespace IMAP {

  namespace Client {

    class Base : public IMAP::Client::Callback::Null {
      public:
        using Write_Fn = std::function<void(std::vector<char> &v)>;
      private:
        boost::log::sources::severity_logger< Log::Severity > &lg_;
        Write_Fn write_fn_;

        IMAP::Client::Tag    tags_;
        std::vector<char>    cmd_;
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
        Base(Write_Fn write_fn,
            boost::log::sources::severity_logger< Log::Severity > &lg);
        void imap_tagged_status_end(IMAP::Server::Response::Status c) override;
    };

  }
}

#endif
