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
#include "ssl_verification.h"

#include <boost/log/sources/record_ostream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>
namespace asio = boost::asio;

using namespace std;

namespace Net {

  namespace SSL {

    Verification::Verification(
        boost::log::sources::severity_logger<Log::Severity > &lg,
        const string &hostname, const string &fingerprint)
      :
        lg_(lg),
        default_(hostname),
        fingerprint_(boost::to_upper_copy<std::string>(fingerprint))
    {
    }
    bool Verification::operator()(bool preverified, asio::ssl::verify_context &ctx)
    {
      ++pos_;

      bool r = default_(preverified, ctx);

      if (!r) {
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        {
          array<char, 128> char_buffer = {{0}};
          X509_NAME_oneline(X509_get_subject_name(cert),
              char_buffer.data(), char_buffer.size());
          BOOST_LOG(lg_) << "Certificate subject: " << char_buffer.data();
        }

        auto digest_type = EVP_sha1();
        {
          array<unsigned char, 128> buffer = {{0}};
          unsigned size = buffer.size();
          int r = X509_digest(cert, digest_type, buffer.data(), &size);
          (void)r;
          ostringstream fp;
          boost::algorithm::hex(buffer.data(), buffer.data() + size,
              ostream_iterator<char>(fp));
          BOOST_LOG(lg_) << "SHA1 fingerprint of server certificate: " << fp.str();
          if (result_)
            return result_;
          if (!fingerprint_.empty()) {
            BOOST_LOG(lg_) << "Verifying fingerprint ...";
            if (pos_ == 1) {
              result_ = fingerprint_ == fp.str();
              if (result_) {
                BOOST_LOG(lg_) << "Fingerprint matches. Authentication finished.";
              } else
                BOOST_LOG(lg_) << "Given fingerprint " << fingerprint_ << " does not"
                  " match the one of the certificate: " << fp.str();
            }
            return result_;
          }
        }

      }

      return r;
    }

  }
}
