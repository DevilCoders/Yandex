LIBRARY()

OWNER(
    g:base
    gluk47
    velavokr
)

SRCS(
    doc_handle.cpp
    doc_route.cpp
    url2docid.cpp
    urlhash.cpp
    urlid.cpp
)

PEERDIR(
    kernel/multilanguage_hosts
    library/cpp/binsaver
    library/cpp/digest/old_crc
    library/cpp/string_utils/url
)

END()
