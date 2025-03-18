LIBRARY()

PROVIDES(test_framework)

NO_UTIL()

OWNER(
    g:yatool
    dmitko
)

SRCS(
    GLOBAL ya_boost_test.cpp
)

PEERDIR(
    contrib/restricted/boost/libs/test/targets/lib
    library/cpp/testing/common
)

END()
