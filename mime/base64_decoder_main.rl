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

#include <mime/base64_decoder.h>

#include <lex_util.h>

using namespace std;

// frontend for base64_decoder, such that in can be tested on its own
// (directives like %%write make problems when including the file, thus
//  it has to be split into two parts)

%%{

machine base64_decoder_main;

# {{{ Actions

action hold_it
{
  fhold;
}

# }}}

include base64_decoder "mime/base64_decoder.rl";

base64_main :=  b_encoded_word ;

}%%

namespace MIME {
  namespace Base64 {

    %% write data;

    Decoder::Decoder(Memory::Buffer::Base &buffer, const string &sentinel)
      :
        conv_buffer_(buffer),
        sentinel_(sentinel)
    {
      %% write init;
    }
    void Decoder::read(const char *begin, const char *end)
    {
      const char *p  = begin;
      const char *pe = end;
      %% write exec;
      if (cs == %%{write error;}%%) {
        if (   !sentinel_.empty()
            && size_t(pe-p) >= sentinel_.size()
            && string(p, sentinel_.size()) == sentinel_) {
          cs = %%{write start;}%%;
          next_ = p;
          return;
        }
        throw_lex_error("Base64 decoder in error state", begin, p, pe);
      }
      next_ = pe;
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
        throw runtime_error("Base64 decoder not in final state");
    }
    const char *Decoder::next() const
    {
      (void)base64_decoder_main_first_final;
      (void)base64_decoder_main_error;
      (void)base64_decoder_main_en_base64_main;

      return next_;
    }
  }
}
