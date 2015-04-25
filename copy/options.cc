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
#include "options.h"

#include "id.h"

#include <exception.h>
#include <net/ssl_util.h>
#include <ixxx/ansi.h>

#include <unordered_set>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include <string.h>
#include <stdlib.h>
#include <cstring.h>


#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace std;
using namespace ixxx;
using namespace Net::SSL;

namespace OPT {
  static const char HELP_S[]         = "help,h"        ;
  static const char HELP[]           = "help"          ;

  static const char HOST[]           = "host"          ;
  static const char SERVICE[]        = "port"          ;
  static const char LOCAL_ADDRESS[]  = "bind"          ;
  static const char LOCAL_PORT[]     = "lport"         ;
  static const char IP[]             = "ip"            ;

  static const char FINGERPRINT[]    = "fp"            ;
  static const char CIPHER[]         = "cipher"        ;
  static const char CIPHER_PRESET[]  = "cipher_preset" ;
  static const char USE_SSL[]        = "ssl"           ;
  static const char CA_FILE[]        = "ca"            ;
  static const char CA_PATH[]        = "ca_path"       ;
  static const char CERT_HOST[]      = "cert_host"     ;
  static const char TLS1[]           = "tls1"          ;

  static const char TRACEFILE[]      = "trace"         ;
  static const char LOGFILE[]        = "log"           ;
//  static const char SEVERITY[]       = "verbose"       ;
  static const char SEVERITY_S[]     = "verbose,v"     ;
  static const char FILE_SEVERITY[]  = "log_v"         ;

  static const char CONFIGFILE[]     = "config"        ;

  static const char ACCOUNT[]        = "account"       ;
//  static const char DELETE[]         = "delete"        ;
  static const char DELETE_S[]       = "delete,d"      ;
  static const char MAILBOX[]        = "mailbox"       ;
  static const char MAILDIR[]        = "maildir"       ;
  static const char GREETING_WAIT[]  = "gwait"         ;
  static const char SIMULATE_ERROR[] = "sim_error"     ;
  static const char JOURNAL_FILE[]   = "journal"       ;
  static const char FETCH_HEADER[]   = "header"        ;
  static const char LIST[]           = "list"          ;
  static const char LIST_REFERENCE[] = "list_reference";
  static const char LIST_MAILBOX[]   = "list_mailbox"  ;
}

namespace KEY {
  static const char USERNAME[]      = "username"      ;
  static const char PASSWORD[]      = "password"      ;

  static const char IP[]            = "ip"            ;
  static const char LOCAL_ADDRESS[] = "bind"          ;
  static const char LOCAL_PORT[]    = "lport"         ;
  static const char HOST[]          = "host"          ;
  static const char SERVICE[]       = "port"          ;

  static const char SSL[]           = "ssl"           ;
  static const char FINGERPRINT[]   = "fingerprint"   ;
  static const char CIPHER[]        = "cipher"        ;
  static const char CIPHER_PRESET[] = "cipher_preset" ;
  static const char CA_FILE[]       = "ca"            ;
  static const char CA_PATH[]       = "ca_path"       ;
  static const char CERT_HOST[]     = "cert_host"     ;
  static const char TLS1[]          = "tls1"          ;

  static const char DELETE[]        = "delete"        ;
  static const char MAILBOX[]       = "mailbox"       ;
  static const char MAILDIR[]       = "maildir"       ;
  static const char JOURNAL_FILE[]   = "journal"       ;

  static const unordered_set<const char*> set = {
    USERNAME,
    PASSWORD,
    IP,
    LOCAL_ADDRESS,
    LOCAL_PORT,
    HOST,
    SERVICE,

    SSL,
    FINGERPRINT,
    CIPHER,
    CIPHER_PRESET,
    CA_FILE,
    CA_PATH,
    CERT_HOST,
    TLS1,

    DELETE,
    MAILBOX,
    MAILDIR,
    JOURNAL_FILE
  };
}

namespace IMAP {
  namespace Copy {
    Options::Options()
    {
    }

    static void help(const char *argv0, const po::options_description &desc,
        std::ostream &o)
    {
      o << "call: " << argv0
        //<< " OPTION* HOST MAILDIR\n"
        << " OPTION*\n"
        << desc << "\n\n"
        << ID::argv0 << ' ' << ID::version << ", " << ID::author
        << " <" << ID::mail << ">, " << ID::date << ", " << ID::licence << "\n\n";
        ;
    }

    class Options_Priv : public Options {
      private:
        void add_general_opts(po::options_description &general_group);
        void add_net_opts    (po::options_description &net_group);
        void add_ssl_opts    (po::options_description &ssl_group);
        void add_test_opts   (po::options_description &test_group);
        void add_imap_opts   (po::options_description &imap_group);

