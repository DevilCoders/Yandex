PROGRAM()

OWNER(
    wd28
    gotmanov
    pmatsula
)

PEERDIR(
    library/cpp/getopt/small
    library/cpp/resource
)

SRCS(
    main.cpp
    lexer.cpp
    svn_keywords.cpp
    parser.cpp
    pragma.cpp
    generate.cpp
    runtime.cpp
)

RESOURCE(
    runtime.h /runtime
)

INDUCED_DEPS(h+cpp
    ${ARCADIA_ROOT}/util/datetime/base.h
    ${ARCADIA_ROOT}/util/generic/strbuf.h
    ${ARCADIA_ROOT}/util/generic/string.h
    ${ARCADIA_ROOT}/util/generic/vector.h
    ${ARCADIA_ROOT}/util/string/cast.h
)

END()

RECURSE(
    ut
    tests
)
