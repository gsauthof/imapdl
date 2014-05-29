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
#include <imap/client_lexer.h>

#include <stdexcept>
#include <string>
#include <iomanip>
#include <sstream>

using namespace std;

#include <boost/lexical_cast.hpp>

#include "lex_util.h"

%%{

# The client side of the IMAP protocol.
#
# The state machine description is derived from the IMAP4v1 formal syntax
# (RFC3501, Section 9, http://tools.ietf.org/html/rfc3501#section-9).
#
# The relevant grammar rules are included as comments, i.e. above each
# Ragel rule the relevant formal syntax snippet (in ABNF) is quoted as comment.
# For a description of the used augmented backus-naur form see RFC2234
# (http://tools.ietf.org/html/rfc2234) - which also contains some basic
# grammar rules used in RFC3501.

machine imap;

# {{{ Actions

action return { fret; }
action call_continue_req_tail { fcall continue_req_tail; }

action call_capability
{
  fcall capability;
}

action status_ok
{
  status_ = Server::Response::Status::OK;
}
action status_no
{
  status_ = Server::Response::Status::NO;
}
action status_bad
{
  status_ = Server::Response::Status::BAD;
}
action status_bye
{
  status_ = Server::Response::Status::BYE;
}
action cb_tagged_status_begin
{
  cb_.imap_tagged_status_begin();
}
action cb_tagged_status_end
{
  cb_.imap_tagged_status_end(status_);
}
action cb_untagged_status_begin
{
  cb_.imap_untagged_status_begin(status_);
}
action cb_untagged_status_end
{
  cb_.imap_untagged_status_end(status_);
}
action cb_data_exists
{
  cb_.imap_data_exists(number_);
}
action cb_data_recent
{
  cb_.imap_data_recent(number_);
}
action cb_data_expunge
{
  cb_.imap_data_expunge(number_);
}
action cb_data_fetch_begin
{
  cb_.imap_data_fetch_begin(number_);
}
action cb_data_fetch_end
{
  cb_.imap_data_fetch_end();
}
action cb_data_flags_begin
{
  cb_.imap_data_flags_begin();
}
action cb_data_flags_end
{
  cb_.imap_data_flags_end();
}
action cb_flag_recent
{
  cb_.imap_flag(IMAP::Flag::RECENT);
}
action cb_flag_answered
{
  cb_.imap_flag(IMAP::Flag::ANSWERED);
}
action cb_flag_flagged
{
  cb_.imap_flag(IMAP::Flag::FLAGGED);
}
action cb_flag_deleted
{
  cb_.imap_flag(IMAP::Flag::DELETED);
}
action cb_flag_seen
{
  cb_.imap_flag(IMAP::Flag::SEEN);
}
action cb_flag_draft
{
  cb_.imap_flag(IMAP::Flag::DRAFT);
}
action cb_flag_atom
{
  cb_.imap_atom_flag();
}
action cb_uid
{
  cb_.imap_uid(number_);
}
action cb_status_code_alert
{
  cb_.imap_status_code(Server::Response::Status_Code::ALERT);
}
action cb_status_code_badcharset
{
  cb_.imap_status_code(Server::Response::Status_Code::BADCHARSET);
}
action cb_status_code_capability
{
  cb_.imap_status_code(Server::Response::Status_Code::CAPABILITY);
}
action cb_status_code_parse
{
  cb_.imap_status_code(Server::Response::Status_Code::PARSE);
}
action cb_status_code_permanentflags
{
  cb_.imap_status_code(Server::Response::Status_Code::PERMANENTFLAGS);
}
action cb_status_code_read_only
{
  cb_.imap_status_code(Server::Response::Status_Code::READ_ONLY);
}
action cb_status_code_read_write
{
  cb_.imap_status_code(Server::Response::Status_Code::READ_WRITE);
}
action cb_status_code_trycreate
{
  cb_.imap_status_code(Server::Response::Status_Code::TRYCREATE);
}
action cb_status_code_uidnext
{
  cb_.imap_status_code(Server::Response::Status_Code::UIDNEXT);
  cb_.imap_status_code_uidnext(number_);
}
action cb_status_code_uidvalidity
{
  cb_.imap_status_code(Server::Response::Status_Code::UIDVALIDITY);
  cb_.imap_status_code_uidvalidity(number_);
}
action cb_status_code_unseen
{
  cb_.imap_status_code(Server::Response::Status_Code::UNSEEN);
  cb_.imap_status_code_unseen(number_);
}

action cb_status_code_capability_begin
{
  cb_.imap_status_code_capability_begin();
  has_imap4rev1_ = false;
}
action cb_status_code_capability_end
{
  cb_.imap_status_code_capability_end();
  if (!has_imap4rev1_) {
    fnext *imap_error;
    throw runtime_error("server does not have imap4rev1 capability");
  }
}
action cb_capability_begin
{
  cb_.imap_capability_begin();
}
action cb_capability_end
{
  cb_.imap_capability_end();
}
action cb_capability_imap4rev1
{
  has_imap4rev1_ = true;
  cb_.imap_capability(Server::Response::Capability::IMAP4rev1);
}

# automatically generated ->
action cb_capability_acl
{
  cb_.imap_capability(Server::Response::Capability::ACL);
}
action cb_capability_annotate_experiment_1
{
  cb_.imap_capability(Server::Response::Capability::ANNOTATE_EXPERIMENT_1);
}
action cb_capability_auth_eq_
{
  cb_.imap_capability(Server::Response::Capability::AUTH_eq_);
}
action cb_capability_auth_eq_plain
{
  cb_.imap_capability(Server::Response::Capability::AUTH_eq_PLAIN);
}
action cb_capability_binary
{
  cb_.imap_capability(Server::Response::Capability::BINARY);
}
action cb_capability_catenate
{
  cb_.imap_capability(Server::Response::Capability::CATENATE);
}
action cb_capability_children
{
  cb_.imap_capability(Server::Response::Capability::CHILDREN);
}
action cb_capability_compress_eq_deflate
{
  cb_.imap_capability(Server::Response::Capability::COMPRESS_eq_DEFLATE);
}
action cb_capability_condstore
{
  cb_.imap_capability(Server::Response::Capability::CONDSTORE);
}
action cb_capability_context_eq_search
{
  cb_.imap_capability(Server::Response::Capability::CONTEXT_eq_SEARCH);
}
action cb_capability_context_eq_sort
{
  cb_.imap_capability(Server::Response::Capability::CONTEXT_eq_SORT);
}
action cb_capability_convert
{
  cb_.imap_capability(Server::Response::Capability::CONVERT);
}
action cb_capability_create_special_use
{
  cb_.imap_capability(Server::Response::Capability::CREATE_SPECIAL_USE);
}
action cb_capability_enable
{
  cb_.imap_capability(Server::Response::Capability::ENABLE);
}
action cb_capability_esearch
{
  cb_.imap_capability(Server::Response::Capability::ESEARCH);
}
action cb_capability_esort
{
  cb_.imap_capability(Server::Response::Capability::ESORT);
}
action cb_capability_filters
{
  cb_.imap_capability(Server::Response::Capability::FILTERS);
}
action cb_capability_i18nlevel_eq_1
{
  cb_.imap_capability(Server::Response::Capability::I18NLEVEL_eq_1);
}
action cb_capability_i18nlevel_eq_2
{
  cb_.imap_capability(Server::Response::Capability::I18NLEVEL_eq_2);
}
action cb_capability_id
{
  cb_.imap_capability(Server::Response::Capability::ID);
}
action cb_capability_idle
{
  cb_.imap_capability(Server::Response::Capability::IDLE);
}
action cb_capability_imapsieve_eq_
{
  cb_.imap_capability(Server::Response::Capability::IMAPSIEVE_eq_);
}
action cb_capability_language
{
  cb_.imap_capability(Server::Response::Capability::LANGUAGE);
}
action cb_capability_list_status
{
  cb_.imap_capability(Server::Response::Capability::LIST_STATUS);
}
action cb_capability_literal_plus_
{
  cb_.imap_capability(Server::Response::Capability::LITERAL_plus_);
}
action cb_capability_login_referrals
{
  cb_.imap_capability(Server::Response::Capability::LOGIN_REFERRALS);
}
action cb_capability_logindisabled
{
  cb_.imap_capability(Server::Response::Capability::LOGINDISABLED);
}
action cb_capability_mailbox_referrals
{
  cb_.imap_capability(Server::Response::Capability::MAILBOX_REFERRALS);
}
action cb_capability_move
{
  cb_.imap_capability(Server::Response::Capability::MOVE);
}
action cb_capability_multiappend
{
  cb_.imap_capability(Server::Response::Capability::MULTIAPPEND);
}
action cb_capability_multisearch
{
  cb_.imap_capability(Server::Response::Capability::MULTISEARCH);
}
action cb_capability_namespace
{
  cb_.imap_capability(Server::Response::Capability::NAMESPACE);
}
action cb_capability_notify
{
  cb_.imap_capability(Server::Response::Capability::NOTIFY);
}
action cb_capability_qresync
{
  cb_.imap_capability(Server::Response::Capability::QRESYNC);
}
action cb_capability_quota
{
  cb_.imap_capability(Server::Response::Capability::QUOTA);
}
action cb_capability_rights_eq_
{
  cb_.imap_capability(Server::Response::Capability::RIGHTS_eq_);
}
action cb_capability_sasl_ir
{
  cb_.imap_capability(Server::Response::Capability::SASL_IR);
}
action cb_capability_search_eq_fuzzy
{
  cb_.imap_capability(Server::Response::Capability::SEARCH_eq_FUZZY);
}
action cb_capability_searchres
{
  cb_.imap_capability(Server::Response::Capability::SEARCHRES);
}
action cb_capability_sort
{
  cb_.imap_capability(Server::Response::Capability::SORT);
}
action cb_capability_sort_eq_display
{
  cb_.imap_capability(Server::Response::Capability::SORT_eq_DISPLAY);
}
action cb_capability_special_use
{
  cb_.imap_capability(Server::Response::Capability::SPECIAL_USE);
}
action cb_capability_starttls
{
  cb_.imap_capability(Server::Response::Capability::STARTTLS);
}
action cb_capability_thread
{
  cb_.imap_capability(Server::Response::Capability::THREAD);
}
action cb_capability_uidplus
{
  cb_.imap_capability(Server::Response::Capability::UIDPLUS);
}
action cb_capability_unselect
{
  cb_.imap_capability(Server::Response::Capability::UNSELECT);
}
action cb_capability_url_partial
{
  cb_.imap_capability(Server::Response::Capability::URL_PARTIAL);
}
action cb_capability_urlauth
{
  cb_.imap_capability(Server::Response::Capability::URLAUTH);
}
action cb_capability_urlfetch_eq_binary
{
  cb_.imap_capability(Server::Response::Capability::URLFETCH_eq_BINARY);
}
action cb_capability_utf8_eq_accept
{
  cb_.imap_capability(Server::Response::Capability::UTF8_eq_ACCEPT);
}
action cb_capability_utf8_eq_only
{
  cb_.imap_capability(Server::Response::Capability::UTF8_eq_ONLY);
}
action cb_capability_within
{
  cb_.imap_capability(Server::Response::Capability::WITHIN);
}
# <-- automatically generated

action tag_start
{
  tag_buffer_.start(p);
}
action tag_finish
{
  tag_buffer_.finish(p);
}

#                    ; (0 <= n < 4,294,967,296)
# the lexical_cast in number_end already does the checking for us ...
# thus not used
action check_number
{
  // check only works for at most 10 digit long strings
  if (strncmp(number_buffer_.begin(), "4294967296",
              number_buffer_.end() - number_buffer_.begin()) >= 0) {
    // fgoto *imap_error;
    fnext *imap_error;
    throw overflow_error("number is >= 4,294,967,296");
  }
}

action cb_body_section_begin
{
  cb_.imap_body_section_begin();
}
action cb_body_section_inner
{
  cb_.imap_body_section_inner();
}
action cb_body_section_end
{
  cb_.imap_body_section_end();
}
action cb_section_header
{
  cb_.imap_section_header();
}
action cb_section_empty
{
  cb_.imap_section_empty();
}

# }}}

include imap_common "imap/common.rl";


# text            = 1*TEXT-CHAR

text = TEXT_CHAR + ;

# resolve ambiguity for resp_text
textNB = (TEXT_CHAR - '[') TEXT_CHAR * ;


#flag-fetch      = flag / "\Recent"

flag_fetch = flag | /\\Recent/i %cb_flag_recent ;

# flag-perm       = flag / "\*"

flag_perm = flag
          | '\\*' ;

# auth-type       = atom
#                     ; Defined by [SASL]

auth_type = atom ;


# capability      = ("AUTH=" auth-type) / atom
#                     ; New capabilities MUST begin with "X" or be
#                     ; registered with IANA as standard or
#                     ; standards-track

#capability = /AUTH/i '=' auth_type
#           | (atom - (/AUTH/i '=' auth_type ) ) ;

capability :=
  (
        /AUTH=/i       auth_type %cb_capability_auth_eq_              |
        /IMAPSIEVE=/i  atom      %cb_capability_imapsieve_eq_         |
        /RIGHTS=/i     atom      %cb_capability_rights_eq_

        /ACL/i                   %cb_capability_acl                   |
        /AUTH=PLAIN/i            %cb_capability_auth_eq_plain         |
        /ANNOTATE-EXPERIMENT-1/i %cb_capability_annotate_experiment_1 |
        /BINARY/i                %cb_capability_binary                |
        /CATENATE/i              %cb_capability_catenate              |
        /CHILDREN/i              %cb_capability_children              |
        /COMPRESS=DEFLATE/i      %cb_capability_compress_eq_deflate   |
        /CONDSTORE/i             %cb_capability_condstore             |
        /CONTEXT=SEARCH/i        %cb_capability_context_eq_search     |
        /CONTEXT=SORT/i          %cb_capability_context_eq_sort       |
        /CONVERT/i               %cb_capability_convert               |
        /CREATE-SPECIAL-USE/i    %cb_capability_create_special_use    |
        /ENABLE/i                %cb_capability_enable                |
        /ESEARCH/i               %cb_capability_esearch               |
        /ESORT/i                 %cb_capability_esort                 |
        /FILTERS/i               %cb_capability_filters               |
        /I18NLEVEL=1/i           %cb_capability_i18nlevel_eq_1        |
        /I18NLEVEL=2/i           %cb_capability_i18nlevel_eq_2        |
        /ID/i                    %cb_capability_id                    |
        /IDLE/i                  %cb_capability_idle                  |
        /LANGUAGE/i              %cb_capability_language              |
        /LIST-STATUS/i           %cb_capability_list_status           |
        /LITERAL+/i              %cb_capability_literal_plus_         |
        /LOGIN-REFERRALS/i       %cb_capability_login_referrals       |
        /LOGINDISABLED/i         %cb_capability_logindisabled         |
        /MAILBOX-REFERRALS/i     %cb_capability_mailbox_referrals     |
        /MOVE/i                  %cb_capability_move                  |
        /MULTIAPPEND/i           %cb_capability_multiappend           |
        /MULTISEARCH/i           %cb_capability_multisearch           |
        /NAMESPACE/i             %cb_capability_namespace             |
        /NOTIFY/i                %cb_capability_notify                |
        /QRESYNC/i               %cb_capability_qresync               |
        /QUOTA/i                 %cb_capability_quota                 |
        /SASL-IR/i               %cb_capability_sasl_ir               |
        /SEARCH=FUZZY/i          %cb_capability_search_eq_fuzzy       |
        /SEARCHRES/i             %cb_capability_searchres             |
        /SORT/i                  %cb_capability_sort                  |
        /SORT=DISPLAY/i          %cb_capability_sort_eq_display       |
        /SPECIAL-USE/i           %cb_capability_special_use           |
        /STARTTLS/i              %cb_capability_starttls              |
        /THREAD/i                %cb_capability_thread                |
        # RFC4315 IMAP UIDPLUS extension
        /UIDPLUS/i               %cb_capability_uidplus               |
        /UNSELECT/i              %cb_capability_unselect              |
        /URL-PARTIAL/i           %cb_capability_url_partial           |
        /URLAUTH/i               %cb_capability_urlauth               |
        /URLFETCH=BINARY/i       %cb_capability_urlfetch_eq_binary    |
        /UTF8=ACCEPT/i           %cb_capability_utf8_eq_accept        |
        /UTF8=ONLY/i             %cb_capability_utf8_eq_only          |
        /WITHIN/i                %cb_capability_within                |

        # mandatory, check in capability_data action
        /IMAP4rev1/i             %cb_capability_imap4rev1             |

        atom
  ) >cb_capability_begin >buffer_start %buffer_finish %cb_capability_end

  # '\r' is for when calling capability data from an untagged
  # response
  ( ' ' | ']' | '\r' ) @return_minus
;

# capability-data = "CAPABILITY" *(SP capability) SP "IMAP4rev1"
#                   *(SP capability)
#                     ; Servers MUST implement the STARTTLS, AUTH=PLAIN,
#                     ; and LOGINDISABLED capabilities
#                     ; Servers which offer RFC 1730 compatibility MUST
#                     ; list "IMAP4" as the first capability.

capability_data =
  /CAPABILITY/i        >cb_status_code_capability_begin

  (SP @call_capability)* %cb_status_code_capability_end ;

# uniqueid        = nz-number
#                    ; Strictly ascending

uniqueid = nz_number ;

# RFC4315 IMAP UIDPLUS extension
# uid-range       = (uniqueid ":" uniqueid)
#                     ; two uniqueid values and all values
#                     ; between these two regards of order.
#                     ; Example: 2:4 and 4:2 are equivalent.

uid_range = ( uniqueid ':' uniqueid )
  ;

# RFC4315 IMAP UIDPLUS extension
# uid-set         = (uniqueid / uid-range) *("," uid-set)

uid_set = ( uniqueid | uid_range) (',' ( uniqueid | uid_range))*
  ;

# RFC4315 IMAP UIDPLUS extension
#append-uid      = uniqueid
#
#   Servers that support [MULTIAPPEND] will have the following extension
#   to the above rules:
#
#   append-uid      =/ uid-set
#                     ; only permitted if client uses [MULTIAPPEND]
#                     ; to append multiple messages.

append_uid = uniqueid
  ;

# RFC4315 IMAP UIDPLUS extension
# resp-code-apnd  = "APPENDUID" SP nz-number SP append-uid

resp_code_apnd  = /APPENDUID/ SP nz_number SP append_uid
  ;

# RFC4315 IMAP UIDPLUS extension
# resp-code-copy  = "COPYUID" SP nz-number SP uid-set SP uid-set

resp_code_copy = /COPYUID/i SP nz_number SP uid_set SP uid_set
  ;

# resp-text-code  = "ALERT" /
#                   "BADCHARSET" [SP "(" astring *(SP astring) ")" ] /
#                   capability-data / "PARSE" /
#                   "PERMANENTFLAGS" SP "("
#                   [flag-perm *(SP flag-perm)] ")" /
#                   "READ-ONLY" / "READ-WRITE" / "TRYCREATE" /
#                   "UIDNEXT" SP nz-number / "UIDVALIDITY" SP nz-number /
#                   "UNSEEN" SP nz-number /
#                   atom [SP 1*<any TEXT-CHAR except "]">]

# RFC4315 IMAP UIDPLUS extension
#   resp-text-code  =/ resp-code-apnd / resp-code-copy / "UIDNOTSTICKY"
#                     ; incorporated before the expansion rule of
#                     ;  atom [SP 1*<any TEXT-CHAR except "]">]
#                     ; that appears in [IMAP]


resp_text_code_head =
    /ALERT/i
  | /BADCHARSET/i
  | /CAPABILITY/i
  | /PARSE/i
  | /PERMANENTFLAGS/i
  | /READ-ONLY/i
  | /READ-WRITE/i
  | /TRYCREATE/i
  | /UIDNEXT/i
  | /UIDVALIDITY/i
  | /UNSEEN/i
  # RFC4315 IMAP UIDPLUS extension
  | /APPENDUID/i
  | /COPYUID/i
  | /UIDNOTSTICKY/i
  ;

resp_text_code = /ALERT/i          %cb_status_code_alert
               | /BADCHARSET/i     %cb_status_code_badcharset
                   ( SP '(' astring (SP astring)* ')' )?
               | capability_data
               | /PARSE/i          %cb_status_code_parse
               | /PERMANENTFLAGS/i %cb_status_code_permanentflags
                   SP '(' (flag_perm (SP flag_perm)* )? ')'
               | /READ-ONLY/i      %cb_status_code_read_only
               | /READ-WRITE/i     %cb_status_code_read_write
               | /TRYCREATE/i      %cb_status_code_trycreate
               | /UIDNEXT/i     SP nz_number %cb_status_code_uidnext
               | /UIDVALIDITY/i SP nz_number %cb_status_code_uidvalidity
               | /UNSEEN/i      SP nz_number %cb_status_code_unseen
               # RFC4315 IMAP UIDPLUS extension
               | resp_code_apnd
               | resp_code_copy
               | /UIDNOTSTICKY/i
               | (atom - resp_text_code_head) (SP (TEXT_CHAR - ']')+ )? ;

# resp-text       = ["[" resp-text-code "]" SP] text

resp_textP = ( '[' resp_text_code ']' SP )?  textNB >buffer_start %buffer_finish

          |
# work around Cyrus 2.3.13 bug where it directly sends a CRLF
# after the first ']', i.e.
#
# ./imap/imapd.c:    prot_printf(imapd_out, "* OK [URLMECH INTERNAL]\r\n");
#
# they fixed it in later versions, e.g. in 2.4.17
#
# ./imap/index.c:    prot_printf(state->out, "* OK [URLMECH INTERNAL] Ok\r\n");

  '[URLMECH INTERNAL]'
  ;

# more general work around because
# GMAIL GImap also does it wrong for other codes, e.g.:
#
# "* OK [HIGHESTMODSEQ 15336]\r\n"

resp_text = '[' resp_text_code ']' @buffer_start @buffer_finish
                ( SP textNB >buffer_start %buffer_finish )?
          | textNB >buffer_start %buffer_finish
;

# resp-cond-bye   = "BYE" SP resp-text

resp_cond_bye = /BYE/i       >status_bye >cb_untagged_status_begin
                SP resp_text ;

# resp-cond-state = ("OK" / "NO" / "BAD") SP resp-text
#                     ; Status condition

resp_cond_state = ( /OK/i %status_ok | /NO/i %status_no | /BAD/i %status_bad )
                  SP resp_text ;

#response-fatal  = "*" SP resp-cond-bye CRLF
#                    ; Server closes connection immediately

# response_fatal = '*' SP resp_cond_bye CRLF ;

# response-tagged = tag SP resp-cond-state CRLF

response_tagged = tag                  >tag_start %tag_finish
                  SP                   @cb_tagged_status_begin
                  resp_cond_state CRLF @cb_tagged_status_end ;

# response-done   = response-tagged / response-fatal

# response_done = response_tagged | response_fatal ;


# addr-adl        = nstring
#                     ; Holds route from [RFC-2822] route-addr if
#                     ; non-NIL

addr_adl = nstring ;

# addr-host       = nstring
#                     ; NIL indicates [RFC-2822] group syntax.
#                     ; Otherwise, holds [RFC-2822] domain name

addr_host = nstring ;

# addr-mailbox    = nstring
#                     ; NIL indicates end of [RFC-2822] group; if
#                     ; non-NIL and addr-host is NIL, holds
#                     ; [RFC-2822] group name.
#                     ; Otherwise, holds [RFC-2822] local-part
#                     ; after removing [RFC-2822] quoting

addr_mailbox = nstring ;

# addr-name       = nstring
#                     ; If non-NIL, holds phrase from [RFC-2822]
#                     ; mailbox after removing [RFC-2822] quoting

addr_name = nstring ;

# address         = "(" addr-name SP addr-adl SP addr-mailbox SP
#                    addr-host ")"

address = '(' addr_name SP addr_adl SP addr_mailbox SP addr_host ')' ;

# env-bcc         = "(" 1*address ")" / nil

env_bcc         = '(' address+ ')' | nil ;

# env-cc          = "(" 1*address ")" / nil

env_cc          = '(' address+ ')' | nil ;

# env-date        = nstring

env_date        = nstring ;

# env-from        = "(" 1*address ")" / nil

env_from        = '(' address+ ')' | nil ;

# env-in-reply-to = nstring

env_in_reply_to = nstring ;

# env-message-id  = nstring

env_message_id  = nstring ;

# env-reply-to    = "(" 1*address ")" / nil

env_reply_to    = '(' address+ ')' | nil ;

# env-sender      = "(" 1*address ")" / nil

env_sender      = '(' address+ ')' | nil ;

# env-subject     = nstring

env_subject     = nstring ;

#env-to          = "(" 1*address ")" / nil

env_to          = '(' address+ ')' | nil ;

# envelope        = "(" env-date SP env-subject SP env-from SP
#                   env-sender SP env-reply-to SP env-to SP env-cc SP
#                   env-bcc SP env-in-reply-to SP env-message-id ")"

envelope = '(' env_date   SP env_subject     SP env_from       SP
               env_sender SP env_reply_to    SP env_to         SP env_cc SP
               env_bcc    SP env_in_reply_to SP env_message_id ')' ;


# body-extension  = nstring / number /
#                   "(" body-extension *(SP body-extension) ")"
#                    ; Future expansion.  Client implementations
#                    ; MUST accept body-extension fields.  Server
#                    ; implementations MUST NOT generate
#                    ; body-extension fields except as defined by
#                    ; future standard or standards-track
#                    ; revisions of this specification.

# body-fld-lang   = nstring / "(" string *(SP string) ")"

# body-fld-loc    = nstring

# body-fld-octets = number

# body-fld-enc    = (DQUOTE ("7BIT" / "8BIT" / "BINARY" / "BASE64"/
#                   "QUOTED-PRINTABLE") DQUOTE) / string

# body-fld-desc   = nstring

# body-fld-id     = nstring

# body-fld-lines  = number

# body-fld-param  = "(" string SP string *(SP string SP string) ")" / nil

# body-fld-md5    = nstring

# body-fld-dsp    = "(" string SP body-fld-param ")" / nil

# body-ext-mpart  = body-fld-param [SP body-fld-dsp [SP body-fld-lang
#                   [SP body-fld-loc *(SP body-extension)]]]
#                   ; MUST NOT be returned on non-extensible
#                   ; "BODY" fetch

# media-subtype   = string
#                    ; Defined in [MIME-IMT]

# body-type-mpart = 1*body SP media-subtype
#                   [SP body-ext-mpart]

# body-ext-1part  = body-fld-md5 [SP body-fld-dsp [SP body-fld-lang
#                   [SP body-fld-loc *(SP body-extension)]]]
#                    ; MUST NOT be returned on non-extensible
#                    ; "BODY" fetch

# media-message   = DQUOTE "MESSAGE" DQUOTE SP DQUOTE "RFC822" DQUOTE
#                    ; Defined in [MIME-IMT]

# media-text      = DQUOTE "TEXT" DQUOTE SP media-subtype
#                    ; Defined in [MIME-IMT]


# body-fields     = body-fld-param SP body-fld-id SP body-fld-desc SP
#                   body-fld-enc SP body-fld-octets

# body-type-text  = media-text SP body-fields SP body-fld-lines

# body-type-msg   = media-message SP body-fields SP envelope
#                   SP body SP body-fld-lines

# media-basic     = ((DQUOTE ("APPLICATION" / "AUDIO" / "IMAGE" /
#                  "MESSAGE" / "VIDEO") DQUOTE) / string) SP
#                  media-subtype
#                    ; Defined in [MIME-IMT]

# body-type-basic = media-basic SP body-fields
#                    ; MESSAGE subtype MUST NOT be "RFC822"

# body-type-1part = (body-type-basic / body-type-msg / body-type-text)
#                   [SP body-ext-1part]

# body            = "(" (body-type-1part / body-type-mpart) ")"

# XXX parse nested structures body structure
body = ( '(' | ')' | SP | string | nil | number )+ ;



# msg-att-static  = "ENVELOPE" SP envelope / "INTERNALDATE" SP date-time /
#                   "RFC822" [".HEADER" / ".TEXT"] SP nstring /
#                   "RFC822.SIZE" SP number /
#                   "BODY" ["STRUCTURE"] SP body /
#                   "BODY" section ["<" number ">"] SP nstring /
#                   "UID" SP uniqueid
#                     ; MUST NOT change for a message

msg_att_static = /ENVELOPE/i     SP envelope
               | /INTERNALDATE/i SP date_time
               | /RFC822/i ( /.HEADER/i | /.TEXT/i )? SP nstring
               | /RFC822.SIZE/i SP number
               | /BODY/i (/STRUCTURE/i)? SP body
               | /BODY/i section ( '<' number '>' )?
                   SP      @cb_body_section_inner
                   nstring %cb_body_section_end
               | /UID/i SP uniqueid %cb_uid ;

# msg-att-dynamic = "FLAGS" SP "(" [flag-fetch *(SP flag-fetch)] ")"
#                    ; MAY change for a message

msg_att_dynamic = /FLAGS/i SP '(' (      flag_fetch
                                     (SP flag_fetch)* )? ")"  ;

# msg-att         = "(" (msg-att-dynamic / msg-att-static)
#                    *(SP (msg-att-dynamic / msg-att-static)) ")"

msg_att = '('      ( msg_att_dynamic | msg_att_static )
              ( SP ( msg_att_dynamic | msg_att_static ) )* ')' ;

# message-data    = nz-number SP ("EXPUNGE" / ("FETCH" SP msg-att))

message_data =
  nz_number SP
   (   /EXPUNGE/i  %cb_data_expunge |
     ( /FETCH/i SP @cb_data_fetch_begin msg_att %cb_data_fetch_end ) ) ;

# mbx-list-sflag  = "\Noselect" / "\Marked" / "\Unmarked"
#                     ; Selectability flags; only one per LIST response

mbx_list_sflag  = '\\' ( /Noselect/i | /Marked/i | /Unmarked/i ) ;

# mbx-list-oflag  = "\Noinferiors" / flag-extension
#                     ; Other flags; multiple possible per LIST response

mbx_list_oflag  = /\\Noinferiors/i | ( flag_extension - /\\Noinferiors/i ) ;

# mbx-list-flags  = *(mbx-list-oflag SP) mbx-list-sflag
#                   *(SP mbx-list-oflag) /
#                   mbx-list-oflag *(SP mbx-list-oflag)

mbx_list_flags  = (mbx_list_oflag SP)* mbx_list_sflag (SP mbx_list_oflag)*
                | mbx_list_oflag (SP mbx_list_oflag)* ;

# mailbox-list    = "(" [mbx-list-flags] ")" SP
#                    (DQUOTE QUOTED-CHAR DQUOTE / nil) SP mailbox

# QUOTED_CHAR is the hierarchy delimiter, nil means no hierarchy/flat
mailbox_list    = '(' (mbx_list_flags)? ')' SP
                   (DQUOTE QUOTED_CHAR DQUOTE | nil) SP mailbox ;

# status-att-list =  status-att SP number *(SP status-att SP number)

status_att_list = status_att SP number (SP status_att SP number)* ;

# mailbox-data    =  "FLAGS" SP flag-list / "LIST" SP mailbox-list /
#                    "LSUB" SP mailbox-list / "SEARCH" *(SP nz-number) /
#                    "STATUS" SP mailbox SP "(" [status-att-list] ")" /
#                    number SP "EXISTS" / number SP "RECENT"

mailbox_data    = /FLAGS/i  SP
                    @cb_data_flags_begin flag_list %cb_data_flags_end
                | /LIST/i   SP mailbox_list
                | /LSUB/i   SP mailbox_list
                | /SEARCH/i (SP nz_number)*
                | /STATUS/i SP mailbox SP '(' (status_att_list)? ')'
                | number SP ( /EXISTS/i %cb_data_exists |
                              /RECENT/i %cb_data_recent   )
                ;

# RFC 2971 - IMAP4 ID extension

# id_response ::= "ID" SPACE id_params_list

id_response = /ID/i SP id_params_list
  ;


# response-data   = "*" SP (resp-cond-state / resp-cond-bye /
#                  mailbox-data / message-data / capability-data) CRLF

#response_data = '*' SP (   resp_cond_state
#                         | resp_cond_bye
#                         | mailbox_data
#                         | message_data
#                         | capability_data
#                       ) CRLF ;

response_data_tail_wolf =  (  resp_cond_state
                         #already referenced via response_fatal
                         #| resp_cond_bye
                         | mailbox_data
                         | message_data
                         | capability_data
                         # RFC 2971 - IMAP4 ID extension
                         | id_response
                       #) CRLF ;
                       ) CR ;


# base64-char     = ALPHA / DIGIT / "+" / "/"
#                    ; Case-sensitive

base64_char = ALPHA | DIGIT | '+' | '/' ;

# base64-terminal = (2base64-char "==") / (3base64-char "=")

base64_terminal = base64_char {2} '==' | base64_char {3} '=' ;

# base64          = *(4base64-char) [base64-terminal]

base64 = ( base64_char {4} ) * base64_terminal ? ;

# continue-req    = "+" SP (resp-text / base64) CRLF

# XXX overlapping?
continue_req_tail := SP ( resp_text | base64 ) CRLF @return ;

# response        = *(continue-req / response-data) response-done

#response = ( continue_req | response_data )* response_done ;

#responses = response + ;

# ragel state chart
responses =
  start: (
    '+' @call_continue_req_tail -> start    |
    '*' SP                      -> untagged |
    # the tagged response_done part
    response_tagged             -> start
  ),
  untagged: (
      # CR LF instead of CRLF is used on purpose
      # such that @action is not executed two times
    resp_cond_bye CR LF        @cb_untagged_status_end      -> start |
    response_data_tail_wolf LF @cb_untagged_status_end      -> start
  );


main := responses ;

prepush {
  if (unsigned(top) == stack_vector_.size()) {
    stack_vector_.push_back(0);
    stack = stack_vector_.data();
  }
}


}%%

