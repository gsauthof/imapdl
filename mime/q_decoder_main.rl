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

# Frontend such that it can be tested separately

machine q_main;


include q_decoder "mime/q_decoder.rl";

q_main :=  q_encoded_word %!conv_buffer_stop ;

}%%

namespace MIME {
  namespace Q {

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
      using namespace Memory;
      const char *p  = begin;
      const char *pe = end;
      const char *eof {nullptr};
      (void)eof;
      Buffer::Resume conv_buffer_resume(conv_buffer_, p, pe);
      %% write exec;
      if (cs == %%{write error;}%%) {
        if (   !sentinel_.empty()
            && size_t(pe-p) >= sentinel_.size()
            && string(p, sentinel_.size()) == sentinel_) {
          next_ = p;
          cs = %%{write start;}%%;
          return;
        }
        throw_lex_error("Q decoder in error state", begin, p, pe);
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
        throw runtime_error("Q decoder not in final state");
    }
    const char *Decoder::next() const
    {
      (void)q_main_first_final;
      (void)q_main_error;
      (void)q_main_en_q_main;

      return next_;
    }
  }
}
