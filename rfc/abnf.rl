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

machine abnf;

# Basic ABNF rules defined in RFC2234 (http://tools.ietf.org/html/rfc2234).
#
# The grammar includes rules which are used by various RFC formal syntax
# descriptions.
#
# The relevant grammar rules are included as comments, i.e. above each
# Ragel rule the relevant formal syntax snippet (in ABNF) is quoted as comment.
# For a description of the used augmented backus-naur form see RFC2234
# (http://tools.ietf.org/html/rfc2234).
#
# May also contain other (useful) basic rules defined in other RFCs (e.g. CHAR8
# which is defined in RFC3501 - IMAP4v1).


# RFC2234
#         CHAR           =  %x01-7F
#                                ; any 7-bit US-ASCII character,
#                                  excluding NUL

CHAR = 0x01 .. 0x7f ;

# RFC3501
# CHAR8           = %x01-ff
#                     ; any OCTET except NUL, %x00

CHAR8 = [^\0] ;

# RFC2234
# SP             =  %x20

SP   = ' ' ;

SPACE = SP ;

# RFC2234
# HTAB           =  %x09
#                               ; horizontal tab

HTAB =  0x09
  ;

# RFC2234
# WSP            =  SP / HTAB
#                               ; white space

WSP =  SP | HTAB
 ;

# RFC2234
# DIGIT          =  %x30-39
#                               ; 0-9

DIGIT = [0-9] ;

# RFC2234
# ALPHA          =  %x41-5A / %x61-7A   ; A-Z / a-z

ALPHA =  [A-Za-z] ;


# RFC2234
#         CTL            =  %x00-1F / %x7F
#                                ; controls

CTL = 0x0 .. 0x1f  | 0x7f ;


CR = '\r' ;
LF = '\n' ;

# RFC2234
# CRLF        =  %d13.10

CRLF = CR LF ;


# RFC2234
# DQUOTE         =  %x22
#                                ; " (Double Quote)

DQUOTE = '"' ;



# RFC3501
# digit-nz        = %x31-39
#                    ; 1-9

digit_nz = [1-9] ;


# RFC3501
# TEXT-CHAR       = <any CHAR except CR and LF>

TEXT_CHAR = CHAR - [\r\n] ;


}%%


