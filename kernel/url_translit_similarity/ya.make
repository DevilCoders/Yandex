OWNER(
    g:factordev
)

LIBRARY()

PEERDIR(
    kernel/querydata/request
    library/cpp/charset
    library/cpp/string_utils/quote
    ysite/yandex/common
    ysite/yandex/reqanalysis
)

SRCS(
    factors.cpp
    hardcomp.cpp
    translate.h
    utils.h
)

END()
