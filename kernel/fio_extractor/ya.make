LIBRARY()

OWNER(g:yane)

PEERDIR(
    kernel/fio
    kernel/indexer/direct_text
    kernel/lemmer
    kernel/lemmer/dictlib
    library/cpp/charset
    library/cpp/containers/dictionary
    library/cpp/token
)

SRCS(
    fioclusterbuilder.cpp
    fioclusterbuilder.old.cpp
    fiofinder2.cpp
    fiowordsequence.cpp
    gramhelper2.cpp
    wordspair.cpp
    name_handler.cpp
    utility.cpp
    dump_fio_handler.cpp
)

END()
