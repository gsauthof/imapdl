// Copyright 2015, Georg Sauthoff <mail@georg.so>

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

machine literal_converter;

# On the fly convert carriage-return + newline to just newline
# during parsing.
#
# Assume that it is not called for empty literals.
#
# This is e.g. needed while storing maildir messages
# that are retrieved via (IMAP).

action lit_ret
{
  ++literal_pos_;
  if (literal_pos_ == number_) {
    if (*p == '\n' || *p == '\r') {
      buffer_.cont(p);
      buffer_.stop(p+1);
    } else {
      buffer_.finish(p+1);
    }
    fret;
  }
}
action add_cr
{
  char c = '\r';
  buffer_.cont(&c);
  buffer_.stop(&c+1);
}
action add_lf
{
  char c = '\n';
  buffer_.cont(&c);
  buffer_.stop(&c+1);
}

convert_literal_tail =
  start: (
    (CHAR8 - (CR|LF)) @lit_ret @buffer_cont         -> s2    |
    CR                @lit_ret                      -> s3
  ),
  s2: (
    (CHAR8 - (CR|LF)) @lit_ret                      -> s2    |
    CR                @lit_ret @buffer_stop         -> s3
  ),
  s3: (
    LF                @lit_ret @add_lf              -> start |
    (CHAR8 - (CR|LF)) @lit_ret @add_cr @buffer_cont -> s2    |
    CR                @lit_ret @add_cr              -> s3
  );

}%%
