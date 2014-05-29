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

%%{

# Base64 Decoder
#
# i.e. implements RFC2045,
# Multipurpose Internet Mail Extensions (MIME) Part One:
# Format of Internet Message Bodies,
# 6.8. Base64 Content-Transfer-Encoding
# http://tools.ietf.org/html/rfc2045#section-6.8

machine base64_decoder;

# {{{ Actions

action set_base64_val_Alph
{
  uint32_t i = fc - 'A';
  base64_value_ = i;
}
action set_base64_val_alph
{
  uint32_t i = fc - 'a';
  base64_value_ = i + 26;
}
action set_base64_val_num
{
  uint32_t i = fc - '0';
  base64_value_ = i + 52;
}
action set_base64_val_plus
{
  base64_value_ = 62;
}
action set_base64_val_slash
{
  base64_value_ = 63;
}

action push_unit
{
  uint32_t i = base64_value_;
  unit_ <<= 6;
  unit_ |= i;
}
action push_pad
{
  unit_ <<= 6;
}
action finish_unit
{
  array<char, 3> a{};
  for (unsigned i = 3; i>0; --i) {
    a[i-1] = unit_ & 0xffu;
    unit_ >>= 8;
  }
  conv_buffer_.cont(a.begin());
  conv_buffer_.stop(a.end());
}
action finish_pad1
{
  array<char, 3> a{};
  unit_ >>= 8;
  for (unsigned i = 2; i>0; --i) {
    a[i-1] = unit_ & 0xffu;
    unit_ >>= 8;
  }
  conv_buffer_.cont(a.begin());
  conv_buffer_.stop(a.end()-1);
}
action finish_pad2
{
  array<char, 3> a{};
  unit_ >>= 16;
  a[0] = unit_ & 0xffu;
  conv_buffer_.cont(a.begin());
  conv_buffer_.stop(a.begin()+1);
}

# }}}

pad = '=' @push_pad
  ;


base64_alph1 = ('A'..'Z') @set_base64_val_Alph
  ;
base64_alph2 = ('a'..'z') @set_base64_val_alph
  ;
base64_alph3 = ('0'..'9') @set_base64_val_num
  ;
base64_alph4 = '+' @set_base64_val_plus
             | '/' @set_base64_val_slash
  ;

base64_alph =
  ( base64_alph1 | base64_alph2 | base64_alph3 | base64_alph4 ) @push_unit
  ;


b_encoded_word =
  start: (
    base64_alph                  -> s2
    # don't allow empty encoded words
    #(print-base64_alph) @hold_it -> final
  ),
  b_start: (
    base64_alph                  -> s2    |
    (print-base64_alph) @hold_it -> final
  ),
  s2: (
    base64_alph -> s3
  ),
  s3: (
    base64_alph -> s4 |
    pad         -> e4
  ),
  s4: (
    base64_alph @finish_unit     -> b_start |
    pad         @finish_pad1     -> final
  ),
  e4: (
    pad         @finish_pad2     -> final
  )
;

}%%

