UNITTEST()

OWNER(dfyz)

PEERDIR(
    ADDINCL kernel/mango/url
)

SRCDIR(kernel/mango/url)

SRCS(
    info_ut.cpp
    normalize_url_ut.cpp
    url_canonizer_ut.cpp
)

END()
