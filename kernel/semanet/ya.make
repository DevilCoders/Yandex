OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    semanet.cpp
    querynet.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/packers
    library/cpp/containers/ext_priority_queue
    library/cpp/langmask
    ysite/yandex/doppelgangers
    library/cpp/string_utils/url
)

END()
