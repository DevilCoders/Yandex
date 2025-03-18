LIBRARY()

OWNER(
    g:clustermaster
)

SRCS(
    wsmst.cpp
)

PEERDIR(
    ADDINCL tools/clustermaster/common
    yweb/config
    library/cpp/xsltransform
    library/cpp/svnversion
)

END()
