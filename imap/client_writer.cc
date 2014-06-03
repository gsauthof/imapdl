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
#include "client_writer.h"

#include <iomanip>
#include <limits>
using namespace std;

#include <boost/interprocess/streams/vectorstream.hpp>
namespace bi = boost::interprocess;
#include <boost/regex.hpp>

namespace IMAP {

  namespace Client {

    Tag::Tag(const std::string &prefix, unsigned width)
      : prefix_(prefix), width_(width)
    {
    }
    void Tag::next(string &tag, Command command)
    {
      if (command_set_.find(command) != command_set_.end()) {
        ostringstream t;
        t << "Command " << command << " is still active.";
        throw logic_error(t.str());
      }
      command_set_.insert(command);
      buffer_.clear();
      buffer_.seekp(0);
      buffer_ << prefix_ << setw(width_) << setfill('0') << value_;
      tag = std::move(buffer_.str());
      auto p = map_.insert(make_pair(tag, command));
      if (!p.second) {
        ostringstream t;
        t << "Tag " << p.first->first << " already inserted.";
        throw logic_error(t.str());
      }
      ++value_;
    }
    void Tag::pop(const std::string &tag)
    {
      auto i = map_.find(tag);
      if (i == map_.end()) {
        stringstream t;
        t << "Trying to pop unknown tag: " << tag;
        throw logic_error(t.str());
      }
      size_t r = command_set_.erase(i->second);
      if (!r) {
        stringstream t;
        t << "Command " << i->second << " for tag " << tag << " unknown";
        throw logic_error(t.str());
      }
      map_.erase(i);
    }


    Writer::Writer(Tag &tag, Write_Fn write_fn)
      :
        parser_(buffer_, tag_buffer_, null_cb_),
        generate_(tag),
        write_fn_(write_fn)
    {
    }
    void Writer::write(std::vector<char> &v)
    {
      // to verify that we send conforming IMAP commands
      parser_.read(v.data(), v.data()+v.size());
      if (write_fn_)
        write_fn_(v);
    }
    void Writer::nullary(Command c, string &tag)
    {
      generate_.next(tag, c);
      v_.clear();
      stream_.swap_vector(v_);
      stream_ << tag << ' ' << c << "\r\n";
      stream_.swap_vector(v_);
      write(v_);
    }
    void Writer::command_start(Command c, string &tag)
    {
      generate_.next(tag, c);
      v_.clear();
      stream_.swap_vector(v_);
      stream_ << tag << ' ' << c << ' ';
    }
    void Writer::command_finish()
    {
      stream_ << "\r\n";
      stream_.swap_vector(v_);
      write(v_);
    }
    void Writer::capability(string &tag)
    {
      nullary(Command::CAPABILITY, tag);
    }
    void Writer::noop(string &tag)
    {
      nullary(Command::NOOP, tag);
    }
    void Writer::logout(string &tag)
    {
      nullary(Command::LOGOUT, tag);
    }
    void Writer::login(const std::string &user,
        const std::string &password, string &tag)
    {
      command_start(Command::LOGIN, tag);
      write_cond_literal(user);
      stream_ << ' ';
      write_cond_literal(password);
      command_finish();
    }
    void Writer::list(const std::string &reference,
        const std::string &mailbox, string &tag)
    {
      command_start(Command::LIST, tag);
      write_literal(reference);
      stream_ << ' ';
      write_literal(mailbox);
      command_finish();
    }
    void Writer::select(const std::string &mailbox, string &tag)
    {
      command_start(Command::SELECT, tag);
      stream_ << mailbox;
      command_finish();
    }
    void Writer::examine(const std::string &mailbox, string &tag)
    {
      command_start(Command::EXAMINE, tag);
      stream_ << mailbox;
      command_finish();
    }
    void Writer::close(string &tag)
    {
      nullary(Command::CLOSE, tag);
    }
    void Writer::expunge(string &tag)
    {
      nullary(Command::EXPUNGE, tag);
    }
    void Writer::write_literal(const string &s)
    {
      stream_ << '{' << s.size() << "}\r\n" << s;
    }
    void Writer::write_cond_literal(const string &s)
    {
      static boost::regex re("^[A-Za-z0-9]+$", boost::regex::extended);
      if (boost::regex_search(s, re))
        stream_ << s;
      else
        write_literal(s);
    }
    void Writer::write_sequence_nr(uint32_t nz)
    {
      if (!nz)
        throw logic_error("zero sequence number not allowed");
      if (nz == numeric_limits<uint32_t>::max())
        stream_ << '*';
      else
        stream_ << nz;
    }
    void Writer::write_sequence(const std::pair<uint32_t, uint32_t> &seq)
    {
      if (seq.first == seq.second) {
        write_sequence_nr(seq.first);
      } else {
        write_sequence_nr(seq.first);
        stream_ << ':';
        write_sequence_nr(seq.second);
      }
    }
    void Writer::write_sequence_set(
        const std::vector<std::pair<uint32_t, uint32_t> > &sequence_set)
    {
      if (sequence_set.empty())
        throw logic_error("sequence must not be empty");
      auto i = sequence_set.begin();
      write_sequence(*i);
      ++i;
      for (;i!=sequence_set.end(); ++i) {
        stream_ << ',';
        write_sequence(*i);
      }
    }
    void Writer::uid_expunge(
        const std::vector<std::pair<uint32_t, uint32_t> > &sequence_set,
        string &tag)
    {
      command_start(Command::UID_EXPUNGE, tag);
      write_sequence_set(sequence_set);
      command_finish();
    }
    void Writer::fetch(const vector<std::pair<uint32_t, uint32_t> > &sequence_set,
            const std::vector<Fetch_Attribute> &as, string &tag)
    {
      if (as.empty())
        throw logic_error("empty fetch attribute list not allowed");
      command_start(Command::FETCH, tag);
      write_sequence_set(sequence_set);
      stream_ << ' ';
      if (as.size() == 1) {
        stream_ << as.front();
      } else {
        stream_ << '(';
        auto i = as.begin();
        stream_ << *i;
        ++i;
        for (; i != as.end(); ++i) {
          stream_ << ' ' << *i;
        }
        stream_ << ')';
      }
      command_finish();
    }
    void Writer::write_flags(const std::vector<IMAP::Flag> &flags)
    {
      if (flags.empty())
        throw std::runtime_error("empty flag list not allowed");
      auto i = flags.begin();
      stream_ << '\\' << *i;
      ++i;
      for (; i != flags.end(); ++i)
        stream_ << " \\" << *i;
    }
    void Writer::uid_store(
        const std::vector<std::pair<uint32_t, uint32_t> > &sequence_set,
        const std::vector<IMAP::Flag> &flags,
        std::string &tag,
        Store_Mode mode,
        bool silent
        )
    {
      command_start(Command::UID_STORE, tag);
      write_sequence_set(sequence_set);
      stream_ << ' ';
      stream_ << mode;
      stream_ << "FLAGS";
      if (silent)
        stream_ << ".SILENT";
      stream_ << ' ';
      write_flags(flags);
      command_finish();
    }

  }

}
