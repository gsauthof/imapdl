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
#ifndef IMAP_H
#define IMAP_H

#include <ostream>
#include <vector>
#include <string>

namespace IMAP {

  namespace Connection {
    enum class Security {
      NONE,
      TLS
    };
    enum class State {
      CLOSED,

      NOT_AUTHENTICATED,
      AUTHENTICATED,
      SELECTED,
      LOGGED_OUT

    };
  }

  enum class Section {
    FIRST_,
    HEADER,
    HEADER_FIELDS,
    HEADER_FIELDS_NOT,
    TEXT,
    LAST_
  };
  std::ostream &operator<<(std::ostream &o, Section &s);

  class Section_Attribute {
    private:
      Section section_ { Section::FIRST_ };
      std::vector<std::string> headers_;
    public:
      Section_Attribute();
      Section_Attribute(Section section);
      Section_Attribute(Section section, const std::vector<std::string> &headers);
      Section_Attribute(Section section, std::vector<std::string> &&headers);
      std::ostream &print(std::ostream &o) const;
  };
  std::ostream &operator<<(std::ostream &o, const Section_Attribute &a);

  enum class Flag {
    FIRST_,
    ANSWERED,
    FLAGGED,
    DELETED,
    SEEN,
    DRAFT,
    RECENT,
    LAST_
  };
  std::ostream &operator<<(std::ostream &o, Flag flag);

  namespace Client {
    enum class Command {
      FIRST_,
      // Any State
      CAPABILITY,
      NOOP,
      LOGOUT,
      // Not-Authenticated
      STARTTLS,
      AUTHENTICATE,
      LOGIN,
      // Authenticated
      SELECT,
      EXAMINE,
      CREATE,
      DELETE,
      RENAME,
      SUBSCRIBE,
      UNSUBSCRIBE,
      LIST,
      LSUB,
      STATUS,
      APPEND,
      // Selected
      CHECK,
      CLOSE,
      EXPUNGE,
      SEARCH,
      FETCH,
      STORE,
      COPY,
      UID_EXPUNGE,
      UID_COPY,
      UID_FETCH,
      UID_SEARCH,
      UID_STORE,
      LAST_
    };
    const char *command_str(Command c);
    std::ostream &operator<<(std::ostream &o, Command c);

    enum class Fetch {
      FIRST_,
      ENVELOPE,
      FLAGS,
      INTERNALDATE,
      RGC822,
      RFC822_HEADER,
      RFC822_SIZE,
      RFC822_TEXT,
      BODYSTRUCTURE_NON_EXTENSIBLE,
      BODYSTRUCTURE,
      UID,
      BODY,
      BODY_PEEK,
      LAST_
    };
    std::ostream &operator<<(std::ostream &o, Fetch &s);
    class Fetch_Attribute {
      private:
        Fetch fetch_ { Fetch::FIRST_ };
        Section_Attribute section_;
      public:
        Fetch_Attribute(Fetch fetch);
        Fetch_Attribute(Fetch fetch,
            const Section_Attribute &section);
        Fetch_Attribute(Fetch fetch,
            Section_Attribute &&section);
        std::ostream &print(std::ostream &o) const;
    };
    std::ostream &operator<<(std::ostream &o, const Fetch_Attribute &a);
    enum class Store_Mode {
      FIRST_,
      REPLACE,
      ADD,
      REMOVE,
      LAST_
    };
    std::ostream &operator<<(std::ostream &o, Store_Mode mode);
  }

  namespace Server {
    namespace Response {
      enum class Group {
        STATUS,
        DATA,
        CMD_CONTINUATION
      };
      enum class Status {
        FIRST_,
        // tagged and untagged
        OK,
        NO,
        BAD,
        // only untagged
        PREAUTH,
        BYE,
        LAST_
      };
      std::ostream &operator<<(std::ostream &o, Status status);
      enum class Kind {
        UNTAGGED,
        TAGGED
      };
      enum class Status_Code {
        FIRST_,
        ALERT,
        BADCHARSET,
        CAPABILITY,
        PARSE,
        PERMANENTFLAGS,
        READ_ONLY,
        READ_WRITE,
        TRYCREATE,
        UIDNEXT,
        UIDVALIDITY,
        UNSEEN,
        LAST_
      };
      std::ostream &operator<<(std::ostream &o, Status_Code code);
      enum class Data {
        // only untagged
        CAPABILITY,
        LIST,
        LSUB,
        STATUS,
        SEARCH,
        FLAGS,
        EXISTS,
        RECENT,
        EXPUNGE,
        FETCH
      };

