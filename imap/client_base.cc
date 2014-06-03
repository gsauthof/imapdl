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
#include "client_base.h"

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/named_scope.hpp>

#include "exception.h"

namespace IMAP {

  namespace Client {

    Base::Base(
            Write_Fn write_fn,
            boost::log::sources::severity_logger< Log::Severity > &lg
        )
      :
        lg_(lg),
        write_fn_(write_fn),
        writer_(tags_, std::bind(&Base::to_cmd, this, std::placeholders::_1))
    {
    }


    void Base::to_cmd(vector<char> &x)
    {
      std::swap(x, cmd_);
    }
    void Base::do_write()
    {
      write_fn_(cmd_);
    }

    void Base::async_capabilities(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.capability(tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Getting CAPABILITIES ..." << " [" << tag << ']';
      do_write();
    }

    void Base::async_login(const std::string &username, const std::string &password,
        std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.login(username, password, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Logging in as |" << username << "| [" << tag << "]";
      BOOST_LOG_SEV(lg_, Log::INSANE) << "Password: |" << password << "|";
      do_write();
    }
    void Base::async_list(const std::string &reference, const std::string &mailbox,
        std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.list(reference, mailbox, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG_SEV(lg_, Log::DEBUG) << "Listing: |" << reference << "| |" << mailbox << "|";
      do_write();
    }
    void Base::async_select(const std::string &mailbox, std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.select(mailbox, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Selecting mailbox: |" << mailbox << "|" << " [" << tag << ']';
      do_write();
    }

    void Base::async_fetch(
            const std::vector<std::pair<uint32_t, uint32_t> > &set,
            const std::vector<IMAP::Client::Fetch_Attribute> &atts,
            std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.fetch(set, atts, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Fetching messages " <<  " ..." << " [" << tag << ']';
      do_write();
    }

    void Base::async_store(
            const std::vector<std::pair<uint32_t, uint32_t> > &set,
            const std::vector<IMAP::Flag> &flags,
            std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.uid_store(set, flags, tag, IMAP::Client::Store_Mode::REPLACE, true);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Storing DELETED flags ..." << " [" << tag << ']';
      do_write();
    }
    void Base::async_uid_expunge(const std::vector<std::pair<uint32_t, uint32_t> > &set,
        std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.uid_expunge(set, tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Expunging messages ..." << " [" << tag << ']';
      do_write();
    }
    void Base::async_expunge(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.expunge(tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Expunging messages (without UIDPLUS) ..." << " [" << tag << ']';
      do_write();
    }

    void Base::async_logout(std::function<void(void)> fn)
    {
      BOOST_LOG_FUNCTION();
      string tag;
      writer_.logout(tag);
      tag_to_fn_[tag] = fn;
      BOOST_LOG(lg_) << "Logging out ..." << " [" << tag << ']';
      //state_ = State::LOGGING_OUT;
      do_write();
    }
    void Base::imap_tagged_status_end(IMAP::Server::Response::Status c)
    {
      BOOST_LOG_FUNCTION();
      string tag(tag_buffer_.begin(), tag_buffer_.end());
      BOOST_LOG(lg_) << "Got status " << c << " for tag " << tag;
      if (c != IMAP::Server::Response::Status::OK) {
        stringstream o;
        o << "Command failed: " << c << " - " << string(buffer_.begin(), buffer_.end());
        THROW_MSG(o.str());
      }
      auto i = tag_to_fn_.find(tag);
      if (i == tag_to_fn_.end()) {
        stringstream o;
        o << "Got unknown tag: " << tag;
        THROW_MSG(o.str());
      }
      tags_.pop(tag);
      auto fn = i->second;
      tag_to_fn_.erase(i);
      fn();
    }



  }
}


