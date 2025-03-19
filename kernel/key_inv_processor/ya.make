LIBRARY()

OWNER(ivanmorozov)

SRCS(
    ki_actor.cpp
    ki_maker.cpp
    memory_portions.cpp
)

PEERDIR(
    kernel/index_mapping
    kernel/keyinv/indexfile
    kernel/walrus
    library/cpp/logger/global
    ysite/yandex/srchmngr
)

END()
