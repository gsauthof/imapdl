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

#include <mime/header_decoder.h>

#include <lex_util.h>
#include <iostream>
using namespace std;

#include <buffer/buffer.h>

%%{

# implements RFC2047,
# MIME (Multipurpose Internet Mail Extensions) Part Three:
# Message Header Extensions for Non-ASCII Text
# 4.2 The "Q" Encoding:
# http://tools.ietf.org/html/rfc2047#section-4.2
#
# and header related parts of RFC2822,
# Internet Message Format
# 2.2. Header Fields
# http://tools.ietf.org/html/rfc2822#section-2.2

# and language related updates from RFC2231
# MIME Parameter Value and Encoded Word Extensions:
# Character Sets, Languages, and Continuations
# http://tools.ietf.org/html/rfc2231

machine header_decoder;

# {{{ Actions


action field_start
{
  field_buffer_.start(p);
}
action field_finish
{
  field_buffer_.finish(p);
}
action buffer_clear
{
  buffer_.clear();
}
action buffer_cont
{
  buffer_.cont(p);
}
action buffer_start
{
  buffer_.start(p);
}
action buffer_finish
{
  buffer_.finish(p);
}
action buffer_stop
{
  buffer_.stop(p);
}
action buffer_eq_cont
{
  char c = '=';
  buffer_.cont(&c);
  buffer_.stop(&c+1);
  buffer_.cont(p);
}
action charset_start
{
  charset_buffer_.start(p);
}
action charset_finish
{
  charset_buffer_.finish(p);
}
action language_tag_start
{
  language_buffer_.start(p);
}
action language_tag_finish
{
  language_buffer_.finish(p);
}
action space_start
{
  space_buffer_.start(p);
}
action space_finish
{
  space_buffer_.finish(p);
}
action cb_header
{
  callback_fn_();
}
action clear_trail_space
{
  space_buffer_.clear();
}
action ins_trail_space
{
  if (!space_buffer_.empty()) {
    buffer_.cont(space_buffer_.begin());
    buffer_.stop(space_buffer_.end());
    space_buffer_.clear();
  }
}
action hold_it
{
  fhold;
}
action convert_buffer
{
  string s;
  convert_fn_(charset_buffer_.range(), language_buffer_.range(), conv_buffer_.range(), s);
  buffer_.cont(s.data());
  buffer_.stop(s.data()+s.size());
  conv_buffer_.clear();
}
action check_cr
{
  if (ending_ == Ending::LF)
    throw_lex_error("Header decoder in LF-only mode", begin, p, pe);
}
action check_lf
{
  if (ending_ == Ending::CRLF)
    throw_lex_error("Header decoder in CRLF mode", begin, p, pe);
}

# }}}

include abnf           "rfc/abnf.rl";
include language       "rfc/language.rl";
include base64_decoder "mime/base64_decoder.rl";
include q_decoder      "mime/q_decoder.rl";

field_body_alph = CHAR - (CR | LF | [=] )
  ;

# RFC2047
# especials = "(" / ")" / "<" / ">" / "@" / "," / ";" / ":" / "
#               <"> / "/" / "[" / "]" / "?" / "." / "="

especials = '(' | ')' | '<' | '>' | '@' | ',' | ';' | ':' |
               '"' | '/' | '[' | ']' | '?' | '.' | '='
  ;


# RFC2047
# token = 1*<Any CHAR except SPACE, CTLs, and especials>

token_char = CHAR - (SPACE | CTL | especials)
  ;

token = token_char+
  ;

# RFC2047
# charset = token    ; see section 3

charset = token >charset_start %charset_finish
  ;

# RFC2231
# language := <registered language tag [RFC-1766]>

# RFC 1766 was obsoleted by RFC 5646 - there the axiom is language_tag

# RFC2047
# encoding = token   ; see section 4

# case insensitive!
#encoding = [Qq] | [Bb]
#  ;

#encoded-text = 1*<Any printable ASCII character other than "?"
#                     or SPACE>
#                  ; (but see "Use of encoded-words in message
#                  ; headers", section 5)


# RFC2047
# encoded-word = "=?" charset "?" encoding "?" encoded-text "?="

#encoded_word = '=?' charset '?' encoding '?' encoded_text '?='
#  ;

# RFC2231
# encoded-word := "=?" charset ["*" language] "?" encoded-text "?="

#########################
# definitions for testing - the real ones are defined in mime/{q,base64}_decoder.rl:
#q_encoded_word = ([^?=]+) >buffer_resume %buffer_stop ;
#b_encoded_word = [^?=]+;
#########################

encoded_word_end = '?='
  ;

encoded_word_mid =
   ( [Qq] '?' q_encoded_word
   | [Bb] '?' b_encoded_word )
   encoded_word_end @convert_buffer
  ;

encoded_word_tail = '?' @clear_trail_space charset ('*'
  language_tag >language_tag_start % language_tag_finish )? '?' encoded_word_mid
  ;

#encoded_word = '=' encoded_word_tail
#  ;



# RFC 2822
#FWS             =       ([*WSP CRLF] 1*WSP) /   ; Folding white space
#                        obs-FWS

# RFC2822
# ftext           =       %d33-57 /               ; Any character except
#                         %d59-126                ;  controls, SP, and
#                                                 ;  ":".

ftext = 33..57 | 59..126
  ;

fa = (field_body_alph - WSP)
  ;

# RFC2822
# field-name      =       1*ftext

field_name = (ftext+) >field_start %field_finish
  ;

#header_field = field_name ': ' field_body
#  ;

#header = (header_field )+
#  ;

header =
  start: (
    field_name ': ' @buffer_clear -> fb_start
  ),
  # work around 'resolves to multiple entry points' error
  start_header: (
    field_name ': ' @buffer_clear  -> fb_start
  ),
  fb_start: (
    WSP   @space_start     -> ws_state          |
    fa    @buffer_cont     -> w_state           |
    '='                    -> ew_state          |
    CR    @check_cr        -> cr_state          |
    LF    @check_lf        -> lf_state
  ),
  ws_state: (
    WSP                    -> ws_state          |
    '='   @space_finish
          @ins_trail_space -> ew_state          |
    fa    @space_finish
          @ins_trail_space
          @buffer_cont     -> w_state           |
    CR    @check_cr
          @space_finish    -> cr_state          |
    LF    @check_lf
          @space_finish    -> lf_state
  ),
  w_state: (
    WSP   @buffer_stop
          @space_start     -> ws_state          |
    '='   @buffer_stop     -> ew_state          |
    fa                     -> w_state           |
    CR    @check_cr
          @buffer_stop     -> cr_state          |
    LF    @check_lf
          @buffer_stop     -> lf_state
  ),
  ew_state: (
    encoded_word_tail      -> ew_tail_state     |
    (fa - '?')
          @buffer_eq_cont  -> w_state
  ),
  ew_tail_state: (
    # application of the robustness principle:
    # allow directly adjacent encoded words (which are not allowed by RFC 2047)
    '='                    -> ew_state          |
    WSP   @space_start     -> after_ew_ws_state |
    CR    @check_cr        -> cr_state          |
    LF    @check_lf        -> lf_state          |
    # perhaps not every alph is allowed after an encoded word ...
    # perhaps only closing comment ')'
    fa    @buffer_cont     -> w_state
  ),
  after_ew_ws_state: (
    WSP                    -> after_ew_ws_state |
    '='                    -> ew_state          |
    fa    @space_finish
          @ins_trail_space
          @buffer_cont     -> w_state           |
    CR    @check_cr        -> cr_state          |
    LF    @check_lf        -> lf_state
  ),
  cr_state: (
    LF                     -> lf_state
  ),
  lf_state: (
    WSP   @ins_trail_space
          @space_start     -> foldmark_state    |
    ftext @ins_trail_space
          @hold_it
          # yields: 'state reference start resolves to multiple entry points' error
          #@cb_header       -> start |
          @cb_header       -> start_header      |
    (0|LF)@ins_trail_space
          @cb_header       -> final
  ),
  foldmark_state: (
    WSP                    -> foldmark_state    |
    fa    @space_finish
          @ins_trail_space
          @buffer_cont     -> w_state           |
    '='                    -> ew_state          #|
    # only whitespace after a foldmark is not allowed
    #CR                     -> final             |
    #LF                     -> final
  )
;


header_main :=  header ;

}%%

