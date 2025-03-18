LIBRARY()

OWNER(agorodilov)

SRCS(
    datawork.h
    datawork_api.h
    datawork_dump.h
    datawork_dumpadd.h
    datawork_filter.h
    datawork_filteradd.h
    datawork_index.h
    datawork_load.h
    datawork_loadinfo.h
    datawork_main.h
    datawork_sort.h
    datawork_util.h
    dataworklibmain.cpp
)

PEERDIR(
    contrib/libs/protobuf
    contrib/libs/zlib
    library/cpp/deprecated/datawork/conf
    library/cpp/deprecated/fgood
    library/cpp/deprecated/mbitmap
    library/cpp/getopt/small
    library/cpp/microbdb
    library/cpp/svnversion
)

END()
