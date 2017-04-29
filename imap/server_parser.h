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
#ifndef IMAP_SERVER_PARSER_H
#define IMAP_SERVER_PARSER_H

#include <vector>
#include <cstdint>

using namespace std;

#include "imap.h"
#include <buffer/buffer.h>

namespace IMAP {

  namespace Server {

    namespace Callback {

      using namespace IMAP::Server::Response;
      class Base {
        private:
        public:
          virtual ~Base();
          virtual bool imapd_login(const Memory::Buffer::Base &userid,
              const Memory::Buffer::Base &password) = 0;
      };

      class Null : public Base {
        private:
        public:
          bool imapd_login(const Memory::Buffer::Base &userid,
              const Memory::Buffer::Base &password) override;
      };
    }

    class Parser {
      private:
        int                      cs             {0};
        const char              *eof            {nullptr};
        vector<int>              stack_vector_;
        int                     *stack          {nullptr};
        int                      top            {0};
        Memory::Buffer::Vector   number_buffer_;
        uint32_t                 number_        {0};
        size_t                   literal_pos_   {0};
        bool                     convert_crlf_  {false};
        Memory::Buffer::Vector   userid_buffer_;
        IMAP::Connection::State  state_
          {IMAP::Connection::State::NOT_AUTHENTICATED};
        bool                     read_only_     {false};

        Memory::Buffer::Base    &buffer_;
        Memory::Buffer::Base    &tag_buffer_;
        Callback::Base          &cb_;
      public:
        Parser(Memory::Buffer::Base &buffer,
            Memory::Buffer::Base &tag_buffer,
            Callback::Base &cb);
        void read(const char *begin, const char *end);
        bool in_start() const;
        bool finished() const;
        void verify_finished() const;

    };

  }
}

#endif
