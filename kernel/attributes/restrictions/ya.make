LIBRARY()

OWNER(
    g:base
)

SRCS(
    attr_restrictions.h
    attr_restrictions_decomposer.cpp
)

PEERDIR(
    kernel/attributes/restrictions/proto
    kernel/qtree/request
    kernel/qtree/richrequest
    ysite/yandex/common
)

END()
