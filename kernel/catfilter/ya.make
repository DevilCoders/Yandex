OWNER(g:cpp-contrib)

LIBRARY()

SRCS(
    catfilter.cpp
    catfiltertrie.cpp
    catfiltertrie.proto
    catfilter_wrapper.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/mirrors
    library/cpp/containers/comptrie
    library/cpp/string_utils/url
    library/cpp/uri
)

END()
