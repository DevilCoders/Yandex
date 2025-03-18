UNITTEST()

OWNER(
    gotmanov
    vasil-sd
)

PEERDIR(
    library/cpp/scheme
)

SRCS(
    test.sc
    struct_ut.cpp
    includes_ut.cpp
    const_attr_ut.cpp
    pragmas_ut.cpp
    svn_kwd_ut.cpp
    lexer_context_ut.cpp
    ../lexer.cpp
    ../parser.cpp
    ../pragma.cpp
    ../generate.cpp
    ../svn_keywords.cpp
)

END()
