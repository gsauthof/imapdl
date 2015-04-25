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
#include "maildir.h"

#include <utility>
#include <sstream>
#include <random>
#include <array>
#include <exception>
#include <stdexcept>
using namespace std;

#include <sys/types.h>
#include <unistd.h>

#include <boost/algorithm/string/replace.hpp> 
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/io/ios_state.hpp>

#include <ixxx/ixxx.h>
using namespace ixxx;

// http://en.wikipedia.org/wiki/Maildir
// http://cr.yp.to/proto/maildir.html

Maildir::Maildir(const string &path, bool create_it)
  :
    path_(path),
    // should use something like /dev/urandom or RdRand
    g(random_device()())
{
  const array<const char*, 3> subs = {{ "cur", "new", "tmp" }};
  for (auto x : subs) {
    string p(path);
    p += '/';
    p += x;
    if (create_it)
      fs::create_directories(p);
    else {
      if (!fs::exists(p)) {
        ostringstream o;
        o << "Maildir sub directory does not exist: " << p;
        throw std::runtime_error(o.str());
      }
    }
  }
  ostringstream p;
  p << path_ << '/';
  p << "tmp";
  tmp_dir_name_ = p.str();
  int dir_fd  = posix::open(path, O_RDONLY);
  tmp_dir_fd_ = posix::openat(dir_fd, "tmp", O_RDONLY);
  new_dir_fd_ = posix::openat(dir_fd, "new", O_RDONLY);
  cur_dir_fd_ = posix::openat(dir_fd, "cur", O_RDONLY);
  posix::close(dir_fd);
}
Maildir::~Maildir()
{
  try {
    posix::close(tmp_dir_fd_);
    posix::close(new_dir_fd_);
    posix::close(cur_dir_fd_);
  } catch (std::exception) {
  }
}

int Maildir::tmp_dir_fd()
{
  return tmp_dir_fd_;
}

void Maildir::add_time(ostream &o)
{
  time_t t = ansi::time(nullptr);
  o << t;
}

void Maildir::add_delivery_id(ostream &o)
{
  boost::io::ios_flags_saver ifs(o);
  o << "P" << ::getpid() << "Q" << delivery_ << "R" << hex << g();
  ++delivery_;
}

void Maildir::add_hostname(ostream &o)
{
  array<char, 256> b = {{0}};
  posix::gethostname(b.data(), b.size()-1);
  string s(b.data());
  boost::replace_all(s, "/", "\\057");
  boost::replace_all(s, ":", "\\072");
  o << s;
}

void Maildir::create_tmp_name(std::string &filename)
{
  if (!name_.empty())
    throw std::runtime_error("last tmp name not delivered - call commit()");

  ostringstream o;
  add_time(o);
  o << '.';
  add_delivery_id(o);
  o << '.';
  add_hostname(o);
  name_ = o.str();

  filename = name_;
}

void Maildir::create_tmp_name(std::string &dirname, std::string &filename)
{
  dirname = tmp_dir_name_;
  create_tmp_name(filename);
}

string Maildir::create_tmp_name()
{
  string d, f;
  create_tmp_name(d, f);
  ostringstream p;
  p << d << '/' << f;
  return p.str();
}

void Maildir::set_flags(const string &flags)
{
  string f(flags);
  sort(f.begin(), f.end());
  auto end = unique(f.begin(), f.end());
  f.resize(distance(f.begin(), end));

  // Flag "P" (passed): the user has resent/forwarded/bounced this message to someone else.
  // Flag "R" (replied): the user has replied to this message.
  // Flag "S" (seen): the user has viewed this message, though perhaps he didn't read all the way through it.
  // Flag "T" (trashed): the user has moved this message to the trash; the trash will be emptied by a later user action.
  // Flag "D" (draft): the user considers this message a draft; toggled at user discretion.
  // Flag "F" (flagged): user-defined flag; toggled at user discretion. 

  const char all[] = "DFPRST";
  if (!includes(all, all+sizeof(all)-1, f.begin(), f.end()))
  {
    ostringstream o;
    o << "Unknown flag in: " << f << " (allowed: " << all << ")";
    throw std::runtime_error(o.str());
  }
  flags_ = f;
}

void Maildir::move_to_cur(const string &flags)
{
  set_flags(flags);
  move(cur_dir_fd_);
}
void Maildir::move_to_new()
{
  move(new_dir_fd_);
}
void Maildir::move(int new_or_cur_fd)
{
  if (name_.empty())
    throw std::runtime_error("no tmp name created");

  string new_name(name_);
  if (new_or_cur_fd == cur_dir_fd_) {
    new_name += ":2,";
    new_name += flags_;
  }

  posix::linkat(tmp_dir_fd_, name_, new_or_cur_fd, new_name, 0);
  // assuming same logic as with open/creat ...
  posix::fsync(new_or_cur_fd);
  posix::unlinkat(tmp_dir_fd_, name_, 0);
  name_.clear();
  flags_.clear();
}

void Maildir::clear()
{
  name_.clear();
  flags_.clear();
}

