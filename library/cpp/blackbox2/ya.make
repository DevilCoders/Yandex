LIBRARY()

OWNER(g:passport_infra)

PEERDIR(
    contrib/libs/libidn
    contrib/libs/libxml
    contrib/libs/openssl
)

SRCS(
    src/accessors.cpp
    src/blackbox2.cpp
    src/options.cpp
    src/responseimpl.cpp
    src/utils.cpp
    src/xconfig.cpp
)

GENERATE_ENUM_SERIALIZATION(blackbox2.h)
GENERATE_ENUM_SERIALIZATION(session_errors.h)

END()

RECURSE_FOR_TESTS(ut)
