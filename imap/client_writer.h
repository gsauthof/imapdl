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
#ifndef IMAP_CLIENT_WRITER_H
#define IMAP_CLIENT_WRITER_H

#include <functional>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <set>
#include <ostream>
#include <inttypes.h>

#include <boost/interprocess/streams/vectorstream.hpp>


#include "imap.h"
#include "server_lexer.h"

namespace IMAP {

  namespace Client {

    class Tag {
      private:
        std::string        prefix_    ;
        unsigned           width_  {3};
        size_t             value_  {0};
        std::ostringstream buffer_    ;

        std::map<std::string, IMAP::Client::Command> map_;
        std::set<IMAP::Client::Command>              command_set_;
      public:
        Tag(const std::string &prefix = "A", unsigned width = 3);

        // also store in map
        void next(std::string &tag, Command command);
        // pop tag from map
        void pop(const std::string &tag);
    };

    class Writer {
      public:
        // may be swapped or moved! Thus non-const ...
        using Write_Fn = std::function<void(std::vector<char> &buffer)>;
      private:
        Memory::Buffer::Proxy        buffer_;
        Memory::Buffer::Proxy        tag_buffer_;
        IMAP::Server::Callback::Null null_cb_;
        IMAP::Server::Parser         parser_;

        Tag      &generate_;
        Write_Fn  write_fn_;

        std::vector<char> v_;
        using VectorStream =
          boost::interprocess::basic_vectorstream<std::vector<char> >;
        VectorStream stream_;

        void write(std::vector<char> &v);
        void command_start(Command c, std::string &tag);
        void command_finish();
        void nullary(Command c, std::string &tag);
        void write_sequence_nr(uint32_t nz);
        void write_sequence(const std::pair<uint32_t, uint32_t> &seq);
        void write_sequence_set(
            const std::vector<std::pair<uint32_t, uint32_t> > &sequence_set);
        void write_flags(const std::vector<IMAP::Flag> &flags);
      public:
        Writer(Tag &tag, Write_Fn write_fn = nullptr);

        void capability(std::string &tag);
        void noop      (std::string &tag);
        void logout    (std::string &tag);

        void login(const std::string &user, const std::string &password,
            std::string &tag);

        void select (const std::string &mailbox, std::string &tag);
        void examine(const std::string &mailbox, std::string &tag);

        void close  (std::string &tag);
        void expunge(std::string &tag);
        void uid_expunge(
            const std::vector<std::pair<uint32_t, uint32_t> > &sequence_set,
            std::string &tag);
        void uid_store(
            const std::vector<std::pair<uint32_t, uint32_t> > &sequence_set,
            const std::vector<IMAP::Flag> &flags,
            std::string &tag,
            Store_Mode mode = Store_Mode::REPLACE,
            bool silent = false
            );
        void fetch(
            const std::vector<std::pair<uint32_t, uint32_t> > &sequence_set,
            const std::vector<Fetch_Attribute> &as, std::string &tag
            );

    };

  }

}

#endif
