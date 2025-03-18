PROGRAM()

PEERDIR(
    dict/recognize/queryrec
    kernel/geo
    library/cpp/deprecated/split
    library/cpp/getopt
    search/pumpkin/nearest_query
    ysite/yandex/doppelgangers
    ysite/yandex/reqanalysis
)

SRCS(
    normalize_requests.cpp
)

END()
