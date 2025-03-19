UNITTEST()

OWNER(
    gotmanov
)

PEERDIR(
    kernel/text_machine/structured_id
)

SRCS(
    id_ut.cpp
)

GENERATE_ENUM_SERIALIZATION(id_ut.h)

END()
