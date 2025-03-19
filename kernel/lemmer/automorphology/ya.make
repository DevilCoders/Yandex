LIBRARY()

OWNER(g:morphology)

SRCS(
    automorphology.cpp
)

PEERDIR(
    library/cpp/archive
    library/cpp/containers/comptrie
    library/cpp/digest/md5
    kernel/lemmer/core
    kernel/lemmer/new_engine
)

END()
