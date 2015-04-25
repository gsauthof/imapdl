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
#ifndef MAILDIR_H
#define MAILDIR_H

#include <string>
#include <random>
#include <ostream>
#include <stddef.h> 

class Maildir {
  private:
    std::string  path_;
    std::string  name_;
    std::string  flags_;
    std::string  tmp_dir_name_;
    int          tmp_dir_fd_   {-1};
    int          new_dir_fd_   {-1};
    int          cur_dir_fd_   {-1};
    size_t       delivery_     {0};
    std::mt19937 g;

    void add_time       (std::ostream &o);
    void add_delivery_id(std::ostream &o);
    void add_hostname   (std::ostream &o);
    void set_flags(const std::string &flags);
    void move(int new_or_cur_fd);
  public:
    Maildir(const Maildir &) =delete;
    Maildir &operator=(const Maildir &) =delete;

    Maildir(const std::string &path, bool create_it = true);
    ~Maildir();

    int tmp_dir_fd();

    std::string create_tmp_name();
    void create_tmp_name(std::string &dirname, std::string &filename);
    void create_tmp_name(std::string &filename);

    void move_to_new();
    void move_to_cur(const std::string &flags = std::string());
    void clear();
};

#endif