namespace IMAP {

  namespace Client {

    using namespace Memory;

    %% write data;

    Lexer::Lexer(Buffer::Base &buffer,
      Buffer::Base &tag_buffer,
      Callback::Base &cb)
      :
        buffer_(buffer), tag_buffer_(tag_buffer), cb_(cb)
    {
      %% write init;
    }

    void Lexer::read(const char *begin, const char *end)
    {
      const char *p  = begin;
      const char *pe = end;
      Buffer::Resume bur(buffer_, p, pe);
      Buffer::Resume tar(tag_buffer_, p, pe);
      Buffer::Resume nur(number_buffer_, p, pe);
      %% write exec;
      if (cs == %%{write error;}%%) {
        throw_lex_error("IMAP client automaton in error state", begin, p, pe);
      }
    }

    bool Lexer::in_start() const
    {
      return cs == %%{write start;}%%;
    }
    bool Lexer::finished() const
    {
      // return cs >= %%{write first_final;}%%;
      // for this machine: start state == final state
      return in_start();
    }

    void Lexer::verify_finished() const
    {
      if (!finished())
        throw runtime_error("IMAP client automaton not in final state");
    }

    // e.g. for mailbox/maildir we want to convert - which is the default
    void Lexer::set_convert_crlf(bool b)
    {
      convert_crlf_ = b;
    }

  }

}