#include <boost/locale/encoding.hpp>

#include <ascii/control_sanitizer.h>

namespace MIME {
  namespace Header {
    void default_convert(const std::pair<const char*, const char*> &charset,
        const std::pair<const char*, const char*> &lang,
        const std::pair<const char*, const char*> &inp,
        std::string &out)
    {
      string s(inp.first, inp.second);
      string cs(charset.first, charset.second);
      string result(boost::locale::conv::to_utf<char>(s, cs));

      Memory::Buffer::Vector v;
      ASCII::Control::Sanitizer sani(v);
      sani.read(result.data(), result.data() + result.size());
      if (sani.seen_ctl()) {
        // no std::move(), string() already is an r-value
        out = string(v.begin(), v.end());
      } else {
        out = std::move(result);
      }
    }

    %% write data;

    Decoder::Decoder(
            Memory::Buffer::Base &field_buffer,
            Memory::Buffer::Base &buffer,
            Memory::Buffer::Vector &charset_buffer,
            Memory::Buffer::Vector &language_buffer,
            Callback_Fn fn)
      :
        field_buffer_(field_buffer),
        buffer_(buffer),
        charset_buffer_(charset_buffer),
        language_buffer_(language_buffer),
        callback_fn_(fn),
        convert_fn_(default_convert)
    {
      %% write init;
    }
    Decoder::Decoder(
            Memory::Buffer::Base &field_buffer,
            Memory::Buffer::Base &buffer,
            Callback_Fn fn,
            Convert_Fn convert_fn)
      :
        field_buffer_(field_buffer),
        buffer_(buffer),
        charset_buffer_(default_charset_buffer_),
        language_buffer_(default_language_buffer_),
        callback_fn_(fn),
        convert_fn_(convert_fn)
    {
      %% write init;
    }
    void Decoder::clear()
    {
      %% write init;
      field_buffer_.clear();
      buffer_.clear();
      charset_buffer_.clear();
      language_buffer_.clear();
      space_buffer_.clear();
    }
    void Decoder::read(const char *begin, const char *end)
    {
      using namespace Memory;
      const char *p  = begin;
      const char *pe = end;
      const char *eof {nullptr};
      (void)eof;
      Buffer::Resume fbr(field_buffer_, p, pe);
      Buffer::Resume bur(buffer_, p, pe);
      Buffer::Resume chr(charset_buffer_, p, pe);
      Buffer::Resume lbr(language_buffer_, p, pe);
      Buffer::Resume spr(space_buffer_, p, pe);
      %% write exec;
      if (cs == %%{write error;}%%) {
        throw_lex_error("Header decoder in error state", begin, p, pe);
      }
    }
    bool Decoder::in_start() const
    {
      return cs == %%{write start;}%%;
    }
    bool Decoder::in_final() const
    {
      return cs >= %%{write first_final;}%%;
    }
    bool Decoder::finished() const
    {
      return in_start() || in_final();
    }
    void Decoder::verify_finished() const
    {
      if (!finished())
        throw runtime_error("Q decoder not in final state");
    }
    void Decoder::set_ending_policy(Ending e)
    {
      (void)header_decoder_first_final;
      (void)header_decoder_error;
      (void)header_decoder_en_header_main;

      ending_ = e;
    }
  }
}
