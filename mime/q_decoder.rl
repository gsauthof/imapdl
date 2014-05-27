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

#include <mime/q_decoder.h>

#include <lex_util.h>

using namespace std;


%%{

# implements RFC2047, 4.2 The "Q" Encoding:
# http://tools.ietf.org/html/rfc2047#section-4.2

machine q_decoder;

# {{{ Actions

action clear_hex
{
  q_value_ = 0;
}
action push_dec
{
  q_value_ <<= 4;
  uint8_t i = fc;
  i -= '0';
  q_value_ |= i & 0xffu;
}
action push_hex
{
  q_value_ <<= 4;
  uint8_t i = fc;
  i -= 'a';
  i += 10;
  q_value_ |= i & 0xffu;
}
action push_Hex
{
  q_value_ <<= 4;
  uint8_t i = fc;
  i -= 'A';
  i += 10;
  q_value_ |= i & 0xffu;
}
action end_hex
{
  conv_buffer_.cont(&q_value_);
  conv_buffer_.stop(&q_value_ + 1);
}
action push_underscore
{
  char c = 0x20; 
  conv_buffer_.cont(&c);
  conv_buffer_.stop(&c + 1);
}
action conv_buffer_start
{
  conv_buffer_.start(p);
}
action conv_buffer_cont
{
  conv_buffer_.cont(p);
}
action conv_buffer_stop
{
  conv_buffer_.stop(p);
}

# }}}

# [ \t] are not printable, but they are included to document the requirement
q_specials  =  [=?_ \t]
  ;

q_printable = 0x21..0x7E
  ;

q_alph = q_printable - q_specials
  ;

q_word = (q_alph q_alph**) >conv_buffer_cont %conv_buffer_stop %^conv_buffer_stop
  ;

# RFC specifies that uppercase 'A'..'F' "should" be used, but apparently
# there are implementations that have good reasons to use lowercase ones
q_hex = ('0'..'9') @push_dec
      | ('A'..'F') @push_Hex
      | ('a'..'f') @push_hex
  ;

q_quoted   = '=' @clear_hex q_hex q_hex @end_hex
  ;

underscore = '_' @push_underscore
  ;

q_encoded_word = ( q_word | q_quoted | underscore )* >conv_buffer_start
  ;

}%%

