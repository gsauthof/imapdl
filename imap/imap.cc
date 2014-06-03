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
#include "imap.h"

#include <stdexcept>
using namespace std;

#include "enum.h"


namespace IMAP {

  static const char * const flag_map[] = {
    "ANSWERED",
    "FLAGGED",
    "DELETED",
    "SEEN",
    "DRAFT",
    "RECENT"
  };
  std::ostream &operator<<(std::ostream &o, Flag flag)
  {
    o << enum_str(flag_map, flag);
    return o;
  }

  static const char * const section_map[] = {
    "HEADER",
    "HEADER.FIELDS",
    "HEADER.FIELDS.NOT",
    "TEXT"
  };
  std::ostream &operator<<(std::ostream &o, Section section)
  {
    o << enum_str(section_map, section);
    return o;
  }
  // Nullary
  Section_Attribute::Section_Attribute()
  {
  }
  Section_Attribute::Section_Attribute(Section section)
    : section_(section)
  {
    if (   section_ == Section::HEADER_FIELDS
        || section_ == Section::HEADER_FIELDS_NOT)
      throw logic_error("HEADER_FIELDS has empty field list");
  }
  Section_Attribute::Section_Attribute(Section section, const std::vector<string> &headers)
    :
      section_(section),
      headers_(headers)
  {
    if (!(   section_ == Section::HEADER_FIELDS
          || section_ == Section::HEADER_FIELDS_NOT))
      throw logic_error("field list only allowed with HEADER_FIELDS");
    if (headers_.empty())
      throw logic_error("empty field list not allowed");
  }
  Section_Attribute::Section_Attribute(Section section, std::vector<string> &&headers)
    :
      section_(section),
      headers_(std::move(headers))
  {
    if (!(   section_ == Section::HEADER_FIELDS
          || section_ == Section::HEADER_FIELDS_NOT))
      throw logic_error("field list only allowed with HEADER_FIELDS");
    if (headers_.empty())
      throw logic_error("empty field list not allowed");
  }
  std::ostream &Section_Attribute::print(ostream &o) const
  {
    if (section_ == Section::FIRST_)
      return o;
    o << section_;
    if (!headers_.empty()) {
      o << " (";
      auto i = headers_.begin();
      o << *i;
      ++i;
      for (;i != headers_.end(); ++i) {
        o << ' ' << *i;
      }
      o << ')';
    }
    return o;
  }
  std::ostream &operator<<(std::ostream &o, const Section_Attribute &a)
  {
    return a.print(o);
  }

  namespace Client {
    static const char * const command_map[] = {
      // Any State
      "CAPABILITY",
      "NOOP",
      "LOGOUT",
      // Not-Authenticated
      "STARTTLS",
      "AUTHENTICATE",
      "LOGIN",
      // Authenticated
      "SELECT",
      "EXAMINE",
      "CREATE",
      "DELETE",
      "RENAME",
      "SUBSCRIBE",
      "UNSUBSCRIBE",
      "LIST",
      "LSUB",
      "STATUS",
      "APPEND",
      // Selected
      "CHECK",
      "CLOSE",
      "EXPUNGE",
      "SEARCH",
      "FETCH",
      "STORE",
      "COPY",
      "UID EXPUNGE",
      "UID COPY",
      "UID FETCH",
      "UID SEARCH",
      "UID STORE"
    };
    const char *command_str(Command c)
    {
      return enum_str(command_map, c);
    }
    std::ostream &operator<<(std::ostream &o, Command c)
    {
      o << command_str(c);
      return o;
    }
    static const char * const fetch_map[] = {
      "ENVELOPE",
      "FLAGS",
      "INTERNALDATE",
      "RGC822",
      "RFC822.HEADER",
      "RFC822.SIZE",
      "RFC822.TEXT",
      "BODY", // BODYSTRUCTURE_NON_EXTENSIBLE
      "BODYSTRUCTURE",
      "UID",
      "BODY",
      "BODY.PEEK",
    };
    std::ostream &operator<<(std::ostream &o, Fetch fetch)
    {
      o << enum_str(fetch_map, fetch);
      return o;
    }
    Fetch_Attribute::Fetch_Attribute(Fetch fetch)
      : fetch_(fetch)
    {
    }
    Fetch_Attribute::Fetch_Attribute(Fetch fetch,
        const Section_Attribute &section)
      :
        fetch_(fetch),
        section_(section)
    {
      if (!(fetch_ == Fetch::BODY || fetch_ == Fetch::BODY_PEEK))
        throw logic_error("sections only allowed with BODY/BODY_PEEK attributes");
    }
    Fetch_Attribute::Fetch_Attribute(Fetch fetch,
        Section_Attribute &&section)
      :
        fetch_(fetch),
        section_(std::move(section))
    {
      if (!(fetch_ == Fetch::BODY || fetch_ == Fetch::BODY_PEEK))
        throw logic_error("sections only allowed with BODY/BODY_PEEK attributes");
    }
    std::ostream &Fetch_Attribute::print(std::ostream &o) const
    {
      o << fetch_;
      if (fetch_ == Fetch::BODY || fetch_ == Fetch::BODY_PEEK) {
        o << '[';
        o << section_;
        o << ']';
      }
      return o;
    }
    std::ostream &operator<<(std::ostream &o, const Fetch_Attribute &a)
    {
      return a.print(o);
    }
    static const char * const store_mode_map[] = {
      "",
      "+",
      "-"
    };
    std::ostream &operator<<(std::ostream &o, Store_Mode mode)
    {
      o << enum_str(store_mode_map, mode);
      return o;
    }
  }

