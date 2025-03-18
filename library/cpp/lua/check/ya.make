LIBRARY()

OWNER(
    galtsev
    g:contrib
    g:yabs-rt
    g:antiinfra
)

PEERDIR(
    library/cpp/lua/luajit-if-no-sanitizers
)

RESOURCE(
    contrib/libs/luacheck/src/luacheck/init.lua luacheck
    contrib/libs/luacheck/src/luacheck/fs.lua luacheck.fs
    contrib/libs/luacheck/src/luacheck/analyze.lua luacheck.analyze
    contrib/libs/luacheck/src/luacheck/argparse.lua luacheck.argparse
    contrib/libs/luacheck/src/luacheck/cache.lua luacheck.cache
    contrib/libs/luacheck/src/luacheck/check.lua luacheck.check
    contrib/libs/luacheck/src/luacheck/config.lua luacheck.config
    contrib/libs/luacheck/src/luacheck/core_utils.lua luacheck.core_utils
    contrib/libs/luacheck/src/luacheck/expand_rockspec.lua luacheck.expand_rockspec
    contrib/libs/luacheck/src/luacheck/filter.lua luacheck.filter
    contrib/libs/luacheck/src/luacheck/format.lua luacheck.format
    contrib/libs/luacheck/src/luacheck/globbing.lua luacheck.globbing
    contrib/libs/luacheck/src/luacheck/inline_options.lua luacheck.inline_options
    contrib/libs/luacheck/src/luacheck/lexer.lua luacheck.lexer
    contrib/libs/luacheck/src/luacheck/linearize.lua luacheck.linearize
    contrib/libs/luacheck/src/luacheck/main.lua luacheck.main
    contrib/libs/luacheck/src/luacheck/multithreading.lua luacheck.multithreading
    contrib/libs/luacheck/src/luacheck/options.lua luacheck.options
    contrib/libs/luacheck/src/luacheck/parser.lua luacheck.parser
    contrib/libs/luacheck/src/luacheck/reachability.lua luacheck.reachability
    contrib/libs/luacheck/src/luacheck/stds.lua luacheck.stds
    contrib/libs/luacheck/src/luacheck/utils.lua luacheck.utils
    contrib/libs/luacheck/src/luacheck/version.lua luacheck.version
    library/cpp/lua/check/wrapper.lua luacheck.cpp.wrapper
)

SRCS(
    luacheck.cpp
)

END()
