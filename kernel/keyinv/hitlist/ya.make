LIBRARY()

OWNER(
    g:base
    leo
)

SRCDIR(kernel/keyinv/hitlist/inv_code)

SRCS(
    longs.cpp
    full_pos.cpp
    hitread.cpp
    hits_coders.cpp
    hits_saver.cpp
    hits_raw.cpp
    invsearch.cpp
    positerator.cpp
    reqserializer.cpp
    subindex.cpp
    performance_counters.cpp # TODO: move to proper place
    dochit_counters.cpp
    inv_code/inv_code.cpp
)

IF (NOT ARCH_K1OM)
    SRCS(
        inv_code/bit_code.cpp
    )
ENDIF()

PEERDIR(
    kernel/keyinv/hitlist/memory
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/packedtypes
    library/cpp/sse
    library/cpp/wordpos
    library/cpp/yappy
)

END()
