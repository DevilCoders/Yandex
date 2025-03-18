UNITTEST_FOR(library/cpp/threading/future)

OWNER(
    g:util
)

SRCS(
    future_mt_ut.cpp
)

IF(SANITIZER_TYPE)
    SIZE(MEDIUM)
ENDIF()


END()
