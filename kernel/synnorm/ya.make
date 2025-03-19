LIBRARY(synnorm)

OWNER(
    ilnurkh # this project was actually ownerless if you want to replace my name here - you are welcome
)

PEERDIR(
    contrib/libs/protobuf
    kernel/gazetteer
    kernel/gazetteer/richtree
    kernel/lemmer/core
    kernel/qtree/request
    kernel/qtree/richrequest
    kernel/reqerror
    kernel/search_daemon_iface
    library/cpp/binsaver
    library/cpp/stopwords
    library/cpp/string_utils/base64
)

SRCS(
    synnorm.cpp
    synset.gztproto
)

END()
