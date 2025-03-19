OWNER(g:cpp-contrib)

LIBRARY()

NO_WSHADOW()

SRCS(
    hostnamehash.cpp
    minifilter.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/deprecated/datafile
    library/cpp/deprecated/fgood
    library/cpp/deprecated/mbitmap
    library/cpp/deprecated/sgi_hash
    library/cpp/on_disk/st_hash
    util/draft
)

END()
