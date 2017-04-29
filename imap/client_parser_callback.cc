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
#include "client_parser.h"

namespace IMAP {
  namespace Client {
    namespace Callback {

      using namespace IMAP::Server::Response;

      Base::~Base() =default;

      void Null::imap_tagged_status_begin()
      {
      }
      void Null::imap_tagged_status_end(Status c)
      {
      }
      void Null::imap_untagged_status_begin(Status c)
      {
      }
      void Null::imap_untagged_status_end(Status c)
      {
      }
      void Null::imap_data_exists(uint32_t number)
      {
      }
      void Null::imap_data_recent(uint32_t number)
      {
      }
      void Null::imap_data_fetch_begin(uint32_t number)
      {
      }
      void Null::imap_data_fetch_end()
      {
      }
      void Null::imap_data_expunge(uint32_t number)
      {
      }
      void Null::imap_data_flags_begin()
      {
      }
      void Null::imap_data_flags_end()
      {
      }
      void Null::imap_flag(Flag flag)
      {
      }
      void Null::imap_atom_flag()
      {
      }
      void Null::imap_uid(uint32_t number)
      {
      }
      void Null::imap_status_code(Status_Code)
      {
      }
      void Null::imap_status_code_uidnext(uint32_t n)
      {
      }
      void Null::imap_status_code_uidvalidity(uint32_t n)
      {
      }
      void Null::imap_status_code_unseen(uint32_t n)
      {
      }
      void Null::imap_status_code_capability_begin()
      {
      }
      void Null::imap_status_code_capability_end()
      {
      }
      void Null::imap_capability_begin()
      {
      }
      void Null::imap_capability(Capability capability)
      {
      }
      void Null::imap_capability_end()
      {
      }
      void Null::imap_body_section_begin()
      {
      }
      void Null::imap_body_section_inner()
      {
      }
      void Null::imap_body_section_end()
      {
      }
      void Null::imap_section_empty()
      {
      }
      void Null::imap_section_header()
      {
      }

      void Null::imap_list_begin()
      {
      }
      void Null::imap_list_end()
      {
      }
      void Null::imap_list_sflag(SFlag flag)
      {
      }
      void Null::imap_list_oflag(OFlag oflag)
      {
      }
      void Null::imap_quoted_char(char c)
      {
      }
      void Null::imap_list_mailbox()
      {
      }


    }

  }
}
