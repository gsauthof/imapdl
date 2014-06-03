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
#ifndef IMAP_COPY_OPTIONS_H
#define IMAP_COPY_OPTIONS_H

#include <net/tcp_client.h>

#include <string>
#include <ostream>

namespace IMAP {
  namespace Copy {
    enum class Task : unsigned {
      FIRST_,
      DOWNLOAD,
      FETCH_HEADER,
      LIST,
      LAST_
    };
    class Options : public Net::TCP::SSL::Client::Options {
      public:
        Options();
        Options(int argc, char **argv);
        void fix();
        void verify();
        void check_configfile();
        void load();
        std::ostream &print(std::ostream &o) const;

        std::string logfile;
        bool        use_ssl        {true};
        std::string account;
        std::string configfile;
        std::string mailbox;
        std::string maildir;
        bool        del            {false};
        std::string username;
        std::string password;
        unsigned    greeting_wait  {100};
        unsigned    simulate_error {0};
        std::string journal_file;
        bool        fetch_header_only {true};
        bool        list           {true};
        std::string list_reference;
        std::string list_mailbox;

        Task        task           {Task::DOWNLOAD};

    };
    std::ostream &operator<<(std::ostream &o, const Options &opts);
  }
}

#endif
