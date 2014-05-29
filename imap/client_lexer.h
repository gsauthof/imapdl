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
#ifndef IMAP_CLIENT_LEXER_H
#define IMAP_CLIENT_LEXER_H

#include <vector>
#include <cstdint>

using namespace std;

#include "imap.h"
#include <buffer/buffer.h>

namespace IMAP {

  namespace Client {

    namespace Callback {

      using namespace IMAP::Server::Response;

      class Base {
        private:
        public:
          //virtual void imap_continuation_request_begin() = 0;
          //virtual void imap_continuation_request_end() = 0;
          //virtual void imap_response_begin(Group g, Kind k) = 0;

          // may consult tag_buffer
          virtual void imap_tagged_status_begin() = 0;
          // may consult buffer
          virtual void imap_tagged_status_end(Status c) = 0;
          virtual void imap_untagged_status_begin(Status c) = 0;
          // may consult buffer
          virtual void imap_untagged_status_end(Status c) = 0;

          //virtual void imap_data_begin(Kind k, Status c) = 0;

          virtual void imap_data_exists(uint32_t number) = 0;
          virtual void imap_data_recent(uint32_t number) = 0;
          virtual void imap_data_fetch_begin(uint32_t number) = 0;
          virtual void imap_data_fetch_end() = 0;
          virtual void imap_data_expunge(uint32_t number) = 0;

          virtual void imap_data_flags_begin() = 0;
          virtual void imap_data_flags_end() = 0;
          virtual void imap_flag(Flag flag) = 0;
          // may consult buffer
          virtual void imap_atom_flag() = 0;
          virtual void imap_uid(uint32_t number) = 0;
          virtual void imap_status_code(Status_Code) = 0;
          virtual void imap_status_code_uidnext(uint32_t n) = 0;
          virtual void imap_status_code_uidvalidity(uint32_t n) = 0;
          virtual void imap_status_code_unseen(uint32_t n) = 0;

          virtual void imap_status_code_capability_begin() = 0;
          virtual void imap_status_code_capability_end() = 0;
          virtual void imap_capability_begin() = 0;
          virtual void imap_capability(Capability capability) = 0;
          virtual void imap_capability_end() = 0;

          virtual void imap_body_section_begin() = 0;
          virtual void imap_body_section_inner() = 0;
          virtual void imap_body_section_end() = 0;
          virtual void imap_section_empty() = 0;
          virtual void imap_section_header() = 0;
      };

      class Null : public Base {
        private:
        public:
          void imap_tagged_status_begin() override;
          void imap_tagged_status_end(Status c) override;
          void imap_untagged_status_begin(Status c) override;
          void imap_untagged_status_end(Status c) override;
          void imap_data_exists(uint32_t number) override;
          void imap_data_recent(uint32_t number) override;
          void imap_data_fetch_begin(uint32_t number) override;
          void imap_data_fetch_end() override;
          void imap_data_expunge(uint32_t number) override;
          void imap_data_flags_begin() override;
          void imap_data_flags_end() override;
          void imap_flag(Flag flag) override;
          void imap_atom_flag() override;
          void imap_uid(uint32_t number) override;
          void imap_status_code(Status_Code) override;
          void imap_status_code_uidnext(uint32_t n) override;
          void imap_status_code_uidvalidity(uint32_t n) override;
          void imap_status_code_unseen(uint32_t n) override;

          void imap_status_code_capability_begin() override;
          void imap_status_code_capability_end() override;
          void imap_capability_begin() override;
          void imap_capability(Capability capability) override;
          void imap_capability_end() override;

          void imap_body_section_begin() override;
          void imap_body_section_inner() override;
          void imap_body_section_end() override;
          void imap_section_empty() override;
          void imap_section_header() override;
      };
    }

    class Lexer {
      private:
        int                      cs             {0};
        vector<int>              stack_vector_;
        int                     *stack          {nullptr};
        int                      top            {0};
        Memory::Buffer::Vector   number_buffer_;
        uint32_t                 number_        {0};
        size_t                   literal_pos_   {0};
        bool                     has_imap4rev1_ {false};

        Memory::Buffer::Base    &buffer_;
        bool                     convert_crlf_  {true};
        Memory::Buffer::Base    &tag_buffer_;
        Callback::Base          &cb_;
        Server::Response::Status status_        {Server::Response::Status::OK};
      public:
        Lexer(Memory::Buffer::Base &buffer,
            Memory::Buffer::Base &tag_buffer,
            Callback::Base &cb);
        void read(const char *begin, const char *end);
        bool in_start() const;
        bool finished() const;
        void verify_finished() const;
        void set_convert_crlf(bool b);

    };

  }
}

#endif