  namespace Server {
    namespace Response {


      static const char * const status_map[] = {
        "OK",
        "NO",
        "BAD",
        "PREAUTH",
        "BYE",
      };

      std::ostream &operator<<(std::ostream &o, Status status)
      {
        o << enum_str(status_map, status);
        return o;
      }


      static const char * const status_code_map[] = {
        "ALERT",
        "BADCHARSET",
        "CAPABILITY",
        "PARSE",
        "PERMANENTFLAGS",
        "READ_ONLY",
        "READ_WRITE",
        "TRYCREATE",
        "UIDNEXT",
        "UIDVALIDITY",
        "UNSEEN",
      };
      std::ostream &operator<<(std::ostream &o, Status_Code status_code)
      {
        o << enum_str(status_code_map, status_code);
        return o;
      }
      static const char * const capability_map[] = {
        "ACL",
        "ANNOTATE-EXPERIMENT_1",
        "AUTH=",
        "AUTH=PLAIN",
        "BINARY",
        "CATENATE",
        "CHILDREN",
        "COMPRESS=DEFLATE",
        "CONDSTORE",
        "CONTEXT=SEARCH",
        "CONTEXT=SORT",
        "CONVERT",
        "CREATE-SPECIAL_USE",
        "ENABLE",
        "ESEARCH",
        "ESORT",
        "FILTERS",
        "I18NLEVEL=1",
        "I18NLEVEL=2",
        "ID",
        "IDLE",
        "IMAP4rev1",
        "IMAPSIEVE=",
        "LANGUAGE",
        "LIST-STATUS",
        "LITERAL+",
        "LOGIN-REFERRALS",
        "LOGINDISABLED",
        "MAILBOX-REFERRALS",
        "MOVE",
        "MULTIAPPEND",
        "MULTISEARCH",
        "NAMESPACE",
        "NOTIFY",
        "QRESYNC",
        "QUOTA",
        "RIGHTS=",
        "SASL-IR",
        "SEARCH=FUZZY",
        "SEARCHRES",
        "SORT",
        "SORT=DISPLAY",
        "SPECIAL-USE",
        "STARTTLS",
        "THREAD",
        "UIDPLUS",
        "UNSELECT",
        "URL-PARTIAL",
        "URLAUTH",
        "URLFETCH=BINARY",
        "UTF8=ACCEPT",
        "UTF8=ONLY",
        "WITHIN"
      };
      std::ostream &operator<<(std::ostream &o, Capability capability)
      {
        o << enum_str(capability_map, capability);
        return o;
      }
      static const char * const sflag_map[] = {
        "NOSELECT",
        "MARKED",
        "UNMARKED"
      };
      std::ostream &operator<<(std::ostream &o, SFlag sflag)
      {
        o << enum_str(sflag_map, sflag);
        return o;
      }
      static const char * const oflag_map[] = {
        "NOINFERIORS",
        "HASCHILDREN",
        "HASNOCHILDREN"
      };
      std::ostream &operator<<(std::ostream &o, OFlag oflag)
      {
        o << enum_str(oflag_map, oflag);
        return o;
      }

    }
  }
}
