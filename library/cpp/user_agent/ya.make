OWNER(
    buryakov
    g:userexp
)

LIBRARY()

PEERDIR(
    library/cpp/regex/pcre
    metrika/uatraits/library
)

SRCS(
    browser_detector.cpp
)

END()

RECURSE_FOR_TESTS(ut)
