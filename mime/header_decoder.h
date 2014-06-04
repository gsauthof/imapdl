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
#ifndef MIME_HEADER_DECODER_H
#define MIME_HEADER_DECODER_H

#include <functional>
#include <stdint.h>

#include <buffer/buffer.h>

namespace MIME {
  namespace Header {

    void default_convert(const std::pair<const char*, const char*> &charset,
        const std::pair<const char*, const char*> &lang,
        const std::pair<const char*, const char*> &inp,
        std::string &out);

    class Decoder {
      public:
        enum class Ending { BOTH, LF, CRLF };
        using Callback_Fn = std::function<void()>;
        using Convert_Fn  = std::function<
          void (const std::pair<const char*, const char*> &charset,
              const std::pair<const char*, const char*> &lang,
              const std::pair<const char*, const char*> &inp,
              std::string &out)
          >;
      private:
        int                     cs            {0};
        Memory::Buffer::Base   &field_buffer_;
        Memory::Buffer::Base   &buffer_;
        Memory::Buffer::Vector &charset_buffer_;
        Memory::Buffer::Vector default_charset_buffer_;
        Memory::Buffer::Vector &language_buffer_;
        Memory::Buffer::Vector  default_language_buffer_;
        Memory::Buffer::Vector  space_buffer_;
        Memory::Buffer::Vector  conv_buffer_;
        Ending                  ending_       {Ending::BOTH};
        Callback_Fn             callback_fn_;
        Convert_Fn              convert_fn_;

        // base64
        uint32_t                unit_         {0};
        uint32_t                base64_value_ {0};
        // q
        char                    q_value_      {0};

      public:
        Decoder(
            Memory::Buffer::Base &field_buffer,
            Memory::Buffer::Base &buffer,
            Memory::Buffer::Vector &charset_buffer,
            Memory::Buffer::Vector &language_buffer,
            Callback_Fn fn);
        Decoder(
            Memory::Buffer::Base &field_buffer,
            Memory::Buffer::Base &buffer,
            Callback_Fn fn,
            Convert_Fn convert_fn = default_convert);
        void clear();
        void read(const char *begin, const char *end);

        bool in_start() const;
        bool in_final() const;
        bool finished() const;
        void verify_finished() const;

        void set_ending_policy(Ending e);
    };

  }
}

#endif