      // cf. http://www.iana.org/assignments/imap-capabilities/imap-capabilities.xhtml
      enum class Capability {
        FIRST_,
          /* ACL                   */ ACL,                   // [RFC4314]
          /* ANNOTATE-EXPERIMENT_1 */ ANNOTATE_EXPERIMENT_1, // [RFC5257]
          /* AUTH=                 */ AUTH_eq_,              // core
          /* AUTH=PLAIN            */ AUTH_eq_PLAIN,         // core
          /* BINARY                */ BINARY,                // [RFC3516]
          /* CATENATE              */ CATENATE,              // [RFC4469]
          /* CHILDREN              */ CHILDREN,              // [RFC3348]
          /* COMPRESS=DEFLATE      */ COMPRESS_eq_DEFLATE,   // [RFC4978]
          /* CONDSTORE             */ CONDSTORE,             // [RFC4551]
          /* CONTEXT=SEARCH        */ CONTEXT_eq_SEARCH,     // [RFC5267]
          /* CONTEXT=SORT          */ CONTEXT_eq_SORT,       // [RFC5267]
          /* CONVERT               */ CONVERT,               // [RFC5259]
          /* CREATE-SPECIAL_USE    */ CREATE_SPECIAL_USE,    // [RFC6154]
          /* ENABLE                */ ENABLE,                // [RFC5161]
          /* ESEARCH               */ ESEARCH,               // [RFC4731]
          /* ESORT                 */ ESORT,                 // [RFC5267]
          /* FILTERS               */ FILTERS,               // [RFC5466]
          /* I18NLEVEL=1           */ I18NLEVEL_eq_1,        // [RFC5255]
          /* I18NLEVEL=2           */ I18NLEVEL_eq_2,        // [RFC5255]
          /* ID                    */ ID,                    // [RFC2971]
          /* IDLE                  */ IDLE,                  // [RFC2177]
          /* IMAP4rev1             */ IMAP4rev1,             // core
          /* IMAPSIEVE=            */ IMAPSIEVE_eq_,         // [RFC_ietf_sieve_imap_sieve_09]
          /* LANGUAGE              */ LANGUAGE,              // [RFC5255]
          /* LIST-STATUS           */ LIST_STATUS,           // [RFC5819]
          /* LITERAL+              */ LITERAL_plus_,         // [RFC2088]
          /* LOGIN-REFERRALS       */ LOGIN_REFERRALS,       // [RFC2221]
          /* LOGINDISABLED         */ LOGINDISABLED,         // [RFC2595][RFC3501]
          /* MAILBOX-REFERRALS     */ MAILBOX_REFERRALS,     // [RFC2193]
          /* MOVE                  */ MOVE,                  // [RFC6851]
          /* MULTIAPPEND           */ MULTIAPPEND,           // [RFC3502]
          /* MULTISEARCH           */ MULTISEARCH,           // [RFC6237]
          /* NAMESPACE             */ NAMESPACE,             // [RFC2342]
          /* NOTIFY                */ NOTIFY,                // [RFC5465]
          /* QRESYNC               */ QRESYNC,               // [RFC5162]
          /* QUOTA                 */ QUOTA,                 // [RFC2087]
          /* RIGHTS=               */ RIGHTS_eq_,            // [RFC4314]
          /* SASL-IR               */ SASL_IR,               // [RFC4959]
          /* SEARCH=FUZZY          */ SEARCH_eq_FUZZY,       // [RFC6203]
          /* SEARCHRES             */ SEARCHRES,             // [RFC5182]
          /* SORT                  */ SORT,                  // [RFC5256]
          /* SORT=DISPLAY          */ SORT_eq_DISPLAY,       // [RFC5957]
          /* SPECIAL-USE           */ SPECIAL_USE,           // [RFC6154]
          /* STARTTLS              */ STARTTLS,              // [RFC2595][RFC3501]
          /* THREAD                */ THREAD,                // [RFC5256]
          /* UIDPLUS               */ UIDPLUS,               // [RFC4315]
          /* UNSELECT              */ UNSELECT,              // [RFC3691]
          /* URL-PARTIAL           */ URL_PARTIAL,           // [RFC5550]
          /* URLAUTH               */ URLAUTH,               // [RFC4467]
          /* URLFETCH=BINARY       */ URLFETCH_eq_BINARY,    // [RFC5524]
          /* UTF8=ACCEPT           */ UTF8_eq_ACCEPT,        // [RFC6855]
          /* UTF8=ONLY             */ UTF8_eq_ONLY,          // [RFC6855]
          /* WITHIN                */ WITHIN,                // [RFC5032]
          LAST_
      };
      std::ostream &operator<<(std::ostream &o, Capability capability);

      enum class SFlag {
        FIRST_,
        NOSELECT,
        MARKED,
        UNMARKED,
        LAST_
      };
      std::ostream &operator<<(std::ostream &o, SFlag capability);
      enum class OFlag {
        FIRST_,
        NOINFERIORS,
        // RFC3348
        HASCHILDREN,
        HASNOCHILDREN,
        LAST_
      };
      std::ostream &operator<<(std::ostream &o, OFlag capability);


    }
  }

}

#include <functional>

namespace std {
  template <> struct hash<IMAP::Server::Response::Capability>
  {
    size_t operator()(IMAP::Server::Response::Capability c) const
    {
      return hash<unsigned>()(static_cast<unsigned>(c));
    }
  };
}

namespace std {
  template <> struct hash<IMAP::Server::Response::OFlag>
  {
    size_t operator()(IMAP::Server::Response::OFlag c) const
    {
      return hash<unsigned>()(static_cast<unsigned>(c));
    }
  };
}

#endif
