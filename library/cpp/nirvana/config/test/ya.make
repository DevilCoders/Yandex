EXECTEST()

OWNER(pozhilov)

DEPENDS(library/cpp/nirvana/config/test/bin)

DATA(arcadia/library/cpp/nirvana/config/test/data)

RUN(
    ENV
    JWD=${ARCADIA_ROOT}/library/cpp/nirvana/config/test/data
    nirvana_config_test
    -c
    ${ARCADIA_ROOT}/library/cpp/nirvana/config/test/data/config.lua
    --param
    command_line=param
    STDOUT
    std.lua
    CANONIZE_LOCALLY
    std.lua
    CANONIZE_LOCALLY
    config.log
)

RUN(
    ENV
    JWD=${ARCADIA_ROOT}/library/cpp/nirvana/config/test/data
    nirvana_config_test
    -c
    ${ARCADIA_ROOT}/library/cpp/nirvana/config/test/data/config.JSON
    --param
    command_line=param
    STDOUT
    std.json
    CANONIZE_LOCALLY
    std.json
    CANONIZE_LOCALLY
    config.log
)

END()
