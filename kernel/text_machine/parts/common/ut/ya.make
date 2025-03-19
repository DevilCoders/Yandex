UNITTEST()

OWNER(gotmanov)

PEERDIR(
    kernel/text_machine/parts/common
)

SRCS(
    static_table_ut.cpp
)

GENERATE_ENUM_SERIALIZATION(enums.h)

END()
