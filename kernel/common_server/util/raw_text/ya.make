LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    contrib/libs/cctz/tzdata
    contrib/libs/libphonenumber
    kernel/common_server/util
)

ADDINCL(
    GLOBAL contrib/libs/libphonenumber/cpp/src
)

IF(OS_WINDOWS)
    CXXFLAGS(-DI18N_PHONENUMBERS_NO_THREAD_SAFETY)
ENDIF()

SRCS(
    datetime.cpp
    phone_number.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
