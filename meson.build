project('imapdl', 'cpp')


botan_dep = dependency('botan-1.10')
openssl_dep = dependency('openssl')
boost_dep = dependency('boost', version: '1.55', modules : [
    'system', # needed by filesystem, log
    'filesystem',
    'regex',
    'test',
    'program_options',
    'serialization',
    'log',
    # needed by log
    'thread',
    #
    'locale',
    'regex'])

executable('hash', 'example/hash.cc', dependencies : botan_dep)

# XXX create proper subprojects
ixxx_inc = include_directories('libixxx')
ixxx_lib = static_library('ixxx', 'libixxx/ixxx/ixxx.cc',
  'libixxx/ixxx/ansi.cc', 'libixxx/ixxx/posix.cc',
  include_directories : ixxx_inc
)

buffer_inc = include_directories('libbuffer')
buffer_lib = static_library('buffer',
  'libbuffer/buffer/buffer.cc', 'libbuffer/buffer/file.cc',
  include_directories : [buffer_inc, ixxx_inc]
)

ragel = find_program('ragel')
ragel_gen = generator(ragel, output: '@BASENAME@.cc',
  arguments: ['-I@SOURCE_DIR@', '-o', '@OUTPUT@', '@INPUT@'])

ragel_imap_src = ragel_gen.process('imap/client_parser.rl',
  'imap/server_parser.rl')
ragel_mime_header_decoder_src = ragel_gen.process('mime/header_decoder.rl')
ragel_ascii_control_sanitizer_src = ragel_gen.process(
    'ascii/control_sanitizer.rl')

ragel_mime_q_decoder_main_src = ragel_gen.process('mime/q_decoder_main.rl')
ragel_mime_base64_decoder_main_src = ragel_gen.process(
    'mime/base64_decoder_main.rl')

executable('imapdl',
  'copy/main.cc',
  'copy/options.cc',
  'copy/client.cc',
  'copy/id.cc',
  'copy/journal.cc',
  'copy/state.cc',
  'copy/fetch_timer.cc',
  'copy/header_printer.cc',
  'net/client.cc',
  'net/client_application.cc',
  'net/tcp_client.cc',
  'net/ssl_util.cc',
  'net/ssl_verification.cc',
  'log/log.cc',
  'imap/imap.cc',
  ragel_imap_src,
  'lex_util.cc',
  'imap/client_parser_callback.cc',
  'imap/client_writer.cc',
  'imap/client_base.cc',
  'maildir/maildir.cc',
  'sequence_set.cc',
  'trace/trace.cc',
  ragel_mime_header_decoder_src,
  ragel_ascii_control_sanitizer_src,

  dependencies: [ boost_dep, openssl_dep],
  link_with: [ ixxx_lib, buffer_lib ],
  include_directories : [buffer_inc, ixxx_inc],
  cpp_args: '-DBOOST_LOG_DYN_LINK'
)


ut = executable('ut',
  'imap/imap.cc',
  'imap/client_parser_callback.cc',
  'imap/client_writer.cc',
  'imap/client_base.cc',
  'maildir/maildir.cc',
  'net/ssl_util.cc',
  'unittest/main.cc',
  'unittest/imap_client_parser.cc',
  'unittest/imap_server_parser.cc',
  'unittest/data.cc',
  'unittest/maildir.cc',
  'unittest/replay.cc',
  'unittest/imap_client_writer.cc',
  'example/server.cc',
  'example/client.cc',
  ragel_imap_src,
  'lex_util.cc',
  'unittest/sequence_set.cc',
  'sequence_set.cc',

  # for imapdl
  'unittest/copy.cc',
  'copy/options.cc',
  'copy/client.cc',
  'copy/id.cc',
  'copy/journal.cc',
  'copy/state.cc',
  'copy/fetch_timer.cc',
  'copy/header_printer.cc',
  'net/client.cc',
  'net/client_application.cc',
  'net/tcp_client.cc',
  'trace/trace.cc',
  'log/log.cc',
  'net/ssl_verification.cc',

  ragel_mime_base64_decoder_main_src,
  ragel_mime_q_decoder_main_src,
  ragel_mime_header_decoder_src,
  ragel_ascii_control_sanitizer_src,
  'unittest/mime.cc',
  'unittest/lex_util.cc',

  dependencies: [ boost_dep, openssl_dep,
    botan_dep # for ut comparison
  ],
  link_with: [ ixxx_lib, buffer_lib ],
  include_directories : [buffer_inc, ixxx_inc],
  cpp_args: '-DBOOST_LOG_DYN_LINK'
)

# default timeout 30s
test('unittest', ut, timeout: 100)

# default workdir: meson.build_root()
#pwd = find_program('pwd')
#test('unittest', pwd)
#test('unittest', pwd, workdir: meson.build_root())

# build with coverage switches:
#     meson -Db_coverage=true build-meson-cov

executable('client',
  'example/client.cc',
  'example/client_main.cc',
  'net/ssl_util.cc',
  'trace/trace.cc',
  'lex_util.cc',

  # executable doesn't really depend on all boost submodules
  # but linker only records only used ones as NEEDED, anyways
  dependencies: [ boost_dep, openssl_dep ],
  link_with: [ ixxx_lib],
  include_directories : [ixxx_inc]
)

executable('server',
  'example/server.cc',
  'example/server_main.cc',
  'net/ssl_util.cc',
  'lex_util.cc',
  'trace/trace.cc',

  dependencies: [ boost_dep, openssl_dep ],
  link_with: [ ixxx_lib],
  include_directories : [ixxx_inc]
)

executable('replay',
  'example/replay.cc',
  'trace/trace.cc',

  dependencies: [ boost_dep]
)