      public:
        void add_groups      (po::options_description &visible_group);
    };


    Options::Options(int argc, char **argv)
    {
      po::options_description hidden_group;
      //hidden_group.add_options()
      //  (OPT::HOST, po::value<string>(&host)->required(), "remote host")
      //  (OPT::MAILDIR, po::value<string>(&maildir)->required(), "maildir destination")
      //  ;

      po::options_description visible_group;
      Options_Priv *d = new (this) Options_Priv;
      d->add_groups(visible_group);
      po::options_description all;
      all.add(visible_group);
      all.add(hidden_group);

      //po::positional_options_description pdesc;
      //pdesc.add(OPT::HOST, 1);
      //pdesc.add(OPT::MAILDIR, 1);
      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv)
          .options(all)
          //.positional(pdesc)
          .run(), vm);

      if (vm.count(OPT::HELP)) {
        help(*argv, visible_group, cout);
        exit(0);
      }
      if (vm.count(OPT::CONFIGFILE))
        configfile = vm[OPT::CONFIGFILE].as<string>();
      if (vm.count(OPT::ACCOUNT))
        account = vm[OPT::ACCOUNT].as<string>();
      load();
      po::notify(vm);

      fix();
      verify();
    }

    void Options_Priv::add_groups(po::options_description &visible_group)
    {
      po::options_description general_group("General Options");
      add_general_opts(general_group);
      po::options_description net_group("Net Options");
      add_net_opts(net_group);
      po::options_description ssl_group("SSL Options");
      add_ssl_opts(ssl_group);
      po::options_description test_group("Test Options");
      add_test_opts(test_group);
      po::options_description imap_group("IMAP Options");
      add_imap_opts(imap_group);

      visible_group.add(general_group);
      visible_group.add(net_group);
      visible_group.add(ssl_group);
      visible_group.add(test_group);
      visible_group.add(imap_group);
    }

    // some default_value()/required_value() are commented out
    // because the defaults are set when reading the RC file
    // and required values may also come from the RC file.
    // Required values are checked in verify().
    void Options_Priv::add_general_opts(po::options_description &general_group)
    {
      general_group.add_options()
        (OPT::HELP_S, "this help screen")
        (OPT::TRACEFILE, po::value<string>(&tracefile)->default_value(""),
           "trace file for capturing send/received messages")
        (OPT::LOGFILE, po::value<string>(&logfile)->default_value(""),
           "also write log messages to a file")
        (OPT::SEVERITY_S,
         po::value<unsigned>(&severity)
           ->default_value(4)
           ->implicit_value(5), //->value_name("bool"),
           "verbosity  level, 0 means nothing - higher means more")
        (OPT::FILE_SEVERITY,
         po::value<unsigned>(&file_severity)
           ->default_value(0, "same as non-file")
           ->implicit_value(7), //->value_name("bool"),
           "default verbosity for log file, level, 0 means nothing - higher means more")
        ;
    }
    void Options_Priv::add_net_opts(po::options_description &net_group)
    {
      net_group.add_options()
        (OPT::IP, po::value<unsigned>(&ip)
           //->default_value(4),
           , "IP version - 4 or 6 (default: 4)")
        (OPT::LOCAL_ADDRESS, po::value<string>(&local_address)
           //->default_value(""),
           , "local address to bind to (default: system default)")
        (OPT::LOCAL_PORT, po::value<unsigned short>(&local_port)
           //->default_value(0),
           , "local port to bind to - 0 means automatic (default: 0)")
        (OPT::SERVICE, po::value<string>(&service)
           //->default_value("", "imaps or imap"),
           , "remote service name or port - usually imaps=993 (SSL) and imap=143"
           " (default: imaps or imap)")
        ;
    }
    void Options_Priv::add_ssl_opts(po::options_description &ssl_group)
    {
      ssl_group.add_options()
        (OPT::USE_SSL,
         po::value<bool>(&use_ssl)
           //->default_value(true, "true")
           ->implicit_value(true, "true")->value_name("bool"),
           "use SSL/TLS (default: true)")
        (OPT::FINGERPRINT, po::value<string>(&fingerprint)
           //->default_value(""),
           , "verify certificate using a known fingerprint (instead of a CA) (default: \"\")")
        (OPT::CA_FILE, po::value<string>(&ca_file)
           //->default_value("server.crt"),
           , "file containing CA/server certificate (default: \"\")")
        (OPT::CA_PATH, po::value<string>(&ca_path)
           //->default_value(""),
           , "directory contained hashed CA certs")
        (OPT::CERT_HOST, po::value<string>(&cert_host)
           //->default_value(""),
           , "hostname used for certificate checking - "
           "if not set the connecting hostname is used")
        (OPT::CIPHER, po::value<string>(&cipher)
           //->default_value(""),
           , "openssl cipher list - default: only ones with forward secrecy (default: \"\")")
        (OPT::CIPHER_PRESET, po::value<unsigned>(&cipher_preset)
           //->default_value(1),
           , "cipher list presets: 1: forward secrecy, 2: TLSv1.2, 3: old, "
           "smaller is better (default: 1)")
        (OPT::TLS1, po::value<bool>(&tls1)
           //->default_value(true, "true")
           ->implicit_value(true, "true"),
           "enable/disable use of TLSv1 - disabling means that only TLSv1.1/TLSv1.2 "
           "are allowed. (default: true)")
        ;
    }
    void Options_Priv::add_test_opts(po::options_description &test_group)
    {
      test_group.add_options()
        (OPT::SIMULATE_ERROR,
           po::value<unsigned>(&simulate_error)
           ->default_value(0)
           , "simulate an error before fetching the n-th message")
        ;
    }
    void Options_Priv::add_imap_opts(po::options_description &imap_group)
    {
      imap_group.add_options()
        (OPT::ACCOUNT, po::value<string>(&account)->default_value("default"),
           "account name - is used to find section in configuration file")
        (OPT::CONFIGFILE,
           po::value<string>(&configfile)
           ->default_value("", "$HOME/.config/" + string(ID::argv0) + "/rc.json"),
           "configuration file where account credentials etc. are read from")
        (OPT::MAILBOX,
           po::value<string>(&mailbox)
           //->default_value("INBOX"),
           , "configuration file where account credentials etc. are read from (default: INBOX)")
        (OPT::GREETING_WAIT,
           po::value<unsigned>(&greeting_wait)
           ->default_value(100)
           , "time (in msec) to wait for untagged capabilities after connect "
             "(before sending a capabilities command)")
        (OPT::DELETE_S,
           po::value<bool>(&del)
           //->default_value(false, "false")
           ->implicit_value(true, "true"),
           "delete fetched messages on the server (default: false)")
        (OPT::HOST, po::value<string>(&host)
           //->required(),
           , "remote host")
        (OPT::MAILDIR, po::value<string>(&maildir)
           //->required(),
           , "maildir destination")
        (OPT::JOURNAL_FILE, po::value<string>(&journal_file)
         ->default_value("", "$HOME/.config/"  + string(ID::argv0) + "/$ACCOUNT.journal"),
           "write already fetched and not yet expunged messages to a journal "
           "for expunging on the next connect")
        (OPT::FETCH_HEADER, po::value<bool>(&fetch_header_only)
         ->default_value(false, "false")
         ->implicit_value(true, "true")
         , "fetch (and display) only header fields")
        (OPT::LIST, po::value<bool>(&list)
         ->default_value(false, "false")
         ->implicit_value(true, "true")
         , "execute IMAP LIST on the server (without fetching anything)")
        (OPT::LIST_REFERENCE, po::value<string>(&list_reference)
         , "LIST reference argument")
        (OPT::LIST_MAILBOX, po::value<string>(&list_mailbox)
         ->default_value("%")
         , "LIST mailbox argument")
        ;
    }

    void Options::fix()
    {
      if (maildir.substr(0, 2) == "~/")
        maildir = ansi::getenv("HOME") + maildir.substr(1);
      if (cert_host.empty())
        cert_host = host;
      if (cipher.empty())
        cipher = Cipher::default_list(Cipher::to_class(cipher_preset));
      if (!(ip == 4 || ip == 6)) {
        ostringstream o;
        o << "Invalid IP version: " << ip;
        THROW_MSG(o.str());
      }
      if (service.empty()) {
        service = use_ssl ? "imaps" : "imap";
      }
      if (!file_severity)
        file_severity = severity;
      if (journal_file.empty()) {
        ostringstream o;
        o << ansi::getenv("HOME") << "/.config/" << ID::argv0 << '/'
          << account << ".journal";
        journal_file = o.str();
      }
      if (fetch_header_only)
        task = Task::FETCH_HEADER;
      if (list)
        task = Task::LIST;
    }
    void Options::verify()
    {
      if (host.empty())
        throw runtime_error("No host specified on the command line/in the rc file");
      if (maildir.empty())
        throw runtime_error("No maildir specified on the command line/in the rc file");
    }

    static const char default_rc_file[] =
