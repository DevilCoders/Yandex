LIBRARY()

OWNER(shuster)

SRCS(
    recogn.cpp
    pdstorage.cpp
    pdstorageconf.h
)

PEERDIR(
    kernel/recshell
    kernel/tarc/iface
    library/cpp/charset
    library/cpp/html/face
    library/cpp/html/pdoc
    library/cpp/html/storage
    library/cpp/html/zoneconf
    library/cpp/langs
    library/cpp/mime/types
)

END()
