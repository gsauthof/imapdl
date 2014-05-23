# Copyright 2014, Georg Sauthoff <mail@georg.so>

# {{{ GPLv3
#
#   This file is part of imapdl.
#
#   imapdl is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   imapdl is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with imapdl.  If not, see <http://www.gnu.org/licenses/>.
#
# }}}
#include <imap/server_lexer.h>

#include <stdexcept>
#include <string>
#include <iomanip>

using namespace std;

#include <boost/lexical_cast.hpp>

#include "lex_util.h"

%%{

# The server side of the IMAP protocol.
#
# The state machine description is derived from the IMAP4v1 formal syntax
# (RFC3501, Section 9, http://tools.ietf.org/html/rfc3501#section-9).
#
# The relevant grammar rules are included as comments, i.e. above each
# Ragel rule the relevant formal syntax snippet (in ABNF) is quoted as comment.
# For a description of the used augmented backus-naur form see RFC2234
# (http://tools.ietf.org/html/rfc2234) - which also contains some basic
# grammar rules used in RFC3501.

machine imapd;

# {{{ Actions

action cb_flag_recent
{
//  cb_.imap_flag(Server::Response::Flag::RECENT);
}
action cb_flag_answered
{
//  cb_.imap_flag(Server::Response::Flag::ANSWERED);
}
action cb_flag_flagged
{
//  cb_.imap_flag(Server::Response::Flag::FLAGGED);
}
action cb_flag_deleted
{
//  cb_.imap_flag(Server::Response::Flag::DELETED);
}
action cb_flag_seen
{
//  cb_.imap_flag(Server::Response::Flag::SEEN);
}
action cb_flag_draft
{
//  cb_.imap_flag(Server::Response::Flag::DRAFT);
}
action cb_flag_atom
{
//  cb_.imap_atom_flag();
}
action cb_body_section_begin
{
}
action cb_section_empty
{
}
action cb_section_header
{
}

action userid_begin
{
  userid_buffer_.start(p);
}
action userid_end
{
  userid_buffer_.finish(p);
}
# from RFC 3501 - IMAP4rev1:
#
#                   +----------------------+
#                   |connection established|
#                   +----------------------+
#                              ||
#                              \/
#            +--------------------------------------+
#            |          server greeting             |
#            +--------------------------------------+
#                      || (1)       || (2)        || (3)
#                      \/           ||            ||
#            +-----------------+    ||            ||
#            |Not Authenticated|    ||            ||
#            +-----------------+    ||            ||
#             || (7)   || (4)       ||            ||
#             ||       \/           \/            ||
#             ||     +----------------+           ||
#             ||     | Authenticated  |<=++       ||
#             ||     +----------------+  ||       ||
#             ||       || (7)   || (5)   || (6)   ||
#             ||       ||       \/       ||       ||
#             ||       ||    +--------+  ||       ||
#             ||       ||    |Selected|==++       ||
#             ||       ||    +--------+           ||
#             ||       ||       || (7)            ||
#             \/       \/       \/                \/
#            +--------------------------------------+
#            |               Logout                 |
#            +--------------------------------------+
#                              ||
#                              \/
#                +-------------------------------+
#                |both sides close the connection|
#                +-------------------------------+
#
#         (1) connection without pre-authentication (OK greeting)
#         (2) pre-authenticated connection (PREAUTH greeting)
#         (3) rejected connection (BYE greeting)
#         (4) successful LOGIN or AUTHENTICATE command
#         (5) successful SELECT or EXAMINE command
#         (6) CLOSE command, or failed SELECT or EXAMINE command
#         (7) LOGOUT command, server shutdown, or connection closed
#
action cb_login
{
  bool r = cb_.imapd_login(userid_buffer_, buffer_);
  if (r) {
    state_ = IMAP::Connection::State::AUTHENTICATED;
  } else {
  }
}
action cb_logout
{
  state_ = IMAP::Connection::State::LOGGED_OUT;
}
action cb_examine
{
  state_ = IMAP::Connection::State::SELECTED;
  read_only_ = true;
}
action cb_select
{
  state_ = IMAP::Connection::State::SELECTED;
  read_only_ = false;
}
action cb_close
{
  state_ = IMAP::Connection::State::AUTHENTICATED;
}
action cb_store
{
  if (read_only_)
    throw std::runtime_error("STORE not allowed an read-only selected mailbox");
}
action cb_expunge
{
  if (read_only_)
    throw std::runtime_error("EXPUNGE not allowed an read-only selected mailbox");
}
action cb_uid_expunge
{
  if (read_only_)
    throw std::runtime_error("UID EXPUNGE not allowed an read-only selected mailbox");
}
# using this instead of the RAGEL state chart syntax
# because state transitions not only depend on the syntax
# but also on the semantic - as evaluated by a callback function
# (e.g. when the login callback verifies a given password against a stored hash)
action jmp_state
{
  switch (state_) {
    case IMAP::Connection::State::CLOSED:
      throw logic_error("not connected yet state - cannot issue a command");
      break;
    case IMAP::Connection::State::NOT_AUTHENTICATED:
      fnext not_authenticated;
      break;
    case IMAP::Connection::State::AUTHENTICATED:
      fnext authenticated;
      break;
    case IMAP::Connection::State::SELECTED:
      fnext selected;
      break;
    case IMAP::Connection::State::LOGGED_OUT:
      fgoto *imapd_first_final;
      state_ = IMAP::Connection::State::CLOSED;
      //throw logic_error("No more commands allowed in logged out state");
      break;
    default:
      throw logic_error("Found unknown connection state");
  }
}
action call_search_key
{
  fcall search_key;
}

# }}}

include imap_common "imap/common.rl";


# x-command       = "X" atom <experimental command arguments>


# RFC2971 IMAP4 ID extension

# id ::= "ID" SPACE id_params_list

id = /ID/i SP id_params_list
  ;

#command-any     = "CAPABILITY" / "LOGOUT" / "NOOP" / x-command
#                    ; Valid in all states

command_any = /CAPABILITY/i
            | /LOGOUT/i %cb_logout
            | /NOOP/i
            # RFC2971 IMAP4 ID extension
            | id
            # add x-commands as needed
  ;


#append          = "APPEND" SP mailbox [SP flag-list] [SP date-time] SP
#                  literal

append = /APPEND/i SP mailbox (SP flag_list)? (SP date_time)? SP literal
  ;

#create          = "CREATE" SP mailbox
#                    ; Use of INBOX gives a NO error

create = /CREATE/i SP mailbox
  ;

#delete          = "DELETE" SP mailbox
#                    ; Use of INBOX gives a NO error

delete = /DELETE/i SP mailbox
  ;

# examine         = "EXAMINE" SP mailbox

examine = /EXAMINE/i SP mailbox %cb_examine
  ;

#list-char       = ATOM-CHAR / list-wildcards / resp-specials

list_char = ATOM_CHAR | list_wildcards | resp_specials
  ;

#list-mailbox    = 1*list-char / string

list_mailbox = list_char+ | string
  ;

#list            = "LIST" SP mailbox SP list-mailbox

list = /LIST/i SP mailbox SP list_mailbox
  ;

#lsub            = "LSUB" SP mailbox SP list-mailbox

lsub = /LSUB/i SP mailbox SP list_mailbox
  ;

#rename          = "RENAME" SP mailbox SP mailbox
#                    ; Use of INBOX as a destination gives a NO error

rename = /RENAME/i SP mailbox SP mailbox
  ;

# select          = "SELECT" SP mailbox

select = /SELECT/i SP mailbox %cb_select
  ;

# status          = "STATUS" SP mailbox SP
#                  "(" status-att *(SP status-att) ")"

status = /STATUS/i SP mailbox SP
                  '(' status_att (SP status_att)* ')'
  ;

# subscribe       = "SUBSCRIBE" SP mailbox

subscribe = /SUBSCRIBE/i SP mailbox
  ;

# unsubscribe     = "UNSUBSCRIBE" SP mailbox

unsubscribe = /UNSUBSCRIBE/ SP mailbox
  ;

#command-auth    = append / create / delete / examine / list / lsub /
#                  rename / select / status / subscribe / unsubscribe
#                    ; Valid only in Authenticated or Selected state

command_auth = append
             | create
             | delete
             | examine
             | list
             | lsub
             | rename
             | select
             | status
             | subscribe
             | unsubscribe
  ;

# authenticate    = "AUTHENTICATE" SP auth-type *(CRLF base64)

# password        = astring

password = astring
  ;

# userid          = astring

userid = astring
  ;

# login           = "LOGIN" SP userid SP password

login = /LOGIN/i SP userid   >userid_begin %userid_end
                 SP password >buffer_begin %buffer_end
                 %cb_login
  ;

#command-nonauth = login / authenticate / "STARTTLS"
#                    ; Valid only when in Not Authenticated state

command_nonauth = login
                # implement as needed
                #| authenticate
                # implement as needed
                # (directly connecting to IMAPS server is the better solution)
                #| /STARTTLS/i
                ;

#seq-number      = nz-number / "*"
#                    ; message sequence number (COPY, FETCH, STORE
#                    ; commands) or unique identifier (UID COPY,
#                    ; UID FETCH, UID STORE commands).
#                    ; * represents the largest number in use.  In
#                    ; the case of message sequence numbers, it is
#                    ; the number of messages in a non-empty mailbox.
#                    ; In the case of unique identifiers, it is the
#                    ; unique identifier of the last message in the
#                    ; mailbox or, if the mailbox is empty, the
#                    ; mailbox's current UIDNEXT value.
#                    ; The server should respond with a tagged BAD
#                    ; response to a command that uses a message
#                    ; sequence number greater than the number of
#                    ; messages in the selected mailbox.  This
#                    ; includes "*" if the selected mailbox is empty.

seq_number = nz_number | '*'
  ;

#seq-range       = seq-number ":" seq-number
#                    ; two seq-number values and all values between
#                    ; these two regardless of order.
#                    ; Example: 2:4 and 4:2 are equivalent and indicate
#                    ; values 2, 3, and 4.
#                    ; Example: a unique identifier sequence range of
#                    ; 3291:* includes the UID of the last message in
#                    ; the mailbox, even if that value is less than 3291.

seq_range = seq_number ':' seq_number
  ;

#sequence-set    = (seq-number / seq-range) *("," sequence-set)
#                    ; set of seq-number values, regardless of order.
#                    ; Servers MAY coalesce overlaps and/or execute the
#                    ; sequence in any order.
#                    ; Example: a message sequence number set of
#                    ; 2,4:7,9,12:* for a mailbox with 15 messages is
#                    ; equivalent to 2,4,5,6,7,9,12,13,14,15
#                    ; Example: a message sequence number set of *:4,5:7
#                    ; for a mailbox with 10 messages is equivalent to
#                    ; 10,9,8,7,6,5,4,5,6,7 and MAY be reordered and
#                    ; overlap coalesced to be 4,5,6,7,8,9,10.

sequence_set = (seq_number | seq_range) (',' (seq_number | seq_range))*
  ;

#copy            = "COPY" SP sequence-set SP mailbox

copy = /COPY/i SP sequence_set SP mailbox
  ;


#fetch-att       = "ENVELOPE" / "FLAGS" / "INTERNALDATE" /
#                  "RFC822" [".HEADER" / ".SIZE" / ".TEXT"] /
#                  "BODY" ["STRUCTURE"] / "UID" /
#                  "BODY" section ["<" number "." nz-number ">"] /
#                  "BODY.PEEK" section ["<" number "." nz-number ">"]

fetch_att = /ENVELOPE/i
          | /FLAGS/i
          | /INTERNALDATE/i
          | /RFC822/i (/.HEADER/i | /.SIZE/i | /.TEXT/i)?
          | /BODY/i   (/STRUCTURE/i)?
          | /UID/i
          | /BODY/i      section ('<' number '.' nz_number '>')?
          | /BODY.PEEK/i section ('<' number '.' nz_number '>')?
  ;

#fetch           = "FETCH" SP sequence-set SP ("ALL" / "FULL" / "FAST" /
#                  fetch-att / "(" fetch-att *(SP fetch-att) ")")

fetch = /FETCH/i SP sequence_set SP
                 ( /ALL/i | /FULL/i | /FAST/i |
                   fetch_att | '(' fetch_att (SP fetch_att)* ')' )
  ;

#store-att-flags = (["+" / "-"] "FLAGS" [".SILENT"]) SP
#                  (flag-list / (flag *(SP flag)))

store_att_flags = ('+'|'-')? /FLAGS/i (/.SILENT/i)? SP
                  (flag_list | (flag (SP flag)*))
  ;

#store           = "STORE" SP sequence-set SP store-att-flags

store = /STORE/i SP sequence_set SP store_att_flags
  %cb_store
  ;

#date-day        = 1*2DIGIT
#                    ; Day of month

date_day = DIGIT{1,2};

#date-text       = date-day "-" date-month "-" date-year

date_text = date_day '-' date_month '-' date_year
  ;

#date            = date-text / DQUOTE date-text DQUOTE

date =        date_text
     | DQUOTE date_text DQUOTE
  ;

#search-key      = "ALL" / "ANSWERED" / "BCC" SP astring /
#                  "BEFORE" SP date / "BODY" SP astring /
#                  "CC" SP astring / "DELETED" / "FLAGGED" /
#                  "FROM" SP astring / "KEYWORD" SP flag-keyword /
#                  "NEW" / "OLD" / "ON" SP date / "RECENT" / "SEEN" /
#                  "SINCE" SP date / "SUBJECT" SP astring /
#                  "TEXT" SP astring / "TO" SP astring /
#                  "UNANSWERED" / "UNDELETED" / "UNFLAGGED" /
#                  "UNKEYWORD" SP flag-keyword / "UNSEEN" /
#                    ; Above this line were in [IMAP2]
#                  "DRAFT" / "HEADER" SP header-fld-name SP astring /
#                  "LARGER" SP number / "NOT" SP search-key /
#                  "OR" SP search-key SP search-key /
#                  "SENTBEFORE" SP date / "SENTON" SP date /
#                  "SENTSINCE" SP date / "SMALLER" SP number /
#                  "UID" SP sequence-set / "UNDRAFT" / sequence-set /
#                  "(" search-key *(SP search-key) ")"

search_key := (
              /ALL/i
            | /ANSWERED/i
            | /BCC/i        SP astring
            | /BEFORE/i     SP date
            | /BODY/i       SP astring
            | /CC/i         SP astring
            | /DELETED/i
            | /FLAGGED/i
            | /FROM/i       SP astring
            | /KEYWORD/i    SP flag_keyword
            | /NEW/i
            | /OLD/i
            | /ON/i         SP date
            | /RECENT/i
            | /SEEN/i
            | /SINCE/i      SP date
            | /SUBJECT/i    SP astring
            | /TEXT/i       SP astring
            | /TO/i         SP astring
            | /UNANSWERED/i
            | /UNDELETED/i
            | /UNFLAGGED/i
            | /UNKEYWORD/i  SP flag_keyword
            | /UNSEEN/i
            # Above this line were in [IMAP2]
            | /DRAFT/i
            | /HEADER/i     SP header_fld_name SP astring
            | /LARGER/i     SP number
            | /NOT/i        SP @call_search_key
            | /OR/i         SP @call_search_key SP @call_search_key
            | /SENTBEFORE/i SP date
            | /SENTON/i     SP date
            | /SENTSINCE/i  SP date
            | /SMALLER/i    SP number
            | /UID/i        SP sequence_set
            | /UNDRAFT/i
            | sequence_set
            | '(' @call_search_key (SP @call_search_key)* ')'
            ) %^return_minus
  ;

#search          = "SEARCH" [SP "CHARSET" SP astring] 1*(SP search-key)
#                    ; CHARSET argument to MUST be registered with IANA

search = /SEARCH/i (SP /CHARSET/i SP astring)? (SP @call_search_key)+
  ;

#uid             = "UID" SP (copy / fetch / search / store)
#                    ; Unique identifiers used instead of message
#                    ; sequence numbers

uid = /UID/i SP (copy | fetch | search | store)
  ;

# RFC4315 IMAP UIDPLUS extension

#uid-expunge     = "UID" SP "EXPUNGE" SP sequence-set

uid_expunge = /UID/i SP /EXPUNGE/i SP sequence_set
  %cb_uid_expunge
  ;

#command-select  = "CHECK" / "CLOSE" / "EXPUNGE" / copy / fetch / store /
#                  uid / search
#                    ; Valid only when in Selected state
# RFC4315 IMAP UIDPLUS extension
# command-select  =/ uid-expunge

command_select = /CHECK/i
               | /CLOSE/i   %cb_close
               | /EXPUNGE/i %cb_expunge
               | copy
               | fetch
               | store
               | search
               | uid
               # RFC4315 IMAP UIDPLUS extension
               | uid_expunge
  ;

#command         = tag SP (command-any / command-auth / command-nonauth /
#                  command-select) CRLF
#                    ; Modal based on state

prefix = tag SP
  ;


selected := ( prefix 
              ( command_select
              | command_auth
              | command_any )         CR LF @jmp_state ) ;

authenticated := ( prefix
                   ( command_auth
                   | command_any )    CR LF @jmp_state ) ;

not_authenticated = ( prefix
                      ( command_nonauth
                      | command_any ) CR LF @jmp_state ) ;

requests = not_authenticated ; 


main := requests ;

prepush {
  if (unsigned(top) == stack_vector_.size()) {
    stack_vector_.push_back(0);
    stack = stack_vector_.data();
  }
}


}%%


namespace IMAP {

  namespace Server {

    using namespace Memory;

    %% write data;

    Lexer::Lexer(Buffer::Base &buffer,
      Buffer::Base &tag_buffer,
      Callback::Base &cb)
      : buffer_(buffer), tag_buffer_(tag_buffer), cb_(cb)
    {
      %% write init;
    }

    void Lexer::read(const char *begin, const char *end)
    {
      const char *p  = begin;
      const char *pe = end;
      buffer_.resume(p);
      tag_buffer_.resume(p);
      %% write exec;
      if (cs == %%{write error;}%%) {
        throw_lex_error("IMAP server automaton in error state", begin, p, pe);
      }
      buffer_.stop(pe);
      tag_buffer_.stop(pe);
    }

    bool Lexer::in_start() const
    {
      return cs == %%{write start;}%%;
    }
    bool Lexer::finished() const
    {
      return cs >= %%{write first_final;}%%;
    }

    void Lexer::verify_finished() const
    {
      if (!finished())
        throw runtime_error("IMAP client automaton not in final state");
    }


    namespace Callback {

      bool Null::imapd_login(const Memory::Buffer::Base &userid,
          const Memory::Buffer::Base &password)
      {
        return true;
      }

    }

  }
}
