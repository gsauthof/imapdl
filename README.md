IMAP4v1 Download Client

2014, Georg Sauthoff <mail@georg.so>

This repository contains a [IMAP4v1][rfc3501] client for fetching messages from
an IMAP server and storing them in a local [maildir][maildir] directory. It is
also able to delete them on the server if they are successfully transferred.

Besides the actual client, the repository may be of interest as a base for
other IMAP clients/servers because it includes [Ragel][ragel] based parsers for
the client and server side of the IMAP protocol. For such use cases it is
recommended to include this repository as Git [submodule][gitm].

## Features

- [Maildir][maildir] support - client makes sure to [fsync][fsync] at the right
  times to minimize data loss in the case of power failure
- [SSL][ssl] enabled by default
- In case the connection is interrupted during a fetch operation, the UIDs of
  completely fetched messages are written to a journal and expunged on next
  program start before the remaining messages are fetched. Useful, when e.g.
  retrieving a large mailbox over an unreliable mobile network
  (think: UMTS when travelling in a high speed train).
- Workarounds for some IMAP server bugs (deviations from the RFC)
- Uses the UIDPLUS extension if available  - thus, excluding side effects with
  concurrently established server connections when purging messages
- Plain [tilde expansion][tilde] in local mailbox paths
- Configuration via [JSON][json] [run control][rc] file
- Written in C++ with some C++11 features
- Asynchronous IO using [Boost ASIO][asio]
- Robust IMAP protocol lexer implemented in [Ragel][ragel]
- Before commands are send to the server they are locally parsed with a Ragel
  grammar that implements the server side of the IMAP spec - thus, the client
  verifies that it does not violate [RFC3501][rfc3501] in an obvious way.
- High test coverage with unittests

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


### Dependencies

- C++11 Compiler (e.g. [GCC][gcc] >= 4.8)
- POSIX like system
- [CMake][cmake] (>= 2.8)
- [Boost][boost] (>= 1.54)
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

- Linux (Fedora 19 x86_64)

### IMAP Servers

- [Dovecot][dovecot] (1.2.9)
- [Cyrus][cyrus] (Cyrus IMAP v2.3.13)
- Google GMail (Gimap)

## Licence

GPLv3+

## Appendix

### Example Subdirectory

A collection of small example programs for testing some related features:

- `length.rl` - lexing a token based on the preceding length value
- `client.cc` - exploring ASIO features, simple example ASIO client, also used for unittesting replay feature
- `server.cc` - exploring ASIO features, also used for replaying IMAP sessions
  in unittests
- `replay.cc` - for dumping serialized network sessions
- `hash.cc` - implement sha256sum using the [Botan][botan] C++ library

### Links

- [RFC 3501 - INTERNET MESSAGE ACCESS PROTOCOL - VERSION 4rev1](http://tools.ietf.org/html/rfc3501)
- [RFC 2683 - IMAP4 Implementation Recommendations](http://tools.ietf.org/html/rfc2683)
- [RFC 2180 - IMAP4 Multi-Accessed Mailbox Practice](http://tools.ietf.org/html/rfc2180)
- [Internet Message Access Protocol (IMAP) Capabilities Registry](http://www.iana.org/assignments/imap-capabilities/imap-capabilities.xhtml)
- [RFC 4315 - Internet Message Access Protocol (IMAP) - UIDPLUS extension](http://tools.ietf.org/html/rfc4315)
- [RFC 6855 - IMAP Support for UTF-8](http://tools.ietf.org/html/rfc6855)
- [RFC 3629 - UTF-8, a transformation format of ISO 10646, Syntax of UTF-8 Byte Sequences](http://tools.ietf.org/html/rfc3629#section-4)
- [RFC 2971 - IMAP4 ID extension](http://tools.ietf.org/html/rfc2971)
- [RFC 4466 - Collected Extensions to IMAP4 ABNF](http://tools.ietf.org/html/rfc4466)
- [RFC 2595 - Using TLS with IMAP, POP3 and ACAP](http://tools.ietf.org/html/rfc2595)
- [RFC 2234 - Augmented BNF for Syntax Specifications: ABNF](http://tools.ietf.org/html/rfc2234)
- [RFC 822 - Standard for the Format of ARPA Internet Text Messages, Notational Conventions](http://tools.ietf.org/html/rfc822#section-2) - as a reference for pre-RFC2234 ABNF constructs
- [RFC 2822 - Internet Message Format](http://tools.ietf.org/html/rfc2822)


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
[json]:    http://en.wikipedia.org/wiki/JSON
[maildir]: http://en.wikipedia.org/wiki/Maildir
[openssl]: http://www.openssl.org/
[ragel]:   http://www.complang.org/ragel/
[rc]:      http://www.faqs.org/docs/artu/ch10s03.html
[rfc3501]: http://tools.ietf.org/html/rfc3501
[ssl]:     http://en.wikipedia.org/wiki/SSL
[tilde]:   http://www.gnu.org/software/libc/manual/html_node/Tilde-Expansion.html
