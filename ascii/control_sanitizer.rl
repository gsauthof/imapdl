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

#include <ascii/control_sanitizer.h>

using namespace std;


%%{

machine control_sanitizer;

# {{{ Actions

action push_ctl
{
  seen_ctl_ = true;
  char s[5] = "\\x";
  uint8_t l = uint8_t(fc) >> 4;
  l += (l>9) ? uint8_t('A') - 10u : uint8_t('0');
  s[2] = l;
  uint8_t r = fc & 0xf;
  r += (r>9) ? uint8_t('A') - 10u : uint8_t('0');
  s[3] = r;
  u_buffer_.cont(s);
  u_buffer_.stop(s + 4);
}
action cont_u
{
  u_buffer_.cont(p);
}
action stop_u
{
  u_buffer_.stop(p);
}
# }}}

include abnf "rfc/abnf.rl";

escape_controls =
  start:(
    CTL        @push_ctl -> start |
    (any-CTL)  @cont_u   -> unescaped
  ),
  unescaped:(
    CTL @stop_u @push_ctl -> start |
    (any-CTL)             -> unescaped
  )
;

main := escape_controls ;

}%%


namespace ASCII {
  namespace Control {

    %% write data;

    Sanitizer::Sanitizer(Memory::Buffer::Base &dst)
      :
        u_buffer_(dst)
    {
      %% write init;
    }
    void Sanitizer::read(const char *begin, const char *end)
    {
      using namespace Memory;
      const char *p  = begin;
      const char *pe = end;
      Buffer::Resume r(u_buffer_, p, pe);
      %% write exec;
    }
    bool Sanitizer::seen_ctl() const
    {
      return seen_ctl_;
    }

  }
}
