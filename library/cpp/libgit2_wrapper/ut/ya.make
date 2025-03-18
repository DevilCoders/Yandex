UNITTEST()

OWNER(g:arc)

IF (NOT MSVC)
    CXXFLAGS(-Wimplicit-fallthrough)
ENDIF()

SRCS(
    libgit2_ut.cpp
    signature_ut.cpp
    unidiff_ut.cpp
)

PEERDIR(
    contrib/libs/libgit2
    library/cpp/libgit2_wrapper
)

IF (OS_WINDOWS)
    ENV(YA_TEST_SHORTEN_WINE_PATH=1)
ENDIF()

DATA(
    sbr://2880875763
)

END()

RECURSE(canonize)
