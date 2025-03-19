LIBRARY()

OWNER(
    dmitryno
    g:wizard
    gotmanov
)

PEERDIR(
    kernel/gazetteer
    kernel/gazetteer/common
    kernel/qtree/richrequest
    library/cpp/deprecated/iter
)

SRCS(
    gztres.cpp
    richnodeiter.cpp
    supernode.cpp
)

END()
