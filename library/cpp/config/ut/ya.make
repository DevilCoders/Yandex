UNITTEST_FOR(library/cpp/config)

OWNER(
    pg
    velavokr
)

PEERDIR(
    library/cpp/config/extra
    library/cpp/scheme/ut_utils
    library/cpp/string_utils/relaxed_escaper
)

SRCS(
    config_ut.cpp
    domscheme_ut.cpp
    sax_ut.cpp
)

END()