R"({
  "default":
  {
    "username"      : "juser",
    "password"      : "secretvery"
  },
  "2nd_example_account":
  {
    "username"      : "juser",
    "password"      : "muchsecret",
    "host"          : "imap.example.org",
    "fingerprint"   : "BAFFBAFFBAFFBAFFBAFFBAFFBAFFBAFFBAFFBAFF",
    "cipher_preset" : 3,
    "maildir"       : "~/maildir/example",
    "delete"        : true
  },
  "3rd_example_account":
  {
    "username"      : "juser",
    "password"      : "muchsecret",
    "host"          : "imap.example.org",
    "ca"            : "some/path",
    "tls1"          : false,
    "maildir"       : "~/maildir/example",
    "delete"        : true
  },
  "4th_example_account":
  {
    "username"      : "juser",
    "password"      : "muchsecret",
    "host"          : "imap.example.org",
    "ca_path"       : "some/path",
    "maildir"       : "~/maildir/example",
    "delete"        : true
  }
}
)";

    static void write_default_rc(const std::string &filename)
    {
      if (fs::exists(filename)) {
        ostringstream o;
        o << "write default rc: " << filename << " DOES exist ...";
        throw logic_error(o.str());
      }
      ofstream f(filename);
      f.exceptions(ofstream::failbit | ofstream::badbit);
      f << default_rc_file;
    }

    void Options::check_configfile()
    {
      if (configfile.empty()) {
        string home(ansi::getenv("HOME"));

        string config_dir;
        config_dir.reserve(128);
        config_dir = home;
        config_dir += "/.config/";
        config_dir += ID::argv0;

        configfile.reserve(128);
        configfile = config_dir;
        configfile += "/rc.json";

        fs::create_directories(config_dir);
        if (!fs::exists(configfile)) {
          cerr
            << "Could not find a run control file at the default location.\n"
            << "Thus, writing default run control file to " << configfile << '\n'
            << "(edit your account details there)\n\n";
          write_default_rc(configfile);
          exit(1);
        }
      }
    }

    static void check_keys(const boost::property_tree::ptree &pt,
        const string &account, const string &filename)
    {
      for (auto &i : pt) {
        if (i.first.find("_comment") == 0)
          continue;

        auto k = KEY::set.find(i.first.c_str());
        if (k == KEY::set.end()) {
          ostringstream o;
          o << "Unknown key |" << i.first << "| in account " << account
            << "(" << filename << ")";
          throw std::runtime_error(o.str());
        }
      }
    }

    void Options::load()
    {
      check_configfile();

      boost::property_tree::ptree pt;
      boost::property_tree::json_parser::read_json(configfile, pt);

      auto sub_tree = pt.get_child(account);
      check_keys(sub_tree, account, configfile);
      username = sub_tree.get<string>(KEY::USERNAME);
      password = sub_tree.get<string>(KEY::PASSWORD);

      ip            = sub_tree.get<unsigned>       (KEY::IP           , 4       );
      local_address = sub_tree.get<string>         (KEY::LOCAL_ADDRESS, ""      );
      local_port    = sub_tree.get<unsigned short> (KEY::LOCAL_PORT   , 0       );
      host          = sub_tree.get<string>         (KEY::HOST         , ""      );
      service       = sub_tree.get<string>         (KEY::SERVICE      , ""      );

      use_ssl       = sub_tree.get<bool>           (KEY::SSL          , true    );
      fingerprint   = sub_tree.get<string>         (KEY::FINGERPRINT  , ""      );
      cipher        = sub_tree.get<string>         (KEY::CIPHER       , ""      );
      cipher_preset = sub_tree.get<unsigned>       (KEY::CIPHER_PRESET, 1       );
      ca_file       = sub_tree.get<string>         (KEY::CA_FILE      , ""      );
      ca_path       = sub_tree.get<string>         (KEY::CA_PATH      , ""      );
      cert_host     = sub_tree.get<string>         (KEY::CERT_HOST    , ""      );
      tls1          = sub_tree.get<bool>           (KEY::TLS1         , true    );

      del           = sub_tree.get<bool>           (KEY::DELETE       , false   );
      mailbox       = sub_tree.get<string>         (KEY::MAILBOX      , "INBOX" );
      maildir       = sub_tree.get<string>         (KEY::MAILDIR      , ""      );
      journal_file  = sub_tree.get<string>         (KEY::JOURNAL_FILE , ""      );
    }
    std::ostream &Options::print(std::ostream &o) const
    {
      o << "username: " << username << '\n';
      o << "password: " << password << '\n';
      return o;
    }
    std::ostream &operator<<(std::ostream &o, const Options &opts)
    {
      return opts.print(o);
    }



  }
}
