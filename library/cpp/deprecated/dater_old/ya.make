LIBRARY()

OWNER(velavokr)

SRCS(
    dater_stats.cpp
    structs.cpp
)

PEERDIR(
    util/draft
    library/cpp/deprecated/dater_old/date_attr_def
)

IF (CLANG)
    CFLAGS(-Wno-bitfield-constant-conversion)
ENDIF()

IF (GCC)
    CFLAGS(-Wno-overflow)
ENDIF()

END()
