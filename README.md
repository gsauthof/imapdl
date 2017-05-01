[![C++](https://img.shields.io/badge/C++-11-blue.svg)](https://en.wikipedia.org/wiki/C++11) [![License](https://img.shields.io/badge/license-GPL--3-blue.svg)][lgpl] [![Build Status](https://travis-ci.org/gsauthof/imapdl.svg?branch=master)](https://travis-ci.org/gsauthof/imapdl) [![Code Coverage](https://codecov.io/github/gsauthof/imapdl/coverage.svg?branch=master)](https://codecov.io/github/gsauthof/imapdl)

IMAP4v1 Download Client

2014, Georg Sauthoff <mail@georg.so>

This repository contains a [IMAP4v1][rfc3501] client for fetching messages from
an IMAP server and storing them in a local [maildir][maildir] directory. It is
also able to delete them on the server if they are successfully transferred.

Besides the actual client, the repository may be of interest as a base for
other IMAP clients/servers because it includes [Ragel][ragel] based parsers for
the client and server side of the IMAP protocol. For such use cases it is
recommended to include this repository as Git [submodule][gitm].

(last change to this readme file: 2017-04-29)

## Features

- [Maildir][maildir] support - client makes sure to [fsync][fsync] at the right
  times to minimize data loss in the case of power failure
- [SSL][ssl] (i.e. [TLS][tls]) enabled by default
- In case the connection is interrupted during a fetch operation, the UIDs of
  completely fetched messages are written to a journal and expunged on next
  program start before the remaining messages are fetched. Useful, when e.g.
  retrieving a large mailbox over an unreliable mobile network
  (think: UMTS when travelling in a high speed train).
- display From/Subject/Date headers during fetching (when INFO severity level
  is turned on)
- Workarounds for some IMAP server bugs (deviations from the RFC)
- Uses the UIDPLUS extension if available  - thus, excluding side effects with
  concurrently established server connections when purging messages
- Plain [tilde expansion][tilde] in local mailbox paths
- Configuration via [JSON][json] [run control][rc] file
- Written in C++ with some C++11 features
- Asynchronous IO using [Boost ASIO][asio]
- Robust IMAP protocol parser implemented in [Ragel][ragel]
- Before commands are send to the server they are locally parsed with a Ragel
  grammar that implements the server side of the IMAP spec - thus, the client
  verifies that it doesn't violate [RFC3501][rfc3501] in any obvious way.
- High test coverage with unittests


## Design Choices

- Use sane and secure defaults (e.g. TLS encryption and certificate validation
  are enabled by default).
- Don't send commands to the server that are expected to fail (e.g. the LOGIN
  command when it is disabled or the UID expunge command when the server has no
  UIDPLUS extension).
- Verify commands before sending them to the server.
- Use asynchronous IO without threads.
- Use state machines where it makes the code more robust, compact, easier to reason about etc.
- Support IPv4 and [IPv6][v6].
- Use layering where it reduces complexity (e.g. in the download client
  the differenes between the Boost ASIO TCP and SSL APIs are abstracted away by a
  layer, on top of that are the IMAP parsers layered,  etc.)
- Give [SASL][sasl] support a low priority, because IMAP over TLS
  generally makes SASL redundant (cf. SASL notes section).
- Make it portable (e.g. via using open standards like ISO C++, POSIX, portable
  tools like CMake, Ragel and portable libraries like Boost).


## Compile

This project includes some [Git submodules][gitm], thus, [make sure][gitm2]
that they are correctly updated after git clone (i.e. verify that the
subdirectories `libbuffer`, `libixxx` and `cmake/ragel` are there. Perhaps you
have to adjust the submodule URLs, in case you don't have your Github ssh
account correctly setup.

Out-of-source builds are recommended, thus, do something like this:

    $ git clone git@github.com:gsauthof/imap.git
    $ cd imap
    $ git submodule update --init
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make imapdl ut

If can also use an alternative build generator with cmake, of course, e.g.:

    $ cmake -G Ninja ..
    $ ninja-build imapdl ut

In case you have Boost installed in a custom location you can do something like:

    $ BOOST_ROOT=/home/juser/local/boost-1.55 cmake -G Ninja ..

For setting some CMake variables:

    $ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=true -G Ninja ..

Or supply a custom cache initialization file via `cmake -C`.

### Meson

Alternatively, this project can be built with
[Meson](http://mesonbuild.com/). The Meson build configuration is
located in `meson.build`. The Meson build system is younger than
CMake, it has more useful defaults, a saner syntax and some nice
features.

Example:

    $ meson --buildtype=release build
    $ cd build
    $ ninja-build imapdl

### Dependencies

- C++11 Compiler (e.g. [GCC][gcc] >= 4.8)
- POSIX like system
- [CMake][cmake] (>= 2.8)
- [Boost][boost] (>= 1.55)
- [Ragel][ragel] (>= 6.6)
- [OpenSSL][openssl] (tested with 1.0.1e)
- [Botan][botan] (tested with 1.8.14, only for unittests)


## Basic usage

On the first run, imapdl generates a run controle file template under:

    $HOME/.config/imapdl/rc.json

You can copy the 'example' section and rename it to 'default' for the default
account. For multiple accounts you can use a section name of your choice and
supply `--account somename` to the `imapdl` command line.

The default cipher list for SSL is somewhat progressive (i.e. it basically only
include [forward secrecy][fwd] providing TLSv1.2 ciphers). You can use a more relaxed
list via setting `cipher_preset` to 2 or 3 (higher means more relaxed) - or you
can configure your own cipher list.

For verifying the authenticity of the certificate provided by the server you can either specify

- the [certificate authority][ca] (CA) root certificate file,
- a directory with CA root certificates, or
- the expected [fingerprint][fp] of the server certificate


## Tested platforms

- Linux (Fedora 19/20 x86_64)

### IMAP Servers

- [Dovecot][dovecot] (1.2.9)
- [Cyrus][cyrus] (Cyrus IMAP v2.3.13)
- Google GMail (Gimap)

## Licence

[GPLv3+][gpl3]

## Appendix

### Example Subdirectory

A collection of small example programs for testing some related features:

- `length.rl` - lexing a token based on the preceding length value
- `client.cc` - exploring ASIO features, simple example ASIO client, also used for unittesting replay feature
- `server.cc` - exploring ASIO features, also used for replaying IMAP sessions
  in unittests
- `replay.cc` - for dumping serialized network sessions
- `hash.cc`   - implement sha256sum using the [Botan][botan] C++ library

### SASL Notes

When securing the connection with TLS, SASL doesn't increase your security. On
the other hand, on unsecured connections SASL only protects your password
somewhat (depending on the used method) but it doesn't help against
[man-in-the-middle][mitm] attacks or eavesdropping on the content of your
mails. SASL is perhaps of interest for certain methods, e.g. to support
Kerberos authentication with IMAP servers, but I haven't come across such
setups, yet.

### Links

- [RFC 3501 - INTERNET MESSAGE ACCESS PROTOCOL - VERSION 4rev1](http://tools.ietf.org/html/rfc3501), Proposed STD, 2003
- [RFC 2683 - IMAP4 Implementation Recommendations](http://tools.ietf.org/html/rfc2683), Informational, 1999
- [RFC 2180 - IMAP4 Multi-Accessed Mailbox Practice](http://tools.ietf.org/html/rfc2180), Informational, 1997
- [Internet Message Access Protocol (IMAP) Capabilities Registry](http://www.iana.org/assignments/imap-capabilities/imap-capabilities.xhtml)
- [RFC 4315 - Internet Message Access Protocol (IMAP) - UIDPLUS extension](http://tools.ietf.org/html/rfc4315), Proposed STD, 2005
- [RFC 6855 - IMAP Support for UTF-8](http://tools.ietf.org/html/rfc6855), Proposed STD, 2013
- [RFC 3629 - UTF-8, a transformation format of ISO 10646, Section 4 Syntax of UTF-8 Byte Sequences](http://tools.ietf.org/html/rfc3629#section-4), STD, 2003
- [RFC 2971 - IMAP4 ID extension](http://tools.ietf.org/html/rfc2971), Proposed STD, 2000
- [RFC 4466 - Collected Extensions to IMAP4 ABNF](http://tools.ietf.org/html/rfc4466), Proposed STD, 2006
- [RFC 2595 - Using TLS with IMAP, POP3 and ACAP](http://tools.ietf.org/html/rfc2595), Proposed STD, 1999
- [RFC 2234 - Augmented BNF for Syntax Specifications: ABNF](http://tools.ietf.org/html/rfc2234), Proposed STD, 1997
- [RFC 822 - Standard for the Format of ARPA Internet Text Messages, Section 2 Notational Conventions](http://tools.ietf.org/html/rfc822#section-2) - as a reference for pre-RFC2234 ABNF constructs, STD, 1982
- [RFC 5234 - Augmented BNF for Syntax Specifications: ABNF](http://tools.ietf.org/html/rfc5234), STD, 2008
- [RFC 2822 - Internet Message Format](http://tools.ietf.org/html/rfc2822), Proposed STD, 2001
- [RFC 2047 - MIME (Multipurpose Internet Mail Extensions) Part Three](http://tools.ietf.org/html/rfc2047): Message Header Extensions for Non-ASCII Text, [Section 4 Encodings](http://tools.ietf.org/html/rfc2047#section-4), Draft STD, 1996
- [RFC 2045 - Multipurpose Internet Mail Extensions (MIME) Part One](http://tools.ietf.org/html/rfc2045): Format of Internet Message Bodies, [Section 6.8 Base64 Content-Transfer-Encoding](http://tools.ietf.org/html/rfc2045#section-6.8), Draft STD, 1996
- [RFC 2231 - MIME Parameter Value and Encoded Word Extensions](http://tools.ietf.org/html/rfc2231): Character Sets, Languages, and Continuations, Proposed STD, 1997
- [RFC 1766 - Tags for Identifying Language](http://tools.ietf.org/html/rfc1766), Proposed STD, 1995
- [RFC 5646 - Tags for Identifying Language](http://tools.ietf.org/html/rfc5646), BCP 47, 2009
- [RFC 3348 - The Internet Message Action Protocol (IMAP4) Child Mailbox Extension](http://tools.ietf.org/html/rfc3348), Informational, 2002


[asio]:    http://www.boost.org/doc/libs/1_55_0/libs/asio/
[boost]:   http://www.boost.org/
[botan]:   http://botan.randombit.net/
[ca]:      http://en.wikipedia.org/wiki/Certificate_authority
[cmake]:   http://www.cmake.org/
[cyrus]:   https://cyrusimap.org/
[dovecot]: http://www.dovecot.org/
[fp]:      http://en.wikipedia.org/wiki/Public_key_fingerprint
[fsync]:   http://en.wikipedia.org/wiki/Sync_(Unix)
[fwd]:     http://en.wikipedia.org/wiki/Forward_secrecy
[gcc]:     http://gcc.gnu.org
[gitm2]:   http://git-scm.com/docs/git-submodule
[gitm]:    http://git-scm.com/book/en/Git-Tools-Submodules
[gpl3]:    http://www.gnu.org/copyleft/gpl.html
[json]:    http://en.wikipedia.org/wiki/JSON
[maildir]: http://en.wikipedia.org/wiki/Maildir
[mitm]:    http://en.wikipedia.org/wiki/Man-in-the-middle_attack
[openssl]: http://www.openssl.org/
[ragel]:   http://www.complang.org/ragel/
[rc]:      http://www.faqs.org/docs/artu/ch10s03.html
[rfc3501]: http://tools.ietf.org/html/rfc3501
[sasl]:    http://en.wikipedia.org/wiki/Simple_Authentication_and_Security_Layer
[ssl]:     http://en.wikipedia.org/wiki/SSL
[tilde]:   http://www.gnu.org/software/libc/manual/html_node/Tilde-Expansion.html
[tls]:     http://en.wikipedia.org/wiki/Transport_Layer_Security
[v6]:      http://en.wikipedia.org/wiki/IPv6
