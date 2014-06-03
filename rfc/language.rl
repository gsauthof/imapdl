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

machine language;

# RFC 5646 Tags for Identifying Languages
# http://tools.ietf.org/html/rfc5646
#
# (replaces RFC 1766)
#
# The relevant RFC grammar rules are included as comments, i.e. above each
# Ragel rule the relevant formal syntax snippet (in ABNF) is quoted as comment.
# For a description of the used augmented backus-naur form see RFC2234
# (http://tools.ietf.org/html/rfc2234) - which also contains some basic
# grammar rules used.

# include this before including this file:
# include abnf "rfc/abnf.rl";


# extlang       = 3ALPHA              ; selected ISO 639 codes
#                 *2("-" 3ALPHA)      ; permanently reserved

extlang = ALPHA{3} ( '-' ALPHA{3} ){0,2}
  ;

# language      = 2*3ALPHA            ; shortest ISO 639 code
#                 ["-" extlang]       ; sometimes followed by
#                                     ; extended language subtags
#               / 4ALPHA              ; or reserved for future use
#               / 5*8ALPHA            ; or registered language subtag

language = ALPHA{2,3} ('-' extlang )?
         | ALPHA{4}
         | ALPHA{5,8}
  ;


# script        = 4ALPHA              ; ISO 15924 code

script = ALPHA{4}
  ;

#  region        = 2ALPHA              ; ISO 3166-1 code
#               / 3DIGIT              ; UN M.49 code

region = ALPHA {2}
       | DIGIT{3}
  ;

# variant       = 5*8alphanum         ; registered variants
#              / (DIGIT 3alphanum)

variant = alphanum{5,8}
        | DIGIT alphanum{3}
  ;

#                                      ; Single alphanumerics
#                                     ; "x" reserved for private use
# singleton     = DIGIT               ; 0 - 9
#               / %x41-57             ; A - W
#               / %x59-5A             ; Y - Z
#               / %x61-77             ; a - w
#               / %x79-7A             ; y - z

singleton = DIGIT | [0-9] | ([A-Z]-'X') | ([a-z]-'x')
  ;

# extension     = singleton 1*("-" (2*8alphanum))

extension = singleton ('-' alphanum{2,8})+
  ;

# privateuse    = "x" 1*("-" (1*8alphanum))

privateuse = 'x' ('-' (alphanum{1,8}))+
  ;

#  langtag       = language
#                 ["-" script]
#                 ["-" region]
#                 *("-" variant)
#                 *("-" extension)
#                 ["-" privateuse]

langtag = language
          ('-' script)?
          ('-' region)?
          ('-' variant)*
          ('-' extension)*
          ('-' privateuse)?
  ;

#  irregular     = "en-GB-oed"         ; irregular tags do not match
#               / "i-ami"             ; the 'langtag' production and
#               / "i-bnn"             ; would not otherwise be
#               / "i-default"         ; considered 'well-formed'
#               / "i-enochian"        ; These tags are all valid,
#               / "i-hak"             ; but most are deprecated
#               / "i-klingon"         ; in favor of more modern
#               / "i-lux"             ; subtags or subtag
#               / "i-mingo"           ; combination
#               / "i-navajo"
#               / "i-pwn"
#               / "i-tao"
#               / "i-tay"
#               / "i-tsu"
#               / "sgn-BE-FR"
#               / "sgn-BE-NL"
#               / "sgn-CH-DE"

irregular    = 'en-GB-oed'
             | 'i-ami'
             | 'i-bnn'
             | 'i-default'
             | 'i-enochian'
             | 'i-hak'
             | 'i-klingon'
             | 'i-lux'
             | 'i-mingo'
             | 'i-navajo'
             | 'i-pwn'
             | 'i-tao'
             | 'i-tay'
             | 'i-tsu'
             | 'sgn-BE-FR'
             | 'sgn-BE-NL'
             | 'sgn-CH-DE'
  ;

#  regular       = "art-lojban"        ; these tags match the 'langtag'
#               / "cel-gaulish"       ; production, but their subtags
#               / "no-bok"            ; are not extended language
#               / "no-nyn"            ; or variant subtags: their meaning
#               / "zh-guoyu"          ; is defined by their registration
#               / "zh-hakka"          ; and all of these are deprecated
#               / "zh-min"            ; in favor of a more modern
#               / "zh-min-nan"        ; subtag or sequence of subtags
#               / "zh-xiang"

regular        = 'art-lojban'
               | 'cel-gaulish'
               | 'no-bok'
               | 'no-nyn'
               | 'zh-guoyu'
               | 'zh-hakka'
               | 'zh-min'
               | 'zh-min-nan'
               | 'zh-xiang'
  ;

# grandfathered = irregular           ; non-redundant tags registered
#               / regular             ; during the RFC 3066 era

grandfathered = irregular
              | regular
  ;

# Language-Tag  = langtag             ; normal language tags
#               / privateuse          ; private use tag
#               / grandfathered       ; grandfathered tags

language_tag = langtag
             | privateuse
             | grandfathered
  ;

}%%


