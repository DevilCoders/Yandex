UNITTEST_FOR(kernel/common_server/util/raw_text)

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/testing/unittest
)

IF(OS_WINDOWS)
    CXXFLAGS(-DI18N_PHONENUMBERS_NO_THREAD_SAFETY)
ENDIF()

SRCS(
    datetime_ut.cpp
    phone_number_ut.cpp
)

END()
